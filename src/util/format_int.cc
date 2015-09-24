
#include "format_int.h"

namespace diutil {

void format_int(int value, std::string& out, int start, int width, char fill)
{
  if (value < 0 || width <= 0 || start < 0)
    return;
  if ((int)out.size() < start + width)
    out.resize(start + width, ' ');
  for (int d=width-1; d>=0; --d) {
    int digit = (value % 10);
    char ch;
    if (digit == 0 && d < width-1)
      ch = fill;
    else
      ch = '0' + digit;
    out[start + d] = ch;
    value /= 10;
  }
}

} // namespace diutil
