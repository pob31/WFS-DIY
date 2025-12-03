#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"

#if JUCE_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#endif

/**
 * Config Tab UI Component
 * Based on WFS-UI_config.csv specification (65 parameters)
 *
 * Sections:
 * - Show (name, location)
 * - I/O (input/output/reverb channels, audio interface, processing toggle)
 * - Stage (dimensions, origin, speed of sound, temperature)
 * - Master Section (level, latency, Haas effect)
 * - Network (IP, ports, targets/servers)
 * - ADM-OSC (offset, scale, flip)
 * - Tracking (protocol, port, offset, scale, flip)
 * - Store/Reload (save/load buttons)
 */
class ConfigTabComponent : public juce::Component,
                           private juce::ValueTree::Listener,
                           private juce::TextEditor::Listener
{
public:
    ConfigTabComponent(WfsParameters& params)
        : parameters(params)
    {
        // Show Section
        addAndMakeVisible(showNameLabel);
        showNameLabel.setText("Name:", juce::dontSendNotification);
        addAndMakeVisible(showNameEditor);

        addAndMakeVisible(showLocationLabel);
        showLocationLabel.setText("Location:", juce::dontSendNotification);
        addAndMakeVisible(showLocationEditor);

        // I/O Section
        addAndMakeVisible(inputChannelsLabel);
        inputChannelsLabel.setText("Input Channels:", juce::dontSendNotification);
        addAndMakeVisible(inputChannelsEditor);

        addAndMakeVisible(outputChannelsLabel);
        outputChannelsLabel.setText("Output Channels:", juce::dontSendNotification);
        addAndMakeVisible(outputChannelsEditor);

        addAndMakeVisible(reverbChannelsLabel);
        reverbChannelsLabel.setText("Reverb Channels:", juce::dontSendNotification);
        addAndMakeVisible(reverbChannelsEditor);

        addAndMakeVisible(audioPatchingButton);
        audioPatchingButton.setButtonText("Audio Interface and Patching Window");

        addAndMakeVisible(processingToggle);
        processingToggle.setButtonText("Processing");
        processingToggle.setClickingTogglesState(true);

        // Stage Section
        addAndMakeVisible(stageWidthLabel);
        stageWidthLabel.setText("Stage Width:", juce::dontSendNotification);
        stageWidthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageWidthEditor);
        addAndMakeVisible(stageWidthUnitLabel);
        stageWidthUnitLabel.setText("m", juce::dontSendNotification);
        stageWidthUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageDepthLabel);
        stageDepthLabel.setText("Stage Depth:", juce::dontSendNotification);
        stageDepthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageDepthEditor);
        addAndMakeVisible(stageDepthUnitLabel);
        stageDepthUnitLabel.setText("m", juce::dontSendNotification);
        stageDepthUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageHeightLabel);
        stageHeightLabel.setText("Stage Height:", juce::dontSendNotification);
        stageHeightLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageHeightEditor);
        addAndMakeVisible(stageHeightUnitLabel);
        stageHeightUnitLabel.setText("m", juce::dontSendNotification);
        stageHeightUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageOriginWidthLabel);
        stageOriginWidthLabel.setText("Origin Width:", juce::dontSendNotification);
        stageOriginWidthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageOriginWidthEditor);
        addAndMakeVisible(stageOriginWidthUnitLabel);
        stageOriginWidthUnitLabel.setText("m", juce::dontSendNotification);
        stageOriginWidthUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageOriginDepthLabel);
        stageOriginDepthLabel.setText("Origin Depth:", juce::dontSendNotification);
        stageOriginDepthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageOriginDepthEditor);
        addAndMakeVisible(stageOriginDepthUnitLabel);
        stageOriginDepthUnitLabel.setText("m", juce::dontSendNotification);
        stageOriginDepthUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageOriginHeightLabel);
        stageOriginHeightLabel.setText("Origin Height:", juce::dontSendNotification);
        stageOriginHeightLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageOriginHeightEditor);
        addAndMakeVisible(stageOriginHeightUnitLabel);
        stageOriginHeightUnitLabel.setText("m", juce::dontSendNotification);
        stageOriginHeightUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(speedOfSoundLabel);
        speedOfSoundLabel.setText("Speed of Sound:", juce::dontSendNotification);
        speedOfSoundLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(speedOfSoundEditor);
        addAndMakeVisible(speedOfSoundUnitLabel);
        speedOfSoundUnitLabel.setText("m/s", juce::dontSendNotification);
        speedOfSoundUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(temperatureLabel);
        temperatureLabel.setText("Temperature:", juce::dontSendNotification);
        temperatureLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(temperatureEditor);
        addAndMakeVisible(temperatureUnitLabel);
        temperatureUnitLabel.setText("C", juce::dontSendNotification);
        temperatureUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Master Section
        addAndMakeVisible(masterLevelLabel);
        masterLevelLabel.setText("Master Level:", juce::dontSendNotification);
        masterLevelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(masterLevelEditor);
        addAndMakeVisible(masterLevelUnitLabel);
        masterLevelUnitLabel.setText("dB", juce::dontSendNotification);
        masterLevelUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(systemLatencyLabel);
        systemLatencyLabel.setText("System Latency:", juce::dontSendNotification);
        systemLatencyLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(systemLatencyEditor);
        addAndMakeVisible(systemLatencyUnitLabel);
        systemLatencyUnitLabel.setText("ms", juce::dontSendNotification);
        systemLatencyUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(haasEffectLabel);
        haasEffectLabel.setText("Haas Effect:", juce::dontSendNotification);
        haasEffectLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(haasEffectEditor);
        addAndMakeVisible(haasEffectUnitLabel);
        haasEffectUnitLabel.setText("ms", juce::dontSendNotification);
        haasEffectUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Network Section
        addAndMakeVisible(networkInterfaceLabel);
        networkInterfaceLabel.setText("Network Interface:", juce::dontSendNotification);
        addAndMakeVisible(networkInterfaceCombo);
        networkInterfaceCombo.onChange = [this]() { updateCurrentIPAddress(); };

        addAndMakeVisible(currentIPLabel);
        currentIPLabel.setText("Current IPv4:", juce::dontSendNotification);
        addAndMakeVisible(currentIPEditor);
        currentIPEditor.setReadOnly(true);

        addAndMakeVisible(udpPortLabel);
        udpPortLabel.setText("UDP Port:", juce::dontSendNotification);
        addAndMakeVisible(udpPortEditor);

        addAndMakeVisible(tcpPortLabel);
        tcpPortLabel.setText("TCP Port:", juce::dontSendNotification);
        addAndMakeVisible(tcpPortEditor);

        addAndMakeVisible(networkLogButton);
        networkLogButton.setButtonText("Open Log Window");

        // Store/Reload Section
        addAndMakeVisible(selectProjectFolderButton);
        selectProjectFolderButton.setButtonText("Select Project Folder");

        addAndMakeVisible(storeCompleteConfigButton);
        storeCompleteConfigButton.setButtonText("Store Complete Configuration");

        addAndMakeVisible(reloadCompleteConfigButton);
        reloadCompleteConfigButton.setButtonText("Reload Complete Configuration");

        addAndMakeVisible(storeSystemConfigButton);
        storeSystemConfigButton.setButtonText("Store System Configuration");

        addAndMakeVisible(reloadSystemConfigButton);
        reloadSystemConfigButton.setButtonText("Reload System Configuration");

        // Set up text editor listeners
        showNameEditor.addListener(this);
        showLocationEditor.addListener(this);
        inputChannelsEditor.addListener(this);
        outputChannelsEditor.addListener(this);
        reverbChannelsEditor.addListener(this);
        stageWidthEditor.addListener(this);
        stageDepthEditor.addListener(this);
        stageHeightEditor.addListener(this);
        stageOriginWidthEditor.addListener(this);
        stageOriginDepthEditor.addListener(this);
        stageOriginHeightEditor.addListener(this);
        speedOfSoundEditor.addListener(this);
        temperatureEditor.addListener(this);
        masterLevelEditor.addListener(this);
        systemLatencyEditor.addListener(this);
        haasEffectEditor.addListener(this);
        udpPortEditor.addListener(this);
        tcpPortEditor.addListener(this);

        // Set up button callbacks
        selectProjectFolderButton.onClick = [this]() { selectProjectFolder(); };
        storeCompleteConfigButton.onClick = [this]() { saveCompleteConfig(); };
        reloadCompleteConfigButton.onClick = [this]() { loadCompleteConfig(); };
        storeSystemConfigButton.onClick = [this]() { saveSystemConfig(); };
        reloadSystemConfigButton.onClick = [this]() { loadSystemConfig(); };

        // Listen to parameter changes
        parameters.getConfigTree().addListener(this);

        // Load initial values from parameters
        loadParametersToUI();

        // Populate network interfaces and update IP address
        populateNetworkInterfaces();
        updateCurrentIPAddress();

        setSize(1400, 700);
    }

    ~ConfigTabComponent() override
    {
        parameters.getConfigTree().removeListener(this);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);

        // Draw section headers
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);

        // Left column headers (x=20, y positions calculated from layout)
        g.drawText("Show", 20, 20, 200, 30, juce::Justification::left);
        g.drawText("I/O", 20, 130, 200, 30, juce::Justification::left);
        g.drawText("Store/Reload", 20, 365, 200, 30, juce::Justification::left);

        // Middle column headers (x=480)
        g.drawText("Stage", 480, 20, 200, 30, juce::Justification::left);
        g.drawText("Master Section", 480, 350, 200, 30, juce::Justification::left);

        // Right column headers (x=940)
        g.drawText("Network", 940, 20, 200, 30, juce::Justification::left);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);

        // Create three columns
        auto leftColumn = area.removeFromLeft(440);
        area.removeFromLeft(20); // spacing
        auto middleColumn = area.removeFromLeft(440);
        area.removeFromLeft(20); // spacing
        auto rightColumn = area;

        // LEFT COLUMN
        // Show Section
        leftColumn.removeFromTop(40); // Skip header
        auto row = leftColumn.removeFromTop(30);
        showNameLabel.setBounds(row.removeFromLeft(120));
        showNameEditor.setBounds(row.removeFromLeft(300));

        leftColumn.removeFromTop(5);
        row = leftColumn.removeFromTop(30);
        showLocationLabel.setBounds(row.removeFromLeft(120));
        showLocationEditor.setBounds(row.removeFromLeft(300));

        // I/O Section
        leftColumn.removeFromTop(25);
        row = leftColumn.removeFromTop(30);
        inputChannelsLabel.setBounds(row.removeFromLeft(140));
        inputChannelsEditor.setBounds(row.removeFromLeft(100));

        leftColumn.removeFromTop(5);
        row = leftColumn.removeFromTop(30);
        outputChannelsLabel.setBounds(row.removeFromLeft(140));
        outputChannelsEditor.setBounds(row.removeFromLeft(100));

        leftColumn.removeFromTop(5);
        row = leftColumn.removeFromTop(30);
        reverbChannelsLabel.setBounds(row.removeFromLeft(140));
        reverbChannelsEditor.setBounds(row.removeFromLeft(100));

        leftColumn.removeFromTop(10);
        audioPatchingButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(350));

        leftColumn.removeFromTop(10);
        processingToggle.setBounds(leftColumn.removeFromTop(30).removeFromLeft(200));

        // Store/Reload Section
        leftColumn.removeFromTop(45);
        selectProjectFolderButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(250));

        leftColumn.removeFromTop(10);
        storeCompleteConfigButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(300));

        leftColumn.removeFromTop(5);
        reloadCompleteConfigButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(300));

        leftColumn.removeFromTop(10);
        storeSystemConfigButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(300));

        leftColumn.removeFromTop(5);
        reloadSystemConfigButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(300));

        // MIDDLE COLUMN
        // Stage Section
        middleColumn.removeFromTop(40);
        const int labelWidth = 140; // Width for labels
        const int editorWidth = 80;
        const int unitOffset = 5;
        const int unitWidth = 40;

        row = middleColumn.removeFromTop(30);
        stageWidthLabel.setBounds(row.removeFromLeft(labelWidth));
        stageWidthEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        stageWidthUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        stageDepthLabel.setBounds(row.removeFromLeft(labelWidth));
        stageDepthEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        stageDepthUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        stageHeightLabel.setBounds(row.removeFromLeft(labelWidth));
        stageHeightEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        stageHeightUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        stageOriginWidthLabel.setBounds(row.removeFromLeft(labelWidth));
        stageOriginWidthEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        stageOriginWidthUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        stageOriginDepthLabel.setBounds(row.removeFromLeft(labelWidth));
        stageOriginDepthEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        stageOriginDepthUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        stageOriginHeightLabel.setBounds(row.removeFromLeft(labelWidth));
        stageOriginHeightEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        stageOriginHeightUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        speedOfSoundLabel.setBounds(row.removeFromLeft(labelWidth));
        speedOfSoundEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        speedOfSoundUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        temperatureLabel.setBounds(row.removeFromLeft(labelWidth));
        temperatureEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        temperatureUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        // Master Section
        middleColumn.removeFromTop(55);
        row = middleColumn.removeFromTop(30);
        masterLevelLabel.setBounds(row.removeFromLeft(labelWidth));
        masterLevelEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        masterLevelUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        systemLatencyLabel.setBounds(row.removeFromLeft(labelWidth));
        systemLatencyEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        systemLatencyUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        haasEffectLabel.setBounds(row.removeFromLeft(labelWidth));
        haasEffectEditor.setBounds(row.removeFromLeft(editorWidth));
        row.removeFromLeft(unitOffset);
        haasEffectUnitLabel.setBounds(row.removeFromLeft(unitWidth));

        // RIGHT COLUMN
        // Network Section
        rightColumn.removeFromTop(40);
        row = rightColumn.removeFromTop(30);
        networkInterfaceLabel.setBounds(row.removeFromLeft(140));
        networkInterfaceCombo.setBounds(row.removeFromLeft(200));

        rightColumn.removeFromTop(5);
        row = rightColumn.removeFromTop(30);
        currentIPLabel.setBounds(row.removeFromLeft(140));
        currentIPEditor.setBounds(row.removeFromLeft(200));

        rightColumn.removeFromTop(5);
        row = rightColumn.removeFromTop(30);
        udpPortLabel.setBounds(row.removeFromLeft(140));
        udpPortEditor.setBounds(row.removeFromLeft(100));

        rightColumn.removeFromTop(5);
        row = rightColumn.removeFromTop(30);
        tcpPortLabel.setBounds(row.removeFromLeft(140));
        tcpPortEditor.setBounds(row.removeFromLeft(100));

        rightColumn.removeFromTop(10);
        networkLogButton.setBounds(rightColumn.removeFromTop(30).removeFromLeft(200));
    }

