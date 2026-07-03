"""WFS-DIY configuration for the MCP tool generator core.

Pure data, no imports: every table the app-agnostic generator core
(spatcore/tools/codegen/generate_mcp_tools.py) needs to turn the
Documentation/WFS-UI_*.csv files into Source/Network/MCP/generated_tools.json
and generated_groups.json. Installed by the thin entry-point wrapper
tools/generate_mcp_tools.py via `core.configure(this_module)`.

Everything in this file is WFS-DIY vocabulary/policy (spatcore Phase 4d
split, see docs/architecture/core-boundary-proposal-control.md §2.1);
the mechanism lives core-side and defines none of these names.
"""

# CSV file -> tool namespace prefix used in tool names and group_keys.
CSV_NAMESPACE = {
    "WFS-UI_input.csv":      "input",
    "WFS-UI_output.csv":     "output",
    "WFS-UI_reverb.csv":     "reverb",
    "WFS-UI_clusters.csv":   "cluster",
    "WFS-UI_network.csv":    "network",
    "WFS-UI_config.csv":     "system",
    "WFS-UI_audioPatch.csv": "audio",
}

# OSC-path convention for the 12-column CSVs that don't carry an explicit path.
OSC_PATH_CONVENTION = {
    "WFS-UI_config.csv":  "/wfs/config",
    "WFS-UI_network.csv": "/wfs/network",
}

# Variable-prefix stripping per CSV (longest match wins).
VARIABLE_PREFIXES = {
    "WFS-UI_input.csv":   ["input"],
    "WFS-UI_output.csv":  ["output"],
    "WFS-UI_reverb.csv":  ["reverb"],
    "WFS-UI_clusters.csv": ["cluster"],
    "WFS-UI_network.csv":  ["network", "tracking", "admCart", "admPolar",
                            "admOsc", "findDevice"],
    "WFS-UI_config.csv":   [],  # variable names are PascalCase here
    "WFS-UI_audioPatch.csv": ["sampler"],
}

# Common abbreviations expanded for human-readable tool names.
# Order matters: longer abbreviations first to avoid sub-matches.
ABBREVIATIONS = [
    ("HFdamping",   "hf_damping"),
    ("HFshelf",     "hf_shelf"),
    ("LFOactive",   "lfo_active"),
    ("LFOperiod",   "lfo_period"),
    ("LFOphase",    "lfo_phase"),
    ("LFOshape",    "lfo_shape"),
    ("LFOrate",     "lfo_rate"),
    ("LFOamplitude", "lfo_amplitude"),
    ("LFOgyrophone", "lfo_gyrophone"),
    ("LSenable",    "live_source_enable"),
    ("LSactive",    "live_source_active"),
    ("LSradius",    "live_source_radius"),
    ("LSshape",     "live_source_shape"),
    ("LSattenuation", "live_source_attenuation"),
    ("LSpeak",      "live_source_peak"),
    ("LSslow",      "live_source_slow"),
    ("LSatten",     "live_source_atten"),
    ("FRactive",    "floor_reflections_active"),
    ("FRattenuation", "floor_reflections_attenuation"),
    ("FRlowCut",    "floor_reflections_low_cut"),
    ("FRhighShelf", "floor_reflections_high_shelf"),
    ("FRdiffusion", "floor_reflections_diffusion"),
    ("FRenable",    "floor_reflections_enable"),
    ("EQfreq",      "eq_frequency"),
    ("EQshape",     "eq_shape"),
    ("EQgain",      "eq_gain"),
    ("EQq",         "eq_q"),
    ("EQslope",     "eq_slope"),
    ("EQenable",    "eq_enable"),
    ("EQenabled",   "eq_enabled"),
    ("eqShape",     "eq_shape"),
    ("eqFrequency", "eq_frequency"),
    ("eqGain",      "eq_gain"),
    ("eqQ",         "eq_q"),
    ("eqSlope",     "eq_slope"),
    ("EQ",          "eq"),
    ("LFO",         "lfo"),
    ("LS",          "live_source"),
    ("FR",          "floor_reflections"),
    ("HF",          "hf"),
    ("Hparallax",   "h_parallax"),
    ("Vparallax",   "v_parallax"),
    ("OSC",         "osc"),
    ("admCart",     "adm_cart"),
    ("admPolar",    "adm_polar"),
    ("admOsc",      "adm_osc"),
    ("RT60",        "rt60"),
    ("SDN",         "sdn"),
    ("FDN",         "fdn"),
    ("IR",          "ir"),
    ("AutomOtion",  "automotion"),
    ("MQTT",        "mqtt"),
    ("PSN",         "psn"),
    ("RTTrP",       "rttrp"),
    ("UDP",         "udp"),
    ("TCP",         "tcp"),
    ("UI",          "ui"),
    ("ID",          "id"),
    ("XY",          "xy"),
    ("XYZ",         "xyz"),
    ("3D",          "3d"),
    ("3d",          "3d"),
    ("Rsph",        "r_spherical"),
    ("Phi",         "phi"),
    ("Theta",       "theta"),
    ("DSP",         "dsp"),
]

