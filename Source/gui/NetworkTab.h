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
 * Network Tab Component
 * Configuration for network settings:
 * - Current IP address
 * - UDP Port
 * - TCP Port
 */
class NetworkTab : public juce::Component,
                   private juce::ValueTree::Listener,
                   private juce::TextEditor::Listener
{
public:
    NetworkTab(WfsParameters& params)
        : parameters(params)
    {
        // Network Section
        addAndMakeVisible(currentIPLabel);
        currentIPLabel.setText("Current IPv4:", juce::dontSendNotification);
        currentIPLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(currentIPEditor);
        currentIPEditor.setReadOnly(true);

        addAndMakeVisible(udpPortLabel);
        udpPortLabel.setText("UDP Port:", juce::dontSendNotification);
        udpPortLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(udpPortEditor);

        addAndMakeVisible(tcpPortLabel);
        tcpPortLabel.setText("TCP Port:", juce::dontSendNotification);
        tcpPortLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(tcpPortEditor);

        // Setup numeric input filtering
        setupNumericEditors();

        // Add text editor listeners
        udpPortEditor.addListener(this);
        tcpPortEditor.addListener(this);

        // Load initial values from parameters
        loadParametersFromValueTree();

        // Listen to parameter changes
        parameters.getConfigTree().addListener(this);

        // Update current IP address
        updateCurrentIP();
    }

    ~NetworkTab() override
    {
        parameters.getConfigTree().removeListener(this);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        // Draw section header
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions().withHeight(16.0f).withStyle("Bold"));
        g.drawText("Network Configuration", 20, 20, 200, 20, juce::Justification::left);
    }

    void resized() override
    {
        const int labelWidth = 150;
        const int editorWidth = 150;
        const int rowHeight = 25;
        const int spacing = 5;

        int x = 20;
        int y = 60;

        currentIPLabel.setBounds(x, y, labelWidth, rowHeight);
        currentIPEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        udpPortLabel.setBounds(x, y, labelWidth, rowHeight);
        udpPortEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        tcpPortLabel.setBounds(x, y, labelWidth, rowHeight);
        tcpPortEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
    }

private:
    WfsParameters& parameters;

    // Network Section
    juce::Label currentIPLabel;
    juce::TextEditor currentIPEditor;
    juce::Label udpPortLabel;
    juce::TextEditor udpPortEditor;
    juce::Label tcpPortLabel;
    juce::TextEditor tcpPortEditor;

    void setupNumericEditors()
    {
        // Port editors - integers only
        udpPortEditor.setInputRestrictions(5, "0123456789");
        tcpPortEditor.setInputRestrictions(5, "0123456789");
    }

    void loadParametersFromValueTree()
    {
        udpPortEditor.setText(juce::String((int)parameters.getConfigParam("UDPPort")), false);
        tcpPortEditor.setText(juce::String((int)parameters.getConfigParam("TCPPort")), false);
    }

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (tree == parameters.getConfigTree())
        {
            if (property == juce::Identifier("UDPPort"))
                udpPortEditor.setText(juce::String((int)parameters.getConfigParam("UDPPort")), false);
            else if (property == juce::Identifier("TCPPort"))
                tcpPortEditor.setText(juce::String((int)parameters.getConfigParam("TCPPort")), false);
        }
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    void textEditorTextChanged(juce::TextEditor& editor) override
    {
        updateParameterFromEditor(&editor);
    }

    void textEditorReturnKeyPressed(juce::TextEditor&) override {}
    void textEditorEscapeKeyPressed(juce::TextEditor&) override {}
    void textEditorFocusLost(juce::TextEditor&) override {}

    void updateParameterFromEditor(juce::TextEditor* editor)
    {
        juce::String text = editor->getText();

        if (editor == &udpPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
                parameters.setConfigParam("UDPPort", value);
        }
        else if (editor == &tcpPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
                parameters.setConfigParam("TCPPort", value);
        }
    }

    void updateCurrentIP()
    {
#if JUCE_WINDOWS
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
        {
            PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
            ULONG outBufLen = 15000;
            ULONG family = AF_INET;  // IPv4

            for (int attempts = 0; attempts < 3; attempts++)
            {
                pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
                if (pAddresses == nullptr)
                    break;

                DWORD result = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);

                if (result == ERROR_BUFFER_OVERFLOW)
                {
                    free(pAddresses);
                    pAddresses = nullptr;
                    continue;
                }

                if (result == NO_ERROR)
                {
                    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
                    while (pCurrAddresses)
                    {
                        if (pCurrAddresses->OperStatus == IfOperStatusUp &&
                            pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
                        {
                            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                            while (pUnicast)
                            {
                                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                                {
                                    sockaddr_in* sa_in = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                                    char ipBuffer[INET_ADDRSTRLEN];
                                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipBuffer, INET_ADDRSTRLEN);
                                    currentIPEditor.setText(juce::String(ipBuffer), false);
                                    free(pAddresses);
                                    WSACleanup();
                                    return;
                                }
                                pUnicast = pUnicast->Next;
                            }
                        }
                        pCurrAddresses = pCurrAddresses->Next;
                    }
                }
                break;
            }

            if (pAddresses)
                free(pAddresses);
            WSACleanup();
        }
#endif
        currentIPEditor.setText("Not available", false);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkTab)
};
