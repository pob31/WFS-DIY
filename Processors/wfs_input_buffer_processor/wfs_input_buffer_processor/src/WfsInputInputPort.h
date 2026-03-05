/*
 * WFS Input Buffer Processor - Input port
 */

#ifndef WFS_INPUT_WFS_INPUT_INPUT_PORT_H
#define WFS_INPUT_WFS_INPUT_INPUT_PORT_H

#include <processor_api/InputPort.h>
#include <processor_api/OutputPort.h>

class WfsInputInputPort : public GPUA::processor::v2::InputPort {
public:
    explicit WfsInputInputPort(GPUA::processor::v2::OutputPort* output_port, uint32_t num_outputs);
    ~WfsInputInputPort() = default;

    WfsInputInputPort& operator=(WfsInputInputPort&&) = delete;

    GPUA::processor::v2::PortId GetPortId() noexcept override;

    GPUA::processor::v2::ErrorCode Connect(const GPUA::processor::v2::OutputPort& data_port) noexcept override;
    GPUA::processor::v2::ErrorCode Disconnect() noexcept override;
    GPUA::processor::v2::ErrorCode InputPortUpdated(GPUA::processor::v2::PortChangedFlags flags, const GPUA::processor::v2::OutputPort& data_port) noexcept override;

    uint32_t GetInputGrain() const noexcept override;

    GPUA::processor::v2::ErrorCode GetPortDescription(const GPUA::processor::v2::PortDescription*& description) const noexcept override;

    uint32_t m_channel_count {};
    uint32_t m_current_buffer_size {};
    uint32_t m_max_buffer_size {};

    bool m_changed {false};

private:
    GPUA::processor::v2::OutputPort* m_output_port;
    uint32_t m_num_outputs {0};
};

#endif // WFS_INPUT_WFS_INPUT_INPUT_PORT_H
