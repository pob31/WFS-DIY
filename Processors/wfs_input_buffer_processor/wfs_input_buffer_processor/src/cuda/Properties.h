/*
 * WFS Input Buffer Processor - GPU parameter structures
 */

#ifndef WFS_INPUT_PROPERTIES_H
#define WFS_INPUT_PROPERTIES_H

// The entries in GPUFUNCTIONS_SCRAMBLED are used to replace the processor device function names
// during compilation to avoid name conflicts between processors.
// clang-format off
#define GPUFUNCTIONS_SCRAMBLED \
qW7kVfR3nZx8mT2yBpLs, \
hJ4dNcQ9wF6gXeA1rKuP, \
tY5bMjS0vH8lUiO3sDfG, \
zC6aEkT1wI9nRoP4uBvX, \
mQ8fLjY2xK7dHgS5aWcN, \
pR3bUkZ6yM0eVtF8iGwJ, \
sD4cXlQ7zN1fWgT9oHaK, \
uE5dYmR8aO2gXhU0pIbL, \
vF6eZnS9bP3hYiV1qJcM, \
wG7fAoT0cQ4iZjW2rKdN
// clang-format on

#if !defined(GPU_AUDIO_MAC)
#include <cstdint>
#endif

// DO NOT REMOVE! Contains macros for device function name substitution.
#include <scheduler/common_macros.h>

namespace wfs_input {

// Parameter struct passed to the GPU task each launch (set in WfsInputProcessor::PrepareChunk).
struct ProcessorParameter {
    uint32_t num_inputs;           // Number of input channels
    uint32_t num_outputs;          // Number of output channels
    uint32_t buffer_capacity;      // Max samples per channel the buffer can hold
    uint32_t buffer_length;        // Current samples per channel in this block
    uint32_t max_delay_samples;    // Maximum delay line length in samples
    uint32_t delay_buffer_offset;  // Write position in circular delay buffer
    // Followed in GPU memory by:
    // - float delays[num_inputs * num_outputs]  (delay in samples, input-major)
    // - float gains[num_inputs * num_outputs]   (linear gain, input-major)
};

// Per-task parameter struct (unused - single task processor).
using TaskParameter = void;

} // namespace wfs_input

#endif // WFS_INPUT_PROPERTIES_H
