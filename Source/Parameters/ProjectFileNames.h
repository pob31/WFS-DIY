#pragma once

#include <juce_core/juce_core.h>

/**
 * Files a project keeps in its own folders (ir/, samples/), named by a plain
 * file name.
 *
 * reverbIRfile and samplerCellFile hold a file name inside the project's ir/
 * or samples/ folder. They also arrive over MCP and in project files someone
 * else wrote, and juce::File::getChildFile follows whatever they say: an
 * absolute path, "..", or a UNC path that makes Windows hand the share the
 * user's credentials (re-audit 2026-09-29, B4).
 */
namespace ProjectFileNames
{

/** A name that can only mean a file directly inside a folder: not empty, no
    path separator, no drive or stream colon, not "." or "..", no control
    character, at most 255 characters. */
inline bool isPlainFileName (const juce::String& name)
{
    if (name.isEmpty() || name.length() > 255 || name == "." || name == ".."
        || name.containsAnyOf ("/\\:"))
        return false;

    for (auto c : name)
        if (c < 0x20)
            return false;

    return true;
}

/** The file `name` stands for directly inside `folder`, or an invalid
    juce::File when there is no folder or `name` is not a plain file name.
    "./" first: on macOS and Linux getChildFile reads a leading '~' as a home
    directory (the R6 trap); the parent is checked as well. */
inline juce::File childFileByName (const juce::File& folder, const juce::String& name)
{
    if (folder == juce::File() || ! isPlainFileName (name))
        return {};

    const auto file = folder.getChildFile ("./" + name);
    return file.getParentDirectory() == folder ? file : juce::File();
}

} // namespace ProjectFileNames
