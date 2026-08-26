#pragma once

#include <string>
#include <vector>
#include "Resident.h"

namespace csms
{

class ResidentValidator
{
public:
    std::vector<std::string> validate(const Resident& resident) const;
    bool isValid(const Resident& resident) const;

private:
    bool isBlank(const std::string& value) const;
    bool isValidContactNumber(const std::string& value) const;
    bool isValidEmail(const std::string& value) const;
    bool isSupportedStatus(const std::string& value) const;
};

} // namespace csms
