#include <drogon/drogon_test.h>
#include "models/Resident.h"
#include "models/ResidentValidator.h"
#include <algorithm>
#include <string>
#include <vector>

static bool containsValidationError(const std::vector<std::string>& errors, const std::string& field)
{
    return std::find(errors.begin(), errors.end(), field) != errors.end();
}

// Test 1: Valid Resident Information
DROGON_TEST(ValidResidentInformationPassesValidation)
{
    csms::Resident resident(
        "Juan",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );
    csms::ResidentValidator validator;
    CHECK(validator.isValid(resident) == true);
}

// Test 2: Missing First Name
DROGON_TEST(MissingFirstNameFailsValidation)
{
    csms::Resident resident(
        "",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );
    csms::ResidentValidator validator;
    const auto errors = validator.validate(resident);
    CHECK(validator.isValid(resident) == false);
    CHECK(containsValidationError(errors, "firstName") == true);
}

// Test 3: Missing Last Name
DROGON_TEST(MissingLastNameFailsValidation)
{
    csms::Resident resident(
        "Juan",
        "",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );
    csms::ResidentValidator validator;
    const auto errors = validator.validate(resident);
    CHECK(validator.isValid(resident) == false);
    CHECK(containsValidationError(errors, "lastName") == true);
}

// Test 4: Missing Address
DROGON_TEST(MissingAddressFailsValidation)
{
    csms::Resident resident(
        "Juan",
        "Dela Cruz",
        "",
        "09171234567",
        "juan@example.com"
    );
    csms::ResidentValidator validator;
    const auto errors = validator.validate(resident);
    CHECK(validator.isValid(resident) == false);
    CHECK(containsValidationError(errors, "address") == true);
}

// Test 5: Whitespace-Only Required Information
DROGON_TEST(WhitespaceOnlyRequiredInformationFailsValidation)
{
    csms::Resident resident(
        "   ",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );
    csms::ResidentValidator validator;
    const auto errors = validator.validate(resident);
    CHECK(validator.isValid(resident) == false);
    CHECK(containsValidationError(errors, "firstName") == true);
}

// Test 6: Invalid Contact Number
DROGON_TEST(InvalidContactNumberFailsValidation)
{
    csms::Resident resident(
        "Juan",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "0917ABC4567",
        "juan@example.com"
    );
    csms::ResidentValidator validator;
    const auto errors = validator.validate(resident);
    CHECK(validator.isValid(resident) == false);
    CHECK(containsValidationError(errors, "contactNumber") == true);
}

// Test 7: Invalid Email
DROGON_TEST(InvalidEmailFailsValidation)
{
    csms::Resident resident(
        "Juan",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan.example.com"
    );
    csms::ResidentValidator validator;
    const auto errors = validator.validate(resident);
    CHECK(validator.isValid(resident) == false);
    CHECK(containsValidationError(errors, "email") == true);
}

// Test 8: Supported Resident Status
DROGON_TEST(SupportedResidentStatusesPassValidation)
{
    csms::Resident activeResident(
        "Juan",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com"
    );
    csms::Resident inactiveResident(
        "Maria",
        "Santos",
        "Barangay Santo Tomas",
        "09181234567",
        "maria@example.com",
        "Inactive"
    );
    csms::ResidentValidator validator;
    CHECK(validator.isValid(activeResident) == true);
    CHECK(validator.isValid(inactiveResident) == true);
}

// Test 9: Unsupported Resident Status
DROGON_TEST(UnsupportedResidentStatusFailsValidation)
{
    csms::Resident resident(
        "Juan",
        "Dela Cruz",
        "Barangay Santo Tomas",
        "09171234567",
        "juan@example.com",
        "Unknown"
    );
    csms::ResidentValidator validator;
    const auto errors = validator.validate(resident);
    CHECK(validator.isValid(resident) == false);
    CHECK(containsValidationError(errors, "status") == true);
}
