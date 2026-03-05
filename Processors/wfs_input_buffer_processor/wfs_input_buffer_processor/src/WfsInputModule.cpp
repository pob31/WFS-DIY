/*
 * WFS Input Buffer Processor - Module factory implementation
 */

#include "WfsInputModule.h"
#include "WfsInputProcessor.h"

WfsInputModule::WfsInputModule(const GPUA::processor::v2::ModuleSpecification& specification) :
    GPUA::processor::v2::ModuleBase(specification) {}

GPUA::processor::v2::ErrorCode WfsInputModule::CreateProcessor(GPUA::processor::v2::ProcessorSpecification& specification, GPUA::processor::v2::Processor*& processor) noexcept {
    try {
        processor = new WfsInputProcessor(specification, *this);
        return GPUA::processor::v2::ErrorCode::eSuccess;
    }
    catch (...) {
    }
    processor = nullptr;
    return GPUA::processor::v2::ErrorCode::eFail;
}

GPUA::processor::v2::ErrorCode WfsInputModule::DeleteProcessor(GPUA::processor::v2::Processor* processor) noexcept {
    if (processor) {
        delete processor;
        return GPUA::processor::v2::ErrorCode::eSuccess;
    }
    return GPUA::processor::v2::ErrorCode::eFail;
}
