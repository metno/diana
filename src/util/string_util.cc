
#include "string_util.h"

namespace diutil {

static bool startsOrEndsWith(const std::string& txt, const std::string& sub,
    int startcompare)
{
  if (sub.empty())
    return true;
  if (txt.size() < sub.size())
    return false;
  return txt.compare(startcompare, sub.size(), sub) == 0;
}

bool startswith(const std::string& txt, const std::string& start)
{
  return startsOrEndsWith(txt, start, 0);
}

bool endswith(const std::string& txt, const std::string& end)
{
  return startsOrEndsWith(txt, end,
      ((int)txt.size()) - ((int)end.size()));
}

void appendText(std::string& text, const std::string& append, const std::string& separator)
{
  if (append.empty())
    return;
  if (!text.empty())
    text += separator;
  text += append;
}

std::string appendedText(const std::string& text, const std::string& append, const std::string& separator)
{
  std::string t(text);
  appendText(t, append, separator);
  return t;
}

void replace_chars(std::string& txt, const char* replace_these, const char replace_with)
{
  size_t pos = 0;
  while (pos < txt.size()
      && (pos = txt.find_first_of(replace_these, pos)) != std::string::npos)
  {
    txt[pos] = replace_with;
    pos += 1;
  }
}

} // namespace diutil
