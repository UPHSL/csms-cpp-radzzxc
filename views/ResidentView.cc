#include "ResidentView.h"
#include "Layout.h"
#include <algorithm>
#include <sstream>

namespace csms
{

namespace
{

const std::string kDeactivateConfirm =
    "Deactivate this resident? Their record stays in the registry and will be marked Inactive.";

bool isBlank(const std::string& value)
{
    return value.find_first_not_of(" \t\n\r\f\v") == std::string::npos;
}

} // namespace

std::string ResidentView::escapeHtml(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (char c : input)
    {
        switch (c)
        {
            case '&':  output.append("&amp;"); break;
            case '<':  output.append("&lt;"); break;
            case '>':  output.append("&gt;"); break;
            case '"':  output.append("&quot;"); break;
            case '\'': output.append("&#39;"); break;
            default:   output.push_back(c); break;
        }
    }
    return output;
}

std::string ResidentView::describeError(const std::string& field)
{
    if (field == "firstName")     return "First name is required.";
    if (field == "lastName")      return "Last name is required.";
    if (field == "address")       return "Address is required.";
    if (field == "contactNumber") return "Contact number must follow the format 09XXXXXXXXX.";
    if (field == "email")         return "Email address is invalid (example: name@domain.com).";
    if (field == "status")        return "Resident status is invalid.";
    return field;
}

std::string ResidentView::renderLayout(
    const std::string& pageTitle,
    const std::string& content)
{
    // Shared shell (styles + top navigation) used by Residents and Service Requests
    return layout::pageStart(pageTitle, "residents") + content + layout::pageEnd();
}

std::string ResidentView::renderStatus(const std::string& status)
{
    const bool isActive = (status == "Active");
    return std::string("<span class=\"status ") + (isActive ? "on" : "off") + "\">" + escapeHtml(status) + "</span>";
}

std::string ResidentView::renderErrorSummary(const std::vector<std::string>& errors)
{
    if (errors.empty())
    {
        return "";
    }

    static const std::vector<std::string> formFields = {"firstName", "lastName", "address", "contactNumber", "email"};

    std::ostringstream ss;
    ss << "<div class=\"errbox\" role=\"alert\">\n"
       << "  <strong>Please fix the following:</strong>\n"
       << "  <ul>\n";
    for (const auto& field : errors)
    {
        const bool linkable = std::find(formFields.begin(), formFields.end(), field) != formFields.end();
        ss << "    <li>";
        if (linkable)
        {
            ss << "<a href=\"#" << escapeHtml(field) << "\">" << escapeHtml(describeError(field)) << "</a>";
        }
        else
        {
            ss << escapeHtml(describeError(field));
        }
        ss << "</li>\n";
    }
    ss << "  </ul>\n"
       << "</div>\n";
    return ss.str();
}

std::string ResidentView::renderField(
    const std::string& field,
    const std::string& label,
    const std::string& type,
    const std::string& value,
    const std::string& hint,
    const std::vector<std::string>& errors,
    const std::string& autocomplete)
{
    const bool invalid = std::find(errors.begin(), errors.end(), field) != errors.end();

    std::string describedBy;
    if (!hint.empty())
    {
        describedBy += field + "-hint";
    }
    if (invalid)
    {
        describedBy += (describedBy.empty() ? "" : " ") + field + "-error";
    }

    std::ostringstream ss;
    ss << "<div class=\"f" << (invalid ? " invalid" : "") << "\">\n"
       << "  <label for=\"" << field << "\">" << escapeHtml(label) << "</label>\n";
    if (!hint.empty())
    {
        ss << "  <span class=\"hint\" id=\"" << field << "-hint\">" << escapeHtml(hint) << "</span>\n";
    }
    if (invalid)
    {
        ss << "  <span class=\"field-error\" id=\"" << field << "-error\">" << escapeHtml(describeError(field)) << "</span>\n";
    }
    ss << "  <input type=\"" << type << "\" id=\"" << field << "\" name=\"" << field << "\" value=\"" << escapeHtml(value) << "\""
       << " autocomplete=\"" << autocomplete << "\" required";
    if (invalid)
    {
        ss << " aria-invalid=\"true\"";
    }
    if (!describedBy.empty())
    {
        ss << " aria-describedby=\"" << describedBy << "\"";
    }
    ss << ">\n"
       << "</div>\n";
    return ss.str();
}

std::string ResidentView::renderFormFields(const FormState& values, const std::vector<std::string>& errors)
{
    std::ostringstream ss;
    ss << "<div class=\"grid2\">\n"
       << renderField("firstName", "First name", "text", values.firstName, "", errors, "given-name")
       << renderField("lastName", "Last name", "text", values.lastName, "", errors, "family-name")
       << "</div>\n"
       << renderField("address", "Address", "text", values.address, "", errors, "street-address")
       << "<div class=\"grid2\">\n"
       << renderField("contactNumber", "Contact number", "tel", values.contactNumber, "Format: 09XXXXXXXXX", errors, "tel")
       << renderField("email", "Email address", "email", values.email, "Example: name@domain.com", errors, "email")
       << "</div>\n";
    return ss.str();
}

std::string ResidentView::renderIndex(
    const std::vector<Resident>& residents,
    const std::string& searchTerm,
    const std::string& successMessage)
{
    const bool searching = !isBlank(searchTerm);
    const std::size_t count = residents.size();
    const std::string countText = std::to_string(count) + (count == 1 ? " resident" : " residents");

    std::ostringstream ss;

    ss << "<div class=\"titlebar\">\n"
       << "  <div>\n"
       << "    <p class=\"eyebrow\">Community records</p>\n"
       << "    <h1 class=\"serif\">Resident Management</h1>\n"
       << "    <p class=\"sub\">";
    if (searching)
    {
        ss << countText << " matching &ldquo;" << escapeHtml(searchTerm) << "&rdquo;";
    }
    else
    {
        ss << countText << " &middot; sorted by last name";
    }
    ss << "</p>\n"
       << "  </div>\n"
       << "  <a class=\"btn btn-primary\" href=\"/residents/register\">Register resident</a>\n"
       << "</div>\n";

    if (!successMessage.empty())
    {
        ss << "<div class=\"flash\" role=\"status\">" << escapeHtml(successMessage) << "</div>\n";
    }

    ss << "<section class=\"card\">\n"
       << "  <div class=\"toolbar\">\n"
       << "    <form class=\"search\" method=\"GET\" action=\"/residents\" role=\"search\">\n"
       << "      <div class=\"field\">\n"
       << "        <label for=\"search\">Search by first or last name</label>\n"
       << "        <input type=\"search\" id=\"search\" name=\"search\" value=\"" << escapeHtml(searchTerm) << "\">\n"
       << "      </div>\n"
       << "      <button class=\"btn btn-dark\" type=\"submit\">Search</button>\n"
       << "      <a class=\"btn btn-link\" href=\"/residents\">Clear search</a>\n"
       << "    </form>\n"
       << "  </div>\n";

    if (residents.empty())
    {
        ss << "  <div class=\"empty\">\n"
           << "    <p class=\"empty-title\">No Residents found.</p>\n"
           << "    <p class=\"hint\">"
           << (searching ? "Try a different name, or clear the search to see everyone."
                         : "Register the first resident to get started.")
           << "</p>\n"
           << "  </div>\n";
    }
    else
    {
        ss << "  <table>\n"
           << "    <caption class=\"sr-only\">Residents</caption>\n"
           << "    <thead><tr>"
           << "<th scope=\"col\">ID</th>"
           << "<th scope=\"col\">Name</th>"
           << "<th scope=\"col\">Address</th>"
           << "<th scope=\"col\">Contact number</th>"
           << "<th scope=\"col\">Email</th>"
           << "<th scope=\"col\">Status</th>"
           << "<th scope=\"col\" class=\"actions\">Actions</th>"
           << "</tr></thead>\n"
           << "    <tbody>\n";

        for (const auto& r : residents)
        {
            const std::string idStr = r.getId().has_value() ? std::to_string(r.getId().value()) : "-";
            const bool isActive = (r.getStatus() == "Active");

            ss << "      <tr>\n"
               << "        <td class=\"id\" data-label=\"ID\">" << idStr << "</td>\n"
               << "        <td class=\"name\" data-label=\"Name\">" << escapeHtml(r.getFirstName()) << " " << escapeHtml(r.getLastName()) << "</td>\n"
               << "        <td class=\"addr\" data-label=\"Address\">" << escapeHtml(r.getAddress()) << "</td>\n"
               << "        <td class=\"nowrap\" data-label=\"Contact\">" << escapeHtml(r.getContactNumber()) << "</td>\n"
               << "        <td class=\"email\" data-label=\"Email\">" << escapeHtml(r.getEmail()) << "</td>\n"
               << "        <td class=\"nowrap\" data-label=\"Status\">" << renderStatus(r.getStatus()) << "</td>\n"
               << "        <td class=\"actions\" data-label=\"Actions\">\n"
               << "          <a class=\"btn btn-quiet btn-sm\" href=\"/residents/" << idStr << "/edit\">Edit</a>\n";

            if (isActive)
            {
                ss << "          <form method=\"POST\" action=\"/residents/" << idStr << "/deactivate\""
                   << " onsubmit=\"return confirm('" << kDeactivateConfirm << "');\">\n"
                   << "            <button class=\"btn btn-quiet btn-sm\" type=\"submit\">Deactivate</button>\n"
                   << "          </form>\n";
            }
            else
            {
                ss << "          <span class=\"muted-note\">Deactivated</span>\n";
            }

            ss << "        </td>\n"
               << "      </tr>\n";
        }

        ss << "    </tbody>\n"
           << "  </table>\n"
           << "  <div class=\"foot\">Showing " << countText << "</div>\n";
    }

    ss << "</section>\n";

    return renderLayout("Resident Management", ss.str());
}

std::string ResidentView::renderRegisterForm(const FormState& state)
{
    std::ostringstream ss;
    ss << "<a class=\"back\" href=\"/residents\">&larr; Back to residents</a>\n"
       << "<section class=\"card formcard\">\n"
       << "  <p class=\"eyebrow\">New record</p>\n"
       << "  <h1 class=\"serif\">Register resident</h1>\n"
       << "  <p class=\"hint\">All fields are required. A resident ID is assigned automatically, and new residents start as Active.</p>\n"
       << renderErrorSummary(state.errors)
       << "  <form class=\"formgrid\" method=\"POST\" action=\"/residents/register\" novalidate>\n"
       << renderFormFields(state, state.errors)
       << "    <div class=\"form-actions\">\n"
       << "      <button class=\"btn btn-primary\" type=\"submit\">Save resident</button>\n"
       << "      <a class=\"btn btn-link\" href=\"/residents\">Cancel</a>\n"
       << "    </div>\n"
       << "  </form>\n"
       << "</section>\n";

    return renderLayout("Register resident", ss.str());
}

std::string ResidentView::renderEditForm(const Resident& resident, const FormState& state)
{
    const std::string idStr = resident.getId().has_value() ? std::to_string(resident.getId().value()) : "-";

    // Show the persisted values first; after a failed submit, show what the user typed
    FormState values = state;
    if (state.errors.empty())
    {
        values.firstName = resident.getFirstName();
        values.lastName = resident.getLastName();
        values.address = resident.getAddress();
        values.contactNumber = resident.getContactNumber();
        values.email = resident.getEmail();
    }

    std::ostringstream ss;
    ss << "<a class=\"back\" href=\"/residents\">&larr; Back to residents</a>\n"
       << "<section class=\"card formcard\">\n"
       << "  <p class=\"eyebrow\">Edit resident</p>\n"
       << "  <h1 class=\"serif\">" << escapeHtml(resident.getFirstName()) << " " << escapeHtml(resident.getLastName()) << "</h1>\n"
       << "  <dl class=\"meta\">\n"
       << "    <div><dt>Resident ID</dt><dd>" << idStr << "</dd></div>\n"
       << "    <div><dt>Status</dt><dd>" << renderStatus(resident.getStatus()) << "</dd></div>\n"
       << "  </dl>\n"
       << "  <p class=\"hint\">Resident ID and status cannot be changed here.</p>\n"
       << renderErrorSummary(state.errors)
       << "  <form class=\"formgrid\" method=\"POST\" action=\"/residents/" << idStr << "/edit\" novalidate>\n"
       << renderFormFields(values, state.errors)
       << "    <div class=\"form-actions\">\n"
       << "      <button class=\"btn btn-primary\" type=\"submit\">Save changes</button>\n"
       << "      <a class=\"btn btn-link\" href=\"/residents\">Cancel</a>\n"
       << "    </div>\n"
       << "  </form>\n"
       << "</section>\n";

    return renderLayout("Edit resident", ss.str());
}

std::string ResidentView::renderNotFound(const std::string& message)
{
    std::ostringstream ss;
    ss << "<section class=\"card notfound\">\n"
       << "  <p class=\"eyebrow\">Error 404</p>\n"
       << "  <h1 class=\"serif\">Resident not found</h1>\n"
       << "  <p class=\"sub\">" << escapeHtml(message) << "</p>\n"
       << "  <a class=\"btn btn-primary\" href=\"/residents\">Back to residents</a>\n"
       << "</section>\n";

    return renderLayout("Resident not found", ss.str());
}

} // namespace csms
