#include "NetworkLogWindow.h"

//==============================================================================
// Color Constants
//==============================================================================

const std::array<juce::Colour, 8> NetworkLogWindowContent::ipColorPalette = {
    juce::Colour(0xFF4488CC),  // Blue
    juce::Colour(0xFF44CC88),  // Green
    juce::Colour(0xFFCC8844),  // Orange
    juce::Colour(0xFF8844CC),  // Purple
    juce::Colour(0xFFCC4488),  // Pink
    juce::Colour(0xFF88CC44),  // Lime
    juce::Colour(0xFF44CCCC),  // Cyan
    juce::Colour(0xFFCCCC44)   // Yellow
};

juce::Colour NetworkLogWindowContent::getProtocolColor(WFSNetwork::Protocol protocol)
{
    switch (protocol)
    {
        case WFSNetwork::Protocol::OSC:      return juce::Colour(0xFF4477AA);
        case WFSNetwork::Protocol::OSCQuery: return juce::Colour(0xFF44AA77);
        case WFSNetwork::Protocol::Remote:   return juce::Colour(0xFFAA7744);
        case WFSNetwork::Protocol::ADMOSC:   return juce::Colour(0xFF7744AA);
        case WFSNetwork::Protocol::PSN:      return juce::Colour(0xFFAA4477);
        case WFSNetwork::Protocol::RTTrP:    return juce::Colour(0xFF77AA44);
        default:                             return juce::Colour(0xFF888888);
    }
}

juce::Colour NetworkLogWindowContent::getTransportColor(WFSNetwork::ConnectionMode transport)
{
    return transport == WFSNetwork::ConnectionMode::TCP
        ? juce::Colour(0xFF44CC88)   // Green for TCP
        : juce::Colour(0xFF4488CC);  // Blue for UDP
}

//==============================================================================
// NetworkLogTableComponent
//==============================================================================

NetworkLogTableComponent::NetworkLogTableComponent()
{
    setupColumns();
    addAndMakeVisible(verticalScrollBar);
    verticalScrollBar.addListener(this);
    verticalScrollBar.setRangeLimits(0.0, 1.0);
}

void NetworkLogTableComponent::setupColumns()
{
    columns.clear();
    columns.push_back({ "Time", 85, false });
    columns.push_back({ "Dir", 35, false });
    columns.push_back({ "IP", 110, false });
    columns.push_back({ "Port", 50, false });
    columns.push_back({ "Trans", 45, false });
    columns.push_back({ "Protocol", 65, false });
    columns.push_back({ "Address", 180, false });
    columns.push_back({ "Arguments", 200, true });  // Flexible column
}

void NetworkLogTableComponent::setEntries(const std::vector<WFSNetwork::LogEntry>& newEntries)
{
    bool wasAtBottom = isAtBottom();
    entries = newEntries;
    updateScrollBar();

    // Auto-scroll if user hasn't scrolled away
    if (wasAtBottom && !userScrolledAway)
    {
        scrollToBottom();
    }

    repaint();
}

void NetworkLogTableComponent::setColorProvider(std::function<juce::Colour(const WFSNetwork::LogEntry&)> provider)
{
    colorProvider = std::move(provider);
    repaint();
}

void NetworkLogTableComponent::scrollToTop()
{
    scrollOffset = 0;
    userScrolledAway = true;
    updateScrollBar();
    repaint();
}

void NetworkLogTableComponent::scrollToBottom()
{
    int contentHeight = static_cast<int>(entries.size()) * rowHeight;
    int viewHeight = getHeight() - headerHeight;
    scrollOffset = juce::jmax(0, contentHeight - viewHeight);
    userScrolledAway = false;
    updateScrollBar();
    repaint();
}

bool NetworkLogTableComponent::isAtBottom() const
{
    int contentHeight = static_cast<int>(entries.size()) * rowHeight;
    int viewHeight = getHeight() - headerHeight;
    return scrollOffset >= contentHeight - viewHeight - rowHeight;
}

void NetworkLogTableComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xFF1E1E1E));

    // Draw header
    drawHeader(g);

    // Draw rows
    int y = headerHeight;
    int viewHeight = bounds.getHeight() - headerHeight;
    int firstVisibleRow = scrollOffset / rowHeight;
    int startY = -(scrollOffset % rowHeight);

    g.reduceClipRegion(0, headerHeight, bounds.getWidth() - 12, viewHeight);

    for (int i = firstVisibleRow; i < static_cast<int>(entries.size()); ++i)
    {
        int rowY = y + startY + (i - firstVisibleRow) * rowHeight;
        if (rowY > bounds.getHeight())
            break;
        if (rowY + rowHeight < headerHeight)
            continue;

        drawRow(g, i, rowY, entries[static_cast<size_t>(i)]);
    }
}

void NetworkLogTableComponent::drawHeader(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xFF252525));
    g.fillRect(0, 0, getWidth(), headerHeight);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(13.0f).boldened());

    int x = 4;
    int availableWidth = getWidth() - 12;  // Account for scrollbar
    int fixedWidth = 0;
    int flexCount = 0;

    for (const auto& col : columns)
    {
        if (col.flexible)
            flexCount++;
        else
            fixedWidth += col.width;
    }

    int flexWidth = flexCount > 0 ? (availableWidth - fixedWidth) / flexCount : 0;

    for (const auto& col : columns)
    {
        int colWidth = col.flexible ? juce::jmax(col.width, flexWidth) : col.width;
        g.drawText(col.name, x, 0, colWidth - 4, headerHeight, juce::Justification::centredLeft, true);
        x += colWidth;
    }

    // Header bottom line
    g.setColour(juce::Colour(0xFF3A3A3A));
    g.drawHorizontalLine(headerHeight - 1, 0.0f, static_cast<float>(getWidth()));
}

void NetworkLogTableComponent::drawRow(juce::Graphics& g, int rowIndex, int y, const WFSNetwork::LogEntry& entry)
{
    int availableWidth = getWidth() - 12;

    // Alternate row background
    if (rowIndex % 2 == 1)
    {
        g.setColour(juce::Colour(0xFF252525));
        g.fillRect(0, y, availableWidth, rowHeight);
    }

    // Color tint based on filter mode
    if (colorProvider)
    {
        juce::Colour rowColor = colorProvider(entry);
        g.setColour(rowColor.withAlpha(0.15f));
        g.fillRect(0, y, availableWidth, rowHeight);
    }

    // Rejected entries have red tint
    if (entry.isRejected)
    {
        g.setColour(juce::Colour(0x30AA4444));
        g.fillRect(0, y, availableWidth, rowHeight);
    }

    // Text
    g.setColour(entry.isRejected ? juce::Colour(0xFFCC8888) : juce::Colours::white);
    g.setFont(juce::Font(12.0f));

    int x = 4;
    int fixedWidth = 0;
    int flexCount = 0;

    for (const auto& col : columns)
    {
        if (col.flexible)
            flexCount++;
        else
            fixedWidth += col.width;
    }

    int flexWidth = flexCount > 0 ? (availableWidth - fixedWidth) / flexCount : 0;

    for (int i = 0; i < static_cast<int>(columns.size()); ++i)
    {
        int colWidth = columns[static_cast<size_t>(i)].flexible
            ? juce::jmax(columns[static_cast<size_t>(i)].width, flexWidth)
            : columns[static_cast<size_t>(i)].width;

        juce::String value = getColumnValue(entry, i);
        g.drawText(value, x, y, colWidth - 4, rowHeight, juce::Justification::centredLeft, true);
        x += colWidth;
    }
}

juce::String NetworkLogTableComponent::getColumnValue(const WFSNetwork::LogEntry& entry, int columnIndex) const
{
    switch (columnIndex)
    {
        case 0:  // Time
            return entry.timestamp.formatted("%H:%M:%S.") +
                   juce::String(entry.timestamp.getMilliseconds()).paddedLeft('0', 3);
        case 1:  // Direction
            return entry.direction;
        case 2:  // IP
            return entry.ipAddress;
        case 3:  // Port
            return entry.port > 0 ? juce::String(entry.port) : "";
        case 4:  // Transport
            return entry.getTransportString();
        case 5:  // Protocol
            return entry.isRejected ? "REJECTED" : entry.getProtocolString();
        case 6:  // Address
            return entry.address;
        case 7:  // Arguments
            return entry.isRejected ? entry.rejectReason : entry.arguments;
        default:
            return "";
    }
}

void NetworkLogTableComponent::resized()
{
    auto bounds = getLocalBounds();
    verticalScrollBar.setBounds(bounds.getWidth() - 12, headerHeight,
                                 12, bounds.getHeight() - headerHeight);
    updateScrollBar();
}