# Common word fragments to expand once the snake_case form is built.
WORD_EXPANSIONS = {
    "atten":  "attenuation",
    "freq":   "frequency",
    "param":  "parameter",
    "config": "configuration",
}

# Tier heuristics (defaults; the CSV `Tier` column and the override file
# take precedence).
# IMPORTANT: matched via word-boundary regex against `variable + label` (not
# substring). "preset" used to mis-match "reset" and tag clusterLFOPresetName
# as tier-3; "reset" is also dropped from the list because the only variables
# with `Reset` in their name (inputOtomoReset, the ad-hoc one above) are
# threshold settings, not destructive operations. If a true factory-reset
# tool ever lands, give it an explicit override in tool_tier_overrides.json.
TIER_KEYWORDS_3 = ["delete", "clear", "remove", "channels",
                   "sampleRate", "runDSP", "reconfigure"]
TIER_KEYWORDS_2 = ["master", "solo", "muteAll", "testTone", "store", "load",
                   "import", "export", "snapshot"]

# Wide attenuation/dB ranges → tier 2 (sudden loud output risk). Applied by
# the heuristic when neither keyword table matched: numeric rows whose
# min→max span is at least `min_span` AND whose name mentions one of the
# `keywords` escalate to `tier`.
TIER_RANGE_RISK = {
    "min_span": 40,
    "keywords": ["attenuation", "level", "atten"],
    "tier": 2,
}

# Domain heuristics. Each tool gets a list of domain tags so AI agents
# can decide whether a write is meaningful for a given goal:
#
#   wfs_synthesis      changes what comes out of the WFS speakers
#   reverb             changes the reverb engine's output
#   binaural           changes the binaural-monitoring render only
#   adm_osc            changes how incoming ADM-OSC data is mapped
#   floor_reflections  affects the FR (Hackoustics) signal
#   live_source        affects the live-source feedback tamer
#   tracking           affects external tracking ingest
#   routing            patching, mutes, sends, channel assignment
#   network            transport / target / port settings
#   visualisation_only on-screen markers, locks — no audio effect
#   metadata           names, themes, language, file paths
#
# Resolution order (first match wins per row):
#   1. Variable-name override (DOMAIN_VARIABLE_OVERRIDES)
#   2. Section keyword in CSV's Section column (DOMAIN_BY_SECTION_KEYWORD)
#   3. Per-CSV default (DOMAIN_DEFAULT_BY_CSV)
#
# Sections embedded inside per-channel tabs (e.g. "Map" rows in input.csv
# that toggle visibility) are caught by Step 1; the rest gets the
# audio-affecting default for the channel kind.

