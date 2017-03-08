
#include "charsets.h"

#include "string_util.h"

#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <stdexcept>

#include <errno.h>
#include <iconv.h>

#define MILOGGER_CATEGORY "diana.charsets"
#include <miLogger/miLogging.h>

namespace diutil {

std::string convertLatin1ToUtf8(const std::string& latin1)
{
  std::string utf8;
  utf8.reserve(latin1.size());
  for (size_t i = 0; i<latin1.size(); ++i) {
    unsigned char uch = static_cast<unsigned char>(latin1[i]);
    if ((uch & 0x80) != 0) {
      unsigned char uch1 = (0xc0 | (uch >> 6)); /* first byte, simplified since our range is only 8-bits */
      utf8 += static_cast<char>(uch1);

      uch = (0x80 | (uch & 0x3f));
    }
    utf8 += static_cast<char>(uch);
  }
  return utf8;
}

const std::string UTF_8 = "utf-8", ISO_8859_1 = "iso-8859-1";

const std::string& CHARSET_INTERNAL() { return ISO_8859_1; }
const std::string& CHARSET_WRITE()    { return UTF_8; }
const std::string& CHARSET_READ()     { return ISO_8859_1; }

CharsetConverter::~CharsetConverter()
{
}

void CharsetConverter::apply(std::string& text)
{
  text = convert(text);
}

class NoopConverter : public CharsetConverter {
public:
  std::string convert(const std::string& in);
  void apply(std::string& text);
};

std::string NoopConverter::convert(const std::string& in)
{
  return in;
}

void NoopConverter::apply(std::string&)
{
}

const CharsetConverter_p NOOP_CONVERTER(new NoopConverter);


class Latin1Utf8Converter : public CharsetConverter {
public:
  std::string convert(const std::string& in);
};

std::string Latin1Utf8Converter::convert(const std::string& in)
{
  return diutil::convertLatin1ToUtf8(in);
}


class IconvConverter : public CharsetConverter {
public:
  IconvConverter(const std::string& from, const std::string& to);
  ~IconvConverter();
  std::string convert(const std::string& in);
private:
  iconv_t handle_;
};

IconvConverter::IconvConverter(const std::string& from, const std::string& to)
  : handle_(iconv_open(to.c_str(), from.c_str()))
{
  if (handle_ == iconv_t(-1))
    throw std::runtime_error("no iconv converter from '" + from + "' to '" + to + "'");
}

IconvConverter::~IconvConverter()
{
  iconv_close(handle_);
}

std::string IconvConverter::convert(const std::string& in)
{
  iconv(handle_, 0, 0, 0, 0);

  const size_t in_size = in.size();
  char* in_c = new char[in_size], *in_p = in_c;
  memcpy(in_c, in.c_str(), in_size);

  const size_t out_size = in_size + 4;
  char* out_c = new char[out_size], *out_p = out_c;
  size_t in_remain = in_size, out_remain = out_size;

  std::string out;
  while (true) {
    const size_t i = iconv(handle_, &in_p, &in_remain, &out_p, &out_remain);
    if (i != size_t(-1))
      break;

    out.insert(out.end(), out_c, out_p);
    out_p = out_c;
    out_remain = out_size;
    if (errno == E2BIG) {
      // not enough output space; keep going
    } else if (errno == EILSEQ) {
      // invalid input; hmm, just skip one byte?
      in_p++;
      in_remain--;
      out += "?";
    } else if (errno == EINVAL) {
      // incomplete input
      out += "?";
      break;
    }
  }

  out.insert(out.end(), out_c, out_p);

  delete[] in_c;
  delete[] out_c;

  return out;
}


static bool isLatin1(const std::string& cs)
{
  return (cs == ISO_8859_1 || cs == "ISO-8859-1" || cs == "latin-1" || cs == "latin1");
}

bool isUtf8(const std::string& cs)
{
  return (cs == UTF_8 || cs == "utf8" || cs == "UTF-8" || cs == "UTF8");
}

CharsetConverter_p findConverter(const std::string& from, const std::string& to)
{
  try {
    if (from == to)
      return NOOP_CONVERTER;

    const bool from_latin1 = isLatin1(from);
    const bool to_latin1 = isLatin1(to);
    const bool from_utf8 = isUtf8(from);
    const bool to_utf8 = isUtf8(to);

    if ((from_latin1 && to_latin1) || (from_utf8 && to_utf8))
      return boost::make_shared<NoopConverter>();

    if (from_latin1 && to_utf8)
      return boost::make_shared<Latin1Utf8Converter>();

    return boost::make_shared<IconvConverter>(from, to);
  } catch (std::exception& ex) {
    METLIBS_LOG_ERROR("no converter from '" << from << "' to '" << to << "': " << ex.what());
    return NOOP_CONVERTER;
  }
}

static const boost::regex RE_CODING("-\\*- +coding: +([a-zA-Z0-9-]+) +-\\*-");

CharsetConverter_p findConverterFromCoding(const std::string& line, const std::string& to, const std::string& comment)
{
  if (diutil::startswith(line, comment)) {
    boost::smatch what;
    if (boost::regex_search(line, what, RE_CODING)) {
      const std::string& from = what.str(1);
      return diutil::findConverter(from, to);
    }
  }
  return CharsetConverter_p();
}

CharsetConverter_p findConverterFromCoding(const std::string& line, const std::string& to)
{
  return findConverterFromCoding(line, to, "#");
}

GetLineConverter::GetLineConverter(const std::string& comment, const std::string& cs_read, const std::string& cs_internal)
  : comment_(comment)
  , cs_internal_(cs_internal)
  , converter_(diutil::findConverter(cs_read, cs_internal_))
{
}

std::istream& GetLineConverter::operator()(std::istream& in, std::string& line)
{
  if (std::getline(in, line)) {
    if (diutil::CharsetConverter_p c = diutil::findConverterFromCoding(line, cs_internal_, comment_))
      converter_ = c;
    converter_->apply(line);
  }
  return in;
};

} // namespace diutil