void NetworkLogTableComponent::updateScrollBar()
{
    int contentHeight = static_cast<int>(entries.size()) * rowHeight;
    int viewHeight = getHeight() - headerHeight;

    if (contentHeight <= viewHeight)
    {
        verticalScrollBar.setVisible(false);
        scrollOffset = 0;
    }
    else
    {
        verticalScrollBar.setVisible(true);
        double thumbSize = static_cast<double>(viewHeight) / static_cast<double>(contentHeight);
        verticalScrollBar.setCurrentRange(
            static_cast<double>(scrollOffset) / static_cast<double>(contentHeight),
            thumbSize);
    }
}

void NetworkLogTableComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    int contentHeight = static_cast<int>(entries.size()) * rowHeight;
    int viewHeight = getHeight() - headerHeight;
    int maxScroll = juce::jmax(0, contentHeight - viewHeight);

    int delta = static_cast<int>(wheel.deltaY * 100.0f);
    scrollOffset = juce::jlimit(0, maxScroll, scrollOffset - delta);

    // Track if user manually scrolled away from bottom
    userScrolledAway = !isAtBottom();

    updateScrollBar();
    repaint();
}

void NetworkLogTableComponent::scrollBarMoved(juce::ScrollBar*, double newRangeStart)
{
    int contentHeight = static_cast<int>(entries.size()) * rowHeight;
    scrollOffset = static_cast<int>(newRangeStart * contentHeight);

    userScrolledAway = !isAtBottom();

    repaint();
}

//==============================================================================
// NetworkLogWindowContent
//==============================================================================

NetworkLogWindowContent::NetworkLogWindowContent(WFSNetwork::OSCLogger& log,
                                                   WFSNetwork::OSCManager& osc,
                                                   const juce::File& folder)
    : logger(log)
    , oscManager(osc)
    , projectFolder(folder)
{
    // Logging toggle
    loggingSwitch.setToggleState(logger.getEnabled(), juce::dontSendNotification);
    loggingSwitch.onClick = [this]()
    {
        logger.setEnabled(loggingSwitch.getToggleState());
    };
    addAndMakeVisible(loggingSwitch);

    // Clear button
    clearButton.onClick = [this]()
    {
        logger.clear();
        allEntries.clear();
        filteredEntries.clear();
        lastKnownEntryCount = 0;
        logTable->setEntries({});
        updateFilterToggles();
    };
    addAndMakeVisible(clearButton);

    // Export button
    exportButton.onClick = [this]()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Export All");
        menu.addItem(2, "Export Filtered");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&exportButton),
            [this](int result)
            {
                if (result == 1)
                    exportToCSV(false);
                else if (result == 2)
                    exportToCSV(true);
            });
    };
    addAndMakeVisible(exportButton);

    // Filter mode selector
    filterModeSelector.addItem("TCP/UDP", 1);
    filterModeSelector.addItem("Protocol", 2);
    filterModeSelector.addItem("Client IP", 3);
    filterModeSelector.addItem("Rejected", 4);
    filterModeSelector.setSelectedId(1, juce::dontSendNotification);
    filterModeSelector.onChange = [this]()
    {
        currentFilterMode = static_cast<NetworkLogFilterMode>(filterModeSelector.getSelectedId() - 1);
        updateFilterToggles();
        applyFilters();
    };
    addAndMakeVisible(filterModeSelector);

    // Navigation buttons
    topButton.onClick = [this]() { logTable->scrollToTop(); };
    bottomButton.onClick = [this]() { logTable->scrollToBottom(); };
    addAndMakeVisible(topButton);
    addAndMakeVisible(bottomButton);

    // Table
    logTable = std::make_unique<NetworkLogTableComponent>();
    logTable->setColorProvider([this](const WFSNetwork::LogEntry& entry)
    {
        return getColorForEntry(entry);
    });
    addAndMakeVisible(*logTable);

    // Initial filter toggles
    updateFilterToggles();

    // Start polling timer (50ms = 20Hz)
    startTimer(50);
}

NetworkLogWindowContent::~NetworkLogWindowContent()
{
    stopTimer();
}

void NetworkLogWindowContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));
}

