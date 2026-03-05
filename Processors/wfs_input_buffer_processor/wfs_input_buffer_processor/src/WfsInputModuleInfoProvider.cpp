/*
 * WFS Input Buffer Processor - Module metadata provider implementation
 */

#include "WfsInputModuleInfoProvider.h"

#include "cuda/Properties.h"

#include <processor_api/DeviceCodeSpecification.h>
#include <processor_api/PlatformInfo.h>

#include <algorithm>
#include <array>
#include <codecvt>
#include <locale>
#include <regex>

#define Q(x) #x
#define QQ(x) Q(x)
#define QUOTE(x) QQ(x)

#define QUOTEW(x) (L"" QUOTE(x))

namespace {

std::vector<GPUA::processor::v2::PlatformInfo> ConstructSpecification(const std::vector<std::wstring>& archs_list) {
    std::vector<GPUA::processor::v2::PlatformInfo> result;
    std::transform(archs_list.begin(), archs_list.end(), std::back_inserter(result), [](auto& str) {
        return GPUA::processor::v2::PlatformInfo {str.c_str()};
    });
    return result;
}

std::vector<std::string> Split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    const std::regex regex(delimiter);
    std::copy(std::sregex_token_iterator(str.begin(), str.end(), regex, -1), std::sregex_token_iterator(), std::back_inserter(result));
    return result;
}

std::vector<std::wstring> GetSupportedArchs() {
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

#if !defined(GPU_AUDIO_MAC)
#if defined(GPU_AUDIO_NV)
    std::vector<std::string> archs = Split(CUDA_ARCHS, ":");
    std::vector<std::wstring> result;
    std::transform(archs.begin(), archs.end(), std::back_inserter(result), [&](auto str) { return converter.from_bytes("sm_" + str); });
    return result;
#elif defined(GPU_AUDIO_AMD)
    std::vector<std::string> archs = Split(HIP_ARCHS, ":");
    std::vector<std::wstring> result;
    std::transform(archs.begin(), archs.end(), std::back_inserter(result), [&](auto str) { return converter.from_bytes(str); });
    return result;
#endif
#else
    std::vector<std::string> archs = Split("arm64", ":");
    std::vector<std::wstring> result;
    std::transform(archs.begin(), archs.end(), std::back_inserter(result), [&](auto str) { return converter.from_bytes(str); });
    return result;
#endif
}
} // namespace

WfsInputModuleInfoProvider::WfsInputModuleInfoProvider() {
}

const std::vector<GPUA::processor::v2::PlatformInfo>& WfsInputModuleInfoProvider::GetCodeSpecs() {
    static std::vector<std::wstring> supported_archs = GetSupportedArchs();
    static std::vector<GPUA::processor::v2::PlatformInfo> supported_specs = ConstructSpecification(supported_archs);
    return supported_specs;
}

uint32_t WfsInputModuleInfoProvider::GetSupportPlatformCount() const noexcept {
    return static_cast<uint32_t>(GetCodeSpecs().size());
}

GPUA::processor::v2::ErrorCode WfsInputModuleInfoProvider::GetSupportPlatformInfo(uint32_t index, const GPUA::processor::v2::PlatformInfo*& platform_info) const noexcept {
    auto& specs = GetCodeSpecs();
    if (index < specs.size()) {
        platform_info = &specs[index];
        return GPUA::processor::v2::ErrorCode::eSuccess;
    }
    platform_info = nullptr;
    return GPUA::processor::v2::ErrorCode::eOutOfRange;
}

GPUA::processor::v2::ErrorCode WfsInputModuleInfoProvider::GetModuleInfo(const GPUA::processor::v2::ModuleInfo*& module_info) const noexcept {
    module_info = &ModuleInfo;
    return GPUA::processor::v2::ErrorCode::eSuccess;
}

GPUA::processor::v2::ErrorCode WfsInputModuleInfoProvider::GetProcessorExecutionInfo(const GPUA::processor::v2::ProcessorEntryInfo*& entry_info) const noexcept {
    // Three mandatory processor device functions. DO NOT MODIFY NEXT THREE LINES!
    static std::wstring declare_processor = std::wstring(QUOTEW(SEL(0)));
    static std::wstring init_processor = std::wstring(QUOTEW(SEL(1)));
    static std::wstring destroy_processor = std::wstring(QUOTEW(SEL(2)));

    // Single GPU task (see WfsInputProcessor.cu)
    static constexpr uint32_t task_cnt = 1;

    // Task function names from GPUFUNCTIONS_SCRAMBLED via SEL macro
    static std::array<std::wstring, 2 * task_cnt> task_names = {
        QUOTEW(SEL(3)),
        QUOTEW(SEL(4))};

    static std::array<const wchar_t*, 2 * task_cnt> task_names_p = {
        task_names[0].c_str(),
        task_names[1].c_str()};

    static GPUA::processor::v2::ProcessorEntryInfo info {
        declare_processor.c_str(),
        destroy_processor.c_str(),
        init_processor.c_str(),
        task_cnt, task_names_p.data()};

    entry_info = &info;
    return GPUA::processor::v2::ErrorCode::eSuccess;
}
