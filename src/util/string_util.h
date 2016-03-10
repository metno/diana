
#ifndef DIANA_UTIL_STRING_UTIL_H
#define DIANA_UTIL_STRING_UTIL_H 1

#include <string>
#include <vector>

namespace diutil {

bool startswith(const std::string& txt, const std::string& start);
bool endswith(const std::string& txt, const std::string& end);

void appendText(std::string& text, const std::string& append, const std::string& separator=" ");
std::string appendedText(const std::string& text, const std::string& append, const std::string& separator=" ");

void replace_chars(std::string& txt, const char* replace, const char with);
inline std::string replaced_chars(const std::string& txt, const char* replace, const char with)
{ std::string t(txt); replace_chars(t, replace, with); return t; }

namespace detail {
void append_chars_split_newline(std::vector<std::string>& lines, const char* buffer, size_t nbuffer);
}

} // namespace diutil

#endif // DIANA_UTIL_STRING_UTIL_H
