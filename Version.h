#pragma once

#include <string_view>

namespace csms
{

// Single authoritative source of truth for CSMS release identity and version metadata.
struct Version
{
    static constexpr std::string_view kApplication = "Community Services Management System";
    static constexpr std::string_view kShortName = "CSMS";
    static constexpr std::string_view kVersion = "1.0.0";
    static constexpr std::string_view kDisplayVersion = "v1.0.0";
    static constexpr std::string_view kStatus = "ok";
};

} // namespace csms