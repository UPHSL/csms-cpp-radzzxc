#pragma once

#include <cstddef>
#include <string>

// Shared page shell and small display helpers used by every CSMS page
// (T12 Resident views and the T13 Service Request CSP templates).
// Presentation only: no business rules live here.
namespace csms::layout
{

// Opening HTML: <head> with the shared styles, the top navigation, and <main>.
// activeNav is "residents", "service-requests", or "" for none.
std::string pageStart(const std::string& pageTitle, const std::string& activeNav);

// Closing HTML for a page started with pageStart().
std::string pageEnd();

// Escapes HTML special characters so stored text is shown, never executed.
std::string escape(const std::string& input);

// Shortens text to at most maxChars characters (UTF-8 safe) and adds "..." when cut.
std::string shorten(const std::string& text, std::size_t maxChars);

// CSS class for a status pill ("Pending" -> "pending", "In Progress" -> "progress", ...).
std::string statusClass(const std::string& status);

} // namespace csms::layout
