#ifndef DIANA_UTIL_FORMAT_INT_H
#define DIANA_UTIL_FORMAT_INT_H 1

#include <string>

namespace diutil {

/*!
 * Write a positive integer into a string, using fixed field width.
 * \param value              the value to write into the string
 * \param out, start, width  std::string to receive the formatted value, starting at start,
 *                           using width chars. out will be resized to accommodate the chars
 * \param fill               the fill character, e.g. space or 0
 */
void format_int(int value, std::string& out, int start, int width, char fill = '0');

} // namespace diutil

#endif // DIANA_UTIL_FORMAT_INT_H
