/*
 * WFS Input Buffer Processor - Module factory
 */

#ifndef WFS_INPUT_WFS_INPUT_MODULE_H
#define WFS_INPUT_WFS_INPUT_MODULE_H

#include <processor_api/ModuleBase.h>
#include <processor_api/ModuleInfoProvider.h>
#include <processor_api/ModuleSpecification.h>

class WfsInputModule : public GPUA::processor::v2::ModuleBase {
public:
    explicit WfsInputModule(const GPUA::processor::v2::ModuleSpecification& specification);
    ~WfsInputModule() override = default;

    WfsInputModule& operator=(WfsInputModule&&) = delete;

    GPUA::processor::v2::ErrorCode CreateProcessor(GPUA::processor::v2::ProcessorSpecification& specification, GPUA::processor::v2::Processor*& processor) noexcept override;
    GPUA::processor::v2::ErrorCode DeleteProcessor(GPUA::processor::v2::Processor* processor) noexcept override;
};

#endif // WFS_INPUT_WFS_INPUT_MODULE_H
