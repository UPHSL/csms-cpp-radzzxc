#pragma once

#include <string>
#include <vector>
#include <optional>
#include "models/Resident.h"

namespace csms
{

class ResidentView
{
public:
    // Form state structure for preserving input and presenting validation feedback
    struct FormState
    {
        std::string firstName;
        std::string lastName;
        std::string address;
        std::string contactNumber;
        std::string email;
        std::vector<std::string> errors;
    };

    // Renders the main Resident Registry page (/residents) with list, search, and flash messages
    static std::string renderIndex(
        const std::vector<Resident>& residents,
        const std::string& searchTerm = "",
        const std::string& successMessage = ""
    );

    // Renders the New Resident Registration form (/residents/register)
    static std::string renderRegisterForm(const FormState& state = {});

    // Renders the Edit Resident form (/residents/{id}/edit)
    static std::string renderEditForm(const Resident& resident, const FormState& state = {});

    // Renders a safe, controlled 404 Not Found response
    static std::string renderNotFound(const std::string& message = "The requested resident record could not be found.");

private:
    // Shared HTML shell applying the Nordic Slate & Teal design system
    static std::string renderLayout(
        const std::string& pageTitle,
        const std::string& activeNav,
        const std::string& breadcrumb,
        const std::string& content
    );

    // Helper: Escapes HTML special characters to prevent injection
    static std::string escapeHtml(const std::string& input);
};

} // namespace csms