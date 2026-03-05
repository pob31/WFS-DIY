/*
 * WFS Input Buffer Processor - Input port implementation
 */

#include "WfsInputInputPort.h"

#include <processor_api/PortDescription.h>

WfsInputInputPort::WfsInputInputPort(GPUA::processor::v2::OutputPort* output_port, uint32_t num_outputs) :
    m_output_port {output_port}, m_num_outputs {num_outputs} {
}

GPUA::processor::v2::PortId WfsInputInputPort::GetPortId() noexcept {
    return m_output_port->GetPortId();
}

GPUA::processor::v2::ErrorCode WfsInputInputPort::Connect(const GPUA::processor::v2::OutputPort& data_port) noexcept {
    using namespace GPUA::processor::v2;

    auto& input_port = data_port.GetPortInfo();

    if (input_port.type != PortType::eRegularPort ||
        input_port.data_type != PortDataType::eSample32) {
        return ErrorCode::eUnsupported;
    }

    m_max_buffer_size = input_port.capacity_in_bytes / sizeof(float);
    m_current_buffer_size = input_port.size_in_bytes / sizeof(float);
    m_channel_count = input_port.channel_count;

    auto& output_port = m_output_port->GetPortInfo();
    output_port = input_port;
    output_port.channel_count = m_num_outputs;
    output_port.grain = input_port.capacity_in_bytes;
    output_port.transfer_to_cpu = false;
    output_port.is_produced = true;

    m_output_port->Changed(PortChangedFlags::eReset);
    m_changed = true;

    return ErrorCode::eSuccess;
}

GPUA::processor::v2::ErrorCode WfsInputInputPort::Disconnect() noexcept {
    using namespace GPUA::processor::v2;

    m_max_buffer_size = m_current_buffer_size = 0;
    m_channel_count = 0;

    auto& output_port = m_output_port->GetPortInfo();
    output_port.capacity_in_bytes = 0;
    output_port.size_in_bytes = 0;
    output_port.channel_count = 0;
    output_port.grain = 0;
    output_port.offset = 0;
    output_port.oversampling_ratio = 0;
    output_port.is_produced = false;
    output_port.transfer_to_cpu = false;

    m_output_port->Changed(PortChangedFlags::eReset);

    return ErrorCode::eSuccess;
}

GPUA::processor::v2::ErrorCode WfsInputInputPort::InputPortUpdated(GPUA::processor::v2::PortChangedFlags flags, const GPUA::processor::v2::OutputPort& data_port) noexcept {
    using namespace GPUA::processor::v2;

    auto& input_port = data_port.GetPortInfo();
    if (input_port.type != PortType::eRegularPort ||
        input_port.data_type != PortDataType::eSample32) {
        Disconnect();
        return ErrorCode::eUnsupported;
    }

    PortChangedFlags new_flags = flags & (~(PortChangedFlags::eGrainChanged | PortChangedFlags::eTransferToCpuChanged | PortChangedFlags::eProduceInfoChanged));
    if (static_cast<uint32_t>(new_flags) == 0) {
        return ErrorCode::eSuccess;
    }

    constexpr PortChangedFlags need_rebuild_flags = PortChangedFlags::eCapacityChanged | PortChangedFlags::eChannelCountChanged;

    if (static_cast<uint32_t>(flags & need_rebuild_flags) != 0) {
        m_changed = true;
    }

    auto& output_port = m_output_port->GetPortInfo();
    output_port = input_port;
    output_port.channel_count = m_num_outputs;
    output_port.grain = input_port.capacity_in_bytes;
    output_port.transfer_to_cpu = false;
    output_port.is_produced = true;

    m_max_buffer_size = input_port.capacity_in_bytes / sizeof(float);
    m_current_buffer_size = input_port.size_in_bytes / sizeof(float);
    m_channel_count = input_port.channel_count;
    if (flags == PortChangedFlags::eReset || flags == PortChangedFlags::eTypeChanged) {
        m_output_port->Changed(PortChangedFlags::eReset);
    } else {
        m_output_port->Changed(new_flags);
    }
    return ErrorCode::eSuccess;
}

uint32_t WfsInputInputPort::GetInputGrain() const noexcept {
    return m_max_buffer_size * sizeof(float);
}

GPUA::processor::v2::ErrorCode WfsInputInputPort::GetPortDescription(const GPUA::processor::v2::PortDescription*& description) const noexcept {
    static GPUA::processor::v2::PortDescription desc {L"WFS Input Port", L"Audio input for WFS delay-sum routing"};
    description = &desc;
    return GPUA::processor::v2::ErrorCode::eSuccess;
}
