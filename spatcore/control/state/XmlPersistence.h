#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <functional>
#include <optional>

namespace spatcore::control::state
{

/**
 * XmlPersistence — app-agnostic ValueTree↔XML persistence machinery.
 *
 * Extracted from the app's WFSFileManager (Phase 4c-3 of the spatcore
 * extraction). Owns the mechanics that carry no app schema:
 *
 *  - XML file write with the commented header convention
 *    (`<!-- <title> --> / <!-- Type: ... --> / <!-- Created: ... -->`).
 *    The title line is app data, injected at construction.
 *  - generic XML file read/parse into a ValueTree, reporting the failure
 *    stage (missing file / parse error / tree conversion) so the app can
 *    map it to its own (localized) error strings.
 *  - timestamped rolling backups: copy-aside into a backups folder with a
 *    `<name>_<yyyymmdd_hhmmss>` suffix, newest-first listing by prefix,
 *    and keep-last-N retention.
 *  - the merge/backfill engine (mergeProperties / mergeTreeRecursive):
 *    "missing = keep" property merge plus recursive child matching by
 *    type + id (and by type + ordinal among id-less siblings). Per-property
 *    value validation is app policy and is injected as a std::function —
 *    the core never names a parameter or a bounds table.
 *
 * Everything schema-shaped — the section-split file layout, manifest
 * handling, snapshots, migrations, dialogs — stays in the app.
 */
class XmlPersistence
{
public:
    //==========================================================================
    // Configuration
    //==========================================================================

    /** Per-property merge validator. Called for every property about to be
     *  merged into the target tree. Return the value to apply (usually the
     *  input value, unchanged), or std::nullopt to reject the property — the
     *  target then keeps its current value. The app owns logging/diagnostics
     *  for rejected values. A null function accepts everything. */
    using PropertyValidator =
        std::function<std::optional<juce::var> (const juce::Identifier& property,
                                                const juce::var& value)>;

    struct Options
    {
        /** First comment line written into every XML file header. */
        juce::String headerTitle;

        /** Property used to match children across trees during merges
         *  (channel-style children carry a stable id). */
        juce::Identifier idProperty { "id" };

        /** Optional per-property merge validator (see PropertyValidator). */
        PropertyValidator propertyValidator;
    };

    explicit XmlPersistence (Options options);

    //==========================================================================
    // XML File I/O
    //==========================================================================

    enum class WriteResult
    {
        ok,
        xmlConversionFailed,   ///< ValueTree could not be converted to XML
        fileWriteFailed        ///< File could not be written
    };

    enum class ReadError
    {
        none,
        fileNotFound,          ///< File does not exist
        parseFailed,           ///< File exists but is not parseable XML
        treeConversionFailed   ///< XML parsed but produced no valid ValueTree
    };

    struct ReadResult
    {
        juce::ValueTree tree;
        ReadError error = ReadError::none;
    };

    /** Write a ValueTree to an XML file with the commented header convention
     *  (human-readable, no JUCE XML declaration of its own — the header
     *  carries it). The header's Type line is the file's base name. */
    WriteResult writeTreeToFile (const juce::ValueTree& tree, const juce::File& file) const;

    /** Read a ValueTree from an XML file, reporting the failure stage. */
    ReadResult readTreeFromFile (const juce::File& file) const;

    /** Build the commented XML header for a given file type. */
    juce::String makeXmlHeader (const juce::String& fileType) const;

    //==========================================================================
    // Rolling Backups
    //==========================================================================

    /** Timestamp suffix used for backup file names (yyyymmdd_hhmmss). */
    static juce::String backupTimestamp();

    /** Copy a file aside into the backup folder as
     *  `<name>_<timestamp><ext>`. Returns true if the source file does not
     *  exist (nothing to back up). Creates the backup folder if needed. */
    static bool createBackup (const juce::File& file, const juce::File& backupFolder);

    /** List backups for a file-name prefix, newest first. */
    static juce::Array<juce::File> listBackups (const juce::File& backupFolder,
                                                const juce::String& filePrefix);

    /** Delete all but the newest keepCount backups for each prefix. */
    static void cleanupBackups (const juce::File& backupFolder,
                                const juce::StringArray& filePrefixes,
                                int keepCount);

    //==========================================================================
    // Merge / Backfill Engine
    //==========================================================================

    /** Merge properties from source into target. Only properties present in
     *  source are copied — missing properties keep their current value.
     *  Each property passes through the injected validator first; rejected
     *  properties keep the target's current value. */
    void mergeProperties (juce::ValueTree& target, const juce::ValueTree& source,
                          juce::UndoManager* undoManager) const;

    /** Recursively merge source into target, preserving target properties
     *  and children absent from source. Children carrying the id property
     *  are matched by type AND id; id-less children are matched by type and
     *  ordinal position among same-type id-less siblings. Unmatched source
     *  children are appended as copies. */
    void mergeTreeRecursive (juce::ValueTree& target, const juce::ValueTree& source,
                             juce::UndoManager* undoManager) const;

private:
    Options options;
};

} // namespace spatcore::control::state
