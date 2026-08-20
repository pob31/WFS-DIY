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
//       that do not exist and hides ones that do.
constexpr int kRemoteProtocolVersion = 4;

} // namespace WFSNetwork
