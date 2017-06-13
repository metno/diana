#ifndef DIKEYVALUE_H
#define DIKEYVALUE_H

#include <iosfwd>
#include <string>
#include <vector>

namespace miutil {

class KeyValue {
public:
  KeyValue();
  explicit KeyValue(const std::string& k);
  explicit KeyValue(const std::string& k, const std::string& v, bool keptQuotes=false);

  const std::string& key() const
    { return mKey; }
  const std::string& value() const
    { return mValue; }

  bool hasValue() const
    { return mHasValue; }
  bool keptQuotes() const
    { return mKeptQuotes; }

  int toInt(bool& ok, int def=0) const;
  int toInt(int def=0) const
    { bool ok; return toInt(ok, def); }

  double toDouble(bool& ok, double def=0) const;
  double toDouble(double def=0) const
    { bool ok; return toDouble(ok, def); }

  float toFloat(bool& ok, float def=0) const;
  float toFloat(float def=0) const
    { bool ok; return toFloat(ok, def); }

  bool toBool(bool& ok, bool def=false) const;
  bool toBool(bool def=false) const
    { bool ok; return toBool(ok, def); }

  bool operator==(const KeyValue& other) const
    { return mKey == other.mKey && mValue == other.mValue && mHasValue == other.mHasValue; }

private:
  std::string mKey, mValue;
  bool mHasValue, mKeptQuotes;
};

typedef std::vector<KeyValue> KeyValue_v;

void add(KeyValue_v& ostr, const std::string& key, bool value);
void add(KeyValue_v& ostr, const std::string& key, int value);
void add(KeyValue_v& ostr, const std::string& key, float value);
void add(KeyValue_v& ostr, const std::string& key, const std::string& value);
void add(KeyValue_v& ostr, const std::string& key, const char* value);
void add(KeyValue_v& ostr, const std::string& key, const std::vector<float>& values);
void add(KeyValue_v& ostr, const std::string& key, const std::vector<std::string>& values);

size_t find(const KeyValue_v& kvs, const std::string& key, size_t start=0);
size_t rfind(const KeyValue_v& kvs, const std::string& key);
size_t rfind(const KeyValue_v& kvs, const std::string& key, size_t start);

std::string mergeKeyValue(const miutil::KeyValue_v& kvs);
KeyValue_v splitKeyValue(const std::string& kvtext, bool keepQuotes=false);

std::ostream& operator<<(std::ostream& out, const miutil::KeyValue& kv);
std::ostream& operator<<(std::ostream& out, const miutil::KeyValue_v& kvs);

} // namespace miutil

#endif // DIKEYVALUE_H
