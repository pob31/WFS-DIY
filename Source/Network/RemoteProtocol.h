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
constexpr int kRemoteProtocolVersion = 3;

} // namespace WFSNetwork
