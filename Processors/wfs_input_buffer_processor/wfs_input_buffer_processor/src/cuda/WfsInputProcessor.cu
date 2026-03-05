/*
 * WFS Input Buffer Processor - CUDA kernel declaration
 */

#include "WfsInputProcessor.cuh"

#include <scheduler/device/processor.cuh>

// DeclareProcessorStep: register the 'process' method as task 0
// Parameters: ProcessorClass, task_index, method_name, sample_type, ProcessorParam, TaskParam
DeclareProcessorStep(WfsInputProcessorDevice<float>, 0, process, float, wfs_input::ProcessorParameter, wfs_input::TaskParameter);

// DeclareProcessor: finalize with total task count
DeclareProcessor(WfsInputProcessorDevice<float>, 1);
