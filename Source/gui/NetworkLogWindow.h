#pragma once

#include <JuceHeader.h>
#include "../Network/OSCLogger.h"
#include "../Network/OSCManager.h"

namespace WFSNetwork { class OSCLogger; class OSCManager; }

/**
 * Filter modes for the network log display
 */
enum class NetworkLogFilterMode
{
    Transport,  // Filter by TCP/UDP
    Protocol,   // Filter by protocol type
    ClientIP,   // Filter by client IP address
    Rejected    // Show only rejected messages
};

/**
 * NetworkLogTableComponent
 * Custom scrollable table for displaying network log entries with color coding.
 */
class NetworkLogTableComponent : public juce::Component,
                                  public juce::ScrollBar::Listener
{
public:
    NetworkLogTableComponent();
    ~NetworkLogTableComponent() override = default;

    /** Set the entries to display */
    void setEntries(const std::vector<WFSNetwork::LogEntry>& newEntries);

    /** Set the color provider function */
    void setColorProvider(std::function<juce::Colour(const WFSNetwork::LogEntry&)> provider);

    /** Scroll to top of log */
    void scrollToTop();

    /** Scroll to bottom of log (newest entries) */
    void scrollToBottom();

    /** Check if currently at bottom (for auto-scroll) */
    bool isAtBottom() const;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void scrollBarMoved(juce::ScrollBar* bar, double newRangeStart) override;

private:
    struct ColumnDef
    {
        juce::String name;
        int width;
        bool flexible = false;  // If true, width is minimum and expands to fill
    };

    void setupColumns();
    void drawHeader(juce::Graphics& g);
    void drawRow(juce::Graphics& g, int rowIndex, int y, const WFSNetwork::LogEntry& entry);
    juce::String getColumnValue(const WFSNetwork::LogEntry& entry, int columnIndex) const;
    void updateScrollBar();

    std::vector<ColumnDef> columns;
    std::vector<WFSNetwork::LogEntry> entries;
    std::function<juce::Colour(const WFSNetwork::LogEntry&)> colorProvider;

    juce::ScrollBar verticalScrollBar { true };
    int rowHeight = 20;
    int headerHeight = 24;
    int scrollOffset = 0;
    bool userScrolledAway = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkLogTableComponent)
};

/**
 * NetworkLogWindowContent
 * Main content component with controls and table for the network log window.
 */
class NetworkLogWindowContent : public juce::Component,
                                 private juce::Timer
{
public:
    NetworkLogWindowContent(WFSNetwork::OSCLogger& logger,
                            WFSNetwork::OSCManager& oscManager,
                            const juce::File& projectFolder);
    ~NetworkLogWindowContent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateTable();
    void applyFilters();
    void updateFilterToggles();
    juce::Colour getColorForEntry(const WFSNetwork::LogEntry& entry);
    void exportToCSV(bool filteredOnly);

    WFSNetwork::OSCLogger& logger;
    WFSNetwork::OSCManager& oscManager;
    juce::File projectFolder;

    // Controls - top bar
    juce::ToggleButton loggingSwitch { "Logging" };
    juce::TextButton clearButton { "CLEAR" };
    juce::TextButton exportButton { "EXPORT" };
    juce::ComboBox filterModeSelector;
    juce::TextButton topButton { juce::CharPointer_UTF8("\xe2\x86\x91") };      // ↑
    juce::TextButton bottomButton { juce::CharPointer_UTF8("\xe2\x86\x93") };   // ↓

    // Filter toggles - dynamically created based on mode
    juce::OwnedArray<juce::ToggleButton> filterToggles;
    std::map<juce::String, bool> toggleStates;  // Preserve toggle states

    // Table
    std::unique_ptr<NetworkLogTableComponent> logTable;

    // Cached data
    std::vector<WFSNetwork::LogEntry> allEntries;
    std::vector<WFSNetwork::LogEntry> filteredEntries;
    int64_t lastKnownEntryCount = 0;

    // Filter state
    NetworkLogFilterMode currentFilterMode = NetworkLogFilterMode::Transport;

    // Color palettes
    std::map<juce::String, juce::Colour> ipColorMap;
    int nextIpColorIndex = 0;
    static const std::array<juce::Colour, 8> ipColorPalette;

    // Protocol colors
    static juce::Colour getProtocolColor(WFSNetwork::Protocol protocol);
    static juce::Colour getTransportColor(WFSNetwork::ConnectionMode transport);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkLogWindowContent)
};

/**
 * NetworkLogWindow
 * Floating window for monitoring network traffic.
 * Can be placed on a second monitor.
 */
class NetworkLogWindow : public juce::DocumentWindow
{
public:
    NetworkLogWindow(WFSNetwork::OSCLogger& logger,
                     WFSNetwork::OSCManager& oscManager,
                     const juce::File& projectFolder)
        : DocumentWindow("Network Log",
                         juce::Colour(0xFF1E1E1E),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        // Create content
        auto* content = new NetworkLogWindowContent(logger, oscManager, projectFolder);
        setContentOwned(content, false);

        // Window size
        const int preferredWidth = 900;
        const int preferredHeight = 600;

        // Get display bounds
        auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* displayPtr = displays.getPrimaryDisplay();
        juce::Rectangle<int> userArea = (displayPtr != nullptr && !displayPtr->userArea.isEmpty())
            ? displayPtr->userArea
            : displays.getTotalBounds(true);

        const int margin = 40;
        const int windowWidth = juce::jmin(preferredWidth, userArea.getWidth() - margin);
        const int windowHeight = juce::jmin(preferredHeight, userArea.getHeight() - margin);

        setResizeLimits(600, 400, userArea.getWidth(), userArea.getHeight());

        centreWithSize(windowWidth, windowHeight);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkLogWindow)
};
