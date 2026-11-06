#pragma once

#include <string>
#include <vector>
#include <optional>
#include "models/Resident.h"

namespace csms
{

// Server-rendered HTML for the Resident Management interface (T12).
// Presentation only: no persistence, searching or validation rules live here.
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

    // Renders the main Resident Management page (/residents) with list, search, and flash messages
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
    // Shared page shell: head, styles, top bar and navigation
    static std::string renderLayout(
        const std::string& pageTitle,
        const std::string& activeNav,
        const std::string& content
    );

    // Text status pill ("Active" / "Inactive")
    static std::string renderStatus(const std::string& status);

    // Summary box listing every validation problem, linked to its field
    static std::string renderErrorSummary(const std::vector<std::string>& errors);

    // The five editable Resident inputs, with per-field error highlighting
    static std::string renderFormFields(const FormState& values, const std::vector<std::string>& errors);

    // One labelled input, marked invalid when its field name is in errors
    static std::string renderField(
        const std::string& field,
        const std::string& label,
        const std::string& type,
        const std::string& value,
        const std::string& hint,
        const std::vector<std::string>& errors,
        const std::string& autocomplete
    );

    // Helper: Escapes HTML special characters to prevent injection
    static std::string escapeHtml(const std::string& input);

    // Helper: Turns a T02 validation field name (e.g. "contactNumber") into a readable message.
    // Wording only - the validation rules themselves stay in ResidentValidator.
    static std::string describeError(const std::string& field);
};

} // namespace csms
