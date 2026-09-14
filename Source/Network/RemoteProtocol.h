#pragma once

namespace WFSNetwork
{

// Version of the OSC remote-control protocol spoken with the Android "WFS Control"
// app. Bumped whenever the contract changes incompatibly (addresses, type tags,
// argument order). Exchanged in the handshake: /remote/ping carries ",ii"
// (sequence, version) and the tablet answers /remote/pong ",ii" (sequence,
// version), so both sides can surface a mismatch instead of silently dropping
// unrecognized messages.
//
// History:
//   1 — implicit version of the original protocol (version-less ",i" ping/pong)
//   2 — versioned ping/pong, /remote/dumpBegin start-of-dump marker, dump
//       sequence number appended to /remote/stateComplete
//   3 — /remote/vis/* visualisation mirroring (config, outputArrays, selection,
//       delays/levels rows) and tablet-side /remote/vis/pin
//   4 — /remote/channelList: the live channel numbers in display order, each
//       paired with its mono/stereo flag. It replaces the channel count as the
//       tablet's enumeration source, because a permanent channel number is no
//       longer an index: deletes leave gaps and a drag-reorder puts the numbers
//       out of ascending order, so enumerating 1..count both demands channels
//       that do not exist and hides ones that do. Also /remoteInput/stereoWidth
//       and /remoteInput/stereoAxisOffset.
//   Still 4 — additive, no bump: an older peer drops an unknown address at its
//       catch-all with no side effect, the reasoning /remote/channelList
//       shipped under.
//       - /remoteInput/inputColour (the stored 24-bit RGB, -1 = derived hue)
//         and /remoteInput/stereoAxisLock.
//       - tablet -> desktop /remote/vis/request [",i" pin, optional]: the
//         desktop answers that tablet only with config + outputArrays +
//         selection + rows (and its pinned rows), without a dump or a
//         selection change. The int restates the tablet's pin (0 = none);
//         a tablet's requests less than 250 ms apart are dropped.
//       - a full state dump (requestResync, config reload) is followed by the
//         same vis state; the dump itself carries only the vis config.
//       - the vis state (config + outputArrays + selection + rows + pinned
//         rows) is repeated to every tablet once 2 s pass with nothing sent,
//         so it no longer goes out only on change.
//       - /remote/vis/delays and /remote/vis/levels travel as two datagrams
//         (one-element bundles, <= 860 B at 128 outputs + 32 reverbs) instead
//         of one bundle, which was IP-fragmented on large rigs. Receivers
//         already took the two independently.
//       - /remoteInput/stereoWidth, stereoAxisOffset, stereoAxisLock and
//         inputColour echo a desktop edit for EVERY channel, not only the
//         tablet's selected one, always as numbers typed by the parameter
//         (",if" width, ",ii" the other three), even after a load left the
//         stored value a string.
//       - the selected channel's echoes never carry a number as ",is": a value
//         a load, a snapshot recall or an undo left as text goes out typed by
//         the parameter (",ii" / ",if"). Only unbounded text (inputName, the
//         mute list) is still ",is".
//       - /remoteInput/arrayAtten1..10 ",if" (channel, dB -60..0): an input's
//         level to each output array, both ways - set and inc/dec from the
//         tablet, and in the full dump, the per-channel resync, the
//         selected-channel dump and the selected channel's echo.
//       - array mute (session state, never saved): tablet -> desktop
//         /arrayAdjust/mute ",ii" (array 1-10, 0/1), an absolute state beside
//         the /arrayAdjust/ deltas; desktop -> tablet /remote/array/mute
//         (count, then 0/1 per array) in the full dump, to every tablet after
//         any change (the sender included: it is the confirmation) and every
//         2 s. An older desktop counts /arrayAdjust/mute as a parse error.
constexpr int kRemoteProtocolVersion = 4;

} // namespace WFSNetwork
