/*
 * WFS Input Buffer Processor - Host-side processor implementation
 */

#include "WfsInputProcessor.h"

#include <processor_api/GpuTaskData.h>
#include <processor_api/PortChangedFlags.h>
#include <processor_api/ProcessorSpecification.h>
#include <processor_api/MemoryManager.h>

#include <algorithm>
#include <cstring>

using namespace GPUA::processor::v2;

#if defined(GPU_AUDIO_MAC)
static constexpr uint32_t g_max_threads_per_block {256u};
#else
static constexpr uint32_t g_max_threads_per_block {512u};
#endif

namespace {
template <typename T>
T divup(T a, T b) {
    return (a + b - 1) / b;
}
} // namespace

Module& WfsInputProcessor::GetModule() const noexcept {
    return m_module;
}

ErrorCode WfsInputProcessor::SetData(void* data, uint32_t data_size) noexcept {
    if (data == nullptr)
        return ErrorCode::eFail;

    // Minimum size: the RoutingMessage header
    if (data_size < sizeof(WfsInputConfig::RoutingMessage))
        return ErrorCode::eFail;

    const auto* msg = reinterpret_cast<const WfsInputConfig::RoutingMessage*>(data);
    if (msg->ThisMessage != WfsInputConfig::RoutingMessage::RoutingType)
        return ErrorCode::eFail;

    const uint32_t matrix_size = msg->numInputs * msg->numOutputs;
    const size_t expected_size = sizeof(WfsInputConfig::RoutingMessage) + matrix_size * sizeof(float) * 2;
    if (data_size < expected_size)
        return ErrorCode::eFail;

    // Extract delay and gain arrays from the routing message payload
    const float* payload = reinterpret_cast<const float*>(msg + 1);

    m_delays.resize(matrix_size);
    m_gains.resize(matrix_size);
    std::memcpy(m_delays.data(), payload, matrix_size * sizeof(float));
    std::memcpy(m_gains.data(), payload + matrix_size, matrix_size * sizeof(float));

    return ErrorCode::eSuccess;
}

ErrorCode WfsInputProcessor::GetData(void* data, uint32_t& data_size) const noexcept {
    return ErrorCode::eFail;
}

uint32_t WfsInputProcessor::GetInputPortCount() const noexcept {
    return 1u;
}

ErrorCode WfsInputProcessor::GetInputPort(uint32_t index, InputPort*& port) noexcept {
    if (index == 0) {
        port = m_input_port.get();
        return ErrorCode::eSuccess;
    }
    port = nullptr;
    return ErrorCode::eOutOfRange;
}

ErrorCode WfsInputProcessor::OnBlueprintRebuild(const ProcessorBlueprint*& blueprint) noexcept {
    if (m_changed || m_input_port->m_changed) {
        // One block per input->output routing pair
        m_gpu_task.block_count = m_num_inputs * m_num_outputs;
        // Threads per block: one per sample, multiples of 32, capped
        m_gpu_task.thread_count = std::min(g_max_threads_per_block, divup(m_input_port->m_max_buffer_size, 32u) * 32u);

        m_changed = m_input_port->m_changed = false;
    }
    blueprint = &m_proc_data;
    return ErrorCode::eSuccess;
}

ErrorCode WfsInputProcessor::PrepareForProcess(const LaunchData& data, uint32_t expected_chunks) noexcept {
    SetData(data.app_data, data.app_data_size);

    if (m_changed || m_input_port->m_changed)
        return ErrorCode::eBlueprintUpdateNeeded;

    return ErrorCode::eNoChangesNeeded;
}

ErrorCode WfsInputProcessor::PrepareChunk(void* proc_data, void** task_data, uint32_t chunk_id) noexcept {
    // The proc_data buffer is sized to hold ProcessorParameter + delay array + gain array.
    // We populate it so the GPU kernel can read everything from a single contiguous block.
    auto* params = reinterpret_cast<wfs_input::ProcessorParameter*>(proc_data);

    params->num_inputs = m_num_inputs;
    params->num_outputs = m_num_outputs;
    params->buffer_capacity = m_input_port->m_max_buffer_size;
    params->buffer_length = m_input_port->m_current_buffer_size;
    params->max_delay_samples = m_max_delay_samples;
    params->delay_buffer_offset = 0; // Reserved for Phase 2 circular delay buffer

    // Copy routing data after the parameter struct
    const uint32_t matrix_size = m_num_inputs * m_num_outputs;
    float* dest = reinterpret_cast<float*>(params + 1);

    if (m_delays.size() == matrix_size) {
        std::memcpy(dest, m_delays.data(), matrix_size * sizeof(float));
    } else {
        std::memset(dest, 0, matrix_size * sizeof(float));
    }

    dest += matrix_size;

    if (m_gains.size() == matrix_size) {
        std::memcpy(dest, m_gains.data(), matrix_size * sizeof(float));
    } else {
        std::memset(dest, 0, matrix_size * sizeof(float));
    }

    return ErrorCode::eSuccess;
}

void WfsInputProcessor::OnProcessingEnd(bool after_fat_transfer) noexcept {
}

ProcessorProfiler* WfsInputProcessor::GetProcessorProfiler() noexcept {
    return nullptr;
}

WfsInputProcessor::WfsInputProcessor(ProcessorSpecification& specification, Module& module) :
    m_module {module},
    m_proc_data {
        1u, // num_calls
        // proc_data_size: ProcessorParameter + 2 matrices of floats
        0u, // will be set below after we know matrix size
        ProcessorEndCallback::eNoCallback,
        1u, // task_count
        &m_gpu_task
    },
    m_port_factory {specification.port_factory},
    m_memory_manager {specification.memory_manager} {

    // Validate construction specification
    const auto* spec = reinterpret_cast<const WfsInputConfig::Specification*>(specification.user_data);
    if (specification.data_size != sizeof(WfsInputConfig::Specification) ||
        spec->ThisType != WfsInputConfig::Specification::ConstructionType) {
        throw std::runtime_error("WfsInputProcessor: invalid specification provided");
    }

    m_num_inputs = spec->numInputs;
    m_num_outputs = spec->numOutputs;
    m_max_delay_samples = spec->maxDelaySamples;

    const uint32_t matrix_size = m_num_inputs * m_num_outputs;
    m_delays.resize(matrix_size, 0.0f);
    m_gains.resize(matrix_size, 0.0f);

    // Set proc_data_size: struct + delays + gains
    m_proc_data.proc_data_size = static_cast<uint32_t>(
        sizeof(wfs_input::ProcessorParameter) + matrix_size * sizeof(float) * 2);

    // Create output port
    PortInfo output_port_info {};
    output_port_info.type = PortType::eRegularPort;
    output_port_info.data_type = PortDataType::eSample32;
    m_output_port = m_port_factory.CreateDataPort(0u, output_port_info);

    // Create input port
    m_input_port = std::make_unique<WfsInputInputPort>(m_output_port.get());

    // GPU task configuration
    m_gpu_task.entry_idx = 0u;
    m_gpu_task.shared_mem_size = 0u;
    m_gpu_task.task_param_size = 0u;
    m_gpu_task.processing_flags = ::ProcessingFlag::eProcessingFlagBlockForBlockAfterPreviousTask;
}
