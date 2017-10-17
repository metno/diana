
#ifndef DIANA_UTIL_CHARSETS_H
#define DIANA_UTIL_CHARSETS_H 1

#include <memory>
#include <string>

namespace diutil {

class CharsetConverter {
public:
  virtual ~CharsetConverter();
  virtual std::string convert(const std::string& text) = 0;
  virtual void apply(std::string& text);
};

typedef std::shared_ptr<CharsetConverter> CharsetConverter_p;

extern const std::string UTF_8, ISO_8859_1;

const std::string& CHARSET_INTERNAL();
const std::string& CHARSET_WRITE();
const std::string& CHARSET_READ();


extern const CharsetConverter_p NOOP_CONVERTER;

CharsetConverter_p findConverter(const std::string& from, const std::string& to);

/*! Return a converter based on a line like "-*- coding: <charset> -*-". Returns null ptr if no coding line. */
CharsetConverter_p findConverterFromCoding(const std::string& line, const std::string& to, const std::string& comment);

/*! Return a converter based on a line like "-*- coding: <charset> -*-" with a comment prefix '#'. Returns null ptr if no coding line. */
CharsetConverter_p findConverterFromCoding(const std::string& line, const std::string& to);


class GetLineConverter {
public:
  GetLineConverter(const std::string& comment, const std::string& cs_read = CHARSET_READ(), const std::string& cs_internal = CHARSET_INTERNAL());

  std::istream& operator()(std::istream& in, std::string& line);

private:
  std::string comment_;
  std::string cs_internal_;
  diutil::CharsetConverter_p converter_;
};


} // namespace diutil

#endif // DIANA_UTIL_CHARSETS_H
