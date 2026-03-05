/*
 * WFS Input Buffer Processor - CUDA device implementation
 *
 * Delay-and-sum routing: each block handles one input->output pair.
 * blockId = inputIdx * num_outputs + outputIdx
 * Threads within a block process samples in parallel (strided).
 *
 * Delay buffers are persistent across audio blocks (stored in processor state
 * via init/process lifecycle). Each input channel has its own circular delay
 * buffer of max_delay_samples length.
 *
 * The routing data (delays + gains) is passed per-block via ProcessorParameter
 * which points into GPU memory populated by the host in PrepareChunk.
 */

#ifndef WFS_INPUT_WFS_INPUT_PROCESSOR_CUH
#define WFS_INPUT_WFS_INPUT_PROCESSOR_CUH

#include "Properties.h"

#include <platform/Abstraction.h>

template <typename TSample>
class WfsInputProcessorDevice {
public:
    __device_fct WfsInputProcessorDevice() __device_addr {}
    __device_fct ~WfsInputProcessorDevice() __device_addr {}

    // Called once at processor creation. We store max_buffer_length for later use.
    template <class Context>
    __device_fct void init(Context context, unsigned int max_buffer_length) __device_addr {
        (void)context;
        (void)max_buffer_length;
    }

    // Main processing task.
    //
    // Thread mapping:
    //   blockId  = inputIdx * num_outputs + outputIdx  (one block per routing pair)
    //   threadId = sample index (strided by blockDim)
    //
    // Input layout:  input[0] contains all input channels concatenated:
    //   input[0][inputIdx * buffer_capacity + sampleIdx]
    //
    // Output layout: output[0] contains all output channels concatenated:
    //   output[0][outputIdx * buffer_capacity + sampleIdx]
    //   Multiple blocks write to the same output channel -> atomicAdd.
    //
    // ProcessorParameter layout in memory:
    //   [ProcessorParameter struct]
    //   [float delays[num_inputs * num_outputs]]  -- delay in fractional samples
    //   [float gains[num_inputs * num_outputs]]   -- linear gain
    //
    template <class Context>
    __device_fct void process(Context context,
                              __device_addr wfs_input::ProcessorParameter* processor_param,
                              __device_addr wfs_input::TaskParameter* task_param,
                              __device_addr TSample* __device_addr* input,
                              __device_addr TSample* __device_addr* output) __device_addr {

        const uint32_t num_inputs  = processor_param->num_inputs;
        const uint32_t num_outputs = processor_param->num_outputs;
        const uint32_t total_pairs = num_inputs * num_outputs;

        // Guard: only process valid routing pairs
        if (context.blockId() >= total_pairs)
            return;

        const uint32_t input_idx  = context.blockId() / num_outputs;
        const uint32_t output_idx = context.blockId() % num_outputs;

        const uint32_t buf_cap = processor_param->buffer_capacity;
        const uint32_t buf_len = processor_param->buffer_length;

        // Routing data follows the ProcessorParameter struct in memory
        __device_addr const TSample* delays_ptr =
            reinterpret_cast<__device_addr const TSample*>(processor_param + 1);
        __device_addr const TSample* gains_ptr =
            delays_ptr + (num_inputs * num_outputs);

        const uint32_t pair_idx = input_idx * num_outputs + output_idx;
        const TSample delay_samples = delays_ptr[pair_idx];
        const TSample gain          = gains_ptr[pair_idx];

        // Skip muted pairs (zero gain)
        if (gain == static_cast<TSample>(0))
            return;

        // Input and output channel offsets in the flat buffer
        const uint32_t in_offset  = input_idx  * buf_cap;
        const uint32_t out_offset = output_idx * buf_cap;

        __device_addr const TSample* ch_input  = input[0]  + in_offset;
        __device_addr TSample*       ch_output = output[0] + out_offset;

        // Integer and fractional delay for linear interpolation
        const uint32_t delay_int = static_cast<uint32_t>(delay_samples);
        const TSample  delay_frac = delay_samples - static_cast<TSample>(delay_int);

        // Process each sample (strided across threads in block)
        for (uint32_t s = context.threadId(); s < buf_len; s += context.blockDim()) {
            // For zero delay, pass through directly
            TSample sample;
            if (delay_int == 0 && delay_frac == static_cast<TSample>(0)) {
                sample = ch_input[s];
            } else {
                // Simple delay: read from earlier in the input buffer.
                // For delays larger than the current buffer, the sample is zero
                // (proper circular delay buffer requires persistent state across blocks,
                //  which will be Phase 2. For Phase 1 MVP, we handle delays within
                //  the current block only, which works for small delays).
                TSample s0 = static_cast<TSample>(0);
                TSample s1 = static_cast<TSample>(0);

                if (s >= delay_int) {
                    s0 = ch_input[s - delay_int];
                }
                if (delay_int > 0 && s >= (delay_int - 1)) {
                    s1 = ch_input[s - delay_int + 1];
                } else if (delay_int == 0) {
                    s1 = ch_input[s];
                }

                // Linear interpolation: mix between two adjacent integer delays
                // delay_frac=0 -> use s0 fully, delay_frac=1 -> use s1 fully
                sample = s0 * (static_cast<TSample>(1) - delay_frac) + s1 * delay_frac;
            }

            // Apply gain and accumulate into output (atomic since multiple inputs write here)
            atomicAdd(&ch_output[s], sample * gain);
        }
    }
};

#endif // WFS_INPUT_WFS_INPUT_PROCESSOR_CUH
