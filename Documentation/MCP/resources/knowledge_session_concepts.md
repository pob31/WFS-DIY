# Sessions, Snapshots, and Scope

A WFS-DIY session is more than just the running audio engine — it includes the venue configuration, the speaker layout, the reverb setup, the input parameters, and a library of snapshots that can be recalled during a performance. This document explains how the session structure works and what's stored where.

## A session = a project folder

Each show or installation lives in its own folder on disk. When starting a session, the operator either:

- **Creates a new folder** for a new project.
- **Navigates to an existing folder** to reload a previous project.

Inside the folder are several XML files (one per major section) and sub-directories for audio samples and reverb impulse responses. The structure means a session can be moved, backed up, archived, or version-controlled as a single coherent unit.

## Section files

The session is divided into separate sections, each with its own file:

- **System configuration** — channel counts, master level, system latency, Haas effect, multithread settings.
- **Stage** — dimensions, origin point, speed of sound / temperature, allowed flip axes.
- **Network** — interface selection, ports, target/server definitions, OSCQuery, tracking protocol setup.
- **Outputs** — every speaker's position, orientation, group assignment, EQ, parallax, attenuation settings.
- **Reverbs** — feed and return channels with their positions and processing settings.
- **Inputs** — every input channel's parameters (position, attenuation, directivity, LFO state, maps, tracking assignment, cluster, mute matrix).

Each section can be:

- **Saved** independently (overwriting the file with current state).
- **Loaded** independently (recalling the saved state for that section only).
- **Exported** to a file (using a file dialog, allowing arbitrary save location or filename).
- **Imported** from a file (selecting any saved file from any project).

Cross-project import lets you reuse, for example, the same speaker configuration in multiple shows at the same venue, without manually re-entering it each time.

## Auto-save and backup

Every change triggers an autosave one second after the last modification. The previous state is kept as a backup, accessible via "Reload Backup" buttons in each section.

The autosave mechanism can be disabled globally if it's interfering (rare).

## Snapshots

Snapshots store **input state** at a moment in time — positions, attenuations, LFO states, mute states, etc., for all 64 inputs. They are intended for cue-based recall during a performance: the show progresses through scenes, and each scene loads a different snapshot.

Snapshots are stored as separate files in the project folder, with names beginning with `snapshot_` followed by a descriptive name or a timestamp.

Operations:

- **Store new** — creates a new snapshot file with the current input state, timestamped by default.
- **Recall (load)** — loads a snapshot into the current input state, applying scope filtering (see below).
- **Update** — overwrites an existing snapshot with the current state.
- **Delete** — removes a snapshot file.
- **Edit scope** — opens the scope window for the selected snapshot (see below).

All snapshot operations are also accessible via OSC, so show control software can trigger them automatically at scene boundaries.

## Snapshot scope

A snapshot doesn't have to apply to every parameter of every input. The **scope** system allows fine-grained filtering: which parameters are recalled, and for which channels.

The scope window provides parameter-level, per-channel granularity:

- For each parameter (position, attenuation, LFO active, etc.), which channels are included in the snapshot.
- For each channel, which parameters are included.

Use cases:

- **Position-only snapshots** — recall positions but leave attenuations and other settings as the operator has them currently. Useful for blocking changes that shouldn't disrupt the live mix.
- **Per-character snapshots** — a snapshot affecting only the channels of specific actors, leaving ambient or playback channels untouched.
- **Layered scenes** — a base snapshot for overall scene state, plus delta snapshots that adjust specific elements without resetting everything.

Scope is stored per-snapshot, so different snapshots can affect different parameter sets.

## Locking

The **Lock** button disables saving and recalling from the interface, while still allowing the system to operate. This is typically engaged once a show is set and rehearsed, to prevent accidental overwrites or unintended snapshot loads during a run.

OSC-driven operations may or may not respect the lock, depending on how the show is set up — generally, scripted recalls from show control should still work even when the UI is locked, but operator-initiated changes should not.

## Sample and impulse response files

In addition to the parameter XML files, the project folder contains:

- **Audio samples** — for the sampler feature, accessible via touch surfaces (Roli Lightpad, Android multi-touch).
- **Impulse responses** — for the convolution reverb engine.

These are stored in sub-directories of the project folder so they travel with the project.

## Naming and organization

Some practical conventions that have proven useful:

- **Project folder name** matches the show name and includes the venue and date (e.g., `MyShow_AvignonPalais_2026-07`).
- **Snapshot names** describe the cue or scene rather than just numbering them (`act1_opening`, `act2_storm_climax`, `bows`).
- **Backup snapshots** before major rehearsal changes (e.g., `working_2026-04-15_v3` before trying a new approach).
- **Configuration files** are XML and human-readable — can be edited manually if needed, version-controlled, diffed, and merged.

## Loading previous sessions

When opening WFS-DIY, the operator selects the project folder. The system loads all section files and the most recent state. From there, snapshots can be recalled to jump to specific cue states.

A common workflow:

1. Open the project folder.
2. Verify the audio interface is connected and the channel counts match.
3. Start the DSP.
4. Recall the rehearsal start snapshot.
5. Step through cues, using OSC from the show control system or manual snapshot recall.

## Session vs. show vs. configuration

Distinguish three concepts that get conflated:

- **Configuration** — what the system *is*: how many channels, where the speakers are, what reverb setup. Changes rarely once the venue is set up.
- **Show state** — what's happening *right now*: source positions, levels, mute states, LFOs. Changes constantly during a performance.
- **Snapshots** — saved show states that can be returned to. The library that maps show structure to system state.

A session encompasses all three. The configuration is shared across performances of the same show in the same venue; the snapshots define the show's structure; the show state is what's actively rendered at any moment.
