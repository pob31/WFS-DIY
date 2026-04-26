#include "MCPResourceRegistry.h"

namespace WFSNetwork
{

namespace
{
    constexpr const char* kMarkdownMime = "text/markdown";

    struct CatalogRow
    {
        const char* uri;
        const char* filename;
        const char* name;
        const char* description;
    };

    // Hand-curated catalog — mirrors the table in
    // Documentation/MCP/specs/MCP_RESOURCES.md §Resource catalog (rows 41–57).
    // When that table changes, update both places.
    constexpr CatalogRow kCatalog[] =
    {
        {
            "wfs://knowledge/psychoacoustics",
            "knowledge_psychoacoustics.md",
            "How humans localize sound",
            "Explains binaural localization (time and intensity differences), the precedence effect, "
            "the Haas effect, and spectral masking. Useful when explaining WHY WFS works the way it does, "
            "or when troubleshooting localization issues."
        },
        {
            "wfs://knowledge/wfs_theory",
            "knowledge_wfs_theory.md",
            "Wave Field Synthesis algorithm",
            "Explains the core WFS algorithm: wave front reconstruction, how delay and attenuation are "
            "computed per source per speaker, the design objectives and trade-offs (Doppler effect, no "
            "focused sources between array and audience). Useful when the user asks conceptual questions "
            "about what the processor is doing."
        },
        {
            "wfs://knowledge/array_design",
            "knowledge_array_design.md",
            "Designing speaker arrays for WFS",
            "Guidelines for speaker selection, spacing, array positioning (front-fill, flown, surround). "
            "Maximum speaker spacing formula based on coverage angle and listener distance. Recommendations "
            "for small, mid-sized, and large venues. Useful when helping with system design before a venue load-in."
        },
        {
            "wfs://knowledge/system_tuning",
            "knowledge_system_tuning.md",
            "WFS system tuning procedure",
            "The four-step tuning procedure: tune lower array alone, tune flown array alone, combine and "
            "adjust delay, fine-tune parallax and Haas effect. What reference material to use (NOT noise or "
            "sine waves). Why WFS tuning is different from conventional PA tuning. Useful when running "
            "`system_tuning_workflow` or when user asks how to tune."
        },
        {
            "wfs://knowledge/parallax_correction",
            "knowledge_parallax_correction.md",
            "Parallax correction explained",
            "Why sources-speakers-listeners are not aligned in real venues, how the per-output target "
            "listener works, how horizontal and vertical parallax compensate for delay differences, the "
            "coupling issues that can arise. Useful when the user reports spatial localization problems."
        },
        {
            "wfs://knowledge/live_source_damping",
            "knowledge_live_source_damping.md",
            "Reducing amplification near loud sources",
            "Explains live source damping: what it does, when to use it (loud acoustic sources on stage, "
            "feedback prevention, musician comfort), the four shape profiles (linear, square, log, sine), "
            "and the peak/slow compression layer."
        },
        {
            "wfs://knowledge/floor_reflections",
            "knowledge_floor_reflections.md",
            "Simulated floor reflections (\"Hackoustics\")",
            "Why floor reflections improve realism for played-back material, when to enable them per-output, "
            "the filtering and diffusion parameters, CPU cost considerations, interaction with parallax correction."
        },
        {
            "wfs://knowledge/reverb_in_wfs",
            "knowledge_reverb_in_wfs.md",
            "Reverb in a WFS context",
            "Why WFS benefits from added reverb (masking speaker reflections, restoring depth), the "
            "difference between reverb feeds and returns, positioning reverb nodes, feedback prevention "
            "by design, the three integrated algorithms (SDN, FDN, IR) and when to use each."
        },
        {
            "wfs://knowledge/gradient_maps",
            "knowledge_gradient_maps.md",
            "Gradient maps",
            "How position-driven maps modulate level, effective height, and HF damping as a source moves "
            "on stage. Application examples: off-stage muting, height matching for stairs and platforms, "
            "creative distance effects, room-based processing."
        },
        {
            "wfs://knowledge/source_movements",
            "knowledge_source_movements.md",
            "Movement automation - LFOs, trajectories, jitter",
            "The available movement modes: offset with rotate/scale, one-shot Move with time and curve, "
            "jitter (random micro-movement), LFO (periodic movement with per-axis shape/rate/amplitude/phase). "
            "Gyrophone for directivity rotation. Global speed control. When to use constant-speed (line) vs. "
            "smooth (sine) acceleration."
        },
        {
            "wfs://knowledge/signal_flow",
            "knowledge_signal_flow.md",
            "Signal flow in a typical session",
            "Console direct-outs post-fader feeding the WFS processor inputs, speaker outputs going back to "
            "the console or direct to amps, reverb sends and returns, the object-oriented mixing paradigm "
            "(per-source parameters, not per-speaker mixing)."
        },
        {
            "wfs://knowledge/session_concepts",
            "knowledge_session_concepts.md",
            "Sessions, snapshots, and scope",
            "How a session is organized (configuration + snapshots + samples + IRs), what's in each saved "
            "file, the snapshot scope system (parameter-level, per-channel granularity), backup/autosave, "
            "OSC-driven snapshot operations."
        },
        {
            "wfs://knowledge/tracking",
            "knowledge_tracking.md",
            "Position tracking",
            "Tracking technologies (UWB, computer vision, LiDAR, IR-LED), supported protocols (OSC, PSN, "
            "RTTrP, MQTT), the OSC message format, coordinate mapping (offset/scale/flip), smoothing "
            "trade-offs, per-input tag assignment, hand-off between tracked and static states. Useful when "
            "setting up tracking, calibrating, or troubleshooting positional issues."
        },
        {
            "wfs://knowledge/help_cards",
            "knowledge_help_cards.md",
            "Quick-reference help cards",
            "Per-section quick help (System Overview, Session Data, Network, Tracking, ADM-OSC, Array Design, "
            "Parallax, System Tuning, Reverb feeds/returns/algorithms, Inputs, Live Source Tamer, Floor "
            "Reflections, LFO, AutomOtion, Gradient Maps, Sampler, Clusters, Map). One paragraph per area "
            "of the application, mirroring what the in-app help cards show. Useful when the operator asks "
            "\"what does this tab do\" or for the AI to ground itself in WFS-DIY-specific terminology before answering."
        },
        {
            "wfs://knowledge/glossary",
            "knowledge_glossary.md",
            "WFS-DIY glossary",
            "App-specific vocabulary the AI is likely to encounter in tool descriptions, parameter names, "
            "and operator queries. Covers spatial concepts, routing/channel terms, the five movement layers, "
            "audio-shaping vocabulary, reverb terminology, state-management terms, network protocols, helper "
            "tools/dialogs, and disambiguations the AI should remember (Input/Source/Channel as synonyms; "
            "Hackoustics = Floor Reflections; Live Source Tamer = Live Source Damping; AutomOtion vs Move; etc.). "
            "Fetch once at session start to ground vocabulary."
        }
    };
} // anonymous namespace

MCPResourceRegistry::MCPResourceRegistry (const juce::File& resourcesDir)
{
    entries.reserve (std::size (kCatalog));
    for (const auto& row : kCatalog)
    {
        ResourceEntry e;
        e.uri         = juce::String (row.uri);
        e.name        = juce::String::fromUTF8 (row.name);
        e.description = juce::String::fromUTF8 (row.description);
        e.mimeType    = juce::String (kMarkdownMime);
        e.file        = resourcesDir.getChildFile (row.filename);
        entries.push_back (std::move (e));
    }
}

const ResourceEntry* MCPResourceRegistry::findByURI (const juce::String& uri) const
{
    for (const auto& e : entries)
        if (e.uri == uri)
            return &e;
    return nullptr;
}

} // namespace WFSNetwork
