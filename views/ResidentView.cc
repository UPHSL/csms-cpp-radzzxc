#include "ResidentView.h"
#include <algorithm>
#include <sstream>

namespace csms
{

namespace
{

// Page styles, served inline so the interface has no external dependencies.
const char* kStyles = R"CSS(
:root {
  --bg: #f5f1e8; --card: #fffdf8; --ink: #2a251f; --muted: #6f655a; --line: #e6ddcf; --field: #d8cebf;
  --accent: #a8492a; --accent-hover: #8f3c21;
  --ok: #4f6b2c; --ok-bg: #edf1e2; --off: #615a52; --off-bg: #ece7de;
  --err: #a3261b; --err-bg: #fbeae6;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--ink); font: 15px/1.5 "Segoe UI", system-ui, -apple-system, Roboto, sans-serif; }
a { color: inherit; }
:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
.serif { font-family: "Iowan Old Style", "Palatino Linotype", Palatino, Georgia, serif; font-weight: 400; letter-spacing: -.01em; }
.sr-only { position: absolute; width: 1px; height: 1px; overflow: hidden; clip: rect(0 0 0 0); white-space: nowrap; }
.wrap { max-width: 1180px; margin: 0 auto; padding: 0 24px; }
main.wrap { padding-bottom: 48px; }

.topbar { padding: 18px 0; }
.topbar .wrap { display: flex; align-items: center; justify-content: space-between; gap: 16px; flex-wrap: wrap; }
.brand { display: flex; align-items: baseline; gap: 10px; text-decoration: none; }
.brand b { font-family: "Iowan Old Style", "Palatino Linotype", Palatino, Georgia, serif; font-size: 22px; font-weight: 400; color: var(--accent); }
.brand span { color: var(--muted); font-size: 13px; }
.nav { display: flex; gap: 2px; background: #ebe4d8; padding: 4px; border-radius: 999px; }
.nav a { text-decoration: none; padding: 7px 16px; border-radius: 999px; font-size: 14px; }
.nav a:hover { background: #e0d7c8; }
.nav a.on { background: var(--ink); color: #fff; }

.titlebar { display: flex; align-items: flex-end; justify-content: space-between; gap: 16px; flex-wrap: wrap; padding: 20px 0 18px; }
.eyebrow { color: var(--accent); font-size: 13px; margin: 0 0 2px; }
h1 { font-size: 34px; line-height: 1.15; margin: 0; }
.sub { color: var(--muted); margin: 4px 0 0; font-size: 14px; }
.back { display: inline-block; margin: 16px 0 4px; color: var(--muted); font-size: 14px; text-decoration: none; }
.back:hover { color: var(--ink); text-decoration: underline; }

.flash { background: var(--ok-bg); color: var(--ok); border: 1px solid #d5dfc0; border-radius: 12px; padding: 12px 16px; margin-bottom: 18px; }
.card { background: var(--card); border: 1px solid var(--line); border-radius: 16px; }

.btn { font: inherit; font-size: 14px; line-height: 1.3; padding: 9px 16px; border-radius: 999px; border: 1px solid transparent; cursor: pointer; text-decoration: none; display: inline-block; }
.btn-primary { background: var(--accent); color: #fff; }
.btn-primary:hover { background: var(--accent-hover); }
.btn-dark { background: var(--ink); color: #fff; }
.btn-quiet { background: transparent; color: var(--ink); border-color: var(--field); }
.btn-quiet:hover { background: #f1ebe0; }
.btn-link { background: none; color: var(--muted); padding: 9px 6px; text-decoration: underline; }
.btn-sm { padding: 6px 14px; font-size: 13px; }

label { font-size: 13px; font-weight: 600; }
input { font: inherit; padding: 9px 12px; border: 1px solid var(--field); border-radius: 10px; background: #fff; color: var(--ink); width: 100%; }
input:focus { outline: 2px solid var(--accent); outline-offset: 1px; }

.toolbar { padding: 16px 20px; border-bottom: 1px solid var(--line); }
.search { display: flex; align-items: flex-end; gap: 8px; flex-wrap: wrap; }
.search .field { display: flex; flex-direction: column; gap: 4px; }
.search input { width: 300px; max-width: 100%; }

table { width: 100%; border-collapse: collapse; font-size: 14px; }
th { text-align: left; font-size: 12px; text-transform: uppercase; letter-spacing: .06em; color: var(--muted); font-weight: 600; padding: 12px 10px; border-bottom: 1px solid var(--line); white-space: nowrap; }
td { text-align: left; vertical-align: top; padding: 14px 10px; border-bottom: 1px solid var(--line); }
th:first-child, td:first-child { padding-left: 20px; }
th:last-child, td:last-child { padding-right: 20px; }
tbody tr:last-child td { border-bottom: 0; }
td.id { color: var(--muted); font-variant-numeric: tabular-nums; }
td.name { font-weight: 600; white-space: nowrap; }
td.addr { min-width: 10em; }
td.nowrap { white-space: nowrap; font-variant-numeric: tabular-nums; }
td.email { min-width: 15em; overflow-wrap: anywhere; }
th.actions, td.actions { text-align: right; white-space: nowrap; }
td.actions form { display: inline; }
.status { display: inline-flex; align-items: center; gap: 6px; padding: 2px 10px; border-radius: 999px; font-size: 13px; font-weight: 600; }
.status::before { content: ""; width: 7px; height: 7px; border-radius: 50%; background: currentColor; }
.status.on { background: var(--ok-bg); color: var(--ok); }
.status.off { background: var(--off-bg); color: var(--off); }
.muted-note { color: var(--muted); font-size: 13px; padding: 0 6px; }
.foot { padding: 12px 20px; color: var(--muted); font-size: 13px; border-top: 1px solid var(--line); }
.empty { padding: 48px 20px; text-align: center; }
.empty p { margin: 4px 0; }
.empty .empty-title { font-size: 17px; font-weight: 600; }
.empty .hint { color: var(--muted); font-size: 14px; }

.formcard { padding: 28px; max-width: 760px; margin-top: 8px; }
.formcard h1 { margin-bottom: 4px; }
.hint { color: var(--muted); font-size: 13px; }
.formcard > .hint { margin: 0 0 4px; }
.meta { display: flex; gap: 28px; flex-wrap: wrap; margin: 16px 0 4px; padding: 12px 16px; background: #f6f1e7; border-radius: 12px; }
.meta div { display: flex; align-items: center; gap: 8px; }
.meta dt { color: var(--muted); font-size: 13px; }
.meta dd { margin: 0; font-weight: 600; }
.errbox { background: var(--err-bg); border: 1px solid #f0c9c1; color: var(--err); border-radius: 12px; padding: 12px 16px; margin: 16px 0; }
.errbox ul { margin: 6px 0 0; padding-left: 20px; }
.errbox a { color: var(--err); }
.formgrid { display: grid; gap: 16px; margin-top: 18px; }
.grid2 { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
.f { display: flex; flex-direction: column; gap: 5px; }
.f.invalid input { border-color: var(--err); background: #fffaf9; }
.f.invalid label { color: var(--err); }
.field-error { color: var(--err); font-size: 13px; font-weight: 600; }
.form-actions { display: flex; align-items: center; gap: 8px; padding-top: 6px; }

.notfound { max-width: 560px; margin: 48px auto; padding: 36px; text-align: center; }
.notfound .btn { margin-top: 18px; }

@media (max-width: 720px) {
  .wrap { padding: 0 16px; }
  h1 { font-size: 28px; }
  .grid2 { grid-template-columns: 1fr; }
  .search, .search .field, .search input { width: 100%; }
  .formcard { padding: 20px; }
  table, tbody, tr, td { display: block; }
  thead { position: absolute; left: -9999px; }
  tr { padding: 12px 16px; border-bottom: 1px solid var(--line); }
  tbody tr:last-child { border-bottom: 0; }
  td, td:first-child, td:last-child { display: flex; gap: 12px; padding: 3px 0; border: 0; min-width: 0; white-space: normal; }
  td::before { content: attr(data-label); flex: 0 0 96px; color: var(--muted); font-size: 13px; }
  td.actions { text-align: left; padding-top: 8px; }
}
)CSS";

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
    const std::string& activeNav,
    const std::string& content)
{
    std::ostringstream ss;
    ss << "<!DOCTYPE html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
       << "  <title>" << escapeHtml(pageTitle) << " - CSMS</title>\n"
       << "  <style>" << kStyles << "</style>\n"
       << "</head>\n"
       << "<body>\n"
       << "<header class=\"topbar\"><div class=\"wrap\">\n"
       << "  <a class=\"brand\" href=\"/residents\"><b>CSMS</b><span>Community Services Management System</span></a>\n"
       << "  <nav class=\"nav\" aria-label=\"Main\">\n"
       << "    <a href=\"/residents\"" << (activeNav == "list" ? " class=\"on\" aria-current=\"page\"" : "") << ">Residents</a>\n"
       << "    <a href=\"/residents/register\"" << (activeNav == "register" ? " class=\"on\" aria-current=\"page\"" : "") << ">Register resident</a>\n"
       << "  </nav>\n"
       << "</div></header>\n"
       << "<main class=\"wrap\">\n"
       << content
       << "</main>\n"
       << "</body>\n"
       << "</html>\n";
    return ss.str();
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

    return renderLayout("Resident Management", "list", ss.str());
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

    return renderLayout("Register resident", "register", ss.str());
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

    return renderLayout("Edit resident", "list", ss.str());
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

    return renderLayout("Resident not found", "list", ss.str());
}

} // namespace csms
