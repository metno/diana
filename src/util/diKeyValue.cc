/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018-2022 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "diKeyValue.h"

#include <puTools/miStringFunctions.h>

#include <set>
#include <sstream>

static const char QUOTE = '"';

namespace miutil {

KeyValue::KeyValue()
  : mHasValue(false)
  , mKeptQuotes(false)
{ }

KeyValue::KeyValue(const std::string& k, const std::string& v, bool keptQuotes)
  : mKey(k)
  , mValue(v)
  , mHasValue(true)
  , mKeptQuotes(keptQuotes)
{ }

KeyValue::KeyValue(const std::string& k)
  : mKey(k)
  , mHasValue(false)
  , mKeptQuotes(false)
{ }

int KeyValue::toInt(bool& ok, int def) const
{
  ok = miutil::is_int(mValue);
  if (ok)
    return miutil::to_int(mValue);
  else
    return def;
}

double KeyValue::toDouble(bool& ok, double def) const
{
  ok = miutil::is_number(mValue);
  if (ok)
    return miutil::to_double(mValue);
  else
    return def;
}

float KeyValue::toFloat(bool& ok, float def) const
{
  ok = miutil::is_number(mValue);
  if (ok)
    return miutil::to_float(mValue);
  else
    return def;
}

bool KeyValue::toBool(bool& ok, bool def) const
{
  ok = true;
  if (mValue == "0")
    return false;
  else if (mValue == "1")
    return true;
  else if (not mValue.empty()) {
    const std::string lvalue = miutil::to_lower(mValue);
    if (lvalue == "yes" or lvalue == "true" or lvalue == "on")
      return true;
    else if (lvalue == "no" or lvalue == "false" or lvalue == "off")
      return false;
  }

  ok = false;
  return def;
}

std::vector<int> KeyValue::toInts() const
{
  const std::vector<std::string> values = miutil::split(mValue, ",");
  std::vector<int> ints;
  ints.reserve(values.size());
  for (const std::string& v : values) {
    if (miutil::is_int(v))
      ints.push_back(miutil::to_int(v));
  }
  return ints;
}

std::vector<float> KeyValue::toFloats() const
{
  const std::vector<std::string> values = miutil::split(mValue, ",");
  std::vector<float> floats;
  floats.reserve(values.size());
  for (const std::string& v : values) {
    if (miutil::is_number(v))
      floats.push_back(miutil::to_float(v));
  }
  return floats;
}

KeyValue kv(const std::string& key, bool value)
{
  return KeyValue(key, value ? "true" : "false");
}

KeyValue kv(const std::string& key, int value)
{
  return KeyValue(key, miutil::from_number(value));
}

KeyValue kv(const std::string& key, float value)
{
  return KeyValue(key, miutil::from_number(value));
}

KeyValue kv(const std::string& key, const std::string& value)
{
  return KeyValue(key, value);
}

KeyValue kv(const std::string& key, const char* value)
{
  return KeyValue(key, value);
}

KeyValue kv(const std::string& key, const std::vector<int>& values)
{
  std::ostringstream ov;
  std::vector<int>::const_iterator it = values.begin();
  ov << *it++;
  for (; it != values.end(); ++it)
    ov << ',' << *it;
  return kv(key, ov.str());
}

KeyValue kv(const std::string& key, const std::vector<float>& values)
{
  std::ostringstream ov;
  if (!values.empty()) {
    std::vector<float>::const_iterator it = values.begin();
    ov << *it++;
    for (; it != values.end(); ++it)
      ov << ',' << miutil::from_number(*it);
  }
  return kv(key, ov.str());
}

KeyValue kv(const std::string& key, const std::vector<std::string>& values)
{
  std::ostringstream ov;
  if (!values.empty()) {
    std::vector<std::string>::const_iterator it = values.begin();
    ov << *it++;
    for (; it != values.end(); ++it)
      ov << ',' << *it;
  }
  return kv(key, ov.str());
}

size_t find(const miutil::KeyValue_v& kvs, const std::string& key, size_t start)
{
  for (; start < kvs.size(); ++start) {
    if (kvs[start].key() == key)
      return start;
  }
  return size_t(-1);
}

size_t rfind(const miutil::KeyValue_v& kvs, const std::string& key)
{
  if (kvs.empty())
    return size_t(-1);
  else
    return rfind(kvs, key, kvs.size()-1);
}

size_t rfind(const miutil::KeyValue_v& kvs, const std::string& key, size_t start)
{
  if (kvs.empty() || start == size_t(-1) || start >= kvs.size())
    return size_t(-1);
  for (; start != size_t(-1); --start) {
    if (kvs[start].key() == key)
      break;
  }
  return start;
}

std::string mergeKeyValue(const miutil::KeyValue_v& kvs)
{
  std::ostringstream out;
  out << kvs;
  return out.str();
}

#if 0
bool Manager__parseKeyValue(const std::string &str, QString &key, QString &value)
{
  // Split each word into a key=value pair.
  std::vector<std::string> wordPieces = miutil::split_protected(str, '"', '"', "=");
  if (wordPieces.size() == 0 || wordPieces.size() > 2)
    return false;
  else if (wordPieces.size() == 1)
    wordPieces.push_back("");

  key = QString::fromStdString(wordPieces[0]);
  value = QString::fromStdString(wordPieces[1]);
  if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2)
    value = value.mid(1, value.size() - 2);

