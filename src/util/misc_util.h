#ifndef MISC_UTIL_H
#define MISC_UTIL_H

#include <set>
#include <vector>

namespace diutil {
template <typename T> void insert_all(std::vector<T>& v, const std::vector<T>& a)
{
  v.insert(v.end(), a.begin(), a.end());
}
template <typename T> void insert_all(std::vector<T>& v, const std::set<T>& a)
{
  v.insert(v.end(), a.begin(), a.end());
}
template <typename T> void insert_all(std::set<T>& v, const std::vector<T>& a)
{
  v.insert(a.begin(), a.end());
}
template <typename T> void insert_all(std::set<T>& v, const std::set<T>& a)
{
  v.insert(a.begin(), a.end());
}
} // namespace diutil

#endif // MISC_UTIL_H
