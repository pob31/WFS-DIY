/*
 * WFS Input Buffer Processor - Host-side processor
 */

#ifndef WFS_INPUT_WFS_INPUT_PROCESSOR_H
#define WFS_INPUT_WFS_INPUT_PROCESSOR_H

#include "WfsInputInputPort.h"
#include "Properties.h"

#include <wfs_input_buffer_processor/WfsInputSpecification.h>

#include <processor_api/GpuTaskData.h>
#include <processor_api/MemoryManager.h>
#include <processor_api/ModuleBase.h>
#include <processor_api/LaunchData.h>
#include <processor_api/Processor.h>
#include <processor_api/ProcessorBlueprint.h>
#include <processor_api/ProcessorProfiler.h>
#include <processor_api/PortFactory.h>

#include <cstdint>
#include <vector>

class WfsInputProcessor : public GPUA::processor::v2::Processor {
public:
    explicit WfsInputProcessor(GPUA::processor::v2::ProcessorSpecification& specification, GPUA::processor::v2::Module& module);
    ~WfsInputProcessor() = default;

    WfsInputProcessor& operator=(WfsInputProcessor&&) = delete;

    GPUA::processor::v2::Module& GetModule() const noexcept override;

    GPUA::processor::v2::ErrorCode SetData(void* data, uint32_t data_size) noexcept override;
    GPUA::processor::v2::ErrorCode GetData(void* data, uint32_t& data_size) const noexcept override;

    uint32_t GetInputPortCount() const noexcept override;
    GPUA::processor::v2::ErrorCode GetInputPort(uint32_t index, GPUA::processor::v2::InputPort*& port) noexcept override;

    GPUA::processor::v2::ErrorCode OnBlueprintRebuild(const GPUA::processor::v2::ProcessorBlueprint*& blueprint) noexcept override;
    GPUA::processor::v2::ErrorCode PrepareForProcess(const GPUA::processor::v2::LaunchData& data, uint32_t expected_chunks) noexcept override;
    GPUA::processor::v2::ErrorCode PrepareChunk(void* proc_data, void** task_data, uint32_t chunk_id) noexcept override;
    void OnProcessingEnd(bool after_fat_transfer) noexcept override;

    GPUA::processor::v2::ProcessorProfiler* GetProcessorProfiler() noexcept override;

private:
    GPUA::processor::v2::Module& m_module;
    GPUA::processor::v2::PortFactory& m_port_factory;
    GPUA::processor::v2::MemoryManager& m_memory_manager;

    GPUA::processor::v2::GpuTaskData m_gpu_task;
    GPUA::processor::v2::ProcessorBlueprint m_proc_data;

    std::unique_ptr<WfsInputInputPort> m_input_port;
    GPUA::processor::v2::OutputPortPointer m_output_port {0, 0};

    // Configuration from construction specification
    uint32_t m_num_inputs {0};
    uint32_t m_num_outputs {0};
    uint32_t m_max_delay_samples {0};

    // Runtime routing data received via SetData each block
    std::vector<float> m_delays;   // [num_inputs * num_outputs] delay in samples
    std::vector<float> m_gains;    // [num_inputs * num_outputs] linear gain

    bool m_changed {true};
};

#endif // WFS_INPUT_WFS_INPUT_PROCESSOR_H