private:
    // Show Section
    juce::Label showNameLabel, showLocationLabel;
    juce::TextEditor showNameEditor, showLocationEditor;

    // I/O Section
    juce::Label inputChannelsLabel, outputChannelsLabel, reverbChannelsLabel;
    juce::TextEditor inputChannelsEditor, outputChannelsEditor, reverbChannelsEditor;
    juce::TextButton audioPatchingButton;
    juce::ToggleButton processingToggle;

    // Stage Section
    juce::Label stageWidthLabel, stageDepthLabel, stageHeightLabel;
    juce::Label stageOriginWidthLabel, stageOriginDepthLabel, stageOriginHeightLabel;
    juce::Label speedOfSoundLabel, temperatureLabel;
    juce::TextEditor stageWidthEditor, stageDepthEditor, stageHeightEditor;
    juce::TextEditor stageOriginWidthEditor, stageOriginDepthEditor, stageOriginHeightEditor;
    juce::TextEditor speedOfSoundEditor, temperatureEditor;
    juce::Label stageWidthUnitLabel, stageDepthUnitLabel, stageHeightUnitLabel;
    juce::Label stageOriginWidthUnitLabel, stageOriginDepthUnitLabel, stageOriginHeightUnitLabel;
    juce::Label speedOfSoundUnitLabel, temperatureUnitLabel;

    // Master Section
    juce::Label masterLevelLabel, systemLatencyLabel, haasEffectLabel;
    juce::TextEditor masterLevelEditor, systemLatencyEditor, haasEffectEditor;
    juce::Label masterLevelUnitLabel, systemLatencyUnitLabel, haasEffectUnitLabel;

    // Network Section
    juce::Label networkInterfaceLabel, currentIPLabel, udpPortLabel, tcpPortLabel;
    juce::ComboBox networkInterfaceCombo;
    juce::TextEditor currentIPEditor, udpPortEditor, tcpPortEditor;
    juce::TextButton networkLogButton;

    // Store/Reload Section
    juce::TextButton selectProjectFolderButton;
    juce::TextButton storeCompleteConfigButton, reloadCompleteConfigButton;
    juce::TextButton storeSystemConfigButton, reloadSystemConfigButton;

    // Parameter system
    WfsParameters& parameters;
    juce::File projectFolder;

    //==============================================================================
    // ValueTree::Listener implementation
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        // Update UI when parameters change from elsewhere
        juce::MessageManager::callAsync([this]() { loadParametersToUI(); });
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    //==============================================================================
    // TextEditor::Listener implementation
    void textEditorTextChanged(juce::TextEditor& editor) override
    {
        updateParameterFromEditor(editor);
    }

    void textEditorReturnKeyPressed(juce::TextEditor&) override {}
    void textEditorEscapeKeyPressed(juce::TextEditor&) override {}
    void textEditorFocusLost(juce::TextEditor&) override {}

    //==============================================================================
    // Helper methods

    struct NetworkInterfaceInfo
    {
        juce::String name;
        juce::String ipAddress;
    };

    juce::Array<NetworkInterfaceInfo> getNetworkInterfaces()
    {
        juce::Array<NetworkInterfaceInfo> interfaces;

        #if JUCE_WINDOWS
        // Use Windows API to get adapter information
        ULONG bufferSize = 15000;
        std::vector<BYTE> buffer(bufferSize);
        PIP_ADAPTER_ADDRESSES adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

        ULONG result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr,
                                           adapterAddresses, &bufferSize);

        if (result == ERROR_BUFFER_OVERFLOW)
        {
            buffer.resize(bufferSize);
            adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
            result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr,
                                         adapterAddresses, &bufferSize);
        }

        if (result == NO_ERROR)
        {
            for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter != nullptr; adapter = adapter->Next)
            {
                // Skip non-operational adapters
                if (adapter->OperStatus != IfOperStatusUp)
                    continue;

                for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress;
                     unicast != nullptr; unicast = unicast->Next)
                {
                    if (unicast->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        SOCKADDR_IN* sockaddr = reinterpret_cast<SOCKADDR_IN*>(unicast->Address.lpSockaddr);
                        char ipStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(sockaddr->sin_addr), ipStr, INET_ADDRSTRLEN);

                        NetworkInterfaceInfo info;
                        info.name = juce::String(adapter->FriendlyName);
                        info.ipAddress = juce::String(ipStr);
                        interfaces.add(info);
                    }
                }
            }
        }
        #elif JUCE_MAC
        // TODO: Implement macOS network interface enumeration
        // Use getifaddrs() to get interface names with IP addresses
        // Example approach:
        //   struct ifaddrs* ifAddrStruct = nullptr;
        //   getifaddrs(&ifAddrStruct);
        //   for (struct ifaddrs* ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
        //   {
        //       if (ifa->ifa_addr->sa_family == AF_INET && (ifa->ifa_flags & IFF_UP))
        //       {
        //           NetworkInterfaceInfo info;
        //           info.name = juce::String(ifa->ifa_name);  // e.g., "en0", "en1"
        //           char addressBuffer[INET_ADDRSTRLEN];
        //           inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr,
        //                    addressBuffer, INET_ADDRSTRLEN);
        //           info.ipAddress = juce::String(addressBuffer);
        //           interfaces.add(info);
        //       }
        //   }
        //   if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
        //
        // For now, fallback to IP addresses only
        juce::Array<juce::IPAddress> addresses;
        juce::IPAddress::findAllAddresses(addresses, false);

        for (const auto& addr : addresses)
        {
            NetworkInterfaceInfo info;
            info.name = addr.toString();
            info.ipAddress = addr.toString();
            interfaces.add(info);
        }
        #else
        // Fallback to IP addresses only on other platforms
        juce::Array<juce::IPAddress> addresses;
        juce::IPAddress::findAllAddresses(addresses, false);

        for (const auto& addr : addresses)
        {
            NetworkInterfaceInfo info;
            info.name = addr.toString();
            info.ipAddress = addr.toString();
            interfaces.add(info);
        }
        #endif

        return interfaces;
    }

    void populateNetworkInterfaces()
    {
        auto interfaces = getNetworkInterfaces();

        networkInterfaceCombo.clear();

        int itemId = 1;
        int firstNonLoopbackId = -1;

        for (const auto& iface : interfaces)
        {
            juce::String displayName;

            if (iface.ipAddress == "127.0.0.1")
                displayName = "Loopback (" + iface.ipAddress + ")";
            else
            {
                displayName = iface.name + " (" + iface.ipAddress + ")";
                if (firstNonLoopbackId == -1)
                    firstNonLoopbackId = itemId;
            }

            networkInterfaceCombo.addItem(displayName, itemId++);
        }

        // Select first non-loopback interface, or first interface if only loopback exists
        if (firstNonLoopbackId != -1)
            networkInterfaceCombo.setSelectedId(firstNonLoopbackId);
        else if (networkInterfaceCombo.getNumItems() > 0)
            networkInterfaceCombo.setSelectedId(1);
    }

    void updateCurrentIPAddress()
    {
        int selectedId = networkInterfaceCombo.getSelectedId();
        if (selectedId <= 0)
        {
            currentIPEditor.setText("127.0.0.1", false);
            parameters.setConfigParam("CurrentIPv4", "127.0.0.1");
            return;
        }

        auto interfaces = getNetworkInterfaces();

        int itemId = 1;
        for (const auto& iface : interfaces)
        {
            if (itemId == selectedId)
            {
                currentIPEditor.setText(iface.ipAddress, false);
                parameters.setConfigParam("CurrentIPv4", iface.ipAddress);
                return;
            }
            itemId++;
        }

        // Fallback
        currentIPEditor.setText("127.0.0.1", false);
        parameters.setConfigParam("CurrentIPv4", "127.0.0.1");
    }

    void loadParametersToUI()
    {
        showNameEditor.setText(parameters.getConfigParam("ShowName").toString(), false);
        showLocationEditor.setText(parameters.getConfigParam("ShowLocation").toString(), false);
        inputChannelsEditor.setText(parameters.getConfigParam("InputChannels").toString(), false);
        outputChannelsEditor.setText(parameters.getConfigParam("OutputChannels").toString(), false);
        reverbChannelsEditor.setText(parameters.getConfigParam("ReverbChannels").toString(), false);
        stageWidthEditor.setText(parameters.getConfigParam("StageWidth").toString(), false);
        stageDepthEditor.setText(parameters.getConfigParam("StageDepth").toString(), false);
        stageHeightEditor.setText(parameters.getConfigParam("StageHeight").toString(), false);
        stageOriginWidthEditor.setText(parameters.getConfigParam("StageOriginWidth").toString(), false);
        stageOriginDepthEditor.setText(parameters.getConfigParam("StageOriginDepth").toString(), false);
        stageOriginHeightEditor.setText(parameters.getConfigParam("StageOriginHeight").toString(), false);
        speedOfSoundEditor.setText(parameters.getConfigParam("SpeedOfSound").toString(), false);
        temperatureEditor.setText(parameters.getConfigParam("Temperature").toString(), false);
        masterLevelEditor.setText(parameters.getConfigParam("MasterLevel").toString(), false);
        systemLatencyEditor.setText(parameters.getConfigParam("SystemLatency").toString(), false);
        haasEffectEditor.setText(parameters.getConfigParam("HaasEffect").toString(), false);
        currentIPEditor.setText(parameters.getConfigParam("CurrentIPv4").toString(), false);
        udpPortEditor.setText(parameters.getConfigParam("UdpPort").toString(), false);
        tcpPortEditor.setText(parameters.getConfigParam("TcpPort").toString(), false);
    }

    void updateParameterFromEditor(juce::TextEditor& editor)
    {
        auto text = editor.getText();

        if (&editor == &showNameEditor)
            parameters.setConfigParam("ShowName", text);
        else if (&editor == &showLocationEditor)
            parameters.setConfigParam("ShowLocation", text);
        else if (&editor == &inputChannelsEditor)
            parameters.setConfigParam("InputChannels", text.getIntValue());
        else if (&editor == &outputChannelsEditor)
            parameters.setConfigParam("OutputChannels", text.getIntValue());
        else if (&editor == &reverbChannelsEditor)
            parameters.setConfigParam("ReverbChannels", text.getIntValue());
        else if (&editor == &stageWidthEditor)
            parameters.setConfigParam("StageWidth", text.getFloatValue());
        else if (&editor == &stageDepthEditor)
            parameters.setConfigParam("StageDepth", text.getFloatValue());
        else if (&editor == &stageHeightEditor)
            parameters.setConfigParam("StageHeight", text.getFloatValue());
        else if (&editor == &stageOriginWidthEditor)
            parameters.setConfigParam("StageOriginWidth", text.getFloatValue());
        else if (&editor == &stageOriginDepthEditor)
            parameters.setConfigParam("StageOriginDepth", text.getFloatValue());
        else if (&editor == &stageOriginHeightEditor)
            parameters.setConfigParam("StageOriginHeight", text.getFloatValue());
        else if (&editor == &speedOfSoundEditor)
            parameters.setConfigParam("SpeedOfSound", text.getFloatValue());
        else if (&editor == &temperatureEditor)
            parameters.setConfigParam("Temperature", text.getFloatValue());
        else if (&editor == &masterLevelEditor)
            parameters.setConfigParam("MasterLevel", text.getFloatValue());
        else if (&editor == &systemLatencyEditor)
            parameters.setConfigParam("SystemLatency", text.getFloatValue());
        else if (&editor == &haasEffectEditor)
            parameters.setConfigParam("HaasEffect", text.getFloatValue());
        else if (&editor == &udpPortEditor)
            parameters.setConfigParam("UdpPort", text.getIntValue());
        else if (&editor == &tcpPortEditor)
            parameters.setConfigParam("TcpPort", text.getIntValue());
    }

    //==============================================================================
    // Save/Load methods
    juce::File getDefaultSaveLocation(const juce::String& filename) const
    {
        // Use project folder if set, otherwise use user's Documents folder
        if (projectFolder != juce::File{})
            return projectFolder.getChildFile(filename);
        else
            return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                       .getChildFile("WFS-DIY").getChildFile(filename);
    }

    juce::File getDefaultLoadLocation() const
    {
        // Use project folder if set, otherwise use user's Documents folder
        if (projectFolder != juce::File{})
            return projectFolder;
        else
            return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                       .getChildFile("WFS-DIY");
    }

    void saveCompleteConfig()
    {
        auto file = getDefaultSaveLocation("complete_config.xml");

        if (parameters.saveCompleteConfig(file))
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "Success", "Complete configuration saved to:\n" + file.getFullPathName());
        else
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Error", "Failed to save complete configuration");
    }

    void loadCompleteConfig()
    {
        auto file = getDefaultSaveLocation("complete_config.xml");

        if (parameters.loadCompleteConfig(file))
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "Success", "Complete configuration loaded from:\n" + file.getFullPathName());
        else
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Error", "Failed to load complete configuration");
    }

    void saveSystemConfig()
    {
        auto file = getDefaultSaveLocation("system_config.xml");

        if (parameters.saveSystemConfig(file))
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "Success", "System configuration saved to:\n" + file.getFullPathName());
        else
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Error", "Failed to save system configuration");
    }

    void loadSystemConfig()
    {
        auto file = getDefaultSaveLocation("system_config.xml");

        if (parameters.loadSystemConfig(file))
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "Success", "System configuration loaded from:\n" + file.getFullPathName());
        else
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Error", "Failed to load system configuration");
    }

    void selectProjectFolder()
    {
        auto defaultFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        auto chooser = std::make_shared<juce::FileChooser>("Select Project Folder",
                                                             defaultFolder,
                                                             "");

        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto folder = fc.getResult();
                if (folder != juce::File{} && folder.isDirectory())
                {
                    projectFolder = folder;
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                        "Success", "Project folder set to:\n" + folder.getFullPathName());
                }
            });
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigTabComponent)
};