void NetworkLogWindowContent::resized()
{
    auto bounds = getLocalBounds();
    int y = 8;

    // Top control bar
    int controlHeight = 28;
    int x = 8;
    int spacing = 8;

    loggingSwitch.setBounds(x, y, 80, controlHeight);
    x += 80 + spacing;

    clearButton.setBounds(x, y, 60, controlHeight);
    x += 60 + spacing;

    exportButton.setBounds(x, y, 70, controlHeight);
    x += 70 + spacing;

    filterModeSelector.setBounds(x, y, 100, controlHeight);
    x += 100 + spacing;

    // Navigation buttons on right
    int navWidth = 30;
    bottomButton.setBounds(bounds.getWidth() - navWidth - 8, y, navWidth, controlHeight);
    topButton.setBounds(bounds.getWidth() - navWidth * 2 - 12, y, navWidth, controlHeight);

    y += controlHeight + 8;

    // Filter toggles row
    int toggleHeight = 24;
    x = 8;
    for (auto* toggle : filterToggles)
    {
        int toggleWidth = toggle->getButtonText().length() * 8 + 24;
        toggle->setBounds(x, y, toggleWidth, toggleHeight);
        x += toggleWidth + 4;
    }

    y += toggleHeight + 8;

    // Table fills remaining space
    logTable->setBounds(8, y, bounds.getWidth() - 16, bounds.getHeight() - y - 8);
}

void NetworkLogWindowContent::timerCallback()
{
    auto currentCount = logger.getTotalEntryCount();
    if (currentCount != lastKnownEntryCount)
    {
        lastKnownEntryCount = currentCount;
        updateTable();
    }
}

void NetworkLogWindowContent::updateTable()
{
    allEntries = logger.getEntries();

    // Update filter toggles if we have new IPs or protocols
    updateFilterToggles();

    applyFilters();
}

void NetworkLogWindowContent::applyFilters()
{
    WFSNetwork::OSCLogger::Filter filter;

    // Check toggle states
    bool showRx = true, showTx = true;
    bool showUDP = true, showTCP = true;

    for (auto* toggle : filterToggles)
    {
        juce::String name = toggle->getButtonText();
        bool enabled = toggle->getToggleState();

        if (name == "Incoming") showRx = enabled;
        else if (name == "Outgoing") showTx = enabled;
        else if (name == "UDP") showUDP = enabled;
        else if (name == "TCP") showTCP = enabled;
    }

    filter.showRx = showRx;
    filter.showTx = showTx;
    filter.showUDP = showUDP;
    filter.showTCP = showTCP;

    // Mode-specific filtering
    if (currentFilterMode == NetworkLogFilterMode::Rejected)
    {
        filter.showRejected = true;
    }
    else if (currentFilterMode == NetworkLogFilterMode::Protocol)
    {
        // Build set of enabled protocols
        for (auto* toggle : filterToggles)
        {
            juce::String name = toggle->getButtonText();
            if (name != "Incoming" && name != "Outgoing" && toggle->getToggleState())
            {
                if (name == "OSC") filter.enabledProtocols.insert(WFSNetwork::Protocol::OSC);
                else if (name == "OSCQuery") filter.enabledProtocols.insert(WFSNetwork::Protocol::OSCQuery);
                else if (name == "Remote") filter.enabledProtocols.insert(WFSNetwork::Protocol::Remote);
                else if (name == "ADM-OSC") filter.enabledProtocols.insert(WFSNetwork::Protocol::ADMOSC);
                else if (name == "PSN") filter.enabledProtocols.insert(WFSNetwork::Protocol::PSN);
                else if (name == "RTTrP") filter.enabledProtocols.insert(WFSNetwork::Protocol::RTTrP);
            }
        }
    }
    else if (currentFilterMode == NetworkLogFilterMode::ClientIP)
    {
        // Build set of enabled IPs
        for (auto* toggle : filterToggles)
        {
            juce::String name = toggle->getButtonText();
            if (name != "Incoming" && name != "Outgoing" && toggle->getToggleState())
            {
                filter.enabledIPs.insert(name);
            }
        }
    }

    filteredEntries = logger.getFilteredEntries(filter);
    logTable->setEntries(filteredEntries);
}