  return true;
}
void parsingFromDrawingManager()
{
  // Split each input line into a collection of "words".
  std::vector<std::string> pieces = miutil::split_protected(cmd->command(), '"', '"');
  // Skip the first piece ("DRAWING").
  pieces.erase(pieces.begin());

  QVariantMap properties;
  QVariantList points;
  std::vector<std::string>::const_iterator it;

  for (it = pieces.begin(); it != pieces.end(); ++it) {

    QString key, value;
    if (!parseKeyValue(*it, key, value))
      continue;

    // Read the specified file, skipping to the next line if successful,
    // but returning false to indicate an error if unsuccessful.
    if (key == "file" || key == "name")
      toLoad.append(value);
  }
}
#endif

KeyValue_v splitKeyValue(const std::string& infostr, bool keepQuotes)
{
  KeyValue_v opts;
  for (const std::string& s : miutil::split_protected(infostr, QUOTE, QUOTE)) {
    const size_t ieq = s.find("="); // FIXME how should this handle quoted keys?
    if (ieq == std::string::npos) {
      opts.push_back(miutil::KeyValue(s)); // hasValue() == false
    } else {
      size_t vbegin = ieq+1, vend = s.size();
      const bool unquote = (!keepQuotes && ieq+2 < s.size() && s[ieq+1] == QUOTE && s.back() == QUOTE);
      if (unquote) {
        vbegin += 1;
        vend -= 1;
      }
      opts.push_back(miutil::KeyValue(miutil::to_lower(s.substr(0, ieq)),
                                      s.substr(vbegin, vend-vbegin), keepQuotes));
    }
  }
  return opts;
}

std::ostream& operator<<(std::ostream& out, const miutil::KeyValue& kv)
{
  out << kv.key();
  if (kv.hasValue()) {
    out << '=';
    // TODO check if this way of enclosing in '"' is correct
    const bool quote = !kv.keptQuotes() && (kv.value().find(" ") != std::string::npos);
    if (quote)
      out << '"';
    out << kv.value(); // FIXME what if this contains '"' ??? => replace(v, '%', "%25") replace(v, '"', "%22") and reverse in decode
    if (quote)
      out << '"';
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const miutil::KeyValue_v& kvs)
{
  bool first = true;
  for (const miutil::KeyValue& kv : kvs) {
    if (!first)
      out << ' ';
    first = false;

    out << kv;
  }
  return out;
}

std::string extract_option(miutil::KeyValue_v& kvs, const std::string& k)
{
  std::string v;
  while (true) {
    const size_t i = miutil::find(kvs, k);
    if (i != size_t(-1)) {
      v = kvs[i].value();
      kvs.erase(kvs.begin() + i);
    } else {
      break;
    }
  }
  return v;
}

void unique_options(miutil::KeyValue_v& kvs)
{
  KeyValue_v uniq;
  std::set<std::string> keys;
  for (const auto& kv : kvs) {
    if (keys.insert(kv.key()).second) {
      uniq << kvs[miutil::rfind(kvs, kv.key())];
    }
  }
  std::swap(kvs, uniq);
}

} // namespace miutil
