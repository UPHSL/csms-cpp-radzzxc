#pragma once

#include <string>
#include <vector>
#include "ServiceRequest.h"

namespace csms
{

class ServiceRequestValidator
{
public:
    std::vector<std::string> validate(const ServiceRequest& request) const;
    bool isValid(const ServiceRequest& request) const;

private:
    bool isBlank(const std::string& value) const;
    bool isValidDate(const std::string& value) const;
};

} // namespace csms
