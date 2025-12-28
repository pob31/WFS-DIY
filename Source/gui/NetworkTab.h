#pragma once

#include <JuceHeader.h>
#include <map>
#include "../WfsParameters.h"
#include "StatusBar.h"
#include "../Network/OSCManager.h"

#if JUCE_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#elif JUCE_MAC
    #include <ifaddrs.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <net/if.h>
    #include <SystemConfiguration/SystemConfiguration.h>
    #include <CoreFoundation/CoreFoundation.h>
#endif

/**
 * Network Tab Component
 * Configuration for network settings:
 * - Network Interface Selector
 * - Current IP address
 * - UDP/TCP Ports
 * - Network Connections Table (up to 6 targets/servers)
 * - ADM-OSC settings (Offset, Scale, Flip)
 * - Tracking settings (Enable, Protocol, Port, Offset, Scale, Flip)
 */
class NetworkTab : public juce::Component,
                   private juce::ValueTree::Listener,
                   private juce::TextEditor::Listener
{
public:
    using NetworkLogWindowCallback = std::function<void()>;
    NetworkTab(WfsParameters& params, StatusBar* statusBarPtr = nullptr)
        : parameters(params), statusBar(statusBarPtr)
    {
        // ==================== NETWORK SECTION ====================
        addAndMakeVisible(networkInterfaceLabel);
        networkInterfaceLabel.setText("Network Interface:", juce::dontSendNotification);
        networkInterfaceLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(networkInterfaceSelector);
        networkInterfaceSelector.onChange = [this]() { onNetworkInterfaceChanged(); };

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

        // ==================== OSC QUERY ====================
        addAndMakeVisible(oscQueryLabel);
        oscQueryLabel.setText("OSC Query:", juce::dontSendNotification);
        oscQueryLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(oscQueryPortEditor);
        oscQueryPortEditor.setText("5005");
        oscQueryPortEditor.setInputRestrictions(5, "0123456789");
        oscQueryPortEditor.addListener(this);

        addAndMakeVisible(oscQueryEnableButton);
        oscQueryEnableButton.setButtonText("Disabled");
        oscQueryEnableButton.setClickingTogglesState(true);
        oscQueryEnableButton.onClick = [this]() {
            bool enabled = oscQueryEnableButton.getToggleState();
            oscQueryEnableButton.setButtonText(enabled ? "Enabled" : "Disabled");
            saveOscQueryToValueTree();
            updateOSCQueryServer();
        };

        // ==================== NETWORK CONNECTIONS TABLE ====================
        setupNetworkConnectionsTable();

        // ==================== ADM-OSC SECTION ====================
        setupAdmOscSection();

        // ==================== TRACKING SECTION ====================
        setupTrackingSection();

        // ==================== FOOTER BUTTONS ====================
        addAndMakeVisible(storeButton);
        storeButton.setButtonText("Store Network Config");
        storeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        storeButton.onClick = [this]() { storeNetworkConfiguration(); };

        addAndMakeVisible(reloadButton);
        reloadButton.setButtonText("Reload Network Config");
        reloadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        reloadButton.onClick = [this]() { reloadNetworkConfiguration(); };

        addAndMakeVisible(reloadBackupButton);
        reloadBackupButton.setButtonText("Reload Backup");
        reloadBackupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF266626));  // Darker green
        reloadBackupButton.onClick = [this]() { reloadNetworkConfigBackup(); };

        addAndMakeVisible(importButton);
        importButton.setButtonText("Import");
        importButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        importButton.onClick = [this]() { importNetworkConfiguration(); };

        addAndMakeVisible(exportButton);
        exportButton.setButtonText("Export");
        exportButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        exportButton.onClick = [this]() { exportNetworkConfiguration(); };

        // Setup numeric input filtering
        setupNumericEditors();

        // Add text editor listeners
        udpPortEditor.addListener(this);
        tcpPortEditor.addListener(this);

        // Populate network interfaces
        populateNetworkInterfaces();

        // Load initial values from parameters
        loadParametersFromValueTree();

        // Listen to parameter changes
        configTree = parameters.getConfigTree();
        configTree.addListener(this);

        // Update current IP address
        updateCurrentIP();

        // Initialize appearance (ADM-OSC greyed out, Tracking greyed out if disabled)
        updateAdmOscAppearance();
        updateTrackingAppearance();

        // Setup mouse listeners for status bar help text
        setupMouseListeners();
    }

    ~NetworkTab() override
    {
        configTree.removeListener(this);
    }

    void setStatusBar(StatusBar* bar)
    {
        statusBar = bar;
    }

    void setNetworkLogWindowCallback(NetworkLogWindowCallback callback)
    {
        onNetworkLogWindowRequested = std::move(callback);
    }

    /** Refresh UI from ValueTree - call after config reload */
    void refreshFromValueTree() { loadParametersFromValueTree(); }

    void setOSCManager(WFSNetwork::OSCManager* manager)
    {
        oscManager = manager;

        // Register callback for connection status changes
        if (oscManager != nullptr)
        {
            oscManager->onConnectionStatusChanged = [this](int targetIndex, WFSNetwork::ConnectionStatus status)
            {
                // Must update UI on message thread
                juce::MessageManager::callAsync([this, targetIndex, status]()
                {
                    updateTargetConnectionStatus(targetIndex, status);
                });
            };
        }

        updateOSCManagerConfig();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        // Footer background (matching Input/Output tabs)
        const int footerHeight = 50;
        g.setColour(juce::Colour(0xFF252525));
        g.fillRect(0, getHeight() - footerHeight, getWidth(), footerHeight);

        // Footer divider line
        g.setColour(juce::Colour(0xFF404040));
        g.drawLine(0.0f, (float)(getHeight() - footerHeight), (float)getWidth(), (float)(getHeight() - footerHeight), 1.0f);

        // Draw section headers
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));
        g.drawText("Network", 20, 10, 200, 20, juce::Justification::left);
        g.drawText("Network Connections", 20, networkConnectionsSectionY - 25, 200, 20, juce::Justification::left);
        g.drawText("ADM-OSC", 20, admOscSectionY - 25, 200, 20, juce::Justification::left);
        g.drawText("Tracking", 20, trackingSectionY - 25, 200, 20, juce::Justification::left);

        // Draw section dividers
        g.setColour(juce::Colour(0xFF404040));
        g.drawLine(20.0f, (float)(networkConnectionsSectionY - 10), (float)(getWidth() - 20), (float)(networkConnectionsSectionY - 10), 1.0f);
        g.drawLine(20.0f, (float)(admOscSectionY - 10), (float)(getWidth() - 20), (float)(admOscSectionY - 10), 1.0f);
        g.drawLine(20.0f, (float)(trackingSectionY - 10), (float)(getWidth() - 20), (float)(trackingSectionY - 10), 1.0f);
    }

    void resized() override
    {
        const int labelWidth = 120;
        const int editorWidth = 80;
        const int unitWidth = 30;
        const int rowHeight = 25;
        const int spacing = 5;
        const int sectionSpacing = 40;

        int x = 20;
        int y = 35;

        // ==================== NETWORK SECTION ====================
        networkInterfaceLabel.setBounds(x, y, labelWidth, rowHeight);
        networkInterfaceSelector.setBounds(x + labelWidth, y, 300, rowHeight);
        y += rowHeight + spacing;

        currentIPLabel.setBounds(x, y, labelWidth, rowHeight);
        currentIPEditor.setBounds(x + labelWidth, y, 150, rowHeight);
        y += rowHeight + spacing;

        udpPortLabel.setBounds(x, y, labelWidth, rowHeight);
        udpPortEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);

        tcpPortLabel.setBounds(x + labelWidth + editorWidth + 20, y, labelWidth, rowHeight);
        tcpPortEditor.setBounds(x + labelWidth * 2 + editorWidth + 20, y, editorWidth, rowHeight);

        // OSC Query on same row as UDP/TCP (aligned to fit within table width)
        // Table width is 3 * (labelWidth + editorWidth + unitWidth) + 2 * 20 = ~730
        const int oscQueryLabelX = x + labelWidth * 2 + editorWidth * 2 + 60;  // After TCP port
        oscQueryLabel.setBounds(oscQueryLabelX, y, 80, rowHeight);  // Shorter label
        oscQueryPortEditor.setBounds(oscQueryLabelX + 85, y, 55, rowHeight);
        oscQueryEnableButton.setBounds(oscQueryLabelX + 150, y, 80, rowHeight);
        y += rowHeight + sectionSpacing;

        // ==================== NETWORK CONNECTIONS TABLE ====================
        networkConnectionsSectionY = y;

        // Calculate total width to match ADM-OSC/Tracking sections
        // col3 end = x + 3 * (labelWidth + editorWidth + unitWidth) + 2 * 20 = ~700 from x
        const int totalTableWidth = 3 * (labelWidth + editorWidth + unitWidth) + 2 * 20;
        const int tableSpacing = 5;
        const int numTableCols = 8;
        const int totalSpacing = (numTableCols - 1) * tableSpacing;

        // Distribute width proportionally across columns
        // Weights: Name=3, Mode=1.5, IP=3, Port=1.5, Rx=1, Tx=1, Protocol=2.5, Remove=1
        const float totalWeight = 3.0f + 1.5f + 3.0f + 1.5f + 1.0f + 1.0f + 2.5f + 1.0f;
        const int availableWidth = totalTableWidth - totalSpacing;

        const int nameColWidth = (int)(availableWidth * 3.0f / totalWeight);
        const int modeColWidth = (int)(availableWidth * 1.5f / totalWeight);
        const int ipColWidth = (int)(availableWidth * 3.0f / totalWeight);
        const int portColWidth = (int)(availableWidth * 1.5f / totalWeight);
        const int rxTxColWidth = (int)(availableWidth * 1.0f / totalWeight);
        const int protocolColWidth = (int)(availableWidth * 2.5f / totalWeight);
        const int removeColWidth = (int)(availableWidth * 1.0f / totalWeight);

        int tableX = x;

        // Header row
        int colX = tableX;
        headerNameLabel.setBounds(colX, y, nameColWidth, rowHeight);
        colX += nameColWidth + tableSpacing;
        headerDataModeLabel.setBounds(colX, y, modeColWidth, rowHeight);
        colX += modeColWidth + tableSpacing;
        headerIpLabel.setBounds(colX, y, ipColWidth, rowHeight);
        colX += ipColWidth + tableSpacing;
        headerTxPortLabel.setBounds(colX, y, portColWidth, rowHeight);
        colX += portColWidth + tableSpacing;
        headerRxEnableLabel.setBounds(colX, y, rxTxColWidth, rowHeight);
        colX += rxTxColWidth + tableSpacing;
        headerTxEnableLabel.setBounds(colX, y, rxTxColWidth, rowHeight);
        colX += rxTxColWidth + tableSpacing;
        headerProtocolLabel.setBounds(colX, y, protocolColWidth, rowHeight);
        colX += protocolColWidth + tableSpacing;
        addTargetButton.setBounds(colX, y, removeColWidth, rowHeight);
        y += rowHeight + spacing;

        // Target rows
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            colX = tableX;

            row.nameEditor.setBounds(colX, y, nameColWidth, rowHeight);
            colX += nameColWidth + tableSpacing;
            row.dataModeSelector.setBounds(colX, y, modeColWidth, rowHeight);
            colX += modeColWidth + tableSpacing;
            row.ipEditor.setBounds(colX, y, ipColWidth, rowHeight);
            colX += ipColWidth + tableSpacing;
            row.txPortEditor.setBounds(colX, y, portColWidth, rowHeight);
            colX += portColWidth + tableSpacing;
            row.rxEnableButton.setBounds(colX, y, rxTxColWidth, rowHeight);
            colX += rxTxColWidth + tableSpacing;
            row.txEnableButton.setBounds(colX, y, rxTxColWidth, rowHeight);
            colX += rxTxColWidth + tableSpacing;
            row.protocolSelector.setBounds(colX, y, protocolColWidth, rowHeight);
            colX += protocolColWidth + tableSpacing;
            row.removeButton.setBounds(colX, y, removeColWidth, rowHeight);

            y += rowHeight + spacing;
        }

        // ==================== BUTTONS BENEATH TABLE ====================
        y += spacing;  // Small gap after table
        const int tableButtonWidth = 140;
        const int filterButtonWidth = 180;
        const int tableButtonSpacing = 15;
        const int buttonsWidth = filterButtonWidth + tableButtonWidth * 2 + tableButtonSpacing * 2;
        const int buttonsX = tableX + (totalTableWidth - buttonsWidth) / 2;  // Center the buttons

        oscSourceFilterButton.setBounds(buttonsX, y, filterButtonWidth, rowHeight);
        openLogWindowButton.setBounds(buttonsX + filterButtonWidth + tableButtonSpacing, y, tableButtonWidth, rowHeight);
        findMyRemoteButton.setBounds(buttonsX + filterButtonWidth + tableButtonWidth + tableButtonSpacing * 2, y, tableButtonWidth, rowHeight);
        y += rowHeight + sectionSpacing;  // Add section spacing before ADM-OSC

        // ==================== ADM-OSC SECTION ====================
        admOscSectionY = y;

        // Row 1: Offset X, Y, Z
        int col1 = x;
        int col2 = x + labelWidth + editorWidth + unitWidth + 20;
        int col3 = col2 + labelWidth + editorWidth + unitWidth + 20;

        admOscOffsetXLabel.setBounds(col1, y, labelWidth, rowHeight);
        admOscOffsetXEditor.setBounds(col1 + labelWidth, y, editorWidth, rowHeight);
        admOscOffsetXUnitLabel.setBounds(col1 + labelWidth + editorWidth, y, unitWidth, rowHeight);

        admOscOffsetYLabel.setBounds(col2, y, labelWidth, rowHeight);
        admOscOffsetYEditor.setBounds(col2 + labelWidth, y, editorWidth, rowHeight);
        admOscOffsetYUnitLabel.setBounds(col2 + labelWidth + editorWidth, y, unitWidth, rowHeight);

        admOscOffsetZLabel.setBounds(col3, y, labelWidth, rowHeight);
        admOscOffsetZEditor.setBounds(col3 + labelWidth, y, editorWidth, rowHeight);
        admOscOffsetZUnitLabel.setBounds(col3 + labelWidth + editorWidth, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Row 2: Scale X, Y, Z
        admOscScaleXLabel.setBounds(col1, y, labelWidth, rowHeight);
        admOscScaleXEditor.setBounds(col1 + labelWidth, y, editorWidth, rowHeight);
        admOscScaleXUnitLabel.setBounds(col1 + labelWidth + editorWidth, y, unitWidth, rowHeight);

        admOscScaleYLabel.setBounds(col2, y, labelWidth, rowHeight);
        admOscScaleYEditor.setBounds(col2 + labelWidth, y, editorWidth, rowHeight);
        admOscScaleYUnitLabel.setBounds(col2 + labelWidth + editorWidth, y, unitWidth, rowHeight);

        admOscScaleZLabel.setBounds(col3, y, labelWidth, rowHeight);
        admOscScaleZEditor.setBounds(col3 + labelWidth, y, editorWidth, rowHeight);
        admOscScaleZUnitLabel.setBounds(col3 + labelWidth + editorWidth, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Row 3: Flip X, Y, Z
        admOscFlipXButton.setBounds(col1, y, labelWidth + editorWidth, rowHeight);
        admOscFlipYButton.setBounds(col2, y, labelWidth + editorWidth, rowHeight);
        admOscFlipZButton.setBounds(col3, y, labelWidth + editorWidth, rowHeight);
        y += rowHeight + sectionSpacing;

        // ==================== TRACKING SECTION ====================
        trackingSectionY = y;

        // Row 1: Enable, Protocol, Port - align port editor with editors below
        const int shortLabelWidth = 65;
        const int protocolSelectorWidth = 130;
        trackingEnabledButton.setBounds(col1, y, labelWidth + editorWidth, rowHeight);
        trackingProtocolLabel.setBounds(col2, y, shortLabelWidth, rowHeight);
        trackingProtocolSelector.setBounds(col2 + shortLabelWidth, y, protocolSelectorWidth, rowHeight);
        // Align Rx Port label and editor with the Z column editors below
        trackingPortLabel.setBounds(col3, y, labelWidth, rowHeight);
        trackingPortEditor.setBounds(col3 + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        // Row 2: Offset X, Y, Z
        trackingOffsetXLabel.setBounds(col1, y, labelWidth, rowHeight);
        trackingOffsetXEditor.setBounds(col1 + labelWidth, y, editorWidth, rowHeight);
        trackingOffsetXUnitLabel.setBounds(col1 + labelWidth + editorWidth, y, unitWidth, rowHeight);

        trackingOffsetYLabel.setBounds(col2, y, labelWidth, rowHeight);
        trackingOffsetYEditor.setBounds(col2 + labelWidth, y, editorWidth, rowHeight);
        trackingOffsetYUnitLabel.setBounds(col2 + labelWidth + editorWidth, y, unitWidth, rowHeight);

        trackingOffsetZLabel.setBounds(col3, y, labelWidth, rowHeight);
        trackingOffsetZEditor.setBounds(col3 + labelWidth, y, editorWidth, rowHeight);
        trackingOffsetZUnitLabel.setBounds(col3 + labelWidth + editorWidth, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Row 3: Scale X, Y, Z
        trackingScaleXLabel.setBounds(col1, y, labelWidth, rowHeight);
        trackingScaleXEditor.setBounds(col1 + labelWidth, y, editorWidth, rowHeight);
        trackingScaleXUnitLabel.setBounds(col1 + labelWidth + editorWidth, y, unitWidth, rowHeight);

        trackingScaleYLabel.setBounds(col2, y, labelWidth, rowHeight);
        trackingScaleYEditor.setBounds(col2 + labelWidth, y, editorWidth, rowHeight);
        trackingScaleYUnitLabel.setBounds(col2 + labelWidth + editorWidth, y, unitWidth, rowHeight);

        trackingScaleZLabel.setBounds(col3, y, labelWidth, rowHeight);
        trackingScaleZEditor.setBounds(col3 + labelWidth, y, editorWidth, rowHeight);
        trackingScaleZUnitLabel.setBounds(col3 + labelWidth + editorWidth, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Row 4: Flip X, Y, Z
        trackingFlipXButton.setBounds(col1, y, labelWidth + editorWidth, rowHeight);
        trackingFlipYButton.setBounds(col2, y, labelWidth + editorWidth, rowHeight);
        trackingFlipZButton.setBounds(col3, y, labelWidth + editorWidth, rowHeight);

        // ==================== FOOTER BUTTONS ====================
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
        exportButton.setBounds(footerArea.removeFromLeft(buttonWidth));
    }

private:
    WfsParameters& parameters;
    juce::ValueTree configTree;  // Store for safe listener removal in destructor
    StatusBar* statusBar = nullptr;
    WFSNetwork::OSCManager* oscManager = nullptr;
    NetworkLogWindowCallback onNetworkLogWindowRequested;

    // ==================== NETWORK CONNECTIONS TABLE ====================
    static constexpr int maxTargets = 6;

    // Structure for each target row
    struct NetworkTargetRow
    {
        juce::TextEditor nameEditor;
        juce::ComboBox dataModeSelector;       // UDP / TCP
        juce::TextEditor ipEditor;
        juce::TextEditor txPortEditor;
        juce::TextButton rxEnableButton;
        juce::TextButton txEnableButton;
        juce::ComboBox protocolSelector;       // DISABLED / OSC / REMOTE / ADM-OSC
        juce::TextButton removeButton;

        bool isActive = false;  // Whether this row has data
    };

    // Header labels for the table
    juce::Label headerNameLabel;
    juce::Label headerDataModeLabel;
    juce::Label headerIpLabel;
    juce::Label headerTxPortLabel;
    juce::Label headerRxEnableLabel;
    juce::Label headerTxEnableLabel;
    juce::Label headerProtocolLabel;
    juce::TextButton addTargetButton;

    // 6 target rows
    NetworkTargetRow targetRows[maxTargets];
    int activeTargetCount = 0;

    // Buttons beneath the table
    juce::TextButton openLogWindowButton;
    juce::TextButton findMyRemoteButton;
    juce::TextButton oscSourceFilterButton;

    // Find My Remote password (session-only, not saved)
    juce::String findDevicePassword;

    // Section Y position
    int networkConnectionsSectionY = 0;

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

    // OSC Query
    juce::Label oscQueryLabel;
    juce::TextEditor oscQueryPortEditor;
    juce::TextButton oscQueryEnableButton;

    // Footer buttons
    juce::TextButton storeButton;
    juce::TextButton reloadButton;
    juce::TextButton reloadBackupButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;

    // Section Y positions for painting
    int admOscSectionY = 0;
    int trackingSectionY = 0;

    // ADM-OSC Section
    juce::Label admOscOffsetXLabel;
    juce::TextEditor admOscOffsetXEditor;
    juce::Label admOscOffsetXUnitLabel;
    juce::Label admOscOffsetYLabel;
    juce::TextEditor admOscOffsetYEditor;
    juce::Label admOscOffsetYUnitLabel;
    juce::Label admOscOffsetZLabel;
    juce::TextEditor admOscOffsetZEditor;
    juce::Label admOscOffsetZUnitLabel;

    juce::Label admOscScaleXLabel;
    juce::TextEditor admOscScaleXEditor;
    juce::Label admOscScaleXUnitLabel;
    juce::Label admOscScaleYLabel;
    juce::TextEditor admOscScaleYEditor;
    juce::Label admOscScaleYUnitLabel;
    juce::Label admOscScaleZLabel;
    juce::TextEditor admOscScaleZEditor;
    juce::Label admOscScaleZUnitLabel;

    juce::TextButton admOscFlipXButton;
    juce::TextButton admOscFlipYButton;
    juce::TextButton admOscFlipZButton;

    // Tracking Section
    juce::TextButton trackingEnabledButton;
    juce::Label trackingProtocolLabel;
    juce::ComboBox trackingProtocolSelector;
    juce::Label trackingPortLabel;
    juce::TextEditor trackingPortEditor;

    juce::Label trackingOffsetXLabel;
    juce::TextEditor trackingOffsetXEditor;
    juce::Label trackingOffsetXUnitLabel;
    juce::Label trackingOffsetYLabel;
    juce::TextEditor trackingOffsetYEditor;
    juce::Label trackingOffsetYUnitLabel;
    juce::Label trackingOffsetZLabel;
    juce::TextEditor trackingOffsetZEditor;
    juce::Label trackingOffsetZUnitLabel;

    juce::Label trackingScaleXLabel;
    juce::TextEditor trackingScaleXEditor;
    juce::Label trackingScaleXUnitLabel;
    juce::Label trackingScaleYLabel;
    juce::TextEditor trackingScaleYEditor;
    juce::Label trackingScaleYUnitLabel;
    juce::Label trackingScaleZLabel;
    juce::TextEditor trackingScaleZEditor;
    juce::Label trackingScaleZUnitLabel;

    juce::TextButton trackingFlipXButton;
    juce::TextButton trackingFlipYButton;
    juce::TextButton trackingFlipZButton;

    void setupAdmOscSection()
    {
        // Offset X
        addAndMakeVisible(admOscOffsetXLabel);
        admOscOffsetXLabel.setText("Offset X:", juce::dontSendNotification);
        admOscOffsetXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(admOscOffsetXEditor);
        addAndMakeVisible(admOscOffsetXUnitLabel);
        admOscOffsetXUnitLabel.setText("m", juce::dontSendNotification);
        admOscOffsetXUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Offset Y
        addAndMakeVisible(admOscOffsetYLabel);
        admOscOffsetYLabel.setText("Offset Y:", juce::dontSendNotification);
        admOscOffsetYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(admOscOffsetYEditor);
        addAndMakeVisible(admOscOffsetYUnitLabel);
        admOscOffsetYUnitLabel.setText("m", juce::dontSendNotification);
        admOscOffsetYUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Offset Z
        addAndMakeVisible(admOscOffsetZLabel);
        admOscOffsetZLabel.setText("Offset Z:", juce::dontSendNotification);
        admOscOffsetZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(admOscOffsetZEditor);
        addAndMakeVisible(admOscOffsetZUnitLabel);
        admOscOffsetZUnitLabel.setText("m", juce::dontSendNotification);
        admOscOffsetZUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Scale X
        addAndMakeVisible(admOscScaleXLabel);
        admOscScaleXLabel.setText("Scale X:", juce::dontSendNotification);
        admOscScaleXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(admOscScaleXEditor);
        addAndMakeVisible(admOscScaleXUnitLabel);
        admOscScaleXUnitLabel.setText("x", juce::dontSendNotification);
        admOscScaleXUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Scale Y
        addAndMakeVisible(admOscScaleYLabel);
        admOscScaleYLabel.setText("Scale Y:", juce::dontSendNotification);
        admOscScaleYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(admOscScaleYEditor);
        addAndMakeVisible(admOscScaleYUnitLabel);
        admOscScaleYUnitLabel.setText("x", juce::dontSendNotification);
        admOscScaleYUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Scale Z
        addAndMakeVisible(admOscScaleZLabel);
        admOscScaleZLabel.setText("Scale Z:", juce::dontSendNotification);
        admOscScaleZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(admOscScaleZEditor);
        addAndMakeVisible(admOscScaleZUnitLabel);
        admOscScaleZUnitLabel.setText("x", juce::dontSendNotification);
        admOscScaleZUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Flip buttons
        addAndMakeVisible(admOscFlipXButton);
        admOscFlipXButton.setButtonText("Flip X: OFF");
        admOscFlipXButton.setClickingTogglesState(true);
        admOscFlipXButton.onClick = [this]() {
            admOscFlipXButton.setButtonText(admOscFlipXButton.getToggleState() ? "Flip X: ON" : "Flip X: OFF");
            parameters.setConfigParam("admOscFlipX", admOscFlipXButton.getToggleState() ? 1 : 0);
        };

        addAndMakeVisible(admOscFlipYButton);
        admOscFlipYButton.setButtonText("Flip Y: OFF");
        admOscFlipYButton.setClickingTogglesState(true);
        admOscFlipYButton.onClick = [this]() {
            admOscFlipYButton.setButtonText(admOscFlipYButton.getToggleState() ? "Flip Y: ON" : "Flip Y: OFF");
            parameters.setConfigParam("admOscFlipY", admOscFlipYButton.getToggleState() ? 1 : 0);
        };

        addAndMakeVisible(admOscFlipZButton);
        admOscFlipZButton.setButtonText("Flip Z: OFF");
        admOscFlipZButton.setClickingTogglesState(true);
        admOscFlipZButton.onClick = [this]() {
            admOscFlipZButton.setButtonText(admOscFlipZButton.getToggleState() ? "Flip Z: ON" : "Flip Z: OFF");
            parameters.setConfigParam("admOscFlipZ", admOscFlipZButton.getToggleState() ? 1 : 0);
        };

        // Add text editor listeners
        admOscOffsetXEditor.addListener(this);
        admOscOffsetYEditor.addListener(this);
        admOscOffsetZEditor.addListener(this);
        admOscScaleXEditor.addListener(this);
        admOscScaleYEditor.addListener(this);
        admOscScaleZEditor.addListener(this);
    }

    void setupTrackingSection()
    {
        // Enable button
        addAndMakeVisible(trackingEnabledButton);
        trackingEnabledButton.setButtonText("Tracking: OFF");
        trackingEnabledButton.setClickingTogglesState(true);
        trackingEnabledButton.onClick = [this]() {
            bool enabling = trackingEnabledButton.getToggleState();
            if (enabling)
            {
                // Check for cluster conflicts before enabling
                checkGlobalTrackingConstraintAsync();
            }
            else
            {
                trackingEnabledButton.setButtonText("Tracking: OFF");
                parameters.setConfigParam("trackingEnabled", 0);
                updateTrackingAppearance();
            }
        };

        // Protocol selector
        addAndMakeVisible(trackingProtocolLabel);
        trackingProtocolLabel.setText("Protocol:", juce::dontSendNotification);
        trackingProtocolLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingProtocolSelector);
        trackingProtocolSelector.addItem("DISABLED", 1);
        trackingProtocolSelector.addItem("OSC", 2);
        trackingProtocolSelector.addItem("PosiStageNet (PSN)", 3);
        trackingProtocolSelector.addItem("RTTrP", 4);
        trackingProtocolSelector.setSelectedId(1, juce::dontSendNotification);
        trackingProtocolSelector.onChange = [this]() {
            int newProtocol = trackingProtocolSelector.getSelectedId() - 1;
            int globalEnabled = static_cast<int>(parameters.getConfigParam("trackingEnabled"));

            // If enabling protocol while global tracking is on, check for conflicts
            if (newProtocol != 0 && globalEnabled != 0)
            {
                checkGlobalTrackingConstraintAsync(true);  // true = called from protocol change
            }
            else
            {
                parameters.setConfigParam("trackingProtocol", newProtocol);
            }
        };

        // Port
        addAndMakeVisible(trackingPortLabel);
        trackingPortLabel.setText("Rx Port:", juce::dontSendNotification);
        trackingPortLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingPortEditor);

        // Offset X
        addAndMakeVisible(trackingOffsetXLabel);
        trackingOffsetXLabel.setText("Offset X:", juce::dontSendNotification);
        trackingOffsetXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingOffsetXEditor);
        addAndMakeVisible(trackingOffsetXUnitLabel);
        trackingOffsetXUnitLabel.setText("m", juce::dontSendNotification);
        trackingOffsetXUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Offset Y
        addAndMakeVisible(trackingOffsetYLabel);
        trackingOffsetYLabel.setText("Offset Y:", juce::dontSendNotification);
        trackingOffsetYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingOffsetYEditor);
        addAndMakeVisible(trackingOffsetYUnitLabel);
        trackingOffsetYUnitLabel.setText("m", juce::dontSendNotification);
        trackingOffsetYUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Offset Z
        addAndMakeVisible(trackingOffsetZLabel);
        trackingOffsetZLabel.setText("Offset Z:", juce::dontSendNotification);
        trackingOffsetZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingOffsetZEditor);
        addAndMakeVisible(trackingOffsetZUnitLabel);
        trackingOffsetZUnitLabel.setText("m", juce::dontSendNotification);
        trackingOffsetZUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Scale X
        addAndMakeVisible(trackingScaleXLabel);
        trackingScaleXLabel.setText("Scale X:", juce::dontSendNotification);
        trackingScaleXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingScaleXEditor);
        addAndMakeVisible(trackingScaleXUnitLabel);
        trackingScaleXUnitLabel.setText("x", juce::dontSendNotification);
        trackingScaleXUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Scale Y
        addAndMakeVisible(trackingScaleYLabel);
        trackingScaleYLabel.setText("Scale Y:", juce::dontSendNotification);
        trackingScaleYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingScaleYEditor);
        addAndMakeVisible(trackingScaleYUnitLabel);
        trackingScaleYUnitLabel.setText("x", juce::dontSendNotification);
        trackingScaleYUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Scale Z
        addAndMakeVisible(trackingScaleZLabel);
        trackingScaleZLabel.setText("Scale Z:", juce::dontSendNotification);
        trackingScaleZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingScaleZEditor);
        addAndMakeVisible(trackingScaleZUnitLabel);
        trackingScaleZUnitLabel.setText("x", juce::dontSendNotification);
        trackingScaleZUnitLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

        // Flip buttons
        addAndMakeVisible(trackingFlipXButton);
        trackingFlipXButton.setButtonText("Flip X: OFF");
        trackingFlipXButton.setClickingTogglesState(true);
        trackingFlipXButton.onClick = [this]() {
            trackingFlipXButton.setButtonText(trackingFlipXButton.getToggleState() ? "Flip X: ON" : "Flip X: OFF");
            parameters.setConfigParam("trackingFlipX", trackingFlipXButton.getToggleState() ? 1 : 0);
        };

        addAndMakeVisible(trackingFlipYButton);
        trackingFlipYButton.setButtonText("Flip Y: OFF");
        trackingFlipYButton.setClickingTogglesState(true);
        trackingFlipYButton.onClick = [this]() {
            trackingFlipYButton.setButtonText(trackingFlipYButton.getToggleState() ? "Flip Y: ON" : "Flip Y: OFF");
            parameters.setConfigParam("trackingFlipY", trackingFlipYButton.getToggleState() ? 1 : 0);
        };

        addAndMakeVisible(trackingFlipZButton);
        trackingFlipZButton.setButtonText("Flip Z: OFF");
        trackingFlipZButton.setClickingTogglesState(true);
        trackingFlipZButton.onClick = [this]() {
            trackingFlipZButton.setButtonText(trackingFlipZButton.getToggleState() ? "Flip Z: ON" : "Flip Z: OFF");
            parameters.setConfigParam("trackingFlipZ", trackingFlipZButton.getToggleState() ? 1 : 0);
        };

        // Add text editor listeners
        trackingPortEditor.addListener(this);
        trackingOffsetXEditor.addListener(this);
        trackingOffsetYEditor.addListener(this);
        trackingOffsetZEditor.addListener(this);
        trackingScaleXEditor.addListener(this);
        trackingScaleYEditor.addListener(this);
        trackingScaleZEditor.addListener(this);
    }

    void setupMouseListeners()
    {
        // ==================== NETWORK SECTION ====================
        networkInterfaceLabel.addMouseListener(this, false);
        networkInterfaceSelector.addMouseListener(this, false);
        currentIPLabel.addMouseListener(this, false);
        currentIPEditor.addMouseListener(this, false);
        udpPortLabel.addMouseListener(this, false);
        udpPortEditor.addMouseListener(this, false);
        tcpPortLabel.addMouseListener(this, false);
        tcpPortEditor.addMouseListener(this, false);
        oscQueryLabel.addMouseListener(this, false);
        oscQueryPortEditor.addMouseListener(this, false);
        oscQueryEnableButton.addMouseListener(this, false);

        // ==================== NETWORK CONNECTIONS TABLE ====================
        // Header labels already have mouse listeners from setupNetworkConnectionsTable
        addTargetButton.addMouseListener(this, false);
        openLogWindowButton.addMouseListener(this, false);
        findMyRemoteButton.addMouseListener(this, false);
        oscSourceFilterButton.addMouseListener(this, false);

        // Target row components
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            row.nameEditor.addMouseListener(this, false);
            row.dataModeSelector.addMouseListener(this, false);
            row.ipEditor.addMouseListener(this, false);
            row.txPortEditor.addMouseListener(this, false);
            row.rxEnableButton.addMouseListener(this, false);
            row.txEnableButton.addMouseListener(this, false);
            row.protocolSelector.addMouseListener(this, false);
            row.removeButton.addMouseListener(this, false);
        }

        // ==================== ADM-OSC SECTION ====================
        admOscOffsetXLabel.addMouseListener(this, false);
        admOscOffsetXEditor.addMouseListener(this, false);
        admOscOffsetYLabel.addMouseListener(this, false);
        admOscOffsetYEditor.addMouseListener(this, false);
        admOscOffsetZLabel.addMouseListener(this, false);
        admOscOffsetZEditor.addMouseListener(this, false);
        admOscScaleXLabel.addMouseListener(this, false);
        admOscScaleXEditor.addMouseListener(this, false);
        admOscScaleYLabel.addMouseListener(this, false);
        admOscScaleYEditor.addMouseListener(this, false);
        admOscScaleZLabel.addMouseListener(this, false);
        admOscScaleZEditor.addMouseListener(this, false);
        admOscFlipXButton.addMouseListener(this, false);
        admOscFlipYButton.addMouseListener(this, false);
        admOscFlipZButton.addMouseListener(this, false);

        // ==================== TRACKING SECTION ====================
        trackingEnabledButton.addMouseListener(this, false);
        trackingProtocolLabel.addMouseListener(this, false);
        trackingProtocolSelector.addMouseListener(this, false);
        trackingPortLabel.addMouseListener(this, false);
        trackingPortEditor.addMouseListener(this, false);
        trackingOffsetXLabel.addMouseListener(this, false);
        trackingOffsetXEditor.addMouseListener(this, false);
        trackingOffsetYLabel.addMouseListener(this, false);
        trackingOffsetYEditor.addMouseListener(this, false);
        trackingOffsetZLabel.addMouseListener(this, false);
        trackingOffsetZEditor.addMouseListener(this, false);
        trackingScaleXLabel.addMouseListener(this, false);
        trackingScaleXEditor.addMouseListener(this, false);
        trackingScaleYLabel.addMouseListener(this, false);
        trackingScaleYEditor.addMouseListener(this, false);
        trackingScaleZLabel.addMouseListener(this, false);
        trackingScaleZEditor.addMouseListener(this, false);
        trackingFlipXButton.addMouseListener(this, false);
        trackingFlipYButton.addMouseListener(this, false);
        trackingFlipZButton.addMouseListener(this, false);

        // ==================== FOOTER BUTTONS ====================
        storeButton.addMouseListener(this, false);
        reloadButton.addMouseListener(this, false);
        reloadBackupButton.addMouseListener(this, false);
        importButton.addMouseListener(this, false);
        exportButton.addMouseListener(this, false);
    }

    void updateOSCManagerConfig()
    {
        if (oscManager == nullptr)
            return;

        // Build allowed IPs list from targets with rxEnabled=true
        juce::StringArray allowedIPs;
        for (int i = 0; i < maxTargets; ++i)
        {
            if (targetRows[i].isActive && targetRows[i].rxEnableButton.getToggleState())
            {
                juce::String ip = targetRows[i].ipEditor.getText().trim();
                if (ip.isNotEmpty() && !allowedIPs.contains(ip))
                    allowedIPs.add(ip);
            }
        }

        // Apply global config including IP filtering
        WFSNetwork::GlobalConfig globalConfig;
        globalConfig.udpReceivePort = udpPortEditor.getText().getIntValue();
        globalConfig.tcpReceivePort = tcpPortEditor.getText().getIntValue();
        globalConfig.ipFilteringEnabled = oscSourceFilterButton.getToggleState();
        globalConfig.allowedIPs = allowedIPs;
        oscManager->applyGlobalConfig(globalConfig);

        // Apply target configs
        for (int i = 0; i < maxTargets; ++i)
        {
            WFSNetwork::TargetConfig targetConfig;
            targetConfig.name = targetRows[i].nameEditor.getText();
            targetConfig.ipAddress = targetRows[i].ipEditor.getText();
            targetConfig.port = targetRows[i].txPortEditor.getText().getIntValue();
            targetConfig.rxEnabled = targetRows[i].rxEnableButton.getToggleState();
            targetConfig.txEnabled = targetRows[i].txEnableButton.getToggleState();

            // Map protocol selector to enum
            int protocolId = targetRows[i].protocolSelector.getSelectedId();
            switch (protocolId)
            {
                case 1: targetConfig.protocol = WFSNetwork::Protocol::Disabled; break;
                case 2: targetConfig.protocol = WFSNetwork::Protocol::OSC; break;
                case 3: targetConfig.protocol = WFSNetwork::Protocol::Remote; break;
                case 4: targetConfig.protocol = WFSNetwork::Protocol::ADMOSC; break;
                default: targetConfig.protocol = WFSNetwork::Protocol::Disabled; break;
            }

            // Map data mode to enum
            int modeId = targetRows[i].dataModeSelector.getSelectedId();
            targetConfig.mode = (modeId == 2) ? WFSNetwork::ConnectionMode::TCP
                                               : WFSNetwork::ConnectionMode::UDP;

            oscManager->applyTargetConfig(i, targetConfig);

            // Connect if protocol is enabled and tx is enabled
            if (targetConfig.protocol != WFSNetwork::Protocol::Disabled && targetConfig.txEnabled)
            {
                DBG("NetworkTab: Connecting target " << i << " to " << targetConfig.ipAddress
                    << ":" << targetConfig.port << " protocol=" << static_cast<int>(targetConfig.protocol));
                bool connected = oscManager->connectTarget(i);
                DBG("NetworkTab: Target " << i << " connected=" << (connected ? "yes" : "no"));
            }
        }

        // Start listening if any target has Rx enabled
        bool anyRxEnabled = false;
        for (int i = 0; i < maxTargets; ++i)
        {
            if (targetRows[i].rxEnableButton.getToggleState())
            {
                anyRxEnabled = true;
                break;
            }
        }

        if (anyRxEnabled && !oscManager->isListening())
        {
            oscManager->startListening();
        }
    }

    void setupNetworkConnectionsTable()
    {
        // ==================== HEADER ROW ====================
        auto setupHeaderLabel = [this](juce::Label& label, const juce::String& text)
        {
            addAndMakeVisible(label);
            label.setText(text, juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, juce::Colours::white);
            label.setFont(juce::FontOptions().withHeight(12.0f).withStyle("Bold"));
            label.setJustificationType(juce::Justification::centred);

            // Add mouse listener for hover help
            label.addMouseListener(this, false);
        };

        setupHeaderLabel(headerNameLabel, "Name");
        setupHeaderLabel(headerDataModeLabel, "Mode");
        setupHeaderLabel(headerIpLabel, "IPv4 Address");
        setupHeaderLabel(headerTxPortLabel, "Tx Port");
        setupHeaderLabel(headerRxEnableLabel, "Rx");
        setupHeaderLabel(headerTxEnableLabel, "Tx");
        setupHeaderLabel(headerProtocolLabel, "Protocol");

        // Add button in header
        addAndMakeVisible(addTargetButton);
        addTargetButton.setButtonText("ADD");
        addTargetButton.onClick = [this]() { addNewTarget(); };

        // ==================== TARGET ROWS ====================
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];

            // Name editor
            addAndMakeVisible(row.nameEditor);
            row.nameEditor.setText("Target " + juce::String(i + 1), false);
            row.nameEditor.setJustification(juce::Justification::centred);

            // Data Mode selector (UDP/TCP)
            addAndMakeVisible(row.dataModeSelector);
            row.dataModeSelector.addItem("UDP", 1);
            row.dataModeSelector.addItem("TCP", 2);
            row.dataModeSelector.setSelectedId(1, juce::dontSendNotification);
            row.dataModeSelector.onChange = [this, i]() {
                saveTargetToValueTree(i);
            };

            // IP editor
            addAndMakeVisible(row.ipEditor);
            row.ipEditor.setText("127.0.0.1", false);
            row.ipEditor.setJustification(juce::Justification::centred);

            // Tx Port editor
            addAndMakeVisible(row.txPortEditor);
            row.txPortEditor.setText("9000", false);
            row.txPortEditor.setInputRestrictions(5, "0123456789");
            row.txPortEditor.setJustification(juce::Justification::centred);

            // Rx Enable button
            addAndMakeVisible(row.rxEnableButton);
            row.rxEnableButton.setButtonText("OFF");
            row.rxEnableButton.setClickingTogglesState(true);
            row.rxEnableButton.onClick = [this, i]() {
                auto& btn = targetRows[i].rxEnableButton;
                btn.setButtonText(btn.getToggleState() ? "ON" : "OFF");
                saveTargetToValueTree(i);
            };

            // Tx Enable button
            addAndMakeVisible(row.txEnableButton);
            row.txEnableButton.setButtonText("OFF");
            row.txEnableButton.setClickingTogglesState(true);
            row.txEnableButton.onClick = [this, i]() {
                auto& btn = targetRows[i].txEnableButton;
                btn.setButtonText(btn.getToggleState() ? "ON" : "OFF");
                saveTargetToValueTree(i);
            };

            // Protocol selector
            addAndMakeVisible(row.protocolSelector);
            row.protocolSelector.addItem("DISABLED", 1);
            row.protocolSelector.addItem("OSC", 2);
            row.protocolSelector.addItem("REMOTE", 3);
            row.protocolSelector.addItem("ADM-OSC", 4);
            row.protocolSelector.setSelectedId(1, juce::dontSendNotification);
            row.protocolSelector.onChange = [this, i]() {
                // Check if trying to select REMOTE when one already exists
                if (targetRows[i].protocolSelector.getSelectedId() == 3)  // REMOTE
                {
                    if (hasExistingRemoteConnection(i))
                    {
                        // Revert to DISABLED and show message
                        targetRows[i].protocolSelector.setSelectedId(1, juce::dontSendNotification);
                        if (statusBar != nullptr)
                            statusBar->setHelpText("Only one REMOTE connection is allowed.");
                        return;
                    }
                }
                // Update ADM-OSC appearance when protocol changes
                updateAdmOscAppearance();
                saveTargetToValueTree(i);
            };

            // Remove button
            addAndMakeVisible(row.removeButton);
            row.removeButton.setButtonText("X");
            row.removeButton.onClick = [this, i]() { confirmRemoveTarget(i); };

            // Add text change listeners
            row.nameEditor.onTextChange = [this, i]() { saveTargetToValueTree(i); };
            row.ipEditor.onTextChange = [this, i]() { saveTargetToValueTree(i); };
            row.txPortEditor.onTextChange = [this, i]() { saveTargetToValueTree(i); };

            // Start with rows disabled - user must add them
            row.isActive = false;
        }

        // Initialize active count to 0 - no active targets by default
        activeTargetCount = 0;
        updateAddButtonState();
        updateTargetRowVisibility();

        // ==================== BUTTONS BENEATH TABLE ====================
        addAndMakeVisible(openLogWindowButton);
        openLogWindowButton.setButtonText("Open Log Window");
        openLogWindowButton.onClick = [this]() { openNetworkLogWindow(); };

        addAndMakeVisible(findMyRemoteButton);
        findMyRemoteButton.setButtonText("Find My Remote");
        findMyRemoteButton.onClick = [this]() { showFindMyRemoteDialog(); };

        // OSC Source Filter toggle
        addAndMakeVisible(oscSourceFilterButton);
        oscSourceFilterButton.setButtonText("OSC Filter: Accept All");
        oscSourceFilterButton.setClickingTogglesState(true);
        oscSourceFilterButton.onClick = [this]() {
            oscSourceFilterButton.setButtonText(
                oscSourceFilterButton.getToggleState()
                    ? "OSC Filter: Registered Only"
                    : "OSC Filter: Accept All"
            );
            saveOscSourceFilterToValueTree();
            updateOSCManagerConfig();
        };
    }

    void addNewTarget()
    {
        if (activeTargetCount >= maxTargets)
        {
            if (statusBar != nullptr)
                statusBar->setHelpText("Maximum Number of Targets/Servers Reached.");
            return;
        }

        // Find first inactive row and activate it
        for (int i = 0; i < maxTargets; ++i)
        {
            if (!targetRows[i].isActive)
            {
                targetRows[i].isActive = true;
                activeTargetCount++;
                updateTargetRowVisibility();
                updateAddButtonState();
                saveTargetToValueTree(i);  // Save new target to ValueTree
                break;
            }
        }
    }

    void confirmRemoveTarget(int index)
    {
        juce::String targetName = targetRows[index].nameEditor.getText();
        if (targetName.isEmpty())
            targetName = "Target " + juce::String(index + 1);

        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::QuestionIcon,
            "Remove Target",
            "Delete target '" + targetName + "'?",
            "OK",
            "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this, index](int result) {
                if (result == 1)  // OK clicked
                {
                    removeTarget(index);
                }
            })
        );
    }

    void removeTarget(int index)
    {
        if (index < 0 || index >= maxTargets)
            return;

        // Remove from ValueTree first
        removeTargetFromValueTree(index);

        // Reset row to defaults
        auto& row = targetRows[index];
        row.nameEditor.setText("Target " + juce::String(index + 1), false);
        row.dataModeSelector.setSelectedId(1, juce::dontSendNotification);
        row.ipEditor.setText("127.0.0.1", false);
        row.txPortEditor.setText("9000", false);
        row.rxEnableButton.setToggleState(false, juce::dontSendNotification);
        row.rxEnableButton.setButtonText("OFF");
        row.txEnableButton.setToggleState(false, juce::dontSendNotification);
        row.txEnableButton.setButtonText("OFF");
        row.protocolSelector.setSelectedId(1, juce::dontSendNotification);
        row.isActive = false;

        activeTargetCount--;
        updateTargetRowVisibility();
        updateAddButtonState();
        updateAdmOscAppearance();
    }

    void updateTargetRowVisibility()
    {
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            float alpha = row.isActive ? 1.0f : 0.2f;  // Very faint when disabled

            row.nameEditor.setAlpha(alpha);
            row.nameEditor.setEnabled(row.isActive);
            row.dataModeSelector.setAlpha(alpha);
            row.dataModeSelector.setEnabled(row.isActive);
            row.ipEditor.setAlpha(alpha);
            row.ipEditor.setEnabled(row.isActive);
            row.txPortEditor.setAlpha(alpha);
            row.txPortEditor.setEnabled(row.isActive);
            row.rxEnableButton.setAlpha(alpha);
            row.rxEnableButton.setEnabled(row.isActive);
            row.txEnableButton.setAlpha(alpha);
            row.txEnableButton.setEnabled(row.isActive);
            row.protocolSelector.setAlpha(alpha);
            row.protocolSelector.setEnabled(row.isActive);
            row.removeButton.setAlpha(alpha);
            row.removeButton.setEnabled(row.isActive);

            // Reset connection status color when visibility changes
            if (row.isActive && oscManager != nullptr)
            {
                updateTargetConnectionStatus(i, oscManager->getTargetStatus(i));
            }
            else
            {
                // Inactive rows get default background
                row.nameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF303030));
            }
        }
    }

    void updateTargetConnectionStatus(int targetIndex, WFSNetwork::ConnectionStatus status)
    {
        if (targetIndex < 0 || targetIndex >= maxTargets)
            return;

        auto& row = targetRows[targetIndex];
        if (!row.isActive)
            return;

        // Set background color based on connection status
        juce::Colour bgColor;
        switch (status)
        {
            case WFSNetwork::ConnectionStatus::Connected:
                bgColor = juce::Colour(0xFF1A4D1A);  // Dark green tint
                break;
            case WFSNetwork::ConnectionStatus::Connecting:
                bgColor = juce::Colour(0xFF4D4D1A);  // Dark yellow tint
                break;
            case WFSNetwork::ConnectionStatus::Error:
                bgColor = juce::Colour(0xFF4D1A1A);  // Dark red tint
                break;
            case WFSNetwork::ConnectionStatus::Disconnected:
            default:
                bgColor = juce::Colour(0xFF303030);  // Default dark gray
                break;
        }

        row.nameEditor.setColour(juce::TextEditor::backgroundColourId, bgColor);
        row.nameEditor.repaint();
    }

    void updateAddButtonState()
    {
        bool canAdd = activeTargetCount < maxTargets;
        addTargetButton.setEnabled(canAdd);
        addTargetButton.setAlpha(canAdd ? 1.0f : 0.4f);
    }

    bool hasExistingRemoteConnection(int excludeIndex = -1)
    {
        for (int i = 0; i < maxTargets; ++i)
        {
            if (i == excludeIndex) continue;
            if (targetRows[i].isActive && targetRows[i].protocolSelector.getSelectedId() == 3)  // REMOTE
                return true;
        }
        return false;
    }

    void updateAdmOscAppearance()
    {
        // For now, ADM-OSC is always editable (greying out when no ADM-OSC target will be added later)
        float alpha = 1.0f;  // TODO: check if ADM-OSC target exists

        admOscOffsetXLabel.setAlpha(alpha);
        admOscOffsetXEditor.setAlpha(alpha);
        admOscOffsetXUnitLabel.setAlpha(alpha);
        admOscOffsetYLabel.setAlpha(alpha);
        admOscOffsetYEditor.setAlpha(alpha);
        admOscOffsetYUnitLabel.setAlpha(alpha);
        admOscOffsetZLabel.setAlpha(alpha);
        admOscOffsetZEditor.setAlpha(alpha);
        admOscOffsetZUnitLabel.setAlpha(alpha);

        admOscScaleXLabel.setAlpha(alpha);
        admOscScaleXEditor.setAlpha(alpha);
        admOscScaleXUnitLabel.setAlpha(alpha);
        admOscScaleYLabel.setAlpha(alpha);
        admOscScaleYEditor.setAlpha(alpha);
        admOscScaleYUnitLabel.setAlpha(alpha);
        admOscScaleZLabel.setAlpha(alpha);
        admOscScaleZEditor.setAlpha(alpha);
        admOscScaleZUnitLabel.setAlpha(alpha);

        admOscFlipXButton.setAlpha(alpha);
        admOscFlipYButton.setAlpha(alpha);
        admOscFlipZButton.setAlpha(alpha);
    }

    void updateTrackingAppearance()
    {
        bool enabled = trackingEnabledButton.getToggleState();
        float alpha = enabled ? 1.0f : 0.4f;

        trackingProtocolLabel.setAlpha(alpha);
        trackingProtocolSelector.setAlpha(alpha);
        trackingPortLabel.setAlpha(alpha);
        trackingPortEditor.setAlpha(alpha);

        trackingOffsetXLabel.setAlpha(alpha);
        trackingOffsetXEditor.setAlpha(alpha);
        trackingOffsetXUnitLabel.setAlpha(alpha);
        trackingOffsetYLabel.setAlpha(alpha);
        trackingOffsetYEditor.setAlpha(alpha);
        trackingOffsetYUnitLabel.setAlpha(alpha);
        trackingOffsetZLabel.setAlpha(alpha);
        trackingOffsetZEditor.setAlpha(alpha);
        trackingOffsetZUnitLabel.setAlpha(alpha);

        trackingScaleXLabel.setAlpha(alpha);
        trackingScaleXEditor.setAlpha(alpha);
        trackingScaleXUnitLabel.setAlpha(alpha);
        trackingScaleYLabel.setAlpha(alpha);
        trackingScaleYEditor.setAlpha(alpha);
        trackingScaleYUnitLabel.setAlpha(alpha);
        trackingScaleZLabel.setAlpha(alpha);
        trackingScaleZEditor.setAlpha(alpha);
        trackingScaleZUnitLabel.setAlpha(alpha);

        trackingFlipXButton.setAlpha(alpha);
        trackingFlipYButton.setAlpha(alpha);
        trackingFlipZButton.setAlpha(alpha);
    }

    void setupNumericEditors()
    {
        // Port editors - integers only
        udpPortEditor.setInputRestrictions(5, "0123456789");
        tcpPortEditor.setInputRestrictions(5, "0123456789");
        trackingPortEditor.setInputRestrictions(5, "0123456789");

        // ADM-OSC editors - floats (offset: -50 to 50, scale: 0.01 to 100)
        admOscOffsetXEditor.setInputRestrictions(8, "-0123456789.");
        admOscOffsetYEditor.setInputRestrictions(8, "-0123456789.");
        admOscOffsetZEditor.setInputRestrictions(8, "-0123456789.");
        admOscScaleXEditor.setInputRestrictions(8, "0123456789.");
        admOscScaleYEditor.setInputRestrictions(8, "0123456789.");
        admOscScaleZEditor.setInputRestrictions(8, "0123456789.");

        // Tracking editors - floats (offset: -50 to 50, scale: 0.01 to 100)
        trackingOffsetXEditor.setInputRestrictions(8, "-0123456789.");
        trackingOffsetYEditor.setInputRestrictions(8, "-0123456789.");
        trackingOffsetZEditor.setInputRestrictions(8, "-0123456789.");
        trackingScaleXEditor.setInputRestrictions(8, "0123456789.");
        trackingScaleYEditor.setInputRestrictions(8, "0123456789.");
        trackingScaleZEditor.setInputRestrictions(8, "0123456789.");

        // Set default values
        admOscOffsetXEditor.setText("0.0", false);
        admOscOffsetYEditor.setText("0.0", false);
        admOscOffsetZEditor.setText("0.0", false);
        admOscScaleXEditor.setText("1.0", false);
        admOscScaleYEditor.setText("1.0", false);
        admOscScaleZEditor.setText("1.0", false);

        trackingPortEditor.setText("5000", false);
        trackingOffsetXEditor.setText("0.0", false);
        trackingOffsetYEditor.setText("0.0", false);
        trackingOffsetZEditor.setText("0.0", false);
        trackingScaleXEditor.setText("1.0", false);
        trackingScaleYEditor.setText("1.0", false);
        trackingScaleZEditor.setText("1.0", false);
    }

    void loadParametersFromValueTree()
    {
        int udpPort = (int)parameters.getConfigParam(WFSParameterIDs::networkRxUDPport.toString());
        int tcpPort = (int)parameters.getConfigParam(WFSParameterIDs::networkRxTCPport.toString());
        if (udpPort <= 0) udpPort = 8000;  // Default
        if (tcpPort <= 0) tcpPort = 8001;  // Default
        udpPortEditor.setText(juce::String(udpPort), false);
        tcpPortEditor.setText(juce::String(tcpPort), false);

        // Load saved network interface
        juce::String savedInterface = parameters.getConfigParam("NetworkInterface").toString();
        if (savedInterface.isNotEmpty())
        {
            int index = interfaceNames.indexOf(savedInterface);
            if (index >= 0)
                networkInterfaceSelector.setSelectedId(index + 1, juce::dontSendNotification);
        }

        // Load network targets
        loadTargetsFromValueTree();

        // Load OSC Source Filter setting
        bool filterEnabled = (int)parameters.getConfigParam("networkOscSourceFilter") != 0;
        oscSourceFilterButton.setToggleState(filterEnabled, juce::dontSendNotification);
        oscSourceFilterButton.setButtonText(filterEnabled ? "OSC Filter: Registered Only" : "OSC Filter: Accept All");

        // Load ADM-OSC parameters
        admOscOffsetXEditor.setText(juce::String((float)parameters.getConfigParam("admOscOffsetX")), false);
        admOscOffsetYEditor.setText(juce::String((float)parameters.getConfigParam("admOscOffsetY")), false);
        admOscOffsetZEditor.setText(juce::String((float)parameters.getConfigParam("admOscOffsetZ")), false);
        admOscScaleXEditor.setText(juce::String((float)parameters.getConfigParam("admOscScaleX")), false);
        admOscScaleYEditor.setText(juce::String((float)parameters.getConfigParam("admOscScaleY")), false);
        admOscScaleZEditor.setText(juce::String((float)parameters.getConfigParam("admOscScaleZ")), false);

        bool flipX = (int)parameters.getConfigParam("admOscFlipX") != 0;
        bool flipY = (int)parameters.getConfigParam("admOscFlipY") != 0;
        bool flipZ = (int)parameters.getConfigParam("admOscFlipZ") != 0;
        admOscFlipXButton.setToggleState(flipX, juce::dontSendNotification);
        admOscFlipXButton.setButtonText(flipX ? "Flip X: ON" : "Flip X: OFF");
        admOscFlipYButton.setToggleState(flipY, juce::dontSendNotification);
        admOscFlipYButton.setButtonText(flipY ? "Flip Y: ON" : "Flip Y: OFF");
        admOscFlipZButton.setToggleState(flipZ, juce::dontSendNotification);
        admOscFlipZButton.setButtonText(flipZ ? "Flip Z: ON" : "Flip Z: OFF");

        // Load Tracking parameters
        bool trackingEnabled = (int)parameters.getConfigParam("trackingEnabled") != 0;
        trackingEnabledButton.setToggleState(trackingEnabled, juce::dontSendNotification);
        trackingEnabledButton.setButtonText(trackingEnabled ? "Tracking: ON" : "Tracking: OFF");

        trackingProtocolSelector.setSelectedId((int)parameters.getConfigParam("trackingProtocol") + 1, juce::dontSendNotification);
        trackingPortEditor.setText(juce::String((int)parameters.getConfigParam("trackingPort")), false);

        trackingOffsetXEditor.setText(juce::String((float)parameters.getConfigParam("trackingOffsetX")), false);
        trackingOffsetYEditor.setText(juce::String((float)parameters.getConfigParam("trackingOffsetY")), false);
        trackingOffsetZEditor.setText(juce::String((float)parameters.getConfigParam("trackingOffsetZ")), false);
        trackingScaleXEditor.setText(juce::String((float)parameters.getConfigParam("trackingScaleX")), false);
        trackingScaleYEditor.setText(juce::String((float)parameters.getConfigParam("trackingScaleY")), false);
        trackingScaleZEditor.setText(juce::String((float)parameters.getConfigParam("trackingScaleZ")), false);

        bool trackFlipX = (int)parameters.getConfigParam("trackingFlipX") != 0;
        bool trackFlipY = (int)parameters.getConfigParam("trackingFlipY") != 0;
        bool trackFlipZ = (int)parameters.getConfigParam("trackingFlipZ") != 0;
        trackingFlipXButton.setToggleState(trackFlipX, juce::dontSendNotification);
        trackingFlipXButton.setButtonText(trackFlipX ? "Flip X: ON" : "Flip X: OFF");
        trackingFlipYButton.setToggleState(trackFlipY, juce::dontSendNotification);
        trackingFlipYButton.setButtonText(trackFlipY ? "Flip Y: ON" : "Flip Y: OFF");
        trackingFlipZButton.setToggleState(trackFlipZ, juce::dontSendNotification);
        trackingFlipZButton.setButtonText(trackFlipZ ? "Flip Z: ON" : "Flip Z: OFF");

        updateTrackingAppearance();

        // Load OSC Query parameters
        int oscQueryPort = (int)parameters.getConfigParam(WFSParameterIDs::networkOscQueryPort.toString());
        if (oscQueryPort <= 0) oscQueryPort = 5005;  // Default port
        oscQueryPortEditor.setText(juce::String(oscQueryPort), false);

        bool oscQueryEnabled = (int)parameters.getConfigParam(WFSParameterIDs::networkOscQueryEnabled.toString()) != 0;
        oscQueryEnableButton.setToggleState(oscQueryEnabled, juce::dontSendNotification);
        oscQueryEnableButton.setButtonText(oscQueryEnabled ? "Enabled" : "Disabled");

        // Start OSC Query server if enabled
        updateOSCQueryServer();
    }

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (tree == parameters.getConfigTree())
        {
            if (property == WFSParameterIDs::networkRxUDPport)
                udpPortEditor.setText(juce::String((int)parameters.getConfigParam(WFSParameterIDs::networkRxUDPport.toString())), false);
            else if (property == WFSParameterIDs::networkRxTCPport)
                tcpPortEditor.setText(juce::String((int)parameters.getConfigParam(WFSParameterIDs::networkRxTCPport.toString())), false);
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

    void mouseEnter(const juce::MouseEvent& e) override
    {
        if (statusBar == nullptr) return;

        auto* source = e.eventComponent;

        // ==================== NETWORK SECTION ====================
        if (source == &networkInterfaceLabel || source == &networkInterfaceSelector)
            statusBar->setHelpText("Select the Network Interface.");
        else if (source == &currentIPLabel || source == &currentIPEditor)
            statusBar->setHelpText("IP address of the Processor.");
        else if (source == &udpPortLabel || source == &udpPortEditor)
            statusBar->setHelpText("UDP Receive Port of the Processor.");
        else if (source == &tcpPortLabel || source == &tcpPortEditor)
            statusBar->setHelpText("TCP Receive Port of the Processor.");
        else if (source == &oscQueryLabel || source == &oscQueryPortEditor)
            statusBar->setHelpText("HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/");
        else if (source == &oscQueryEnableButton)
            statusBar->setHelpText("Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.");

        // ==================== NETWORK CONNECTIONS TABLE ====================
        else if (source == &headerNameLabel)
            statusBar->setHelpText("Network Target Name.");
        else if (source == &headerDataModeLabel)
            statusBar->setHelpText("Select UDP or TCP data transmission.");
        else if (source == &headerIpLabel)
            statusBar->setHelpText("IP Address of the Target (use 127.0.0.1 for local host).");
        else if (source == &headerTxPortLabel)
            statusBar->setHelpText("Transmit Port for this Target.");
        else if (source == &headerRxEnableLabel)
            statusBar->setHelpText("Enable or Disable Data Reception.");
        else if (source == &headerTxEnableLabel)
            statusBar->setHelpText("Enable or Disable Data Transmission.");
        else if (source == &headerProtocolLabel)
            statusBar->setHelpText("Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.");
        else if (source == &addTargetButton)
            statusBar->setHelpText("Add new network target.");
        else if (source == &openLogWindowButton)
            statusBar->setHelpText("Open Network Logging window.");
        else if (source == &findMyRemoteButton)
            statusBar->setHelpText("Make your Remote Flash and Buzz to Find it.");
        else if (source == &oscSourceFilterButton)
            statusBar->setHelpText("Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.");

        // ==================== ADM-OSC SECTION ====================
        else if (source == &admOscOffsetXLabel || source == &admOscOffsetXEditor)
            statusBar->setHelpText("Offset ADM-OSC X Coordinate.");
        else if (source == &admOscOffsetYLabel || source == &admOscOffsetYEditor)
            statusBar->setHelpText("Offset ADM-OSC Y Coordinate.");
        else if (source == &admOscOffsetZLabel || source == &admOscOffsetZEditor)
            statusBar->setHelpText("Offset ADM-OSC Z Coordinate.");
        else if (source == &admOscScaleXLabel || source == &admOscScaleXEditor)
            statusBar->setHelpText("Scale ADM-OSC X Coordinate.");
        else if (source == &admOscScaleYLabel || source == &admOscScaleYEditor)
            statusBar->setHelpText("Scale ADM-OSC Y Coordinate.");
        else if (source == &admOscScaleZLabel || source == &admOscScaleZEditor)
            statusBar->setHelpText("Scale ADM-OSC Z Coordinate.");
        else if (source == &admOscFlipXButton)
            statusBar->setHelpText("Invert Axis of ADM-OSC X Coordinate.");
        else if (source == &admOscFlipYButton)
            statusBar->setHelpText("Invert Axis of ADM-OSC Y Coordinate.");
        else if (source == &admOscFlipZButton)
            statusBar->setHelpText("Invert Axis of ADM-OSC Z Coordinate.");

        // ==================== TRACKING SECTION ====================
        else if (source == &trackingEnabledButton)
            statusBar->setHelpText("Enable or Disable Incoming Tracking data processing.");
        else if (source == &trackingProtocolLabel || source == &trackingProtocolSelector)
            statusBar->setHelpText("Select the type of Tracking Protocol.");
        else if (source == &trackingPortLabel || source == &trackingPortEditor)
            statusBar->setHelpText("Specify the Port to receive Tracking data.");
        else if (source == &trackingOffsetXLabel || source == &trackingOffsetXEditor)
            statusBar->setHelpText("Offset Tracking X Coordinate.");
        else if (source == &trackingOffsetYLabel || source == &trackingOffsetYEditor)
            statusBar->setHelpText("Offset Tracking Y Coordinate.");
        else if (source == &trackingOffsetZLabel || source == &trackingOffsetZEditor)
            statusBar->setHelpText("Offset Tracking Z Coordinate.");
        else if (source == &trackingScaleXLabel || source == &trackingScaleXEditor)
            statusBar->setHelpText("Scale Tracking X Coordinate.");
        else if (source == &trackingScaleYLabel || source == &trackingScaleYEditor)
            statusBar->setHelpText("Scale Tracking Y Coordinate.");
        else if (source == &trackingScaleZLabel || source == &trackingScaleZEditor)
            statusBar->setHelpText("Scale Tracking Z Coordinate.");
        else if (source == &trackingFlipXButton)
            statusBar->setHelpText("Invert Axis of Tracking X Coordinate.");
        else if (source == &trackingFlipYButton)
            statusBar->setHelpText("Invert Axis of Tracking Y Coordinate.");
        else if (source == &trackingFlipZButton)
            statusBar->setHelpText("Invert Axis of Tracking Z Coordinate.");

        // ==================== FOOTER BUTTONS ====================
        else if (source == &storeButton)
            statusBar->setHelpText("Store Network Configuration to file.");
        else if (source == &reloadButton)
            statusBar->setHelpText("Reload Network Configuration from file.");
        else if (source == &reloadBackupButton)
            statusBar->setHelpText("Reload Network Configuration from backup file.");
        else if (source == &importButton)
            statusBar->setHelpText("Import Network Configuration from file.");
        else if (source == &exportButton)
            statusBar->setHelpText("Export Network Configuration to file.");

        // ==================== TARGET ROW COMPONENTS ====================
        else
        {
            for (int i = 0; i < maxTargets; ++i)
            {
                auto& row = targetRows[i];
                if (source == &row.nameEditor)
                    { statusBar->setHelpText("Network Target Name."); return; }
                else if (source == &row.dataModeSelector)
                    { statusBar->setHelpText("Select UDP or TCP data transmission."); return; }
                else if (source == &row.ipEditor)
                    { statusBar->setHelpText("IP Address of the Target (use 127.0.0.1 for local host)."); return; }
                else if (source == &row.txPortEditor)
                    { statusBar->setHelpText("Transmit Port for this Target."); return; }
                else if (source == &row.rxEnableButton)
                    { statusBar->setHelpText("Enable or Disable Data Reception."); return; }
                else if (source == &row.txEnableButton)
                    { statusBar->setHelpText("Enable or Disable Data Transmission."); return; }
                else if (source == &row.protocolSelector)
                    { statusBar->setHelpText("Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC."); return; }
                else if (source == &row.removeButton)
                    { statusBar->setHelpText("Delete this Network Target."); return; }
            }
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Refresh network interfaces when clicking on the dropdown
        // This allows detecting newly connected adapters
        if (e.eventComponent == &networkInterfaceSelector)
        {
            refreshNetworkInterfacesBeforePopup();
        }
    }

    void updateParameterFromEditor(juce::TextEditor* editor)
    {
        juce::String text = editor->getText();

        if (editor == &udpPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
                parameters.setConfigParam(WFSParameterIDs::networkRxUDPport.toString(), value);
        }
        else if (editor == &tcpPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
                parameters.setConfigParam(WFSParameterIDs::networkRxTCPport.toString(), value);
        }
        // ADM-OSC Offset parameters
        else if (editor == &admOscOffsetXEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("admOscOffsetX", value);
        }
        else if (editor == &admOscOffsetYEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("admOscOffsetY", value);
        }
        else if (editor == &admOscOffsetZEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("admOscOffsetZ", value);
        }
        // ADM-OSC Scale parameters
        else if (editor == &admOscScaleXEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("admOscScaleX", value);
        }
        else if (editor == &admOscScaleYEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("admOscScaleY", value);
        }
        else if (editor == &admOscScaleZEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("admOscScaleZ", value);
        }
        // Tracking Port
        else if (editor == &trackingPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
                parameters.setConfigParam("trackingPort", value);
        }
        // Tracking Offset parameters
        else if (editor == &trackingOffsetXEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("trackingOffsetX", value);
        }
        else if (editor == &trackingOffsetYEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("trackingOffsetY", value);
        }
        else if (editor == &trackingOffsetZEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("trackingOffsetZ", value);
        }
        // Tracking Scale parameters
        else if (editor == &trackingScaleXEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("trackingScaleX", value);
        }
        else if (editor == &trackingScaleYEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("trackingScaleY", value);
        }
        else if (editor == &trackingScaleZEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("trackingScaleZ", value);
        }
        // OSC Query Port
        else if (editor == &oscQueryPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
            {
                saveOscQueryToValueTree();
            }
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

    void refreshNetworkInterfacesBeforePopup()
    {
        // Remember current selection
        juce::String previousSelection;
        int selectedId = networkInterfaceSelector.getSelectedId();
        if (selectedId > 0)
        {
            int index = selectedId - 1;
            if (index < interfaceNames.size())
                previousSelection = interfaceNames[index];
        }

        // Refresh the interface list
        populateNetworkInterfaces();

        // Try to re-select the previously selected interface
        if (previousSelection.isNotEmpty())
        {
            int newIndex = interfaceNames.indexOf(previousSelection);
            if (newIndex >= 0)
                networkInterfaceSelector.setSelectedId(newIndex + 1, juce::dontSendNotification);
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
        // First, get friendly names from SystemConfiguration framework
        juce::HashMap<juce::String, juce::String> bsdToFriendlyName;
        CFArrayRef interfaces = SCNetworkInterfaceCopyAll();
        if (interfaces != nullptr)
        {
            CFIndex count = CFArrayGetCount(interfaces);
            for (CFIndex i = 0; i < count; ++i)
            {
                SCNetworkInterfaceRef interface = (SCNetworkInterfaceRef)CFArrayGetValueAtIndex(interfaces, i);
                if (interface != nullptr)
                {
                    // Get BSD name (e.g., "en0", "en1")
                    CFStringRef bsdName = SCNetworkInterfaceGetBSDName(interface);
                    // Get display/friendly name (e.g., "Ethernet", "Wi-Fi")
                    CFStringRef displayName = SCNetworkInterfaceGetLocalizedDisplayName(interface);
                    
                    if (bsdName != nullptr)
                    {
                        // Convert CFStringRef to juce::String
                        char bsdNameBuffer[256];
                        if (CFStringGetCString(bsdName, bsdNameBuffer, sizeof(bsdNameBuffer), kCFStringEncodingUTF8))
                        {
                            juce::String bsdNameStr = juce::String(bsdNameBuffer);
                            juce::String friendlyNameStr;
                            
                            if (displayName != nullptr)
                            {
                                char displayNameBuffer[256];
                                if (CFStringGetCString(displayName, displayNameBuffer, sizeof(displayNameBuffer), kCFStringEncodingUTF8))
                                {
                                    friendlyNameStr = juce::String(displayNameBuffer);
                                }
                                else
                                {
                                    // Fallback to BSD name if conversion fails
                                    friendlyNameStr = bsdNameStr;
                                }
                            }
                            else
                            {
                                // Fallback to BSD name if no display name
                                friendlyNameStr = bsdNameStr;
                            }
                            
                            bsdToFriendlyName.set(bsdNameStr, friendlyNameStr);
                        }
                    }
                }
            }
            CFRelease(interfaces);
        }
        
        // Now get actual interfaces with IP addresses using getifaddrs
        struct ifaddrs* ifaddrsList = nullptr;
        if (getifaddrs(&ifaddrsList) == 0)
        {
            // Map to track interfaces we've already added (to avoid duplicates)
            juce::HashMap<juce::String, juce::String> interfaceMap;
            
            int interfaceIndex = 1;
            for (struct ifaddrs* ifa = ifaddrsList; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr)
                    continue;
                
                // Only process IPv4 addresses
                if (ifa->ifa_addr->sa_family != AF_INET)
                    continue;
                
                // Skip loopback interfaces
                if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
                    continue;
                
                // Only process interfaces that are up and running
                if ((ifa->ifa_flags & IFF_UP) == 0 || (ifa->ifa_flags & IFF_RUNNING) == 0)
                    continue;
                
                // Get interface BSD name
                juce::String bsdName = juce::String(ifa->ifa_name);
                
                // Get friendly name if available, otherwise use BSD name
                juce::String interfaceName = bsdToFriendlyName.contains(bsdName) 
                    ? bsdToFriendlyName[bsdName] 
                    : bsdName;
                
                // Get IPv4 address
                struct sockaddr_in* sa_in = (struct sockaddr_in*)ifa->ifa_addr;
                char ipBuffer[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &(sa_in->sin_addr), ipBuffer, INET_ADDRSTRLEN) != nullptr)
                {
                    juce::String ipAddress = juce::String(ipBuffer);
                    
                    // Check if we've already added this interface (some interfaces can have multiple addresses)
                    // We'll use the first IPv4 address we find for each interface name
                    if (!interfaceMap.contains(bsdName))
                    {
                        // Add to combo box with format: "Friendly Name (IP)"
                        juce::String displayName = interfaceName + " (" + ipAddress + ")";
                        networkInterfaceSelector.addItem(displayName, interfaceIndex++);
                        
                        // Store BSD name for later use (for matching when saving/loading)
                        interfaceNames.add(bsdName);
                        interfaceIPs.add(ipAddress);
                        
                        interfaceMap.set(bsdName, ipAddress);
                    }
                }
            }
            
            freeifaddrs(ifaddrsList);
        }
        
        if (networkInterfaceSelector.getNumItems() == 0)
        {
            networkInterfaceSelector.addItem("No network adapters found", 1);
        }
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
#elif JUCE_MAC
        struct ifaddrs* ifaddrsList = nullptr;
        if (getifaddrs(&ifaddrsList) == 0)
        {
            for (struct ifaddrs* ifa = ifaddrsList; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr)
                    continue;
                
                // Only process IPv4 addresses
                if (ifa->ifa_addr->sa_family != AF_INET)
                    continue;
                
                // Skip loopback interfaces
                if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
                    continue;
                
                // Only process interfaces that are up and running
                if ((ifa->ifa_flags & IFF_UP) != 0 && (ifa->ifa_flags & IFF_RUNNING) != 0)
                {
                    struct sockaddr_in* sa_in = (struct sockaddr_in*)ifa->ifa_addr;
                    char ipBuffer[INET_ADDRSTRLEN];
                    if (inet_ntop(AF_INET, &(sa_in->sin_addr), ipBuffer, INET_ADDRSTRLEN) != nullptr)
                    {
                        currentIPEditor.setText(juce::String(ipBuffer), false);
                        freeifaddrs(ifaddrsList);
                        return;
                    }
                }
            }
            freeifaddrs(ifaddrsList);
        }
#endif
        currentIPEditor.setText("Not available", false);
    }

    // ==================== NETWORK LOG WINDOW ====================
    void openNetworkLogWindow()
    {
        if (onNetworkLogWindowRequested)
            onNetworkLogWindowRequested();
    }

    // ==================== FIND MY REMOTE ====================
    void showFindMyRemoteDialog()
    {
        auto* alertWindow = new juce::AlertWindow(
            "Find My Remote",
            "Enter the password for your remote device:",
            juce::AlertWindow::QuestionIcon
        );

        alertWindow->addTextEditor("password", findDevicePassword, "Password:", true);
        alertWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, alertWindow](int result)
            {
                if (result == 1)  // OK clicked
                {
                    findDevicePassword = alertWindow->getTextEditorContents("password");
                    if (findDevicePassword.isNotEmpty())
                    {
                        sendFindDeviceOsc();
                    }
                    else
                    {
                        if (statusBar != nullptr)
                            statusBar->setHelpText("Password cannot be empty.");
                    }
                }
                delete alertWindow;
            }
        ), true);
    }

    void sendFindDeviceOsc()
    {
        // TODO: Send OSC /findDevice with findDevicePassword
        // For now, just show a confirmation
        if (statusBar != nullptr)
            statusBar->setHelpText("Sending Find Device command...");

        // The actual OSC sending will be implemented when OSC infrastructure is ready
        DBG("Find My Remote: Sending /findDevice with password");
    }

    // Helper to show status bar messages
    void showStatusMessage(const juce::String& message, int durationMs = 3000)
    {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage(message, durationMs);
    }

    // Store/Reload methods (saves network config separately from system config)
    void storeNetworkConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder in System Config first.");
            return;
        }

        // Backup is created automatically by file manager before overwrite
        if (fileManager.saveNetworkConfig())
            showStatusMessage("Network configuration saved.");
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void reloadNetworkConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder in System Config first.");
            return;
        }

        auto configFile = fileManager.getNetworkConfigFile();
        if (!configFile.existsAsFile())
        {
            showStatusMessage("Network config file not found.");
            return;
        }

        if (fileManager.loadNetworkConfig())
        {
            loadParametersFromValueTree();
            showStatusMessage("Network configuration reloaded.");
        }
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void reloadNetworkConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder in System Config first.");
            return;
        }

        auto backups = fileManager.getBackups("network");
        if (backups.isEmpty())
        {
            showStatusMessage("No backup files found.");
            return;
        }

        if (fileManager.loadNetworkConfigBackup(0))
        {
            loadParametersFromValueTree();
            showStatusMessage("Network configuration loaded from backup.");
        }
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void importNetworkConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Import Network Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                auto& fileManager = parameters.getFileManager();
                if (fileManager.importNetworkConfig(result))
                {
                    loadParametersFromValueTree();
                    showStatusMessage("Network configuration imported.");
                }
                else
                    showStatusMessage("Error: " + fileManager.getLastError());
            }
        });
    }

    void exportNetworkConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Export Network Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result != juce::File())
            {
                if (!result.hasFileExtension(".xml"))
                    result = result.withFileExtension(".xml");

                auto& fileManager = parameters.getFileManager();
                if (fileManager.exportNetworkConfig(result))
                    showStatusMessage("Network configuration exported.");
                else
                    showStatusMessage("Error: " + fileManager.getLastError());
            }
        });
    }

    // ==================== OSC SOURCE FILTER VALUETREE METHODS ====================

    void saveOscSourceFilterToValueTree()
    {
        parameters.setConfigParam("networkOscSourceFilter", oscSourceFilterButton.getToggleState() ? 1 : 0);
    }

    // ==================== TARGET VALUETREE METHODS ====================

    void saveTargetToValueTree(int index)
    {
        if (index < 0 || index >= maxTargets)
            return;

        auto& row = targetRows[index];
        if (!row.isActive)
            return;  // Don't save inactive targets

        // Get the Network section from Config
        auto config = parameters.getConfigTree();
        if (!config.isValid())
            return;

        auto network = config.getChildWithName(WFSParameterIDs::Network);
        if (!network.isValid())
            return;

        // Find or create target child
        juce::ValueTree target;
        for (int i = 0; i < network.getNumChildren(); ++i)
        {
            auto child = network.getChild(i);
            if (child.getType() == WFSParameterIDs::NetworkTarget)
            {
                int targetId = child.getProperty(WFSParameterIDs::id, -1);
                if (targetId == index)
                {
                    target = child;
                    break;
                }
            }
        }

        // Create new target if not found
        if (!target.isValid())
        {
            target = juce::ValueTree(WFSParameterIDs::NetworkTarget);
            target.setProperty(WFSParameterIDs::id, index, nullptr);
            network.appendChild(target, nullptr);
        }

        // Save all properties
        target.setProperty(WFSParameterIDs::networkTSname, row.nameEditor.getText(), nullptr);
        target.setProperty(WFSParameterIDs::networkTSdataMode, row.dataModeSelector.getSelectedId() - 1, nullptr);
        target.setProperty(WFSParameterIDs::networkTSip, row.ipEditor.getText(), nullptr);
        target.setProperty(WFSParameterIDs::networkTSport, row.txPortEditor.getText().getIntValue(), nullptr);
        target.setProperty(WFSParameterIDs::networkTSrxEnable, row.rxEnableButton.getToggleState() ? 1 : 0, nullptr);
        target.setProperty(WFSParameterIDs::networkTStxEnable, row.txEnableButton.getToggleState() ? 1 : 0, nullptr);
        target.setProperty(WFSParameterIDs::networkTSProtocol, row.protocolSelector.getSelectedId() - 1, nullptr);

        // Update OSCManager with new configuration
        updateOSCManagerConfig();
    }

    void removeTargetFromValueTree(int index)
    {
        if (index < 0 || index >= maxTargets)
            return;

        auto config = parameters.getConfigTree();
        if (!config.isValid())
            return;

        auto network = config.getChildWithName(WFSParameterIDs::Network);
        if (!network.isValid())
            return;

        // Find and remove target with matching id
        for (int i = network.getNumChildren() - 1; i >= 0; --i)
        {
            auto child = network.getChild(i);
            if (child.getType() == WFSParameterIDs::NetworkTarget)
            {
                int targetId = child.getProperty(WFSParameterIDs::id, -1);
                if (targetId == index)
                {
                    network.removeChild(i, nullptr);
                    break;
                }
            }
        }
    }

    // ==================== OSC QUERY ====================

    void saveOscQueryToValueTree()
    {
        int port = oscQueryPortEditor.getText().getIntValue();
        if (port < 0 || port > 65535)
            port = 5005;

        parameters.setConfigParam(WFSParameterIDs::networkOscQueryPort.toString(), port);
        parameters.setConfigParam(WFSParameterIDs::networkOscQueryEnabled.toString(),
                                  oscQueryEnableButton.getToggleState() ? 1 : 0);
    }

    void updateOSCQueryServer()
    {
        if (oscManager == nullptr)
            return;

        bool enabled = oscQueryEnableButton.getToggleState();
        int httpPort = oscQueryPortEditor.getText().getIntValue();
        if (httpPort <= 0 || httpPort > 65535)
            httpPort = 5005;

        if (enabled)
        {
            // Get UDP port for OSC from current config
            int oscPort = udpPortEditor.getText().getIntValue();
            if (oscPort <= 0)
                oscPort = 9001;

            if (!oscManager->isOSCQueryRunning())
            {
                oscManager->startOSCQuery(oscPort, httpPort);
            }
        }
        else
        {
            if (oscManager->isOSCQueryRunning())
            {
                oscManager->stopOSCQuery();
            }
        }
    }

    void loadTargetsFromValueTree()
    {
        // Reset all rows first
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            row.nameEditor.setText("Target " + juce::String(i + 1), false);
            row.dataModeSelector.setSelectedId(1, juce::dontSendNotification);
            row.ipEditor.setText("127.0.0.1", false);
            row.txPortEditor.setText("9000", false);
            row.rxEnableButton.setToggleState(false, juce::dontSendNotification);
            row.rxEnableButton.setButtonText("OFF");
            row.txEnableButton.setToggleState(false, juce::dontSendNotification);
            row.txEnableButton.setButtonText("OFF");
            row.protocolSelector.setSelectedId(1, juce::dontSendNotification);
            row.isActive = false;
        }
        activeTargetCount = 0;

        // Get the Network section from Config
        auto config = parameters.getConfigTree();
        if (!config.isValid())
        {
            updateTargetRowVisibility();
            updateAddButtonState();
            return;
        }

        auto network = config.getChildWithName(WFSParameterIDs::Network);
        if (!network.isValid())
        {
            updateTargetRowVisibility();
            updateAddButtonState();
            return;
        }

        // Load all targets
        for (int i = 0; i < network.getNumChildren(); ++i)
        {
            auto child = network.getChild(i);
            if (child.getType() == WFSParameterIDs::NetworkTarget)
            {
                int targetId = child.getProperty(WFSParameterIDs::id, 0);
                if (targetId >= 0 && targetId < maxTargets)
                {
                    auto& row = targetRows[targetId];
                    row.isActive = true;
                    activeTargetCount++;

                    row.nameEditor.setText(child.getProperty(WFSParameterIDs::networkTSname, "Target " + juce::String(targetId + 1)).toString(), false);
                    row.dataModeSelector.setSelectedId((int)child.getProperty(WFSParameterIDs::networkTSdataMode, 0) + 1, juce::dontSendNotification);
                    row.ipEditor.setText(child.getProperty(WFSParameterIDs::networkTSip, "127.0.0.1").toString(), false);
                    row.txPortEditor.setText(juce::String((int)child.getProperty(WFSParameterIDs::networkTSport, 9000)), false);

                    bool rxEnabled = (int)child.getProperty(WFSParameterIDs::networkTSrxEnable, 0) != 0;
                    row.rxEnableButton.setToggleState(rxEnabled, juce::dontSendNotification);
                    row.rxEnableButton.setButtonText(rxEnabled ? "ON" : "OFF");

                    bool txEnabled = (int)child.getProperty(WFSParameterIDs::networkTStxEnable, 0) != 0;
                    row.txEnableButton.setToggleState(txEnabled, juce::dontSendNotification);
                    row.txEnableButton.setButtonText(txEnabled ? "ON" : "OFF");

                    row.protocolSelector.setSelectedId((int)child.getProperty(WFSParameterIDs::networkTSProtocol, 0) + 1, juce::dontSendNotification);
                }
            }
        }

        updateTargetRowVisibility();
        updateAddButtonState();
        updateAdmOscAppearance();
    }

    /**
     * Check for cluster conflicts when enabling global tracking.
     * If any cluster has more than one input with local tracking enabled,
     * show a warning and keep only the first one, disabling the others.
     * @param fromProtocolChange true if called from protocol selector change
     */
    void checkGlobalTrackingConstraintAsync(bool fromProtocolChange = false)
    {
        int protocolEnabled = fromProtocolChange ?
            (trackingProtocolSelector.getSelectedId() - 1) :
            static_cast<int>(parameters.getConfigParam("trackingProtocol"));

        // If protocol is disabled, no conflict possible
        if (protocolEnabled == 0 && !fromProtocolChange)
        {
            trackingEnabledButton.setButtonText("Tracking: ON");
            parameters.setConfigParam("trackingEnabled", 1);
            updateTrackingAppearance();
            return;
        }

        // Find clusters with multiple locally-tracked inputs
        int numInputs = parameters.getNumInputChannels();
        std::map<int, std::vector<int>> clusterTrackedInputs;  // cluster -> list of inputs with local tracking

        for (int i = 0; i < numInputs; ++i)
        {
            int cluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (cluster > 0)  // Not "Single"
            {
                int localTracking = static_cast<int>(parameters.getInputParam(i, "inputTrackingActive"));
                if (localTracking != 0)
                {
                    clusterTrackedInputs[cluster].push_back(i);
                }
            }
        }

        // Find conflicts (clusters with more than one tracked input)
        std::vector<std::pair<int, std::vector<int>>> conflicts;
        for (const auto& [cluster, inputs] : clusterTrackedInputs)
        {
            if (inputs.size() > 1)
            {
                conflicts.push_back({cluster, inputs});
            }
        }

        if (conflicts.empty())
        {
            // No conflicts, proceed with enabling
            if (fromProtocolChange)
            {
                parameters.setConfigParam("trackingProtocol", trackingProtocolSelector.getSelectedId() - 1);
            }
            else
            {
                trackingEnabledButton.setButtonText("Tracking: ON");
                parameters.setConfigParam("trackingEnabled", 1);
                updateTrackingAppearance();
            }
            return;
        }

        // Build conflict message
        juce::String conflictMsg = "The following clusters have multiple inputs with tracking enabled:\n\n";
        for (const auto& [cluster, inputs] : conflicts)
        {
            conflictMsg += "Cluster " + juce::String(cluster) + ": Inputs ";
            for (size_t j = 0; j < inputs.size(); ++j)
            {
                if (j > 0) conflictMsg += ", ";
                conflictMsg += juce::String(inputs[j] + 1);
            }
            conflictMsg += "\n";
        }
        conflictMsg += "\nOnly one tracked input per cluster is allowed. "
                       "If you continue, tracking will be kept only for the first input in each cluster.";

        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Tracking Conflicts Detected",
            conflictMsg,
            "Continue",
            "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this, conflicts, fromProtocolChange](int result) {
                if (result == 1)  // Continue
                {
                    // Disable tracking on all but the first input in each conflicting cluster
                    for (const auto& [cluster, inputs] : conflicts)
                    {
                        for (size_t j = 1; j < inputs.size(); ++j)
                        {
                            parameters.setInputParam(inputs[j], "inputTrackingActive", 0);
                        }
                    }

                    // Now enable global tracking or protocol
                    if (fromProtocolChange)
                    {
                        parameters.setConfigParam("trackingProtocol", trackingProtocolSelector.getSelectedId() - 1);
                    }
                    else
                    {
                        trackingEnabledButton.setButtonText("Tracking: ON");
                        parameters.setConfigParam("trackingEnabled", 1);
                        updateTrackingAppearance();
                    }
                }
                else  // Cancel
                {
                    // Revert the toggle/selector
                    if (fromProtocolChange)
                    {
                        // Revert to DISABLED
                        trackingProtocolSelector.setSelectedId(1, juce::dontSendNotification);
                    }
                    else
                    {
                        trackingEnabledButton.setToggleState(false, juce::dontSendNotification);
                        trackingEnabledButton.setButtonText("Tracking: OFF");
                    }
                }
            })
        );
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkTab)
};
