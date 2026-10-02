#include "ServiceRequestValidator.h"
#include <algorithm>
#include <cctype>
#include <chrono>

namespace csms
{

std::vector<std::string> ServiceRequestValidator::validate(const ServiceRequest& request) const
{
    std::vector<std::string> errors;

    // Rule 1: ID must be unassigned before submission
    if (request.getId().has_value())
    {
        errors.push_back("id");
    }

    // Rule 2: residentId must be a positive integer identifier
    if (request.getResidentId() <= 0)
    {
        errors.push_back("residentId");
    }

    // Rule 3: serviceType is required and cannot be whitespace-only
    if (isBlank(request.getServiceType()))
    {
        errors.push_back("serviceType");
    }

    // Rule 4: description is required and cannot be whitespace-only
    if (isBlank(request.getDescription()))
    {
        errors.push_back("description");
    }

    // Rule 5: dateRequested is required and must follow YYYY-MM-DD
    if (!isValidDate(request.getDateRequested()))
    {
        errors.push_back("dateRequested");
    }

    // Rule 6: Initial status for a new service request must be "Pending"
    if (request.getStatus() != "Pending")
    {
        errors.push_back("status");
    }

    return errors;
}

bool ServiceRequestValidator::isValid(const ServiceRequest& request) const
{
    return validate(request).empty();
}

bool ServiceRequestValidator::isBlank(const std::string& value) const
{
    if (value.empty())
    {
        return true;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isspace(character) != 0;
        }
    );
}

bool ServiceRequestValidator::isValidDate(const std::string& value) const
{
    // Must be exactly YYYY-MM-DD (10 characters)
    if (value.size() != 10)
    {
        return false;
    }

    if (value[4] != '-' || value[7] != '-')
    {
        return false;
    }

    for (size_t i = 0; i < value.size(); ++i)
    {
        if (i == 4 || i == 7)
        {
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(value[i])))
        {
            return false;
        }
    }

    int year = std::stoi(value.substr(0, 4));
    int month = std::stoi(value.substr(5, 2));
    int day = std::stoi(value.substr(8, 2));

    if (year < 1900 || year > 2100)
    {
        return false;
    }
    std::chrono::year_month_day ymd{
        std::chrono::year{year},
        std::chrono::month{static_cast<unsigned>(month)},
        std::chrono::day{static_cast<unsigned>(day)}
    };
    return ymd.ok();
}

} // namespace csms
