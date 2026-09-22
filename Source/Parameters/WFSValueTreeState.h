#pragma once

#include <JuceHeader.h>
#include "WFSParameterIDs.h"
#include "WFSParameterDefaults.h"
#include "../Helpers/ReverbNodePlacement.h"
#include "../../spatcore/control/state/TreeParameterStore.h"
#include "InputChannelIdentity.h"
#include "ArrayMuteState.h"
#include <vector>
#include <map>
#include <array>
#include <cstdint>
#include <functional>
#include <set>

/**
 * Undo domain — each tab has its own undo history.
 */
enum class UndoDomain
{
    Input,      // InputsTab
    Output,     // OutputsTab + OutputArrayHelperWindow
    Reverb,     // ReverbTab
    Map,        // MapTab (input positions via map drag)
    Config,     // SystemConfigTab + NetworkTab
    Clusters,   // ClustersTab
    Effects,    // EffectsTab + the effectsGlobal* config block
    COUNT
};

/**
 * WFS ValueTree State Manager
 *
 * Central management class for all WFS processor parameters using JUCE ValueTree.
 * Derives from spatcore::control::state::TreeParameterStore, which owns the
 * app-agnostic mechanics (root tree, typed get/set, listener registry +
 * change-notification dispatch, per-domain UndoManager array, origin-aware
 * undo suppression, post-write hook). This class supplies everything
 * WFS-schema-shaped:
 * - Hierarchical parameter organization (section builders + accessors)
 * - Scope routing (getTreeForParameter / getParameterScope)
 * - Semantic invariants (cluster shared-position, tracking uniqueness),
 *   registered into the core post-write hook
 * - The seven WFS tab undo domains (UndoDomain), mapped onto the core's
 *   integer domain indices
 */
class WFSValueTreeState : public spatcore::control::state::TreeParameterStore
{
public:
    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    WFSValueTreeState();
    ~WFSValueTreeState() override;

    //==========================================================================
    // State Access
    //==========================================================================

    /** Get config section */
    juce::ValueTree getConfigState();
    juce::ValueTree getConfigState() const;
    juce::ValueTree getShowState();
    juce::ValueTree getIOState();
    juce::ValueTree getStageState();
    juce::ValueTree getMasterState();
    juce::ValueTree getNetworkState();
    /** The <UI> section: Stream Deck / Sampler / Lightpad toggles, the colour
        scheme and the sampler controller mode. */
    juce::ValueTree getUIState();

    juce::ValueTree getADMOSCState();

    /** One ADM-OSC mapping, by its `id` property rather than child position —
        Cartesian and Polar mappings are siblings under <ADMOSC>, so an ordinal
        would run off the end of one kind into the other. Both are numbered 0-3
        within their own kind. */
    juce::ValueTree getADMCartMapping (int mappingIndex);
    juce::ValueTree getADMPolarMapping (int mappingIndex);

    /** One axis of a Cartesian mapping, by `admCartAxisId` (0=X, 1=Y, 2=Z). */
    juce::ValueTree getADMCartAxis (int mappingIndex, int axisIndex);
    juce::ValueTree getTrackingState();
    juce::ValueTree getClustersState();
    juce::ValueTree getClustersState() const;
    juce::ValueTree getClusterState (int clusterIndex);
    juce::ValueTree getClusterLFOSection (int clusterIndex);
    juce::ValueTree getClusterLFOPresetsSection();
    juce::ValueTree ensureClusterLFOPreset (int presetIndex);

    /** Recall a cluster LFO preset into a cluster (applies non-shape props first, then shapes) */
    void recallClusterLFOPreset (int clusterId, int presetIndex);

    juce::ValueTree getBinauralState();
    juce::ValueTree getBinauralState() const;

    /** Get input/output states */
    juce::ValueTree getInputsState();
    juce::ValueTree getInputsState() const;
    juce::ValueTree getInputState (int channelIndex);
    juce::ValueTree getOutputsState();
    juce::ValueTree getOutputsState() const;
    juce::ValueTree getOutputState (int channelIndex);

    /** Get reverb states */
    juce::ValueTree getReverbsState();
    juce::ValueTree getReverbsState() const;
    juce::ValueTree getReverbState (int channelIndex);

    /** Get effects states.

        <Effects> holds ONLY <Effect> children - no global siblings, which is
        the one shape decision that separates this family from <Reverbs>. The
        reverb globals (Algorithm / PreComp / PostEQ / PostExp) are siblings of
        the reverb channels; the effects globals live in Config/EffectsGlobal
        instead. NOTHING this application writes may ever be appended beside the
        channels - that constraint is what this comment is for.

        It is still only a constraint on US. A FILE can hold anything:
        mergeTreeRecursive appends an unmatched source child verbatim and
        WFSFileManager::applyEffectsSection is the path a hand-edited or foreign
        effects.xml takes. getEffectState therefore counts by type exactly as
        getReverbState does - not because the shape decision was wrong, but
        because the accessor and getNumEffectChannels must agree about which
        channel is the nth one even when the container holds something neither of
        them recognises. Indexing positionally made them disagree, and a channel
        both copies of the count promised was then unreachable to every
        accessor. */
    juce::ValueTree getEffectsState();
    juce::ValueTree getEffectsState() const;
    juce::ValueTree getEffectState (int channelIndex);

    /** <Config><EffectsGlobal> - the nine effectsGlobal* settings. Invalid only
        on a half-built tree: ensureCompleteSchema backfills it on every load. */
    juce::ValueTree getEffectsGlobalSection() const;

    /** The value a NEW effects channel's effectLinkMode is stamped with: the
        live effectsGlobalLinkMode, or the constant on a half-built tree. R5-5
        demoted the global to exactly this - a stamp, never a live read. */
    int getDefaultEffectLinkMode() const;

    /** effectArrayAtten1..10 by 0-based array index. A table, because
        effectArrayAtten1 is a strict prefix of effectArrayAtten10. */
    static const juce::Identifier& getEffectArrayAttenId (int arrayIndex);

    /** True for effectArrayAtten1..10. Ten == tests, never a startsWith:
        effectArrayAtten1 is a strict prefix of effectArrayAtten10. */
    static bool isEffectArrayAttenId (const juce::Identifier& paramId);

    /** Get audio patch state */
    juce::ValueTree getAudioPatchState();

    //==========================================================================
    // Parameter Access - Type Safe
    //==========================================================================
    // Typed getters (getFloatParameter / getIntParameter / getStringParameter /
    // getParameter) are inherited from TreeParameterStore; they resolve through
    // this class's getTreeForParameter override.

    /** Set a parameter value (routes channel-count writes to setNumXChannels) */
    void setParameter (const juce::Identifier& id, const juce::var& value, int channelIndex = -1) override;

    /** Set a parameter value without undo */
    void setParameterWithoutUndo (const juce::Identifier& id, const juce::var& value, int channelIndex = -1) override;

    /** Would a setParameter write with these arguments actually land anywhere?

        TreeParameterStore::setParameter is `if (tree.isValid()) write(...)` with no
        else and a void return, so a parameter this class cannot resolve is dropped
        in silence — the caller sees nothing at all. Every remote surface needs to
        be able to ask BEFORE writing, so it can report an error instead of a
        success it did not earn. Answers for the generic path only: callers that
        write a subtree directly (EQ bands today) never consult this. */
    bool canWriteParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Is the audio engine running? (`runDSP` on <IO>, what the GUI calls
        "ProcessingEnabled".) Structural channel edits are stopped-only, and until
        now that rule was enforced nowhere but by greying the System Config fields —
        so any remote surface could restructure channels mid-show. */
    bool isProcessingEnabled() const;

    //==========================================================================
    // Input Channel Access
    //==========================================================================

    /** Get input channel parameter */
    juce::var getInputParameter (int channelIndex, const juce::Identifier& id) const;