DOMAIN_VARIABLE_OVERRIDES = {
    # Map-display toggles — visualisation only, no audio impact.
    "inputMapLocked":         ["visualisation_only"],
    "inputMapVisible":        ["visualisation_only"],
    "outputMapVisible":       ["visualisation_only"],
    "outputArrayMapVisible":  ["visualisation_only"],
    "reverbsMapVisible":      ["visualisation_only"],

    # Names — metadata.
    "showName":               ["metadata"],
    "inputName":              ["metadata"],
    "outputName":             ["metadata"],
    "reverbName":             ["metadata"],
    "samplerSetName":         ["metadata"],
    "clusterName":            ["metadata"],
    "clusterLfoPresetName":   ["metadata"],

    # Channel counts — wfs_synthesis (changes the speaker array shape)
    # and routing (changes patch matrix availability).
    "inputChannels":          ["wfs_synthesis", "routing"],
    "outputChannels":         ["wfs_synthesis", "routing"],
    "reverbChannels":         ["reverb", "routing"],

    # Cluster reference / plane selectors — wfs_synthesis (transforms
    # apply through these).
    "clusterReferenceMode":   ["wfs_synthesis"],
    "clusterPlane":           ["wfs_synthesis"],
}

# (csv_namespace, section_substring_lowercase) -> domains.
# Section is matched substring-style against row.section.lower().
DOMAIN_BY_SECTION_KEYWORD = [
    # config.csv
    ("system", "stage",            ["wfs_synthesis"]),
    ("system", "master",           ["wfs_synthesis"]),
    ("system", "binaural",         ["binaural"]),
    ("system", "controllers",      ["routing"]),
    ("system", "ui",               ["metadata"]),
    ("system", "files",            ["metadata"]),
    ("system", "diagnostics",      ["metadata"]),
    ("system", "show",             ["metadata"]),
    ("system", "i/o",              ["wfs_synthesis", "routing"]),
    ("system", "wfs processor",    ["wfs_synthesis"]),

    # network.csv
    ("network", "adm-osc",         ["adm_osc"]),
    ("network", "tracking",        ["tracking"]),
    ("network", "find my remote",  ["network"]),
    ("network", "network",         ["network"]),
    ("network", "osc",             ["network"]),
    ("network", "connections",     ["network"]),

    # input.csv embedded sub-sections
    ("input", "map",               ["visualisation_only"]),
    ("input", "live source",       ["live_source"]),
    ("input", "hackoustics",       ["floor_reflections"]),
    ("input", "tracking",          ["tracking"]),
    ("input", "mutes",             ["routing"]),
    ("input", "sampler",           ["wfs_synthesis", "routing"]),

    # output.csv embedded sub-sections
    ("output", "map",              ["visualisation_only"]),
    ("output", "options",          ["wfs_synthesis"]),

    # reverb.csv: every section is reverb-related
    ("reverb", "",                 ["reverb"]),

    # cluster.csv: transforms move sources, so wfs_synthesis
    ("cluster", "",                ["wfs_synthesis"]),

    # audio.csv: patching / test signals
    ("audio", "",                  ["routing"]),
]

# Per-CSV fallback when no section keyword matches.
DOMAIN_DEFAULT_BY_CSV = {
    "WFS-UI_input.csv":      ["wfs_synthesis"],
    "WFS-UI_output.csv":     ["wfs_synthesis"],
    "WFS-UI_reverb.csv":     ["reverb"],
    "WFS-UI_clusters.csv":   ["wfs_synthesis"],
    "WFS-UI_network.csv":    ["network"],
    "WFS-UI_config.csv":     ["metadata"],
    "WFS-UI_audioPatch.csv": ["routing"],
}