void NetworkLogWindowContent::updateFilterToggles()
{
    // Save current states
    for (auto* toggle : filterToggles)
    {
        toggleStates[toggle->getButtonText()] = toggle->getToggleState();
    }

    filterToggles.clear();

    auto addToggle = [this](const juce::String& name, bool defaultState = true)
    {
        auto* toggle = new juce::ToggleButton(name);
        bool state = toggleStates.count(name) > 0 ? toggleStates[name] : defaultState;
        toggle->setToggleState(state, juce::dontSendNotification);
        toggle->onClick = [this]() { applyFilters(); };
        filterToggles.add(toggle);
        addAndMakeVisible(toggle);
    };

    switch (currentFilterMode)
    {
        case NetworkLogFilterMode::Transport:
            addToggle("Incoming");
            addToggle("Outgoing");
            addToggle("UDP");
            addToggle("TCP");
            break;

        case NetworkLogFilterMode::Protocol:
        {
            addToggle("Incoming");
            addToggle("Outgoing");

            // Add toggles for protocols seen in log
            auto protocols = logger.getUniqueProtocols();
            for (auto proto : protocols)
            {
                WFSNetwork::LogEntry temp;
                temp.protocol = proto;
                addToggle(temp.getProtocolString());
            }
            break;
        }

        case NetworkLogFilterMode::ClientIP:
        {
            addToggle("Incoming");
            addToggle("Outgoing");

            // Add toggles for unique IPs
            auto ips = logger.getUniqueIPs();
            for (const auto& ip : ips)
            {
                addToggle(ip);

                // Assign color if not already assigned
                if (ipColorMap.find(ip) == ipColorMap.end())
                {
                    ipColorMap[ip] = ipColorPalette[static_cast<size_t>(nextIpColorIndex) % ipColorPalette.size()];
                    nextIpColorIndex++;
                }
            }
            break;
        }

        case NetworkLogFilterMode::Rejected:
            // No toggles for rejected mode - shows all rejected
            break;
    }

    resized();
}

juce::Colour NetworkLogWindowContent::getColorForEntry(const WFSNetwork::LogEntry& entry)
{
    switch (currentFilterMode)
    {
        case NetworkLogFilterMode::Transport:
            return getTransportColor(entry.transport);

        case NetworkLogFilterMode::Protocol:
            return getProtocolColor(entry.protocol);

        case NetworkLogFilterMode::ClientIP:
        {
            auto it = ipColorMap.find(entry.ipAddress);
            if (it != ipColorMap.end())
                return it->second;
            return juce::Colour(0xFF888888);
        }

        case NetworkLogFilterMode::Rejected:
            return juce::Colour(0xFFAA4444);

        default:
            return juce::Colour(0xFF888888);
    }
}

void NetworkLogWindowContent::exportToCSV(bool filteredOnly)
{
    auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    auto filename = "network_log_" + timestamp + ".csv";

    juce::File exportFile;
    if (projectFolder.exists())
    {
        exportFile = projectFolder.getChildFile(filename);
    }
    else
    {
        // Fallback to desktop
        exportFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile(filename);
    }

    juce::FileOutputStream output(exportFile);
    if (!output.openedOk())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Export Failed", "Could not create file: " + exportFile.getFullPathName());
        return;
    }

    // Header
    output.writeText("Timestamp,Direction,IP,Port,Transport,Protocol,Address,Arguments,Rejected,RejectReason\n",
                     false, false, nullptr);

    // Data
    const auto& entries = filteredOnly ? filteredEntries : allEntries;
    for (const auto& entry : entries)
    {
        juce::String line;
        line += "\"" + entry.timestamp.formatted("%Y-%m-%d %H:%M:%S.") +
                juce::String(entry.timestamp.getMilliseconds()).paddedLeft('0', 3) + "\",";
        line += "\"" + entry.direction + "\",";
        line += "\"" + entry.ipAddress + "\",";
        line += juce::String(entry.port) + ",";
        line += "\"" + entry.getTransportString() + "\",";
        line += "\"" + entry.getProtocolString() + "\",";
        line += "\"" + entry.address.replace("\"", "\"\"") + "\",";
        line += "\"" + entry.arguments.replace("\"", "\"\"") + "\",";
        line += entry.isRejected ? "true" : "false";
        line += ",\"" + entry.rejectReason.replace("\"", "\"\"") + "\"";
        line += "\n";

        output.writeText(line, false, false, nullptr);
    }

    output.flush();

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "Export Complete",
        "Exported " + juce::String(static_cast<int>(entries.size())) + " entries to:\n" + exportFile.getFullPathName());
}
