#include "ResidentValidator.h"
#include <algorithm>
#include <cctype>

namespace csms
{

std::vector<std::string> ResidentValidator::validate(const Resident& resident) const
{
    std::vector<std::string> errors;

    if (isBlank(resident.getFirstName()))
    {
        errors.push_back("firstName");
    }

    if (isBlank(resident.getLastName()))
    {
        errors.push_back("lastName");
    }

    if (isBlank(resident.getAddress()))
    {
        errors.push_back("address");
    }

    if (!isValidContactNumber(resident.getContactNumber()))
    {
        errors.push_back("contactNumber");
    }

    if (!isValidEmail(resident.getEmail()))
    {
        errors.push_back("email");
    }

    if (!isSupportedStatus(resident.getStatus()))
    {
        errors.push_back("status");
    }

    return errors;
}

bool ResidentValidator::isValid(const Resident& resident) const
{
    return validate(resident).empty();
}

bool ResidentValidator::isBlank(const std::string& value) const
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

bool ResidentValidator::isValidContactNumber(const std::string& value) const
{
    if (value.size() != 11)
    {
        return false;
    }

    if (value[0] != '0' || value[1] != '9')
    {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isdigit(character) != 0;
        }
    );
}

bool ResidentValidator::isValidEmail(const std::string& value) const
{
    if (value.empty())
    {
        return false;
    }

    const bool containsWhitespace = std::any_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isspace(character) != 0;
        }
    );

    if (containsWhitespace)
    {
        return false;
    }

    const std::size_t atPosition = value.find('@');

    if (atPosition == std::string::npos
        || atPosition == 0
        || atPosition == value.size() - 1)
    {
        return false;
    }

    if (atPosition != value.rfind('@'))
    {
        return false;
    }

    const std::string domain = value.substr(atPosition + 1);

    const std::size_t dotPosition = domain.find('.');

    if (dotPosition == std::string::npos
        || dotPosition == 0
        || dotPosition == domain.size() - 1)
    {
        return false;
    }

    return true;
}

bool ResidentValidator::isSupportedStatus(const std::string& value) const
{
    return value == "Active" || value == "Inactive";
}

} // namespace csms