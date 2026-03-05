/*
 * WFS Input Buffer Processor - Device code provider implementation
 */

#include "WfsInputDeviceCodeProvider.h"

#include "cmrc/cmrc.hpp"

#include <codecvt>
#include <iostream>
#include <locale>
#include <string>

CMRC_DECLARE(BG::wfs_input_buffer_processor);

namespace {

#if WIN32
static const std::string g_code_prefix = "";
#else
static const std::string g_code_prefix = "lib";
#endif

static const std::string g_code_filename = g_code_prefix + "wfs_input_buffer_processor.";
#if defined(GPU_AUDIO_NV)
static const std::string g_code_file_ext = ".cubin";
#elif defined(GPU_AUDIO_AMD)
static const std::string g_code_file_ext = ".o";
#elif defined(GPU_AUDIO_MAC)
static const std::string g_code_file_ext = ".metallib";
#endif

} // namespace

WfsInputDeviceCodeProvider::WfsInputDeviceCodeProvider(const GPUA::processor::v2::DeviceCodeSpecification& specification) :
    m_platform {specification.platform} {
}

GPUA::processor::v2::ErrorCode WfsInputDeviceCodeProvider::GetDeviceCode(GPUA::processor::v2::InputStream*& input_stream) noexcept {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string platform_str = converter.to_bytes(m_platform);

    std::string filename {g_code_filename + platform_str + g_code_file_ext};

    try {
        auto fs = cmrc::BG::wfs_input_buffer_processor::get_filesystem();
        if (fs.exists(filename) && fs.is_file(filename)) {
            auto file = fs.open(filename);
            const auto size = std::distance(file.begin(), file.end());
            m_stream = std::make_unique<StreamAdapter>(file.begin(), size);
            input_stream = m_stream.get();
            return GPUA::processor::v2::ErrorCode::eSuccess;
        }
    }
    catch (const std::exception& exc) {
        std::cout << exc.what() << std::endl;
    }

    input_stream = nullptr;
    return GPUA::processor::v2::ErrorCode::eFail;
}
