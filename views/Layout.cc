#include "Layout.h"
#include <sstream>
#include "Version.h"

namespace csms::layout
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

/* ---- Shared form controls ---- */
select, textarea { font: inherit; padding: 9px 12px; border: 1px solid var(--field); border-radius: 10px; background: #fff; color: var(--ink); width: 100%; }
textarea { min-height: 120px; resize: vertical; }
select:focus, textarea:focus { outline: 2px solid var(--accent); outline-offset: 1px; }
.f.invalid select, .f.invalid textarea { border-color: var(--err); background: #fffaf9; }
.search select { width: 180px; }
.flash.error { background: var(--err-bg); color: var(--err); border-color: #f0c9c1; }

/* ---- Service Request statuses ---- */
.status.pending { background: #fbf0d9; color: #7a4f00; }
.status.progress { background: #e3ecf7; color: #2c4f7c; }
.status.completed { background: var(--ok-bg); color: var(--ok); }
.status.cancelled { background: var(--off-bg); color: var(--off); }

/* ---- Service Request list and details ---- */
td.stype { font-weight: 600; min-width: 9em; }
td.desc { min-width: 10em; color: var(--muted); }
.workflow { display: inline-flex; gap: 6px; flex-wrap: nowrap; justify-content: flex-end; align-items: center; }
.btn-view { background: none; border: 0; padding: 6px 4px; color: var(--ink); text-decoration: underline; }
.workflow form { display: inline; }
.fields { display: grid; grid-template-columns: 170px 1fr; gap: 12px 16px; margin: 20px 0; }
.fields dt { color: var(--muted); font-size: 14px; }
.fields dd { margin: 0; }
.fields .long { white-space: pre-wrap; overflow-wrap: anywhere; }
.panel-actions { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; padding-top: 16px; border-top: 1px solid var(--line); }
.panel-actions .hint { margin: 0; }
@media (max-width: 720px) {
  .search select { width: 100%; }
  .fields { grid-template-columns: 1fr; gap: 2px; }
  .fields dd { margin-bottom: 10px; }
  .workflow { justify-content: flex-start; flex-wrap: wrap; }
}

/* ---- Dashboard (T14) ---- */
.dash-top { display: grid; grid-template-columns: minmax(0, 3fr) minmax(0, 2fr); gap: 18px; margin-bottom: 18px; }
.dash-card { padding: 20px 22px; margin-bottom: 18px; }
.dash-top .dash-card { margin-bottom: 0; }
.dash-head { display: flex; align-items: center; justify-content: space-between; gap: 10px 16px; flex-wrap: wrap; margin-bottom: 14px; }
.dash-title { font-size: 22px; line-height: 1.2; margin: 0; }
.stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 10px; margin: 0; }
.stat { background: #f6f1e7; border-radius: 12px; padding: 12px 14px; min-width: 0; }
.stat.total { background: #fff; border: 1px solid var(--line); }
.stat dt { font-size: 13px; color: var(--muted); font-weight: 600; }
.stat dd { margin: 6px 0 0; font-size: 30px; font-weight: 600; line-height: 1.1; font-variant-numeric: tabular-nums; overflow-wrap: anywhere; }
.dash-note { margin: 12px 0 0; }
.open-card { display: flex; flex-direction: column; background: #fbf3ea; border-color: #ecd6c4; }
.open-value { margin: 10px 0 0; font-size: 52px; font-weight: 600; line-height: 1; color: var(--accent); font-variant-numeric: tabular-nums; overflow-wrap: anywhere; }
.open-links { display: flex; gap: 8px; flex-wrap: wrap; margin-top: auto; padding-top: 14px; }
.recent-head { display: flex; align-items: center; justify-content: space-between; gap: 10px 16px; flex-wrap: wrap; padding: 18px 20px; border-bottom: 1px solid var(--line); }
.recent-head .sub { margin-top: 2px; }
@media (max-width: 720px) {
  .dash-top { grid-template-columns: 1fr; }
  .dash-card { padding: 18px 16px; }
  .recent-head { padding: 16px; }
  .stat dd { font-size: 26px; }
  .open-value { font-size: 44px; }
}
)CSS";

} // namespace

std::string pageStart(const std::string& pageTitle, const std::string& activeNav)
{
    auto navLink = [&](const std::string& key, const std::string& href, const std::string& label) {
        const bool on = (activeNav == key);
        return "    <a href=\"" + href + "\"" + (on ? " class=\"on\" aria-current=\"page\"" : "") + ">" + label + "</a>\n";
    };

    std::ostringstream ss;
    ss << "<!DOCTYPE html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
       << "  <title>" << escape(pageTitle) << " - CSMS</title>\n"
       << "  <style>" << kStyles << "</style>\n"
       << "</head>\n"
       << "<body>\n"
       << "<header class=\"topbar\"><div class=\"wrap\">\n"
       << "  <a class=\"brand\" href=\"/\"><b>" << csms::Version::kShortName 
       << "</b><span>" << csms::Version::kApplication << "</span> <span style=\"font-size: 11px; font-weight: 600; padding: 2px 8px; border-radius: 999px; background: #e0d7c8; color: var(--ink);\">" 
       << csms::Version::kDisplayVersion << "</span></a>\n"
       << "  <nav class=\"nav\" aria-label=\"Main\">\n"
       << navLink("dashboard", "/dashboard", "Dashboard")
       << navLink("residents", "/residents", "Residents")
       << navLink("service-requests", "/service-requests", "Service Requests")
       << "  </nav>\n"
       << "</div></header>\n"
       << "<main class=\"wrap\">\n";
    return ss.str();
}

std::string pageEnd()
{
    std::ostringstream ss;
    ss << "</main>\n"
       << "<footer class=\"foot wrap\" style=\"border-top: 1px solid var(--line); margin-top: 36px; padding: 20px 0; color: var(--muted); font-size: 13px; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 8px;\">\n"
       << "  <span>" << csms::Version::kApplication << "</span>\n"
       << "  <span>Release " << csms::Version::kDisplayVersion << "</span>\n"
       << "</footer>\n"
       << "</body>\n</html>\n";
    return ss.str();
}

std::string escape(const std::string& input)
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

std::string shorten(const std::string& text, std::size_t maxChars)
{
    std::size_t chars = 0;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        // Count only the first byte of each UTF-8 character, so multi-byte letters are never split
        if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80)
        {
            if (chars == maxChars)
            {
                std::size_t end = i;
                while (end > 0 && text[end - 1] == ' ')
                {
                    --end;
                }
                return text.substr(0, end) + "...";
            }
            ++chars;
        }
    }
    return text;
}

std::string statusClass(const std::string& status)
{
    if (status == "Active")      return "on";
    if (status == "Inactive")    return "off";
    if (status == "Pending")     return "pending";
    if (status == "In Progress") return "progress";
    if (status == "Completed")   return "completed";
    if (status == "Cancelled")   return "cancelled";
    return "off";
}

} // namespace csms::layout