    /** Set input channel parameter */
    void setInputParameter (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Get the ValueTree for a specific input channel subsection */
    juce::ValueTree getInputChannelSection (int channelIndex);
    juce::ValueTree getInputPositionSection (int channelIndex);
    juce::ValueTree getInputAttenuationSection (int channelIndex);
    juce::ValueTree getInputDirectivitySection (int channelIndex);
    juce::ValueTree getInputLiveSourceSection (int channelIndex);
    juce::ValueTree getInputHackousticsSection (int channelIndex);
    juce::ValueTree getInputLFOSection (int channelIndex);
    juce::ValueTree getInputAutoMotionSection (int channelIndex);
    juce::ValueTree getInputMutesSection (int channelIndex);

    /** The canonical form of an input's per-output mute list (inputMutes): one
        "0" or "1" token per output, comma-separated, "1" = muted. Any token that
        parses to a non-zero number counts as muted (older files carry "1.0").
        Tokens at or past keepTokens are forced to "0"; the list is then padded
        with "0" or cut to numOutputs. numOutputs <= 0 canonicalises the tokens
        without resizing. Static so every reader and writer shares one rule. */
    static juce::String normaliseMuteList (const juce::var& list, int numOutputs,
                                           int keepTokens = std::numeric_limits<int>::max());

    /** THE SHAPE TEST EVERY PACKED ROW IS GUARDED BY: true when `proposed` is a
        string that can BE a row - a comma-separated list of well-formed numbers,
        of more than one column unless `expectedColumns` really is 1. False for
        everything else, and the two falses are the two writes that used to eat a
        row whole:

          - a bare NUMBER (an int or float var): a QLab cue, an OSC scalar, an
            MCP numeric enum. The original loss, and the reason this guard exists;
          - a ONE-TOKEN STRING: the same scalar, typed as text. The MCP surface
            advertises reverb_set_mutes with its value as the string enum
            "unmute" / "MUTE" and the OSC list form accepts any non-numeric
            string, so this is a write that can be made today, not a hypothesis.
            It used to normalise into a full row of DEFAULTS, which is worse than
            the number it replaced: the damage is well-formed, and an unmuted row
            cannot be told from a deliberate unmute-all.

        A string with one junk token in it is refused WHOLE rather than repaired
        token by token. The normalisers repair what a FILE carries, where there
        is no writer left to refuse; a live write that cannot spell its own row
        is not a row that lost a column. */
    static bool isPackedRowWrite (const juce::var& proposed, int expectedColumns);

    /** `proposed`'s tokens over `existing`'s, column by column: every column
        `proposed` does not name is kept, and a `proposed` longer than `existing`
        extends the row. A writer speaks for the columns it knows about and for
        no others - a grid or a tablet showing 16 outputs of a 32-output rig
        writes 16 tokens, and the other 16 are not its to clear. */
    static juce::String mergePackedRow (const juce::var& existing, const juce::var& proposed);

    /** The width a per-output row is fitted to: the live output count, or the
        row's own width when that is wider. A PER-OUTPUT ROW IS NEVER CUT BACK.
        The columns past the live count are not stale - they are the mutes of
        outputs that are not there today (an interface that dropped, a System
        Config edit about to be undone, a snapshot recalled ahead of the count) -
        and cutting them replaces them with "0" the moment the rig comes back,
        silently and then saved that way. normaliseMuteList pads AND cuts, so
        every per-output caller passes it this width rather than a live count. */
    static int perOutputRowWidth (const juce::var& row, int numOutputs);

    /** Mute or unmute ONE output of an input, leaving the others as they are.
        outputIndex is 0-based. Writes through setInputParameter, so it is undoable
        and listeners see it. False when the slot or the output does not exist. */
    bool setInputOutputMute (int channelIndex, int outputIndex, bool muted);

    juce::ValueTree getInputGradientMapsSection (int channelIndex);
    juce::ValueTree getInputGradientLayer (int channelIndex, int layerIndex);

    /** One shape within a gradient-map layer, by position in draw order. A layer
        starts with none — they are created in the gradient-map editor — so an
        invalid tree here means "no such shape", not "no such layer". */
    juce::ValueTree getInputGradientShape (int channelIndex, int layerIndex, int shapeIndex);

    /** Ensure a GradientMaps section exists for a given input (migration helper) */
    juce::ValueTree ensureInputGradientMapsSection (int channelIndex);

    /** Get the Sampler section for a given input channel */
    juce::ValueTree getInputSamplerSection (int channelIndex);

    /** Ensure a Sampler section exists for a given input (migration helper) */
    juce::ValueTree ensureInputSamplerSection (int channelIndex);

    /** One sampler cell of the 6x6 grid, matched on its `id` property (0-35).
        Cells are pre-created and never added or removed, so the id is stable. */
    juce::ValueTree getInputSamplerCell (int channelIndex, int cellIndex);

    /** One sampler set, by ORDINAL among the SamplerSet children — deliberately
        not by its `id`, because deleting a set does not renumber the survivors,
        so ids go stale while positions do not. Every existing consumer counts
        the nth SamplerSet child; this is that walk, once. A fresh input has no
        sets at all, so an invalid tree here means "no set at that index". */
    juce::ValueTree getInputSamplerSet (int channelIndex, int setIndex);

    /** Append a sampler set to an input and return it, or an invalid tree when
        the input has none or the 16-set ceiling is reached. Defaults come from
        SamplerData::SamplerSet's own member initialisers, so this cannot drift
        from what the Sampler tab creates. */
    juce::ValueTree addInputSamplerSet (int channelIndex, const juce::String& setName);

    /** Remove a sampler set by ORDINAL. Survivors are NOT renumbered — the id
        property is already unreliable for that reason, and every consumer counts
        positions. Returns false when there is no set at that index. */
    bool removeInputSamplerSet (int channelIndex, int setIndex);

    /** How many sampler sets an input currently has. */
    int getNumInputSamplerSets (int channelIndex);

    //==========================================================================
    // Output Channel Access
    //==========================================================================

    /** Get output channel parameter */
    juce::var getOutputParameter (int channelIndex, const juce::Identifier& id) const;

    /** Set output channel parameter */
    void setOutputParameter (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Set output channel parameter with array propagation
     *  If the output is part of an array and applyToArray is enabled,
     *  propagates the change to other array members.
     *  @param channelIndex The output channel being modified (0-based)
     *  @param id The parameter identifier
     *  @param value The new value
     *  @param propagateToArray If true, considers array linking mode (default true)
     */
    void setOutputParameterWithArrayPropagation (int channelIndex,
                                                  const juce::Identifier& id,
                                                  const juce::var& value,
                                                  bool propagateToArray = true);

    /** Set output EQ band parameter with array propagation
     *  @param propagateToArray If true, considers array linking mode (default true)
     */
    void setOutputEQBandParameterWithArrayPropagation (int channelIndex,
                                                        int bandIndex,
                                                        const juce::Identifier& id,
                                                        const juce::var& value,
                                                        bool propagateToArray = true);

    /** Check if a parameter is array-linked (should propagate to array members) */
    static bool isArrayLinkedParameter (const juce::Identifier& paramId);

    /** Check if an EQ band parameter is array-linked */
    static bool isArrayLinkedEQParameter (const juce::Identifier& paramId);

    /** Check if an output parameter is an on/off toggle (no meaningful relative delta) */
    static bool isBooleanOutputParameter (const juce::Identifier& paramId);

    /** Per-array output mute: session state held beside the tree, never in it
        (not saved, not undoable). Cleared by replaceState and by an output
        config load; see ArrayMuteState. */
    ArrayMuteState& getArrayMutes() noexcept { return arrayMutes; }
    const ArrayMuteState& getArrayMutes() const noexcept { return arrayMutes; }

    /** Get the ValueTree for a specific output channel subsection */
    juce::ValueTree getOutputChannelSection (int channelIndex);
    juce::ValueTree getOutputPositionSection (int channelIndex);
    juce::ValueTree getOutputOptionsSection (int channelIndex);
    juce::ValueTree getOutputEQSection (int channelIndex);

    /** One EQ band - the nth <Band> BY TYPE, not the nth child.

        EVERY band and tap accessor in this class resolves that way, for the
        reason nthChildOfType records in the .cpp: the container is built holding
        one node type and callers address it as if the nth child were the nth of
        that type, but a merged FILE can leave an unrecognised node in the list,
        and a positional index then hands the caller the wrong node - or nothing
        at all for the last band, which has been pushed off the end.

        That is not a lookup miss. The wrong node is a valid tree, so the write
        succeeds, the remote surface reports success, the value is saved onto it
        and read straight back off it, and only the audio is missing. Counting by
        type answers with the band the caller named or with nothing. */
    juce::ValueTree getOutputEQBand (int channelIndex, int bandIndex);

    //==========================================================================
    // Reverb Channel Access
    //==========================================================================

    /** Get reverb channel parameter */
    juce::var getReverbParameter (int channelIndex, const juce::Identifier& id) const;

    /** Set reverb channel parameter */
    void setReverbParameter (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Get the ValueTree for a specific reverb channel subsection */
    juce::ValueTree getReverbChannelSection (int channelIndex);
    juce::ValueTree getReverbPositionSection (int channelIndex);
    juce::ValueTree getReverbFeedSection (int channelIndex);

    /** Drop attributes the reverb schema no longer has (reverbLSenable) after a
        file merge, which carries everything the file holds and removes nothing.
        Public because WFSFileManager::applyReverbsSection is a merge path too. */
    void stripObsoleteReverbProperties();

    /** The effects family's eviction hook, and the reason it is table-free.

        Same job as stripObsoleteReverbProperties - mergeTreeRecursive and
        backfillFromTemplate both only ever ADD, so an attribute the schema has
        retired rides along in the live tree and is re-saved for ever - but it
        names almost nothing. Every property anywhere under an <Effect> is
        stamped by exactly one builder under createDefaultEffectChannel, so "what
        the schema declares" IS that template: this walks each channel against a
        freshly built one and removes any property the template does not carry. A
        name deleted from a builder is therefore evicted from every loaded file
        with no second list to keep in step - which is precisely the maintenance
        the reverb hook's hand-written legacy identifier demands.

        THE EXEMPTION IS GONE, and its removal is this design working rather
        than a regression. A template diff cannot tell PENDING from RETIRED: both
        are absent from a freshly built channel, and while <Sends> was built
        empty the four send rows were declared, written at runtime and therefore
        indistinguishable from retired names - so they were named here by hand
        and skipped. createEffectSendsSection now stamps all four, the template
        carries them like every other property, and the hand-maintained list that
        stood in for that has been deleted, exactly as its own comment said it
        must be the day it stopped being needed. A row NAME that appears anywhere
        but on <Sends> is now a genuine ghost and is evicted like any other.

        The corollary is a rule, not an accident: nothing may stamp a property
        onto an <Effect> subtree that createDefaultEffectChannel does not also
        stamp. A runtime-only flag parked there is evicted on the next load and
        belongs outside the persisted subtree.

        Whatever it drops, it SAYS so: one warning naming the distinct attributes
        removed. Eviction is not undoable by design, so a wrong one has to be
        visible somewhere.

        THE LIMIT, stated so nobody has to rediscover it: this works at property
        granularity only. A NODE the template does not have is neither deleted
        nor descended into, so every property beneath an unrecognised node is out
        of reach. Retiring a whole module type is a deliberate edit here, not a
        consequence of deleting its builder.

        Public because WFSFileManager::applyEffectsSection is a merge path. */
    void stripObsoleteEffectProperties();

    /** The eviction hook's opposite number: stamp onto every loaded <Effect>
        whatever createDefaultEffectChannel declares and the file does not carry.

        Every other family reaches its apply*Section with its channels already
        built from the count in <IO>, so mergeTreeRecursive lands the file ONTO a
        schema-complete node and a property the file lacks simply keeps its
        default. Effects are built from <IO>/effectChannels too now, but the
        merge still APPENDS any <Effect> the file holds beyond that count,
        verbatim - and an appended half-built channel is worse than a missing
        one: setEffectParameter only writes where some child already
        hasProperty(), so a later GUI/OSC/MCP write of the absent parameter is a
        silent no-op for the life of the show. This closes that on the merge path
        the same way ensureCompleteSchema closes it on the replaceState path - it
        IS that pass, shared by both.

        ADDS only, like the schema backfill it is made of. Retired names are the
        other direction: stripObsoleteEffectProperties, which runs beside it. */
    void backfillEffectChannelsFromTemplate();
    juce::ValueTree getReverbEQSection (int channelIndex);
    juce::ValueTree ensureReverbEQSection (int channelIndex);  // Creates if missing
    /** One pre-EQ band - the nth <Band> BY TYPE; see getOutputEQBand. This is
        the most exposed of the five: OSC (/wfs/reverb/n/eq/b/...), the MCP band
        tools and the GUI tab all resolve through it. */
    juce::ValueTree getReverbEQBand (int channelIndex, int bandIndex);
    juce::ValueTree getReverbReturnSection (int channelIndex);

    /** Get the global reverb algorithm section (child of Reverbs node) */
    juce::ValueTree getReverbAlgorithmSection();
    juce::ValueTree ensureReverbAlgorithmSection();  // Creates if missing

    /** Get the global reverb pre-compressor section (child of Reverbs node) */
    juce::ValueTree getReverbPreCompSection();
    juce::ValueTree ensureReverbPreCompSection();  // Creates if missing

    /** Get the global reverb post-processing EQ section (child of Reverbs node) */
    juce::ValueTree getReverbPostEQSection();
    juce::ValueTree ensureReverbPostEQSection();  // Creates if missing
    /** One post-EQ band - the nth <PostEQBand> BY TYPE; see getOutputEQBand.
        The node type is NOT <Band>: the post EQ is a global sibling of the
        reverb channels and its bands carry their own type, which is what keeps
        the two id namespaces apart. */
    juce::ValueTree getReverbPostEQBand (int bandIndex);

    /** Get the global reverb post-expander section (child of Reverbs node) */
    juce::ValueTree getReverbPostExpSection();
    juce::ValueTree ensureReverbPostExpSection();  // Creates if missing

    //==========================================================================
    // Effects Channel Access
    //==========================================================================
    // An <Effect> holds six flat sections (Channel, Position, Feed, Return,
    // AutomOtion, Chain), the eleven id-less module nodes named for
    // spatcore::effects::kSlots, and <Sends>. Two module TYPES exist twice per
    // channel (FxEq1/FxEq2 and FxDyn1/FxDyn2) and carry identical property
    // names, which is the one deliberate exception to the rule that a property
    // lives on exactly one node type - so every accessor for them takes an
    // instance, and the generic by-name searches below skip them outright.
    //
    // WHAT THE EXCEPTION COSTS, stated in full because every later surface pays
    // it: 31 of the 155 per-channel identifiers - 20% - cannot be reached by
    // name, so canWriteParameter answers FALSE for all of them. Twenty-four are
    // the doubled types proper (effectEQBypass and the 23 effectDyn*); the
    // other seven are the index-addressed repeats carried by the <Band> and
    // <Tap> siblings (effectEQshape/freq/gain/q/slope, effectDelayTapTime and
    // effectDelayTapLevel), which <Reverb> repeats the same way. That FALSE is
    // invisible in one stroke to MCP, OSC, OSCQuery, the plugin and any
    // snapshot loop discriminating on hasProperty: none of them can address a
    // property that no single node claims. Every surface that wants this 20%
    // must therefore carry its own instance-taking dispatch onto
    // getEffectEQBand / getEffectDynSection / getEffectDelayTap, and a snapshot
    // scope built the way the input scopes are built can never cover it.

    /** Read a per-channel effect parameter.

        Resolves the six flat sections and the seven SINGLE-instance module
        nodes. Deliberately returns a void var for the doubled modules
        (effectEQ* / effectDyn*, on FxEq1/FxEq2 and FxDyn1/FxDyn2) and for the
        per-band / per-tap properties: those names exist on more than one node,
        so a first-hit walk could only ever report instance 1 while claiming
        success. Use getEffectEQBand / getEffectDelayTap / getEffectModuleSection
        for those - the extra index is the whole point of their signatures. */
    juce::var getEffectParameter (int channelIndex, const juce::Identifier& id) const;

    /** Write a per-channel effect parameter. Same resolution rule - and the same
        deliberate refusal - as getEffectParameter. A position or return-offset
        write latches effect position ownership (see markEffectPositionsUserOwned). */
    void setEffectParameter (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** The flat sections of one effect channel. */
    juce::ValueTree getEffectChannelSection (int channelIndex);
    juce::ValueTree getEffectPositionSection (int channelIndex);
    juce::ValueTree getEffectFeedSection (int channelIndex);
    juce::ValueTree getEffectReturnSection (int channelIndex);
    juce::ValueTree getEffectAutoMotionSection (int channelIndex);
    juce::ValueTree getEffectChainSection (int channelIndex);

    /** The <Sends> node: the four packed send rows of one effect channel. */
    juce::ValueTree getEffectSendsSection (int channelIndex);

    //--------------------------------------------------------------------------
    // The send matrix
    //--------------------------------------------------------------------------
    /* FIVE PACKED ROWS, THREE SHAPES, AND NO ONE HELPER ACROSS THEM. They look
       alike - a comma-separated string on one node - and the ways they differ
       are exactly the ways a shared helper gets them wrong:

         row                | width             | a column is       | tokens
         effectMutes        | LIVE output count | an output INDEX   | 0 / 1
         effectSendLevels   | maxInputChannels  | an input NUMBER   | dB
         effectSendOns      | maxInputChannels  | an input NUMBER   | 0 / 1
         effectFxSendLevels | maxEffectChannels | an effect INDEX   | dB
         effectFxSendOns    | maxEffectChannels | an effect INDEX   | 0 / 1

       1. effectMutes FOLLOWS THE LIVE OUTPUT COUNT, the way inputMutes and
          reverbMutes do, and setNumOutputChannels refits it. The four send rows
          do NOT: a column is an input's PERMANENT NUMBER, which can be anything
          up to maxInputChannels whatever the live count is and can have gaps, so
          a live-width row would drop the columns of channels that still exist.
          They are stamped at their fixed width and never resized.
       2. The LEVEL rows hold dB, not flags. normaliseMuteList coerces every
          token to 0 or 1, so putting a level row through it silently sets every
          send to unity or to nothing. They get their own normaliser, which
          clamps into the declared range and KEEPS the value.
       3. The fx rows carry a DIAGONAL that has to stay off - effect n may not
          feed itself - and it is forced where the row is WRITTEN (the builder,
          the cell setters, the write interceptor and the column maintenance),
          not only where it is read.

       KEYING, because both columns are ints and only one of them is an index: an
       effectSend* column is an input PERMANENT NUMBER minus one, and an
       effectFxSend* column is a DENSE effect index. The accessors below say
       which in the function name and in the parameter name, so no call site has
       to remember it.

       ROW writes go through setEffectParameter - the row identifiers resolve to
       <Sends> like any other property - and the interceptor normalises them,
       but only after isPackedRowWrite has agreed that what arrived is a ROW.
       A string is not a row because it is a string: one token is a scalar
       whatever it spells, and normalising one into a full row of defaults is
       how a single MCP enum value used to clear an entire routing.
       CELL writes go through the eight accessors below, which read-modify-write
       the whole row through that same setter: one undo entry, one listener
       notification carrying the whole row, in the shape of setInputOutputMute.

       THE FOUR CELL PSEUDO-IDENTIFIERS (effectSendLevel, effectSendOn,
       effectFxSendLevel, effectFxSendOn) are NOT these rows and no node ever
       carries one. They exist so the OSC parser, the ramper and the OSCQuery
       cell nodes have something to validate a single cell against, while the
       generic parameter path finds no tree for them and refuses the write
       instead of dropping a scalar onto a whole row. Never stamp one. */

    /** The canonical form of a packed LEVEL row: `width` comma-separated dB
        values inside [minDb, maxDb]. A token that is already a well-formed
        number in range is kept VERBATIM, spelling and all - normalisation is
        there to repair a row, not to re-spell an operator's values, and a
        normaliser that rewrites every token makes a file diff on nothing. A
        missing or malformed token becomes defaultDb, an out-of-range one is
        clamped. width <= 0 canonicalises the tokens without resizing. */
    static juce::String normaliseSendLevelList (const juce::var& list, int width,
                                                float minDb, float maxDb, float defaultDb);

    /** The canonical form of a packed SWITCH row: `width` comma-separated "0" or
        "1". Any token that parses to a non-zero number is on, the rule
        normaliseMuteList uses and for the same reason (older files carry "1.0").
        width <= 0 canonicalises without resizing. */
    static juce::String normaliseSendSwitchList (const juce::var& list, int width);

    /** One input's send INTO this effect. The column is the input's PERMANENT
        number (1-based, as getInputChannelNumber reports it), never a slot and
        never a display position: a row keyed by slot would repoint every send
        the first time a channel is dragged. The getter answers the row's default
        when the channel, the row or the column is not there; the setter returns
        false and writes nothing. */
    float getEffectSendLevelFromInput (int channelIndex, int inputPermanentNumber) const;
    bool  setEffectSendLevelFromInput (int channelIndex, int inputPermanentNumber, float levelDb);
    bool  getEffectSendOnFromInput    (int channelIndex, int inputPermanentNumber) const;
    bool  setEffectSendOnFromInput    (int channelIndex, int inputPermanentNumber, bool on);

    /** One effect's send into this effect. The column is a DENSE effect index
        (0-based, the index every other effect accessor takes), because effect
        ids are dense and a delete closes the hole up - which is why
        removeEffectChannel has to shift these columns while the input-keyed rows
        are left alone.

        THE DIAGONAL IS REFUSED: channelIndex == sourceEffectIndex returns false
        and writes nothing, and the interceptor forces that cell back off even
        when a whole-row write sets it. An effect feeding itself is a feedback
        loop around a delay line, not a routing choice. */
    float getEffectFxSendLevelFromEffect (int channelIndex, int sourceEffectIndex) const;
    bool  setEffectFxSendLevelFromEffect (int channelIndex, int sourceEffectIndex, float levelDb);
    bool  getEffectFxSendOnFromEffect    (int channelIndex, int sourceEffectIndex) const;
    bool  setEffectFxSendOnFromEffect    (int channelIndex, int sourceEffectIndex, bool on);

    /** The four send rows of one effect, canonicalised and unpacked in one
        pass, for a reader that wants every cell rather than one: the
        calculation engine rebuilds its source x effect gains from these on
        every effects recalc. inLevelsDb / inOns are indexed by input PERMANENT
        NUMBER minus one, the row's own keying; fxLevelsDb / fxOns by dense
        effect index, with the diagonal already forced off. A missing channel
        or row fills the defaults (every send off, levels at the row default),
        which is what an absent row means. */
    void readEffectSendRows (int channelIndex,
                             std::array<float, WFSParameterDefaults::maxInputChannels>& inLevelsDb,
                             std::array<uint8_t, WFSParameterDefaults::maxInputChannels>& inOns,
                             std::array<float, WFSParameterDefaults::maxEffectChannels>& fxLevelsDb,
                             std::array<uint8_t, WFSParameterDefaults::maxEffectChannels>& fxOns) const;

    //==========================================================================
    // Effects link groups
    //
    // The output-array funnel, not the cluster one (R5-6): membership
    // (effectLinkGroup) plus a MODE ON EVERY MEMBER (effectLinkMode), and the
    // receiver's mode is consulted as well as the origin's. Only writes made
    // through these methods propagate - OSC, MCP, snapshots and file loads
    // call the plain setters and reach one channel, as in every other family.
    //==========================================================================

    /** Parameters a link group must never share: identity, position, routing
        and AutomOtion - and the mutes, which are an ACTION instead (R5-1). */
    static bool isEffectLinkExcluded (const juce::Identifier& paramId);

    /** Discrete parameters: copied outright in any mode, never delta'd. The
        table is the CSV's "enum" column plus effectChainOrder. */
    static bool isEffectLinkAbsoluteOnly (const juce::Identifier& paramId);

    /** 0 = unlinked, 1..8. */
    int getEffectLinkGroup (int channelIndex);

    /** 0 = OFF (detached), 1 = ABSOLUTE, 2 = RELATIVE. */
    int getEffectLinkMode (int channelIndex);

    /** Write a per-channel effect parameter, propagating to the rest of its
        link group unless propagateToGroup is false. Instanced module
        parameters (FxEq1/2, FxDyn1/2), EQ bands and delay taps have their own
        entry points below - this one resolves the property on the channel's
        non-instanced children, exactly as setEffectParameter does. */
    void setEffectParameterWithLinkPropagation (int channelIndex,
                                                const juce::Identifier& paramId,
                                                const juce::var& value,
                                                bool propagateToGroup);

    /** Write a parameter on one module node, named by its node type, so the
        doubled EQ and dynamics instances are addressable. */
    void setEffectModuleParameterWithLinkPropagation (int channelIndex,
                                                      const juce::Identifier& moduleType,
                                                      const juce::Identifier& paramId,
                                                      const juce::var& value,
                                                      bool propagateToGroup);

    /** Write one EQ band of one instance. Propagates to the same band of the
        same instance on every member: a link group shares a chain. */
    void setEffectEQBandParameterWithLinkPropagation (int channelIndex,
                                                      int eqInstance,
                                                      int bandIndex,
                                                      const juce::Identifier& paramId,
                                                      const juce::var& value,
                                                      bool propagateToGroup);

    /** Write one delay tap. Propagates to the same tap on every member. */
    void setEffectDelayTapParameterWithLinkPropagation (int channelIndex,
                                                        int tapIndex,
                                                        const juce::Identifier& paramId,
                                                        const juce::var& value,
                                                        bool propagateToGroup);

    /** Mute or unmute every member of a link group in one undo transaction.
        An ACTION, not a coupling (R5-2): each member stays independently
        editable afterwards, which propagation could not express. Writes
        effectMute only - never the per-output effectMutes row, which is
        spatial routing rather than a mute shortcut. */
    void setEffectGroupMute (int group, bool muted);

    /** One chain slot's module node, by slot index 0..10 in the declared order
        (dist, eq1, eq2, dyn1, dyn2, mod, phaser, trem, reverb, delay, crush) or
        by node type. This is the only way to address FxEq1 vs FxEq2 and FxDyn1
        vs FxDyn2, whose properties are indistinguishable by name. */
    juce::ValueTree getEffectModuleSection (int channelIndex, int slotIndex);
    juce::ValueTree getEffectModuleSection (int channelIndex, const juce::Identifier& moduleType);

    /** One EQ band: channel, EQ instance (0 = FxEq1, 1 = FxEq2), band 0..5 -
        the nth <Band> BY TYPE inside the instance, see getOutputEQBand. The
        channel index is resolved by type too, in getEffectState; a file that can
        leave an unknown node in <Effects> can leave one in <FxEq1>. */
    juce::ValueTree getEffectEQSection (int channelIndex, int eqInstance);
    juce::ValueTree getEffectEQBand (int channelIndex, int eqInstance, int bandIndex);

    /** One dynamics stage: channel, instance (0 = FxDyn1, 1 = FxDyn2). */
    juce::ValueTree getEffectDynSection (int channelIndex, int dynInstance);

    /** One multitap delay tap: channel, tap 0..7 - the nth <Tap> BY TYPE under
        <FxDelay>, see getOutputEQBand. All eight taps always exist, which is
        what lets the schema backfill match them by id; it is not a licence to
        index straight into the child list. */
    juce::ValueTree getEffectDelayTap (int channelIndex, int tapIndex);

    /** The eleven chain slot node types in their declared order. A slot index
        outside [0, numEffectModuleSlots) returns an invalid Identifier, which
        getChildWithName can never match. */
    static const juce::Identifier& getEffectModuleType (int slotIndex);

    /** True for the two node types that exist TWICE per channel and therefore
        repeat their property names (FxEq1/FxEq2, FxDyn1/FxDyn2). Every generic
        by-name search skips these: resolving such a property by name alone can
        only ever mean instance 1, and would report success while doing it. */
    static bool isInstancedEffectModuleType (const juce::Identifier& nodeType);

    //==========================================================================
    // Cluster Access
    //==========================================================================

    /** Get cluster parameter (1-based cluster index) */
    juce::var getClusterParameter (int clusterIndex, const juce::Identifier& id) const;

    /** Set cluster parameter (1-based cluster index) */
    void setClusterParameter (int clusterIndex, const juce::Identifier& id, const juce::var& value);

    /** If sourceInputIndex belongs to a cluster whose referenceMode == 2 (Shared
        Position), copy the source input's inputPositionX/Y/Z to every other
        member so the shared-position invariant is maintained. No-op for inputs
        not in a Shared-mode cluster. */
    void propagateSharedClusterPosition (int sourceInputIndex);

    /** Enforce the shared-position invariant on a cluster (1-based clusterIndex):
        if referenceMode == 2 and the cluster has at least one member, snap every
        member's position to the first-ordered member's position. Idempotent. */
    void enforceSharedClusterInvariant (int clusterIndex);

    /** Enforce the shared-position invariant on every cluster. Needed after any
        bulk state apply (project load, config import, snapshot recall): those
        paths write positions via raw setProperty, bypassing the per-write
        propagation, so a Shared-mode cluster saved by an older version (or with
        diverged member positions in the file) would otherwise load diverged. */
    void enforceAllSharedClusterInvariants();

    //==========================================================================
    // Binaural Enable/Solo Access
    //==========================================================================

    /** Get binaural processing enabled state */
    bool getBinauralEnabled() const;

    /** Set binaural processing enabled state */
    void setBinauralEnabled (bool isEnabled);

    /** Get binaural solo mode (0=Single, 1=Multi) */
    int getBinauralSoloMode() const;

    /** Set binaural solo mode */
    void setBinauralSoloMode (int mode);

    /** Check if an input is soloed */
    bool isInputSoloed (int inputIndex) const;

    /** Set input solo state */
    void setInputSoloed (int inputIndex, bool soloed);

    /** Clear all input solo states */
    void clearAllSoloStates();

    /** Get number of currently soloed inputs */
    int getNumSoloedInputs() const;

    /** Get binaural output channel (-1 = disabled) */
    int getBinauralOutputChannel() const;

    /** Set binaural output channel */
    void setBinauralOutputChannel (int channel);

    //==========================================================================
    // Network Target Access
    //==========================================================================

    /** Get number of network targets */
    int getNumNetworkTargets() const;

    /** Add a network target */
    void addNetworkTarget();

    /** Remove a network target */
    void removeNetworkTarget (int targetIndex);

    /** Get network target state */
    juce::ValueTree getNetworkTargetState (int targetIndex);

    //==========================================================================
    // Channel Management
    //==========================================================================

    /** Get current channel counts */
    int getNumInputChannels() const;
    int getNumOutputChannels() const;
    int getNumReverbChannels() const;

    /** Live effect channels - counted from the <Effect> children, never read
        off the `count` property, for the reason getNumReverbChannels records:
        a writer that bypasses setNumEffectChannels makes the property lie, and
        every id lookup built on it then refuses valid channels.

        By TYPE, and getEffectState resolves the nth channel the same way, so the
        two cannot disagree about a container that holds a foreign child. */
    int getNumEffectChannels() const;

    /** Set channel counts. For inputs this is the blunt legacy entry point:
        growth appends default mono channels, reduction removes the
        highest-numbered channels; nothing in the middle ever moves. Not
        undoable.

        Its only remaining caller is the CONFIG LOAD path (WFSFileManager, for
        files written before the channel inventory existed). MCP used to land
        here too, via a setParameter(inputChannels) re-route, which is why its
        removal rule disagreed with the GUI's; the two hand-written count tools
        now call setInputChannelCounts instead. */
    void setNumInputChannels (int numChannels);

    /** Number of live stereo channels — derived from the per-channel type
        (inputChannelType), NOT positional. Mono count =
        getNumInputChannels() − this. */
    int getNumStereoInputChannels() const;

    /** Two-count entry point (System Config count fields): a thin loop over the
        structural ops. Additions append after the last channel; reductions
        remove the LAST channel of that type in DISPLAY order - the bottom of the
        Arrange list, which the operator can see - never the highest-numbered
        one, which after a drag on a latched list may sit anywhere. (The
        one-count setNumInputChannels, now the config-load path only, does
        remove by number; the asymmetry is noted there.) Not undoable. */
    void setInputChannelCounts (int numMono, int numStereo);

    //==========================================================================
    // Stable channel numbers + per-channel type (stable-number rework)
    //==========================================================================
    // Every <Input> carries a permanent 1-based number (its `id` property) and
    // a type (`inputChannelType`: "mono"/"stereo"). The number is the external
    // address (OSC, snapshots, QLab cues, DAW plugin, MCP); the child index
    // ("slot") stays the internal dense key (patch row, render-source slot,
    // meter row). Tree order is the user's DISPLAY order: new channels append
    // at the end, drag-to-reorder moves a node (and its patch row) to a new
    // slot — numbers never change once the session has latched them, so no
    // external reference can break. Before the latch (a fresh session) the
    // structural ops renumber to dense display order instead — see
    // `channelNumbersUserOwned` in WFSParameterIDs.h.

    /** Permanent channel number of the channel at a slot (0-based child
        index); 0 if the slot is invalid. */
    int getInputChannelNumber (int slot) const;

    /** Slot holding a permanent channel number; -1 if no live channel has it. */
    int getSlotForChannelNumber (int number) const;

    /** Type of the channel at a slot (reads inputChannelType; absent = mono). */
    bool isInputChannelStereo (int slot) const;

    /** Highest live channel number (0 when the list is empty). */
    int getHighestChannelNumber() const;

    /** Number the next added channel gets (highest live + 1). */
    int getNextChannelNumber() const;

    /** Lowest number in 1..maxInputChannels with no live channel (0 if the
        list is full). Used by the number-exhaustion gap-reuse dialog. */
    int getLowestFreeChannelNumber() const;

    //==========================================================================
    // Structural channel ops (stable-number model)
    //==========================================================================
    // Each op edits the channel tree AND its input-patch row together, keeps
    // the count properties in step, and clears the undo history (structural
    // edits are deliberately not undoable: ValueTree undo cannot span the
    // flat patchData string safely). The caller runs the reconfiguration
    // pass afterwards (MainComponent::handleChannelCountChange tail:
    // recompute render sources, sanitize/auto-patch, reload patches).

    /** Append a channel (number = highest + 1) or, with explicitNumber > 0,
        re-create a retired number in its sorted slot (the UI confirms gap
        reuse with the user first). New channel's patch row continues the
        diagonal past everything already patched (two columns for stereo) —
        that is the LATCHED behaviour, and it follows CREATION order. Until the
        session latches channel numbers, the list is renumbered to display order
        afterwards and the whole patch is re-flowed with it, into a gapless
        diagonal in DISPLAY order. Its default name is one past the highest
        "Mono n" / "Stereo n" ordinal any live name already claims, so a latched
        session — which never resequences — cannot end up with two channels
        sharing a default name. */
    juce::Result addInputChannel (bool stereo, int explicitNumber = 0);

    /** Remove a live channel by permanent number. Its number is retired
        (gap); every other channel keeps its number, slot and patch row. Until
        the session latches channel numbers, the gap is closed by a renumber to
        display order instead. */
    juce::Result removeInputChannel (int channelNumber);

    /** Flip a live channel's type in place. NOT exposed in the UI or MCP —
        the channel's data (patch columns, width, decomposition) cannot
        meaningfully follow a type change, so composition is edited through
        the counts instead. Kept for the self-test and internal use. */
    juce::Result setInputChannelType (int channelNumber, bool stereo);

    /** Move a live channel to a new display slot (drag-to-reorder). The
        channel node and its patch row move together; the permanent number is
        untouched once the session is latched — before that the list is
        renumbered so the numbers follow the new display order. Stopped-only,
        like every structural edit. */
    juce::Result moveInputChannel (int channelNumber, int targetSlot);

    /** Move a channel node TOGETHER with its patch row and remap the slot-keyed
        cluster orders - the live-drag contract. Shared by moveInputChannel and
        by the display-order restore in WFSFileManager::applyInputsSection, which
        used to raw-move the node alone: correct for a complete load, where the
        file's own patch overwrites every row afterwards, and silently wrong for
        Reload Input Config on its own, which loads no patch and left the live
        rows behind. No latch, no undo bookkeeping - callers own both. */
    void moveInputChannelNodeAndRow (int fromSlot, int toSlot);

    //==========================================================================
    // Channel identity (pre-load check + reconciliation)
    //==========================================================================

    /** 1-based hardware inputs the channel at `slot` holds in its patch row,
        ascending (lower = L for a stereo pair); 0..2 entries. Nothing at tree
        level answered this before - MainComponent derived it at runtime - and
        the saved fingerprint needs it. */
    std::vector<int> getInputPatchHardwareInputs (int slot) const;

    /** The live session's identity: every channel's (slot, number, name, type,
        hardware inputs) in display order. A superset of
        buildInputChannelInventory, which stays the file format. Pure. */
    InputChannelIdentity getInputChannelIdentity() const;

    /** RELABEL - give the channel at each slot the number numbersBySlot[slot].
        The operator's "one-time reorder of the internal channel ID", by
        position: every channel keeps its place, settings, name and hardware
        inputs; only its NUMBER changes, so snapshots, cues and OSC written
        against the file's numbers reach the right channels afterwards.
        Validates completely first (size, range, uniqueness) and touches
        nothing on failure. Patch rows, cluster orders and names are positional
        and untouched. Latches ownership - the numbers just became an external
        contract - and clears every undo history. */
    juce::Result assignInputChannelNumbersBySlot (const std::vector<int>& numbersBySlot,
                                                  const juce::String& reason);

    /** REORDER - arrange the live channels so their numbers read
        numbersInDisplayOrder top to bottom. Numbers, settings and hardware
        inputs travel with their channel (moveInputChannel). Unknown numbers
        are skipped; unlisted live channels end up after the listed ones with
        their relative order kept. Latches FIRST: unlatched, every move would
        recompact the numbers underneath the loop. */
    juce::Result reorderInputChannelsToNumbers (const std::vector<int>& numbersInDisplayOrder,
                                                const juce::String& reason);

    /** The clamp setInputChannelCounts applies, so a prediction clamps identically. */
    static void clampInputChannelCounts (int& numMono, int& numStereo);

    /** Which channels setInputChannelCounts (numMono, numStereo) WOULD remove,
        captured before anything mutates: the last channels of each type in
        DISPLAY order, stereo victims first. By SLOT, never by number - on an
        unlatched session removeInputChannel renumbers after every removal, so a
        by-number prediction is stale by the second one. The dialog that shows
        this used to predict by highest number, which is a different channel
        the moment a latched list has been dragged. */
    std::vector<InputChannelRef> predictInputChannelReduction (int numMono, int numStereo) const;

    //==========================================================================
    // Channel inventory (system-config persistence)
    //==========================================================================

    /** Build the <InputChannelList> node the system config carries: one <Ch>
        per live channel, in DISPLAY order, with its permanent number and type.

        `inputChannels` is only a sum, and the mono/stereo split and the display
        order live solely on the <Input> nodes — which are saved to inputs.xml,
        not system.xml. So a system config reloaded on its own rebuilt every
        channel as mono, and the positional patch rows then landed on the wrong
        channels (a stereo row's two hardware columns on a mono channel). This
        node is what closes that hole. It is a FILE artifact: derived at save
        time, consumed at load time, never stored in the runtime tree, so it
        cannot desync from the nodes it describes. */
    juce::ValueTree buildInputChannelInventory() const;

    /** Reconcile the live channel list to an inventory read from a file:
        remove channels it does not list, create the ones it does with their
        recorded number and type, correct any type that disagrees, then order
        the list to match. Numbers, types, display order and permanent-number
        gaps all survive. Structural and not undoable, like the ops it calls.

        Caller must have latched channel numbers first — otherwise the ops'
        fresh-session tail renumbers the very list this is restoring. */
    void applyInputChannelInventory (const juce::ValueTree& inventory);

    /** Reconcile the input-patch row COUNT to the channel list after a
        wholesale patchData rewrite (config load): truncate extras, append
        diagonal-continue rows (capacity from the channel type). The count only —
        an existing row is never re-columned, so a loaded patch is never
        rewritten under the operator. A no-op on a re-flowed patch
        (compactInputPatchToDisplayOrder already emits exactly one row per live
        channel). Idempotent. */
    void normalizeInputPatchRows();

    /** Convert every cluster's `clusterInputOrder` between the SLOT keying used
        in memory and the permanent-channel-NUMBER keying used on disk.

        Slots are defined by the channel list; the CSV is persisted in
        system.xml while that list comes from inputs.xml, so a slot-keyed CSV
        crossing the file boundary is only valid while the two agree. They do
        not agree mid-load: the channel-list reconciliation deletes and reorders
        underneath a CSV the merge has already brought in. Number keying removes
        the dependency entirely — the same boundary conversion
        serializeExtendedScope/deserializeExtendedScope already use for snapshot
        scope, which is proven correct across a reorder.

        Tokens that cannot be resolved are dropped (a number with no live
        channel, a slot past the end). Both are message-thread only and write
        with no undo manager: this is bookkeeping, not an operator edit. */
    void convertClusterOrdersSlotsToNumbers();
    void convertClusterOrdersNumbersToSlots();

    /** One-time model migration for loaded states: repairs missing/duplicate
        ids (one dense renumber — the last renumbering that can ever happen to
        a file) and stamps inputChannelType from the legacy tail split when
        the whole list lacks it. Tree order is preserved (it is the user's
        display order). Idempotent, not undoable. Must run BEFORE
        ensureCompleteSchema on wholesale-replace loads: the schema template
        stamps mono, which would otherwise preempt the tail-split stamp. */
    void migrateInputChannelModel();

    /** Renumber every input channel to its display slot + 1 (dense 1..N). The
        tracking id, stamped FROM the number, follows it; anything the user
        changed stays. Names do NOT follow the number: a default name is a
        per-type ordinal maintained by resequenceDefaultInputNames(). Only ever
        called while the session has not latched channel numbers. Idempotent.

        Anything keyed by the permanent NUMBER has to move with it, and the
        effect send rows are: the whole permutation is computed before the walk
        and applied once after it (see remapEffectSendColumnsByInputNumber, and
        the note below about the list briefly holding a number twice). */
    void compactChannelNumbersToDisplayOrder();

    /** Re-flow the whole input patch into a gapless diagonal in DISPLAY order:
        walk the slots top to bottom handing out consecutive hardware input
        columns — two ADJACENT columns for a stereo row (lower = L), one for a
        mono row. Strict packing: no gaps, and no alignment to the interface's
        odd/even pairs, so N mono + M stereo always fit in N + 2M inputs.
        Rebuilt FROM the channel list, so the stored rows are discarded outright
        and a wrong row count is repaired rather than reconciled. The diagonal
        may legitimately run past activeHardwareInputs — cols widens and the
        matrix dims those columns, deliberately, so the patch does not depend on
        which interface happened to be plugged in at edit time. Shares the
        `channelNumbersUserOwned` latch with compactChannelNumbersToDisplayOrder()
        (there is no second, patch-specific ownership flag) and is only ever
        called while the session has not latched. Idempotent. */
    void compactInputPatchToDisplayOrder();

    /** Renumber the DEFAULT input names to "Mono n" / "Stereo n", counting each
        type independently in display order. A name the user typed is left
        alone: only the three shapes the app itself stamps — "Input n" (legacy),
        "Mono n", "Stereo n", with any n — count as still-default. Called when
        the arrange dialog closes on a session that has not latched channel
        numbers; latching is not implied by it. Idempotent. */
    void resequenceDefaultInputNames();

    /** previousCount: how many outputs existed before this change, when the
        caller has already added output nodes itself (an output-config merge);
        -1 = the current node count. Decides which mute-list entries are new.

        Refits the per-output mute row of all THREE families that carry one:
        inputMutes, reverbMutes and effectMutes. Only inputMutes was refitted
        before, so a reverb's row stayed at whatever width it was created at and
        self-healed only because ReverbTab rewrites it whole whenever the user
        opens that tab - with a hard-coded fallback of 16, which is what hid the
        gap. The send rows of <Sends> are deliberately NOT touched here: they are
        keyed by input number and by effect index, neither of which has anything
        to do with how many outputs the rig has.

        THE REFIT ONLY GROWS (perOutputRowWidth). A row is padded up to the live
        count and never cut down to it, because an output count that goes DOWN is
        usually temporary - an interface that dropped, an edit about to be undone
        - and a row cut to fit comes back padded with "0" where the operator's
        mutes were. The single exception is the legacy 64/128 grid list, whose
        tail the keepTokens window has just declared meaningless anyway. */
    void setNumOutputChannels (int numChannels, int previousCount = -1);
    void setNumReverbChannels (int numChannels);

    /** Resize the effect channel list to [0, maxEffectChannels]. Zero is a
        legal count and the default: a show that uses no effects carries an
        empty container and nothing else.

        Growth appends default channels laid out for the TARGET count, so the
        whole set sits on one ring rather than on the ring each channel happened
        to be born under; reduction drops from the end. Stopped-only and NOT
        undoable, like every other structural edit: the undo histories are
        cleared when the count actually moves.

        That clear is the INPUT precedent rather than an effects invention.
        setNumInputChannels is non-undoable in exactly this way and reaches the
        same end through addInputChannel / removeInputChannel, which clear on
        every call - so a remote write of inputChannels already empties the
        stack today, over the same setParameter route effectChannels will use.
        setNumOutputChannels and setNumReverbChannels clear nothing because
        their structural writes go through getActiveUndoManager and sit on the
        stack like any other edit; a family whose writes pass nullptr has no
        such option. Leaving the old history standing here would let entries
        recorded above a removal replay onto nodes that have since been
        renumbered - ids in this family are dense - which is precisely what the
        clear buys out. */
    void setNumEffectChannels (int numChannels);

    /** Append one effect channel. Fails when the list is already at
        maxEffectChannels. Stopped-only, not undoable. */
    juce::Result addEffectChannel();

    /** Remove one effect channel by its dense index (0-based).

        Effect ids are DENSE (id == index + 1) - unlike input channel numbers,
        which are permanent and leave gaps - so removing a channel renumbers
        every channel above it. That is why the family has no permanent-number
        latch and no gap reuse: an effect return is addressed by its position in
        the list, and a delete is expected to close up behind it. Stopped-only,
        not undoable. */
    juce::Result removeEffectChannel (int channelIndex);

    /** Update hardware channel count in patch trees based on actual audio device.
     *  Pass 0 for either count when no device is connected to trigger the
     *  "default to 64 or highest patched channel" policy. */
    void updateHardwareChannelCount (int hwInputs, int hwOutputs);

    /** Recompute patch-matrix column counts using the most recent device
     *  channel counts stored on the patch trees. Call after any patch edit
     *  so cols can shrink once overflow routes are removed. */
    void recomputePatchCols();

    /** Max hardware channels the patch matrix can address. */
    static constexpr int maxHardwarePatchChannels = 512;

    //==========================================================================
    // Undo / Redo  (per-domain — one UndoManager per tab)
    //==========================================================================
    // The UndoManager array, active-domain state, undo/redo/canUndo/canRedo,
    // beginUndoTransaction, clearUndoHistory/clearAllUndoHistories and the
    // MCP-origin undo suppression live in TreeParameterStore. These thin
    // wrappers map the WFS tab-domain enum onto the core's integer indices.

    /** Set the currently active undo domain (called by MainComponent on tab change) */
    void setActiveDomain (UndoDomain domain)
    {
        TreeParameterStore::setActiveDomain (static_cast<int> (domain));
    }

    /** Get the currently active undo domain */
    UndoDomain getActiveDomain() const
    {
        return static_cast<UndoDomain> (TreeParameterStore::getActiveDomain());
    }

    /** Get UndoManager for a specific domain */
    juce::UndoManager* getUndoManagerForDomain (UndoDomain domain)
    {
        return TreeParameterStore::getUndoManagerForDomain (static_cast<int> (domain));
    }

    /** RAII helper: writes made while alive bypass the UndoManager entirely.
        Used for externally triggered snapshot recalls (MIDI note, OSC) so a
        cue-driven show does not bury the operator's own edits under one undo
        entry per cue. See TreeParameterStore::ScopedUndoSuppression. */
    using ScopedUndoSuppression = TreeParameterStore::ScopedUndoSuppression;

    /** RAII helper: temporarily switch the active undo domain, restoring on destruction */
    struct ScopedUndoDomain
    {
        ScopedUndoDomain (WFSValueTreeState& s, UndoDomain d)
            : state (s), previous (s.getActiveDomain()) { state.setActiveDomain (d); }
        ~ScopedUndoDomain() { state.setActiveDomain (previous); }
        WFSValueTreeState& state;
        UndoDomain previous;
    };

    //==========================================================================
    // State Management
    //==========================================================================

    /** Reset all parameters to defaults */
    void resetToDefaults();

    /** Reset input channel to defaults */
    void resetInputToDefaults (int channelIndex);

    /** Reset output channel to defaults */
    void resetOutputToDefaults (int channelIndex);

    /** Reset reverb channel to defaults */
    void resetReverbToDefaults (int channelIndex);

    /** Redistribute all input positions evenly within the current stage bounds */
    void redistributeAllInputPositions();

    /** Re-lays ALL reverb nodes on the initial arc for the current stage and
        node count (the reverb twin of redistributeAllInputPositions). */
    void redistributeAllReverbPositions();

    /** Re-lays ALL effect returns on their default ring for the current stage
        and channel count, feed orientations included. The effects twin of
        redistributeAllReverbPositions; gated by the effects-only ownership
        latch, never by the shared input/reverb one. */
    void redistributeAllEffectPositions();

    //==========================================================================
    // Position ownership (see positionsUserOwned in WFSParameterIDs.h).

    /** True once the user owns the channel positions: they opened the Map tab
        or manually edited an input/output/reverb position. One-way, persisted
        with the session. While false, stage size/shape/origin and channel-count
        changes silently re-run the initial placement (inputs grid, reverb arc);
        once true, nothing repositions automatically — only the explicit
        buttons do. */
    bool arePositionsUserOwned();

    /** Latches position ownership to the user. Idempotent; not undoable on
        purpose (Ctrl+Z must not re-arm auto-placement). */
    void markPositionsUserOwned();

    //==========================================================================
    // Effect position ownership - ITS OWN LATCH, stored on <Effects> as
    // effectPositionsUserOwned, never the shared positionsUserOwned flag on
    // <Stage>.
    //
    // Sharing the flag looks harmless and is not: positionsUserOwned is latched
    // by opening the Map tab and by any input, output or reverb position edit,
    // so in a real session it is true long before the first effect channel
    // exists. setNumEffectChannels would then skip the layout pass for every
    // effect channel ever created, and the whole family would stack on the
    // origin - which is also where the angular feed attenuation and the
    // inter-node geometry are least meaningful. A separate latch costs one
    // property and keeps "the user has placed the effect returns" answerable on
    // its own terms.

    /** True once the user owns the effect return positions. One-way, and it
        will persist with the family - but NOTHING persists the family yet:
        <Effects> has no section writer, so today the latch and the positions it
        protects are both session-lived and both come back false on the next
        load. Make this sentence unconditional in the commit that adds
        effects.xml, not before. While false, a channel-count change re-lays the
        whole ring. */
    bool areEffectPositionsUserOwned() const;

    /** Latches effect position ownership. Idempotent; not undoable, for the
        same reason its input/reverb twin is not. */
    void markEffectPositionsUserOwned();

    //==========================================================================
    // Channel-number ownership (see channelNumbersUserOwned in WFSParameterIDs.h).

    /** True once the input channel numbers are permanent. While false — a
        fresh session nothing has ever exposed a number from — every structural
        edit renumbers the list to dense display order. Returns TRUE when the IO
        tree is invalid: renumbering rewrites ids across the whole list, so
        half-built or malformed state must fall back to the permanent regime
        rather than be mistaken for "fresh". (arePositionsUserOwned returns
        false there; the destructive direction is the opposite one.) */
    bool areChannelNumbersUserOwned();

    /** Latches the channel numbers as permanent. Idempotent; not undoable on
        purpose (Ctrl+Z must not re-arm renumbering). The reason names the
        trigger and is logged once, on the actual fresh->owned transition —
        the latch is otherwise invisible and a spent one is indistinguishable
        from a broken re-flow. */
    void markChannelNumbersUserOwned (const juce::String& reason);

    /** Scale all input positions proportionally from old stage bounds to current bounds */
    void scaleAllInputPositions (float oldW, float oldD, float oldH,
                                 float oldOW, float oldOD, float oldOH);

    /** Clamp all input positions to within the current stage bounds */
    void fitAllInputPositionsToStage();

    /** Shift all input positions by a 3D delta */
    void shiftAllInputPositions (float dx, float dy, float dz);

    /** Shift all output positions by a 3D delta */
    void shiftAllOutputPositions (float dx, float dy, float dz);

    /** Shift all reverb positions by a 3D delta */
    void shiftAllReverbPositions (float dx, float dy, float dz);

    /** Replace entire state (e.g., when loading) */
    void replaceState (const juce::ValueTree& newState);

    /** Validate state structure */
    bool validateState (const juce::ValueTree& stateToValidate) const;

    /** Copy state from another WFSValueTreeState */
    void copyStateFrom (const WFSValueTreeState& other);

    /** Migrate old flat ADM-OSC section to new nested mapping structure */
    void migrateADMOSCSection();

    /** Ensure all inputs have the inputAdmMapping property (migration) */
    void ensureInputAdmMappingProperty();

    /** Back-fill any sections/properties that the default schema defines but a
        loaded state is missing. Used on the wholesale-replace load path
        (importCompleteConfig / full-config snapshot recall) so that an incomplete
        or scope-filtered file cannot leave parameters permanently absent. */
    void ensureCompleteSchema();

protected:
    //==========================================================================
    // TreeParameterStore seams (change-notification dispatch hooks)
    //==========================================================================

    /** Derive the channel index for a changed node (Input/Output/Reverb parent id) */
    int resolveChannelIndex (const juce::ValueTree& changedNode) const override;

    /** POST-WRITE HOOK — WFS semantic invariants (cluster tracking uniqueness,
        shared-position snap) run here, before listener dispatch. */
    void handlePostWrite (juce::ValueTree& changedNode, const juce::Identifier& property,
                          const juce::var& value, int channelIndex) override;

private:
    /** The member half of a link-group write: the source channel has already
        been written by the caller. sectionFor resolves the node carrying the
        property on a given member, which is what lets one core serve plain
        parameters, module nodes, EQ bands and delay taps. */
    void applyEffectLinkPropagation (int channelIndex,
                                     const juce::Identifier& paramId,
                                     const juce::var& newValue,
                                     const juce::var& oldValue,
                                     const std::function<juce::ValueTree (int)>& sectionFor);

    /** The child of one <Effect> carrying paramId, skipping the instanced
        module types for the reason getEffectParameter documents. */
    juce::ValueTree findEffectSectionCarrying (int channelIndex, const juce::Identifier& paramId);

    /** Set one channel's permanent number, dragging its tracking id along only
        while that still matched the old number. Raw setProperty: a renumber is
        bookkeeping and must carry no undo entry, dirty mark or ownership latch.
        Shared by the fresh-session compaction and the relabel. */
    void setInputChannelNumberAtSlot (int slot, int newNumber);
    //==========================================================================
    // Initialization
    //==========================================================================

    void initializeDefaultState();
    void createConfigSection();
    void createShowSection (juce::ValueTree& config);
    void createIOSection (juce::ValueTree& config);
    void createStageSection (juce::ValueTree& config);
    void createMasterSection (juce::ValueTree& config);
    void createNetworkSection (juce::ValueTree& config);

    /** Move config properties older files kept in <IO> into the sections that now
        stamp them, so no value is left duplicated across two homes. */
    void migrateStrayConfigProperties();
    void createADMOSCSection (juce::ValueTree& config);
    void createTrackingSection (juce::ValueTree& config);
    void createClustersSection (juce::ValueTree& config);
    void createBinauralSection (juce::ValueTree& config);
    void createUISection (juce::ValueTree& config);

    /** <Config><EffectsGlobal> - the nine effectsGlobal* settings.

        A child of <Config>, NOT a sibling of the <Effect> channels: that is what
        keeps <Effects> holding only <Effect> children, which is what lets
        getEffectState index the child list instead of walking it by type the way
        every reverb accessor must. getParameterScope already routes all nine
        names here (the "effectsGlobal" test sits ahead of the per-channel
        "effect" prefix), and getTreeForParameter's Config branch searches this
        node with the others - until it existed, every one of those writes
        resolved to Config, found no node carrying the property, and was dropped
        with no error at all. */
    void createEffectsGlobalSection (juce::ValueTree& config);
    void createInputsSection();
    void createOutputsSection();
    void createReverbsSection();

    /** <Effects count="0" effectPositionsUserOwned="0"> - the container only,
        at the default count of zero. Appended unconditionally by
        initializeDefaultState so every fresh tree carries the family even when
        no show ever uses it, and created by ensureCompleteSchema for any state
        that predates it (validateState does not require the node, so a loaded
        project can arrive without one). */
    void createEffectsSection();
    void createAudioPatchSection();

    /** Create a single default input channel */
    /** @param totalInputsIn  target channel count; pass it explicitly while
        growing the list, since the tree still holds the old count then.
        <= 0 reads the tree (correct for single-channel resets). */
    /** @param channelNumber  permanent channel number; <= 0 derives it from
        the slot (index + 1, dense creation). */
    juce::ValueTree createDefaultInputChannel (int index, int totalInputsIn = -1, int channelNumber = -1);

    /** Create input channel subsections */
    /** @param ordinal  1-based position among the channels of that type; it is
        the counter behind the "Mono n" / "Stereo n" default name. */
    juce::ValueTree createInputChannelSection (bool stereo, int ordinal);
    juce::ValueTree createInputPositionSection (int index, int totalInputs);
    juce::ValueTree createInputAttenuationSection();
    juce::ValueTree createInputDirectivitySection();
    juce::ValueTree createInputLiveSourceSection();
    juce::ValueTree createInputHackousticsSection();
    juce::ValueTree createInputLFOSection();
    juce::ValueTree createInputAutoMotionSection();
    juce::ValueTree createInputMutesSection (int numOutputs);
    juce::ValueTree createInputGradientMapsSection();
    juce::ValueTree createInputSamplerSection();

    /** Create a single default output channel */
    juce::ValueTree createDefaultOutputChannel (int index);

    /** Create output channel subsections */
    juce::ValueTree createOutputChannelSection (int index);
    juce::ValueTree createOutputPositionSection();
    juce::ValueTree createOutputOptionsSection();
    juce::ValueTree createOutputEQSection();

    /** Create a single default reverb channel */
    juce::ValueTree createDefaultReverbChannel (int index, int totalCount);

    /** Stage dimensions in the form the reverb node placement helper wants.
        Falls back to a nominal extent when no stage section exists yet. */
    ReverbNodePlacement::Stage getStageForPlacement();

    /** The default layout node for one reverb channel — position AND the feed
        orientation that goes with it. Both sections need the same node, and the
        orientation is only meaningful next to the position it was derived from,
        so they come from one call rather than being recomputed apart. */
    ReverbNodePlacement::Node getDefaultReverbNode (int index, int totalCount);


    /** One default effect channel: <Effect id="index + 1"> with its six flat
        sections, its eleven module nodes and a <Sends> holding all four rows.

        totalCount is the TARGET channel count, so a channel built while the
        list is growing lands on the ring the finished set will use. The layout
        node is computed ONCE here and handed to the position and feed builders:
        the feed bearing is only meaningful beside the position it came from,
        and the reverb twin's habit of recomputing the whole layout in each of
        them is what makes that path quadratic. */
    juce::ValueTree createDefaultEffectChannel (int index, int totalCount);

    /** Default ring for the effect returns: the reverb arc helper, pushed
        outwards so the two families do not land on each other when their counts
        happen to match. Returns the whole set; index it, do not recompute it
        per channel. */
    std::vector<ReverbNodePlacement::Node> layoutEffectNodes (int totalCount);

    /** Create effect channel subsections. The two doubled module builders take
        the node TYPE rather than an instance number: FxEq1 and FxEq2 differ
        only by type and carry identical defaults, so one builder stamps
        whichever type it is asked for. */
    juce::ValueTree createEffectChannelSection (int index);
    juce::ValueTree createEffectPositionSection (const ReverbNodePlacement::Node& node);
    juce::ValueTree createEffectFeedSection (int orientationDeg);
    juce::ValueTree createEffectReturnSection (int numOutputs);
    juce::ValueTree createEffectAutoMotionSection();
    juce::ValueTree createEffectChainSection();

    /** <Sends> with all four rows stamped at their fixed widths and defaults.
        Takes the channel's DENSE index for one reason: the fx diagonal. A row
        built without knowing which channel it belongs to cannot say which of its
        32 columns is the channel itself, and a self-send that starts on is a
        loop the operator never asked for. */
    juce::ValueTree createEffectSendsSection (int channelIndex);
    juce::ValueTree createEffectDistSection();
    juce::ValueTree createEffectEQSection (const juce::Identifier& nodeType);
    juce::ValueTree createEffectDynSection (const juce::Identifier& nodeType);
    juce::ValueTree createEffectModSection();
    juce::ValueTree createEffectPhaserSection();
    juce::ValueTree createEffectTremSection();
    juce::ValueTree createEffectReverbSection();
    juce::ValueTree createEffectDelaySection();
    juce::ValueTree createEffectCrushSection();

    //==========================================================================
    // Send-row internals (see "The send matrix" in the public section)
    //==========================================================================

    /** The canonical form of ONE named send row: the single place that maps a
        row identifier to its width, its value kind and its diagonal rule. Every
        writer goes through it - the builder, the cell setters, the column
        maintenance and the write interceptor - so a row cannot acquire a shape
        that depends on which door it came in by.

        selfEffectIndex is the DENSE index of the channel the row belongs to, and
        -1 when that cannot be resolved (a detached node under construction); the
        fx diagonal is only forced when it is known. Passing an identifier that is
        not one of the four rows is a programming error and returns the input
        unchanged. */
    static juce::String canonicalEffectSendRow (const juce::Identifier& rowId,
                                                const juce::var& list,
                                                int selfEffectIndex);

    /** The dense index of the <Effect> a node lives under, by the same
        count-by-type walk getEffectState uses, or -1 when the node is not under
        a live effect channel. The interceptor needs it: a <Sends> node knows its
        own diagonal only through its channel. */
    int denseEffectIndexOfNode (const juce::ValueTree& node) const;

    /** One cell of one row, as the text stored for it; empty when the channel,
        the row or the column does not exist. The read canonicalises first, so a
        hand-edited short row answers for every column it is supposed to have. */
    juce::String readEffectSendCell (int channelIndex, const juce::Identifier& rowId,
                                     int column) const;

    /** Write one cell: read-modify-write of the WHOLE row through
        setEffectParameter, so it carries one undo entry and one notification
        (setInputOutputMute is the precedent). False when the channel, the row or
        the column is out of reach. */
    bool writeEffectSendCell (int channelIndex, const juce::Identifier& rowId,
                              int column, const juce::String& token);

    /** INPUT DELETE. Reset one input's column to the row default in both
        input-keyed rows of every effect channel. Called by removeInputChannel
        BEFORE the compaction, so what the compaction then shifts is a row with
        no dead channel left in it: without this, a channel later re-created on
        that retired number inherits the dead channel's sends. */
    void zeroEffectSendColumnsForInput (int inputPermanentNumber);

    /** INPUT RENUMBER. Apply ONE permutation of input permanent numbers to the
        input-keyed rows of every effect channel.

        It takes the whole map and applies it in one pass on purpose. Both
        renumber paths walk slot by slot through setInputChannelNumberAtSlot, and
        mid-walk the list can hold the same number twice (the compaction's own
        comment records it), so a remap driven one write at a time would move a
        column onto one that has not moved yet and collapse two channels' sends
        into one. A swap - the case a shift-shaped renumber never produces -
        breaks that way every time.

        Columns the map does not mention are left ALONE rather than cleared:
        this must not infer from "no live channel owns that number today" that
        the column is dead. The one exception is a column the map VACATES (a
        source that is nothing's destination), which is reset to the row default
        because its owner has demonstrably moved away. */
    void remapEffectSendColumnsByInputNumber (const std::map<int, int>& oldToNewNumbers);

    /** EFFECT DELETE. Drop one column from the fx-keyed rows of every surviving
        channel and shift the ones above it down, then force each survivor's
        diagonal at its NEW index. The fx rows are keyed by dense index, so a
        removal renumbers every column above the hole exactly as it renumbers the
        channels themselves; leaving them put would re-point every send above the
        deleted channel by one. Call it AFTER the node is gone and the ids have
        been re-stamped. */
    void dropEffectFxSendColumn (int removedEffectIndex);

    /** Create reverb channel subsections */
    juce::ValueTree createReverbChannelSection (int index);
    juce::ValueTree createReverbPositionSection (int index, int totalCount);
    juce::ValueTree createReverbFeedSection (int orientationDeg);
    juce::ValueTree createReverbEQSection();
    juce::ValueTree createReverbReturnSection (int numOutputs);
    juce::ValueTree createReverbAlgorithmSection();
    juce::ValueTree createReverbPreCompSection();
    juce::ValueTree createReverbPostEQSection();
    juce::ValueTree createReverbPostExpSection();

    /** Create a default network target */
    juce::ValueTree createDefaultNetworkTarget (int index);

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /** Find the correct ValueTree for a given parameter ID (core schema-routing seam) */
    juce::ValueTree getTreeForParameter (const juce::Identifier& id, int channelIndex) const override;

    /** Determine if a parameter belongs to input, output, reverb, or config */
    enum class ParameterScope { Config, Input, Output, Reverb, Effect, Cluster, AudioPatch, Unknown };
    ParameterScope getParameterScope (const juce::Identifier& id) const;

    /** Enforce cluster tracking constraint: only one tracked input per cluster
     *  Called when inputTrackingActive or inputCluster changes */
    void enforceClusterTrackingConstraint (int changedInputIndex);

    /** Migration-only: stamp the pre-rework tail split ("the LAST
        stereoCountOverride channels are stereo") onto the per-channel type
        property. The caller reads the count from the legacy IO property. */
    void stampChannelTypesFromLegacySplit (juce::UndoManager* um, int stereoCountOverride);

    /** Patch-row halves of the structural ops: insert a diagonal-continue
        row for a new channel at its slot / remove a deleted channel's row.
        Rows are positional (row = slot), so they must mirror every channel
        tree edit in the same op.

        Once the session has latched, these three ARE the whole story. Unlatched
        their result is immediately overwritten by
        compactInputPatchToDisplayOrder(), and that redundancy is deliberate:
        doing the row bookkeeping here keeps the row count in step with the
        channel list at every instant (a synchronous listener can never observe a
        tree whose row count disagrees with the channel count), it keeps the
        re-flow a pure function of the channel list rather than a repair step it
        is required to perform, and it stops the latched regime resting on the
        unlatched one's correctness. */
    void insertInputPatchRow (int slot, bool stereo);
    void removeInputPatchRow (int slot);
    void moveInputPatchRow (int fromSlot, int toSlot);

    /** clusterInputOrder csvs hold 0-based slot indices; remap them whenever
        a structural edit shifts slots (delete/reorder). Returning -1 from the
        mapper drops the entry. */
    void remapClusterInputOrders (const std::function<int (int)>& oldSlotToNewSlot);

    /** Clamp a value to the valid range for a given output parameter */
    static float clampOutputParamToRange (const juce::Identifier& paramId, float value);

    /** Set output parameter directly without array propagation (internal use) */
    void setOutputParameterDirect (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Set EQ band parameter directly without array propagation (internal use) */
    void setOutputEQBandParameterDirect (int channelIndex, int bandIndex, const juce::Identifier& id, const juce::var& value);

    ArrayMuteState arrayMutes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSValueTreeState)
};
