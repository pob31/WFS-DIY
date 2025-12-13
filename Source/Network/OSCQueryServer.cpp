#include "OSCQueryServer.h"
#include "../Parameters/WFSParameterIDs.h"

namespace WFSNetwork
{

OSCQueryServer::OSCQueryServer(WFSValueTreeState& stateRef)
    : Thread("OSCQueryServer")
    , state(stateRef)
{
    state.getState().addListener(this);
}

OSCQueryServer::~OSCQueryServer()
{
    stop();
    state.getState().removeListener(this);
}

bool OSCQueryServer::start(int oscPortParam, int httpPortParam)
{
    if (running.load())
        stop();

    oscPort = oscPortParam;
    httpPort = httpPortParam;

    serverSocket = std::make_unique<juce::StreamingSocket>();

    if (!serverSocket->createListener(httpPort))
    {
        DBG("OSCQueryServer: Failed to create listener on port " << httpPort);
        serverSocket.reset();
        return false;
    }

    running = true;
    startThread();

    DBG("OSCQueryServer: Started on HTTP port " << httpPort << " (OSC port " << oscPort << ")");
    return true;
}

void OSCQueryServer::stop()
{
    if (!running.load())
        return;

    running = false;

    if (serverSocket)
        serverSocket->close();

    stopThread(2000);
    serverSocket.reset();

    DBG("OSCQueryServer: Stopped");
}

void OSCQueryServer::run()
{
    while (!threadShouldExit() && running.load())
    {
        if (serverSocket->waitUntilReady(true, 100) == 1)
        {
            auto* clientSocket = serverSocket->waitForNextConnection();
            if (clientSocket != nullptr)
            {
                std::unique_ptr<juce::StreamingSocket> client(clientSocket);
                handleHttpRequest(*client);
            }
        }
    }
}

void OSCQueryServer::handleHttpRequest(juce::StreamingSocket& client)
{
    // Read the HTTP request
    char buffer[4096];
    int bytesRead = client.read(buffer, sizeof(buffer) - 1, false);

    if (bytesRead <= 0)
        return;

    buffer[bytesRead] = '\0';
    juce::String request(buffer);

    // Parse the path from the request
    juce::String path = parseHttpPath(request);

    // Generate JSON response
    juce::String jsonResponse = generateJsonResponse(path);

    // Build HTTP response
    juce::String httpResponse;
    httpResponse << "HTTP/1.1 200 OK\r\n";
    httpResponse << "Content-Type: application/json\r\n";
    httpResponse << "Access-Control-Allow-Origin: *\r\n";
    httpResponse << "Content-Length: " << jsonResponse.length() << "\r\n";
    httpResponse << "Connection: close\r\n";
    httpResponse << "\r\n";
    httpResponse << jsonResponse;

    // Send response
    client.write(httpResponse.toRawUTF8(), (int)httpResponse.getNumBytesAsUTF8());
}

juce::String OSCQueryServer::parseHttpPath(const juce::String& request)
{
    // Parse "GET /path HTTP/1.1"
    if (!request.startsWith("GET "))
        return "/";

    int pathStart = 4;
    int pathEnd = request.indexOf(pathStart, " ");
    if (pathEnd < 0)
        pathEnd = request.indexOf(pathStart, "\r");
    if (pathEnd < 0)
        return "/";

    return request.substring(pathStart, pathEnd);
}

juce::String OSCQueryServer::generateJsonResponse(const juce::String& path)
{
    if (path == "/" || path.isEmpty())
        return buildRootJson();

    return buildNodeJson(path);
}

juce::String OSCQueryServer::buildRootJson()
{
    // Build the root namespace JSON according to OSC Query spec
    juce::String json;
    json << "{\n";
    json << "  \"FULL_PATH\": \"/\",\n";
    json << "  \"ACCESS\": 0,\n";
    json << "  \"DESCRIPTION\": \"WFS-DIY Wave Field Synthesis\",\n";
    json << "  \"CONTENTS\": {\n";
    json << "    \"wfs\": {\n";
    json << "      \"FULL_PATH\": \"/wfs\",\n";
    json << "      \"ACCESS\": 0,\n";
    json << "      \"DESCRIPTION\": \"WFS Parameters\",\n";
    json << "      \"CONTENTS\": {\n";
    json << "        \"input\": {\n";
    json << "          \"FULL_PATH\": \"/wfs/input\",\n";
    json << "          \"ACCESS\": 0,\n";
    json << "          \"DESCRIPTION\": \"Input Channels\",\n";
    json << "          \"CONTENTS\": {\n";

    // Add input channels
    int inputCount = state.getNumInputChannels();
    for (int i = 0; i < inputCount; ++i)
    {
        if (i > 0) json << ",\n";
        json << "            \"" << i << "\": " << buildInputChannelJson(i);
    }

    json << "\n          }\n";
    json << "        },\n";
    json << "        \"output\": {\n";
    json << "          \"FULL_PATH\": \"/wfs/output\",\n";
    json << "          \"ACCESS\": 0,\n";
    json << "          \"DESCRIPTION\": \"Output Channels\",\n";
    json << "          \"CONTENTS\": {\n";

    // Add output channels
    int outputCount = state.getNumOutputChannels();
    for (int i = 0; i < outputCount; ++i)
    {
        if (i > 0) json << ",\n";
        json << "            \"" << i << "\": " << buildOutputChannelJson(i);
    }

    json << "\n          }\n";
    json << "        }\n";
    json << "      }\n";
    json << "    }\n";
    json << "  },\n";
    json << "  \"OSC_PORT\": " << oscPort << "\n";
    json << "}\n";

    return json;
}

juce::String OSCQueryServer::buildInputChannelJson(int channelIndex)
{
    juce::String basePath = "/wfs/input/" + juce::String(channelIndex);
    juce::String json;

    json << "{\n";
    json << "              \"FULL_PATH\": \"" << basePath << "\",\n";
    json << "              \"ACCESS\": 0,\n";
    json << "              \"CONTENTS\": {\n";

    // Position X
    float posX = state.getFloatParameter(WFSParameterIDs::inputPositionX, channelIndex);
    json << "                \"positionX\": {\n";
    json << "                  \"FULL_PATH\": \"" << basePath << "/positionX\",\n";
    json << "                  \"TYPE\": \"f\",\n";
    json << "                  \"ACCESS\": 3,\n";
    json << "                  \"VALUE\": [" << posX << "],\n";
    json << "                  \"RANGE\": [{\"MIN\": 0, \"MAX\": 50}],\n";
    json << "                  \"DESCRIPTION\": \"X Position (meters)\"\n";
    json << "                },\n";

    // Position Y
    float posY = state.getFloatParameter(WFSParameterIDs::inputPositionY, channelIndex);
    json << "                \"positionY\": {\n";
    json << "                  \"FULL_PATH\": \"" << basePath << "/positionY\",\n";
    json << "                  \"TYPE\": \"f\",\n";
    json << "                  \"ACCESS\": 3,\n";
    json << "                  \"VALUE\": [" << posY << "],\n";
    json << "                  \"RANGE\": [{\"MIN\": 0, \"MAX\": 50}],\n";
    json << "                  \"DESCRIPTION\": \"Y Position (meters)\"\n";
    json << "                },\n";

    // Position Z
    float posZ = state.getFloatParameter(WFSParameterIDs::inputPositionZ, channelIndex);
    json << "                \"positionZ\": {\n";
    json << "                  \"FULL_PATH\": \"" << basePath << "/positionZ\",\n";
    json << "                  \"TYPE\": \"f\",\n";
    json << "                  \"ACCESS\": 3,\n";
    json << "                  \"VALUE\": [" << posZ << "],\n";
    json << "                  \"RANGE\": [{\"MIN\": 0, \"MAX\": 50}],\n";
    json << "                  \"DESCRIPTION\": \"Z Position (meters)\"\n";
    json << "                },\n";

    // Attenuation
    float atten = state.getFloatParameter(WFSParameterIDs::inputAttenuation, channelIndex);
    json << "                \"attenuation\": {\n";
    json << "                  \"FULL_PATH\": \"" << basePath << "/attenuation\",\n";
    json << "                  \"TYPE\": \"f\",\n";
    json << "                  \"ACCESS\": 3,\n";
    json << "                  \"VALUE\": [" << atten << "],\n";
    json << "                  \"RANGE\": [{\"MIN\": -92, \"MAX\": 0}],\n";
    json << "                  \"DESCRIPTION\": \"Attenuation (dB)\"\n";
    json << "                },\n";

    // Mute Macro (overall mute state)
    int muteMacro = state.getIntParameter(WFSParameterIDs::inputMuteMacro, channelIndex);
    json << "                \"muteMacro\": {\n";
    json << "                  \"FULL_PATH\": \"" << basePath << "/muteMacro\",\n";
    json << "                  \"TYPE\": \"i\",\n";
    json << "                  \"ACCESS\": 3,\n";
    json << "                  \"VALUE\": [" << muteMacro << "],\n";
    json << "                  \"RANGE\": [{\"MIN\": 0, \"MAX\": 4}],\n";
    json << "                  \"DESCRIPTION\": \"Mute macro (0=unmute all, 1=mute all, etc.)\"\n";
    json << "                }\n";

    json << "              }\n";
    json << "            }";

    return json;
}

juce::String OSCQueryServer::buildOutputChannelJson(int channelIndex)
{
    juce::String basePath = "/wfs/output/" + juce::String(channelIndex);
    juce::String json;

    json << "{\n";
    json << "              \"FULL_PATH\": \"" << basePath << "\",\n";
    json << "              \"ACCESS\": 0,\n";
    json << "              \"CONTENTS\": {\n";

    // Position X
    float posX = state.getFloatParameter(WFSParameterIDs::outputPositionX, channelIndex);
    json << "                \"positionX\": {\n";
    json << "                  \"FULL_PATH\": \"" << basePath << "/positionX\",\n";
    json << "                  \"TYPE\": \"f\",\n";
    json << "                  \"ACCESS\": 3,\n";
    json << "                  \"VALUE\": [" << posX << "],\n";
    json << "                  \"RANGE\": [{\"MIN\": -100, \"MAX\": 100}],\n";
    json << "                  \"DESCRIPTION\": \"X Position (meters)\"\n";
    json << "                },\n";

    // Position Y
    float posY = state.getFloatParameter(WFSParameterIDs::outputPositionY, channelIndex);
    json << "                \"positionY\": {\n";
    json << "                  \"FULL_PATH\": \"" << basePath << "/positionY\",\n";
    json << "                  \"TYPE\": \"f\",\n";
    json << "                  \"ACCESS\": 3,\n";
    json << "                  \"VALUE\": [" << posY << "],\n";
    json << "                  \"RANGE\": [{\"MIN\": -100, \"MAX\": 100}],\n";
    json << "                  \"DESCRIPTION\": \"Y Position (meters)\"\n";
    json << "                },\n";

    // Attenuation
    float atten = state.getFloatParameter(WFSParameterIDs::outputAttenuation, channelIndex);
    json << "                \"attenuation\": {\n";
    json << "                  \"FULL_PATH\": \"" << basePath << "/attenuation\",\n";
    json << "                  \"TYPE\": \"f\",\n";
    json << "                  \"ACCESS\": 3,\n";
    json << "                  \"VALUE\": [" << atten << "],\n";
    json << "                  \"RANGE\": [{\"MIN\": -92, \"MAX\": 0}],\n";
    json << "                  \"DESCRIPTION\": \"Attenuation (dB)\"\n";
    json << "                }\n";

    json << "              }\n";
    json << "            }";

    return json;
}

juce::String OSCQueryServer::buildNodeJson(const juce::String& path)
{
    // For specific paths, return node info
    // This is a simplified implementation - full impl would parse the path
    // and return appropriate node/parameter info

    juce::String json;
    json << "{\n";
    json << "  \"FULL_PATH\": \"" << path << "\",\n";
    json << "  \"ACCESS\": 0,\n";
    json << "  \"DESCRIPTION\": \"Node at " << path << "\"\n";
    json << "}\n";

    return json;
}

void OSCQueryServer::valueTreePropertyChanged(juce::ValueTree& /*tree*/,
                                              const juce::Identifier& /*property*/)
{
    // In a full implementation, we would update the JSON cache here
    // For now, we regenerate JSON on each request
}

} // namespace WFSNetwork