# UI-control vocabulary that disqualifies a row from becoming a tool.
NON_PARAMETER_UI = {
    "Display label", "Read-only label", "Read-only labels (X / Y / Z)",
    "Display label (X / Y / Z)", "Visual indicator (live)",
    "Visual indicator", "Tab selector", "Window", "Internal state",
    "Internal state (atomic)", "Display label", "Mouse gesture",
    "Mouse / touch gesture", "Keyboard", "Scrollbar (16 px)",
    "2D grid (scrollable)", "interactive display", "6x6 cell grid",
    "Generator (DSP)", "meter", "SVG drawable", "Help card button",
    "(internal)", "interactive display", "(no editor)", "Read-only progress dial",
    "Help button", "Mouse / touch", "Mouse",
    "Band toggle",  # UI affordance for the EQ-band on/off shared paramId
    "interactive display",
    "Read-only progress dial",
    "Mouse / touch", "Mouse",
}

# Variables that are purely display / non-persisted UI affordances.
NON_PARAMETER_VARIABLES_PREFIXES = (
    "JoystickXY", "JoystickZ",
)

# Per-CSV channel-id argument: (arg_name, min, max, schema description).
CHANNEL_ID_RANGE = {
    "WFS-UI_input.csv":   ("input_id", 1, 64,
                           "Input channel number (1-based)."),
    "WFS-UI_output.csv":  ("output_id", 1, 128,
                           "Output channel number (1-based)."),
    "WFS-UI_reverb.csv":  ("reverb_id", 1, 32,
                           "Reverb channel number (1-based)."),
    "WFS-UI_clusters.csv": ("cluster_id", 1, 10,
                            "Clusters channel number (1-based)."),
}

# Placeholder marking per-band rows in the Variable cell, and the schema
# argument the generator exposes for them.
BAND_PLACEHOLDER = "<band>"
BAND_ARG = {
    "name": "band",
    "min": 1,
    "max": 6,
    "description": "EQ band number (1-6).",
}

# Numeric-suffix families whose sub-index arg gets a semantic name/range
# instead of the generic `index`. Matched on csv_file + lowercase stem
# suffix (first hit wins): the inputArrayAtten1..10 family exposes
# `array` 1-10 (output array number).
FAMILY_ARG_RULES = [
    {
        "csv_file": "WFS-UI_input.csv",
        "stem_suffix": "atten",
        "name": "array",
        "min": 1,
        "max": 10,
        "description": "Output array number (1-10).",
    },
]

# Rows in non-per-channel CSVs that carry a grid-cell selector argument
# (the sampler's 6x6 grid). Matched on csv_file + Label substring.
CELL_INDEX_RULES = [
    {
        "csv_file": "WFS-UI_audioPatch.csv",
        "label_substring": "Cell",
        "name": "cell_id",
        "min": 0,
        "max": 35,
        "description": "Sampler cell index (0-35, 6x6 grid).",
    },
]

# Coordinate-system suffixes stripped from Section before computing
# group_key / tool names ("Position (Cylindrical)" -> "Position").
COORDINATE_SECTION_SUFFIXES = ("Cylindrical", "Spherical", "Cartesian")

# Hint appended to every per-channel tool description so AI clients fetch
# state before guessing channel ranges.
PER_CHANNEL_HINT = (
    "Use `session_get_state()` first if unsure which channels exist."
)

# Input CSVs in consumption order (also the hash order), and the default
# CLI paths (relative to the repo root, where the generator is run from).
CSV_FILES_ORDER = [
    "WFS-UI_config.csv",
    "WFS-UI_input.csv",
    "WFS-UI_output.csv",
    "WFS-UI_reverb.csv",
    "WFS-UI_clusters.csv",
    "WFS-UI_network.csv",
    "WFS-UI_audioPatch.csv",
]

DEFAULT_CSV_DIR = "Documentation"
DEFAULT_OVERRIDES_TIER = "tools/mcp/tool_tier_overrides.json"
DEFAULT_OVERRIDES_IGNORE = "tools/mcp/tool_generation_ignores.json"
DEFAULT_OUTPUT = "Source/Network/MCP/generated_tools.json"
DEFAULT_GROUPS_OUTPUT = "Source/Network/MCP/generated_groups.json"
