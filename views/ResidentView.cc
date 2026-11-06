#include "ResidentView.h"
#include <sstream>

namespace csms
{

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

std::string ResidentView::renderLayout(
    const std::string& pageTitle,
    const std::string& activeNav,
    const std::string& breadcrumb,
    const std::string& content)
{
    std::ostringstream ss;
    ss << "<!DOCTYPE html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
       << "  <title>" << escapeHtml(pageTitle) << " - CSMS</title>\n"
       << "  <script src=\"https://www.gstatic.com/antigravity/web/dev/tailwindcss.min.js\"></script>\n"
       << "  <style>\n"
       << "    :root {\n"
       << "      --theme-sidebar: #0f172a;\n"
       << "      --theme-sidebar-border: #1e293b;\n"
       << "      --theme-primary: #0f766e;\n"
       << "      --theme-primary-hover: #115e59;\n"
       << "      --theme-accent: #0d9488;\n"
       << "      --theme-accent-light: #ccfbf1;\n"
       << "      --theme-accent-text: #115e59;\n"
       << "      --theme-canvas: #f0fdfa;\n"
       << "      --theme-card-border: #ccf0eb;\n"
       << "    }\n"
       << "  </style>\n"
       << "</head>\n"
       << "<body class=\"bg-[var(--theme-canvas)] text-slate-900 antialiased font-sans p-3 sm:p-5 min-h-screen\">\n"
       << "  <div class=\"max-w-7xl mx-auto bg-white rounded-2xl border border-[var(--theme-card-border)] shadow-xl overflow-hidden flex flex-col md:flex-row min-h-[820px]\">\n"
       
       // Administrative Sidebar
       << "    <aside class=\"w-full md:w-64 bg-[var(--theme-sidebar)] text-white/80 flex flex-col justify-between p-5 border-r border-[var(--theme-sidebar-border)]\">\n"
       << "      <div class=\"space-y-6\">\n"
       << "        <div class=\"flex items-center space-x-3 pb-5 border-b border-white/10\">\n"
       << "          <div class=\"w-10 h-10 rounded-xl bg-[var(--theme-primary)] text-white flex items-center justify-center font-bold text-sm shadow-inner border border-teal-400/30\">CS</div>\n"
       << "          <div>\n"
       << "            <span class=\"block text-xs font-bold uppercase tracking-wider text-white\">CSMS PORTAL</span>\n"
       << "            <span class=\"block text-[11px] text-teal-300/80\">Community Registry</span>\n"
       << "          </div>\n"
       << "        </div>\n"
       << "        <div class=\"space-y-1\">\n"
       << "          <p class=\"text-[10px] font-bold uppercase tracking-widest text-teal-200/50 px-3 mb-2\">Directory Services</p>\n"
       << "          <a href=\"/residents\" class=\"w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-semibold "
       << (activeNav == "list" ? "bg-[var(--theme-primary)] text-white" : "text-white/70 hover:text-white hover:bg-white/10")
       << " transition\">\n"
       << "            <span>Resident Records</span>\n"
       << "          </a>\n"
       << "          <a href=\"/residents/register\" class=\"w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-medium "
       << (activeNav == "register" ? "bg-[var(--theme-primary)] text-white" : "text-white/70 hover:text-white hover:bg-white/10")
       << " transition\">\n"
       << "            <span>Register Resident</span>\n"
       << "          </a>\n"
       << "        </div>\n"
       << "        <div class=\"space-y-1 pt-4 border-t border-white/10\">\n"
       << "          <p class=\"text-[10px] font-bold uppercase tracking-widest text-teal-200/50 px-3 mb-2\">Service Modules</p>\n"
       << "          <div class=\"flex items-center justify-between px-3 py-2 text-xs text-white/60\">\n"
       << "            <span>Service Requests</span>\n"
       << "            <span class=\"text-[10px] px-2 py-0.5 rounded bg-black/30 text-teal-300 font-mono\">T13</span>\n"
       << "          </div>\n"
       << "          <a href=\"/health\" class=\"flex items-center justify-between px-3 py-2 text-xs text-white/60 hover:text-white transition\">\n"
       << "            <span>System Health</span>\n"
       << "            <span class=\"text-[10px] px-2 py-0.5 rounded bg-teal-900/60 text-teal-300 font-mono\">/health</span>\n"
       << "          </a>\n"
       << "        </div>\n"
       << "      </div>\n"
       << "      <div class=\"pt-4 border-t border-white/10 flex items-center space-x-3\">\n"
       << "        <div class=\"w-8 h-8 rounded-full bg-teal-900/80 border border-teal-500/40 flex items-center justify-center font-bold text-xs text-teal-200\">SD</div>\n"
       << "        <div class=\"text-[11px] leading-tight\">\n"
       << "          <span class=\"block font-semibold text-white\">Barangay Desk 01</span>\n"
       << "          <span class=\"block text-teal-200/70\">Staff Portal (Drogon C++20)</span>\n"
       << "        </div>\n"
       << "      </div>\n"
       << "    </aside>\n"

       // Workspace
       << "    <main class=\"flex-1 flex flex-col bg-[var(--theme-canvas)]\">\n"
       << "      <header class=\"bg-white border-b border-[var(--theme-card-border)] px-6 py-4 flex flex-wrap items-center justify-between gap-4\">\n"
       << "        <div>\n"
       << "          <nav class=\"flex items-center space-x-1.5 text-xs text-slate-500 mb-1\">\n"
       << "            <span>Community Portal</span>\n"
       << "            <span>/</span>\n"
       << "            <span>Resident Administration</span>\n"
       << "            <span>/</span>\n"
       << "            <span class=\"text-slate-800 font-semibold\">" << escapeHtml(breadcrumb) << "</span>\n"
       << "          </nav>\n"
       << "          <h2 class=\"text-lg font-bold text-slate-900 tracking-tight\">" << escapeHtml(pageTitle) << "</h2>\n"
       << "        </div>\n"
       << "      </header>\n"
       << "      <div class=\"p-6 space-y-6 flex-1 overflow-y-auto\">\n"
       << content
       << "      </div>\n"
       << "    </main>\n"
       << "  </div>\n"
       << "</body>\n"
       << "</html>\n";

    return ss.str();
}

std::string ResidentView::renderIndex(
    const std::vector<Resident>& residents,
    const std::string& searchTerm,
    const std::string& successMessage)
{
    std::ostringstream ss;

    // Success Flash Notification
    if (!successMessage.empty())
    {
        ss << "<div class=\"bg-teal-50 border-l-4 border-teal-600 p-4 rounded-r-xl shadow-xs flex items-center justify-between\">\n"
           << "  <div class=\"flex items-center space-x-3\">\n"
           << "    <svg class=\"w-5 h-5 text-teal-600 flex-shrink-0\" fill=\"none\" stroke=\"currentColor\" viewBox=\"0 0 24 24\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z\"></path></svg>\n"
           << "    <div>\n"
           << "      <p class=\"text-xs font-bold text-teal-900\">" << escapeHtml(successMessage) << "</p>\n"
           << "    </div>\n"
           << "  </div>\n"
           << "</div>\n";
    }

    // Search and Action Bar
    ss << "<div class=\"bg-white p-4 rounded-xl border border-[var(--theme-card-border)] shadow-xs flex flex-col md:flex-row md:items-center justify-between gap-4\">\n"
       << "  <form method=\"GET\" action=\"/residents\" class=\"flex items-center space-x-2 flex-1 max-w-lg\">\n"
       << "    <div class=\"relative w-full\">\n"
       << "      <input type=\"text\" name=\"search\" id=\"search-input\" placeholder=\"Search by first name or last name...\" value=\"" << escapeHtml(searchTerm) << "\" class=\"w-full pl-3 pr-3 py-2 bg-slate-50 border border-slate-300 rounded-lg text-xs text-slate-800 placeholder-slate-400 focus:bg-white focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "    </div>\n"
       << "    <button type=\"submit\" class=\"px-4 py-2 bg-slate-800 hover:bg-slate-900 text-white text-xs font-semibold rounded-lg shadow-xs transition\">Search</button>\n"
       << "    <a href=\"/residents\" class=\"px-3 py-2 text-xs font-medium text-slate-600 hover:text-slate-900\">Clear</a>\n"
       << "  </form>\n"
       << "  <a href=\"/residents/register\" class=\"inline-flex items-center justify-center space-x-2 px-4 py-2 bg-[var(--theme-primary)] hover:bg-[var(--theme-primary-hover)] text-white text-xs font-semibold rounded-lg shadow-xs transition\">\n"
       << "    <span>+ Register New Resident</span>\n"
       << "  </a>\n"
       << "</div>\n";

    // Resident Records Table
    ss << "<div class=\"bg-white rounded-xl border border-[var(--theme-card-border)] shadow-xs overflow-hidden\">\n";

    if (residents.empty())
    {
        ss << "<div class=\"p-12 text-center space-y-3\">\n"
           << "  <p class=\"text-sm font-semibold text-slate-700\">No Residents found.</p>\n"
           << "  <p class=\"text-xs text-slate-400\">Try refining your search keyword or register a new resident into the registry.</p>\n"
           << "</div>\n";
    }
    else
    {
        ss << "<div class=\"overflow-x-auto\">\n"
           << "  <table class=\"w-full text-left text-xs border-collapse\">\n"
           << "    <thead class=\"bg-slate-50 text-slate-700 uppercase tracking-wider text-[11px] font-bold border-b border-slate-200\">\n"
           << "      <tr>\n"
           << "        <th class=\"py-3.5 px-4 w-16\">ID</th>\n"
           << "        <th class=\"py-3.5 px-4\">Resident Name</th>\n"
           << "        <th class=\"py-3.5 px-4\">Address</th>\n"
           << "        <th class=\"py-3.5 px-4\">Contact Number</th>\n"
           << "        <th class=\"py-3.5 px-4\">Email Address</th>\n"
           << "        <th class=\"py-3.5 px-4 w-28\">Status</th>\n"
           << "        <th class=\"py-3.5 px-4 text-right w-44\">Operations</th>\n"
           << "      </tr>\n"
           << "    </thead>\n"
           << "    <tbody class=\"divide-y divide-slate-100\">\n";

        for (const auto& r : residents)
        {
            std::string idStr = r.getId().has_value() ? std::to_string(r.getId().value()) : "-";
            bool isActive = (r.getStatus() == "Active");

            ss << "      <tr class=\"hover:bg-teal-50/30 transition " << (isActive ? "" : "bg-slate-50/30") << "\">\n"
               << "        <td class=\"py-3.5 px-4 font-mono font-bold text-slate-500\">" << idStr << "</td>\n"
               << "        <td class=\"py-3.5 px-4\"><span class=\"font-bold text-slate-900\">"
               << escapeHtml(r.getFirstName()) << " " << escapeHtml(r.getLastName()) << "</span></td>\n"
               << "        <td class=\"py-3.5 px-4 text-slate-700 max-w-xs truncate\">" << escapeHtml(r.getAddress()) << "</td>\n"
               << "        <td class=\"py-3.5 px-4 font-mono text-slate-800\">" << escapeHtml(r.getContactNumber()) << "</td>\n"
               << "        <td class=\"py-3.5 px-4 text-slate-700\">" << escapeHtml(r.getEmail()) << "</td>\n"
               << "        <td class=\"py-3.5 px-4\">\n";

            if (isActive)
            {
                ss << "          <span class=\"inline-flex items-center px-2.5 py-0.5 rounded-full text-[11px] font-semibold bg-emerald-100 text-emerald-800 border border-emerald-300\">\n"
                   << "            <span class=\"w-1.5 h-1.5 rounded-full bg-emerald-600 mr-1.5\"></span>Active\n"
                   << "          </span>\n";
            }
            else
            {
                ss << "          <span class=\"inline-flex items-center px-2.5 py-0.5 rounded-full text-[11px] font-semibold bg-slate-200 text-slate-700 border border-slate-300\">\n"
                   << "            <span class=\"w-1.5 h-1.5 rounded-full bg-slate-500 mr-1.5\"></span>Inactive\n"
                   << "          </span>\n";
            }

            ss << "        </td>\n"
               << "        <td class=\"py-3.5 px-4 text-right space-x-1.5 whitespace-nowrap\">\n"
               << "          <a href=\"/residents/" << idStr << "/edit\" class=\"inline-block px-2.5 py-1.5 rounded-md bg-white hover:bg-slate-100 text-slate-700 border border-slate-300 text-xs font-semibold transition\">Edit</a>\n";

            if (isActive)
            {
                ss << "          <form method=\"POST\" action=\"/residents/" << idStr << "/deactivate\" class=\"inline-block\" onsubmit=\"return confirm('Are you sure you want to deactivate this resident?');\">\n"
                   << "            <button type=\"submit\" class=\"px-2.5 py-1.5 rounded-md bg-white hover:bg-rose-50 text-rose-700 border border-rose-300 text-xs font-semibold transition\">Deactivate</button>\n"
                   << "          </form>\n";
            }
            else
            {
                ss << "          <span class=\"inline-block px-2.5 py-1.5 text-slate-400 text-[11px] italic\">Deactivated</span>\n";
            }

            ss << "        </td>\n"
               << "      </tr>\n";
        }

        ss << "    </tbody>\n"
           << "  </table>\n"
           << "</div>\n"
           << "<div class=\"bg-slate-50 px-6 py-3 border-t border-slate-200 flex flex-wrap items-center justify-between text-[11px] text-slate-600\">\n"
           << "  <span>Ordering: <strong>lastName ASC, firstName ASC, id ASC</strong> (Preserved via T05)</span>\n"
           << "  <span>Total records displayed: <strong>" << residents.size() << "</strong></span>\n"
           << "</div>\n";
    }

    ss << "</div>\n";

    return renderLayout("Resident Management Registry", "list", "Registry Directory", ss.str());
}

std::string ResidentView::renderRegisterForm(const FormState& state)
{
    std::ostringstream ss;

    ss << "<div class=\"bg-white rounded-xl border border-[var(--theme-card-border)] shadow-xs p-6 sm:p-8 max-w-3xl space-y-6\">\n"
       << "  <div class=\"border-b border-slate-200 pb-4\">\n"
       << "    <h3 class=\"text-base font-bold text-slate-900\">New Resident Registration Record</h3>\n"
       << "    <p class=\"text-xs text-slate-500 mt-1\">Enter resident demographic information. All submitted data is validated against authoritative domain rules before persistence.</p>\n"
       << "  </div>\n"
       << "  <div class=\"bg-teal-50/70 border-l-4 border-[var(--theme-primary)] p-4 rounded-r-xl text-xs text-teal-950 space-y-1\">\n"
       << "    <p class=\"font-bold\">System Record Assignment</p>\n"
       << "    <p class=\"text-teal-900/80\">Resident ID is automatically generated upon submission. Initial lifecycle status is automatically registered as <strong>Active</strong> and cannot be entered manually.</p>\n"
       << "  </div>\n";

    if (!state.errors.empty())
    {
        ss << "  <div class=\"bg-rose-50 border-l-4 border-rose-600 p-4 rounded-r-xl text-xs text-rose-900 space-y-1.5\">\n"
           << "    <div class=\"font-bold flex items-center space-x-1.5 text-rose-800\">\n"
           << "      <span>There is a problem with the submitted form:</span>\n"
           << "    </div>\n"
           << "    <ul class=\"list-disc list-inside space-y-0.5 pl-2 text-rose-800\">\n";
        for (const auto& err : state.errors)
        {
            ss << "      <li>" << escapeHtml(err) << "</li>\n";
        }
        ss << "    </ul>\n"
           << "  </div>\n";
    }

    ss << "  <form method=\"POST\" action=\"/residents/register\" class=\"space-y-4\">\n"
       << "    <div class=\"grid grid-cols-1 sm:grid-cols-2 gap-4\">\n"
       << "      <div>\n"
       << "        <label for=\"firstName\" class=\"block text-xs font-bold text-slate-800 mb-1\">First Name <span class=\"text-rose-600\">*</span></label>\n"
       << "        <input type=\"text\" name=\"firstName\" id=\"firstName\" value=\"" << escapeHtml(state.firstName) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "      </div>\n"
       << "      <div>\n"
       << "        <label for=\"lastName\" class=\"block text-xs font-bold text-slate-800 mb-1\">Last Name <span class=\"text-rose-600\">*</span></label>\n"
       << "        <input type=\"text\" name=\"lastName\" id=\"lastName\" value=\"" << escapeHtml(state.lastName) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "      </div>\n"
       << "    </div>\n"
       << "    <div>\n"
       << "      <label for=\"address\" class=\"block text-xs font-bold text-slate-800 mb-1\">Complete Residential Address <span class=\"text-rose-600\">*</span></label>\n"
       << "      <input type=\"text\" name=\"address\" id=\"address\" value=\"" << escapeHtml(state.address) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "    </div>\n"
       << "    <div class=\"grid grid-cols-1 sm:grid-cols-2 gap-4\">\n"
       << "      <div>\n"
       << "        <label for=\"contactNumber\" class=\"block text-xs font-bold text-slate-800 mb-1\">Contact Number <span class=\"text-rose-600\">*</span></label>\n"
       << "        <input type=\"text\" name=\"contactNumber\" id=\"contactNumber\" placeholder=\"09XXXXXXXXX\" value=\"" << escapeHtml(state.contactNumber) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 font-mono focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "        <span class=\"text-[10px] text-slate-500 mt-1 block\">Must follow Philippine mobile format (09XXXXXXXXX)</span>\n"
       << "      </div>\n"
       << "      <div>\n"
       << "        <label for=\"email\" class=\"block text-xs font-bold text-slate-800 mb-1\">Email Address <span class=\"text-rose-600\">*</span></label>\n"
       << "        <input type=\"email\" name=\"email\" id=\"email\" placeholder=\"name@domain.com\" value=\"" << escapeHtml(state.email) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "      </div>\n"
       << "    </div>\n"
       << "    <div class=\"pt-5 border-t border-slate-200 flex items-center space-x-3\">\n"
       << "      <button type=\"submit\" class=\"px-5 py-2.5 bg-[var(--theme-primary)] hover:bg-[var(--theme-primary-hover)] text-white text-xs font-bold rounded-lg shadow-xs transition\">Save Resident Record</button>\n"
       << "      <a href=\"/residents\" class=\"px-4 py-2.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-lg transition\">Cancel</a>\n"
       << "    </div>\n"
       << "  </form>\n"
       << "</div>\n";

    return renderLayout("Register New Resident", "register", "Registration", ss.str());
}

std::string ResidentView::renderEditForm(const Resident& resident, const FormState& state)
{
    std::ostringstream ss;
    std::string idStr = resident.getId().has_value() ? std::to_string(resident.getId().value()) : "-";

    std::string firstNameVal = state.errors.empty() ? resident.getFirstName() : state.firstName;
    std::string lastNameVal = state.errors.empty() ? resident.getLastName() : state.lastName;
    std::string addressVal = state.errors.empty() ? resident.getAddress() : state.address;
    std::string contactVal = state.errors.empty() ? resident.getContactNumber() : state.contactNumber;
    std::string emailVal = state.errors.empty() ? resident.getEmail() : state.email;

    ss << "<div class=\"bg-white rounded-xl border border-[var(--theme-card-border)] shadow-xs p-6 sm:p-8 max-w-3xl space-y-6\">\n"
       << "  <div class=\"border-b border-slate-200 pb-4\">\n"
       << "    <h3 class=\"text-base font-bold text-slate-900\">Update Resident Record #" << idStr << "</h3>\n"
       << "    <p class=\"text-xs text-slate-500 mt-1\">Edit permitted resident information. Per system design, identity and lifecycle status remain immutable.</p>\n"
       << "  </div>\n"

       // Read-only Identifiers Panel
       << "  <div class=\"bg-slate-50 border border-slate-200 rounded-xl p-4 flex flex-wrap items-center justify-between gap-4 text-xs\">\n"
       << "    <div class=\"flex items-center space-x-2\">\n"
       << "      <span class=\"text-slate-500 font-medium\">Record ID:</span>\n"
       << "      <span class=\"font-mono font-bold text-slate-800 bg-white px-2.5 py-1 rounded border border-slate-200\">#" << idStr << "</span>\n"
       << "      <span class=\"text-[11px] text-slate-400 italic\">(Read-only)</span>\n"
       << "    </div>\n"
       << "    <div class=\"flex items-center space-x-2\">\n"
       << "      <span class=\"text-slate-500 font-medium\">Lifecycle Status:</span>\n";

    if (resident.getStatus() == "Active")
    {
        ss << "      <span class=\"inline-flex items-center px-2.5 py-0.5 rounded-full text-[11px] font-semibold bg-emerald-100 text-emerald-800 border border-emerald-300\">\n"
           << "        <span class=\"w-1.5 h-1.5 rounded-full bg-emerald-600 mr-1.5\"></span>Active\n"
           << "      </span>\n";
    }
    else
    {
        ss << "      <span class=\"inline-flex items-center px-2.5 py-0.5 rounded-full text-[11px] font-semibold bg-slate-200 text-slate-700 border border-slate-300\">\n"
           << "        <span class=\"w-1.5 h-1.5 rounded-full bg-slate-500 mr-1.5\"></span>Inactive\n"
           << "      </span>\n";
    }

    ss << "      <span class=\"text-[11px] text-slate-400 italic\">(Immutable via edit form)</span>\n"
       << "    </div>\n"
       << "  </div>\n";

    if (!state.errors.empty())
    {
        ss << "  <div class=\"bg-rose-50 border-l-4 border-rose-600 p-4 rounded-r-xl text-xs text-rose-900 space-y-1.5\">\n"
           << "    <div class=\"font-bold flex items-center space-x-1.5 text-rose-800\">\n"
           << "      <span>There is a problem with the submitted form:</span>\n"
           << "    </div>\n"
           << "    <ul class=\"list-disc list-inside space-y-0.5 pl-2 text-rose-800\">\n";
        for (const auto& err : state.errors)
        {
            ss << "      <li>" << escapeHtml(err) << "</li>\n";
        }
        ss << "    </ul>\n"
           << "  </div>\n";
    }

    ss << "  <form method=\"POST\" action=\"/residents/" << idStr << "/edit\" class=\"space-y-4\">\n"
       << "    <div class=\"grid grid-cols-1 sm:grid-cols-2 gap-4\">\n"
       << "      <div>\n"
       << "        <label for=\"firstName\" class=\"block text-xs font-bold text-slate-800 mb-1\">First Name <span class=\"text-rose-600\">*</span></label>\n"
       << "        <input type=\"text\" name=\"firstName\" id=\"firstName\" value=\"" << escapeHtml(firstNameVal) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "      </div>\n"
       << "      <div>\n"
       << "        <label for=\"lastName\" class=\"block text-xs font-bold text-slate-800 mb-1\">Last Name <span class=\"text-rose-600\">*</span></label>\n"
       << "        <input type=\"text\" name=\"lastName\" id=\"lastName\" value=\"" << escapeHtml(lastNameVal) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "      </div>\n"
       << "    </div>\n"
       << "    <div>\n"
       << "      <label for=\"address\" class=\"block text-xs font-bold text-slate-800 mb-1\">Complete Residential Address <span class=\"text-rose-600\">*</span></label>\n"
       << "      <input type=\"text\" name=\"address\" id=\"address\" value=\"" << escapeHtml(addressVal) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "    </div>\n"
       << "    <div class=\"grid grid-cols-1 sm:grid-cols-2 gap-4\">\n"
       << "      <div>\n"
       << "        <label for=\"contactNumber\" class=\"block text-xs font-bold text-slate-800 mb-1\">Contact Number <span class=\"text-rose-600\">*</span></label>\n"
       << "        <input type=\"text\" name=\"contactNumber\" id=\"contactNumber\" value=\"" << escapeHtml(contactVal) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 font-mono focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
       << "      </div>\n"
       << "      <div>\n"
        << "        <label for=\"email\" class=\"block text-xs font-bold text-slate-800 mb-1\">Email Address <span class=\"text-rose-600\">*</span></label>\n"
        << "        <input type=\"email\" name=\"email\" id=\"email\" value=\"" << escapeHtml(emailVal) << "\" required class=\"w-full px-3 py-2 bg-white border border-slate-300 rounded-lg text-xs text-slate-800 focus:outline-none focus:ring-2 focus:ring-[var(--theme-primary)] focus:border-[var(--theme-primary)]\">\n"
        << "      </div>\n"
        << "    </div>\n"
        << "    <div class=\"pt-5 border-t border-slate-200 flex items-center space-x-3\">\n"
        << "      <button type=\"submit\" class=\"px-5 py-2.5 bg-[var(--theme-primary)] hover:bg-[var(--theme-primary-hover)] text-white text-xs font-bold rounded-lg shadow-xs transition\">Save Changes</button>\n"
        << "      <a href=\"/residents\" class=\"px-4 py-2.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-semibold rounded-lg transition\">Cancel</a>\n"
        << "    </div>\n"
        << "  </form>\n"
        << "</div>\n";

    return renderLayout("Edit Resident Record", "list", "Update Record", ss.str());
}

std::string ResidentView::renderNotFound(const std::string& message)
{
    std::ostringstream ss;
    ss << "<div class=\"max-w-md mx-auto bg-white rounded-xl border border-[var(--theme-card-border)] p-8 text-center shadow-xs space-y-4 my-10\">\n"
       << "  <div class=\"w-12 h-12 rounded-full bg-slate-100 text-slate-600 mx-auto flex items-center justify-center font-bold text-lg\">!</div>\n"
       << "  <div>\n"
       << "    <h4 class=\"text-base font-bold text-slate-900\">404 - Resident Record Not Found</h4>\n"
       << "    <p class=\"text-xs text-slate-500 mt-2 leading-relaxed\">" << escapeHtml(message) << "</p>\n"
       << "  </div>\n"
       << "  <div class=\"p-3 bg-slate-50 rounded-lg border border-slate-200 text-[11px] text-slate-600 font-mono text-left\">\n"
       << "    HTTP GET &rarr; 404 Handled Gracefully\n"
       << "  </div>\n"
       << "  <div>\n"
       << "    <a href=\"/residents\" class=\"inline-block px-4 py-2 bg-slate-800 hover:bg-slate-900 text-white text-xs font-semibold rounded-lg transition\">Return to Resident Directory</a>\n"
       << "  </div>\n"
       << "</div>\n";

    return renderLayout("Resident Not Found", "list", "Not Found", ss.str());
}

} // namespace csms