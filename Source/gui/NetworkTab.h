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
 * - Network Interface Selector
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
        // Network Interface Selector
        addAndMakeVisible(networkInterfaceLabel);
        networkInterfaceLabel.setText("Network Interface:", juce::dontSendNotification);
        networkInterfaceLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(networkInterfaceSelector);
        networkInterfaceSelector.onChange = [this]() { onNetworkInterfaceChanged(); };

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

        // Footer buttons - Store/Reload
        addAndMakeVisible(storeButton);
        storeButton.setButtonText("Store Network Config");
        storeButton.onClick = [this]() { storeNetworkConfiguration(); };

        addAndMakeVisible(reloadButton);
        reloadButton.setButtonText("Reload Network Config");
        reloadButton.onClick = [this]() { reloadNetworkConfiguration(); };

        addAndMakeVisible(reloadBackupButton);
        reloadBackupButton.setButtonText("Reload Backup");
        reloadBackupButton.onClick = [this]() { reloadNetworkConfigBackup(); };

        addAndMakeVisible(importButton);
        importButton.setButtonText("Import");
        importButton.onClick = [this]() { importNetworkConfiguration(); };

        addAndMakeVisible(exportButton);
        exportButton.setButtonText("Export");
        exportButton.onClick = [this]() { exportNetworkConfiguration(); };

        // Populate network interfaces
        populateNetworkInterfaces();

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
        const int editorWidth = 250;
        const int rowHeight = 25;
        const int spacing = 5;

        int x = 20;
        int y = 60;

        networkInterfaceLabel.setBounds(x, y, labelWidth, rowHeight);
        networkInterfaceSelector.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        currentIPLabel.setBounds(x, y, labelWidth, rowHeight);
        currentIPEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        udpPortLabel.setBounds(x, y, labelWidth, rowHeight);
        udpPortEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        tcpPortLabel.setBounds(x, y, labelWidth, rowHeight);
        tcpPortEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);

        // Footer buttons - full width at bottom - 5 equal-width buttons (matching Output tab)
        const int footerHeight = 50;
        const int footerPadding = 10;
        auto footerArea = getLocalBounds().removeFromBottom(footerHeight).reduced(footerPadding, footerPadding);
        const int buttonWidth = (footerArea.getWidth() - spacing * 4) / 5;

        storeButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        reloadButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        reloadBackupButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        importButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        exportButton.setBounds(footerArea);
    }

private:
    WfsParameters& parameters;

    // Network Interface Section
    juce::Label networkInterfaceLabel;
    juce::ComboBox networkInterfaceSelector;
    juce::StringArray interfaceNames;
    juce::StringArray interfaceIPs;

    // Network Section
    juce::Label currentIPLabel;
    juce::TextEditor currentIPEditor;
    juce::Label udpPortLabel;
    juce::TextEditor udpPortEditor;
    juce::Label tcpPortLabel;
    juce::TextEditor tcpPortEditor;

    // Footer buttons
    juce::TextButton storeButton;
    juce::TextButton reloadButton;
    juce::TextButton reloadBackupButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;

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

        // Load saved network interface
        juce::String savedInterface = parameters.getConfigParam("NetworkInterface").toString();
        if (savedInterface.isNotEmpty())
        {
            int index = interfaceNames.indexOf(savedInterface);
            if (index >= 0)
                networkInterfaceSelector.setSelectedId(index + 1, juce::dontSendNotification);
        }
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

    void onNetworkInterfaceChanged()
    {
        int selectedId = networkInterfaceSelector.getSelectedId();
        if (selectedId > 0)
        {
            int index = selectedId - 1;
            if (index < interfaceNames.size())
            {
                // Save selected interface name to parameters
                parameters.setConfigParam("NetworkInterface", interfaceNames[index]);

                // Update current IP display
                if (index < interfaceIPs.size())
                    currentIPEditor.setText(interfaceIPs[index], false);
            }
        }
    }

    void populateNetworkInterfaces()
    {
        networkInterfaceSelector.clear();
        interfaceNames.clear();
        interfaceIPs.clear();

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
                    int interfaceIndex = 1;

                    while (pCurrAddresses)
                    {
                        // Skip loopback interfaces
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

                                    // Convert adapter name from wide string
                                    juce::String adapterName = juce::String(pCurrAddresses->FriendlyName);
                                    juce::String ipAddress = juce::String(ipBuffer);

                                    // Add to combo box with format: "Adapter Name (IP)"
                                    juce::String displayName = adapterName + " (" + ipAddress + ")";
                                    networkInterfaceSelector.addItem(displayName, interfaceIndex++);

                                    // Store for later use
                                    interfaceNames.add(adapterName);
                                    interfaceIPs.add(ipAddress);

                                    break;  // Only take first IPv4 address per adapter
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

        if (networkInterfaceSelector.getNumItems() == 0)
        {
            networkInterfaceSelector.addItem("No network adapters found", 1);
        }
#elif JUCE_MAC
        // macOS implementation placeholder
        networkInterfaceSelector.addItem("macOS network interface selection - Not yet implemented", 1);
        interfaceNames.add("macOS Placeholder");
        interfaceIPs.add("0.0.0.0");
#else
        networkInterfaceSelector.addItem("Network interface selection not supported on this platform", 1);
        interfaceNames.add("Unsupported");
        interfaceIPs.add("0.0.0.0");
#endif

        // Select first item by default if nothing saved
        if (networkInterfaceSelector.getSelectedId() == 0 && networkInterfaceSelector.getNumItems() > 0)
        {
            networkInterfaceSelector.setSelectedId(1, juce::sendNotification);
        }
    }

    void updateCurrentIP()
    {
        // If a network interface is selected, show its IP
        int selectedId = networkInterfaceSelector.getSelectedId();
        if (selectedId > 0)
        {
            int index = selectedId - 1;
            if (index < interfaceIPs.size())
            {
                currentIPEditor.setText(interfaceIPs[index], false);
                return;
            }
        }

        // Otherwise fall back to detecting any active interface
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

    // Store/Reload stub methods (to be implemented)
    void storeNetworkConfiguration() {}
    void reloadNetworkConfiguration() {}
    void reloadNetworkConfigBackup() {}
    void importNetworkConfiguration() {}
    void exportNetworkConfiguration() {}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkTab)
};
