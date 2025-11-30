/*
 * WFS Input Buffer Processor Specification
 * 
 * This file defines the data structures used for processor construction
 * and runtime routing updates.
 */

#ifndef WFS_INPUT_WFS_INPUT_SPECIFICATION_H
#define WFS_INPUT_WFS_INPUT_SPECIFICATION_H

#include <cstdint>
#include <stddef.h>

namespace WfsInputConfig
{
    /**
     * Construction-time specification for the WFS Input Buffer processor.
     * Passed when creating a processor instance.
     */
    struct Specification
    {
        static constexpr uint32_t ConstructionType = 0x57534649; // 'WFSI'
        uint32_t ThisType {ConstructionType};

        uint32_t numInputs {0};
        uint32_t numOutputs {0};
        uint32_t maxSamplesPerChannel {0};
        uint32_t maxDelaySamples {0};
    };

    /**
     * Runtime routing message sent each processing block.
     * Contains delay samples and gain values for each input-output pair.
     * 
     * Memory layout after the header:
     * - numInputs * numOutputs floats: delay samples (input-major order)
     * - numInputs * numOutputs floats: gain values (input-major order)
     */
    struct RoutingMessage
    {
        static constexpr uint32_t RoutingType = 0x57534652; // 'WFSR'
        uint32_t ThisMessage {RoutingType};
        uint32_t numInputs {0};
        uint32_t numOutputs {0};
        // Followed in memory by `numInputs * numOutputs` floats of delay (samples),
        // then the same number of floats for gains.
    };
} // namespace WfsInputConfig

#endif // WFS_INPUT_WFS_INPUT_SPECIFICATION_H

