/*
 * Test cases for the miString class
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "miSetupParser.h"
#include <gtest/gtest.h>

using miutil::SetupParser;

TEST(miSetupParserTest, splitKeyValue1)
{
  std::string k, v;
  SetupParser::splitKeyValue("key = value", k, v, true);
  EXPECT_EQ("key", k);
  EXPECT_EQ("value", v);

  SetupParser::splitKeyValue("kEy = VaLuE", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ("VaLuE", v);

  SetupParser::splitKeyValue("kEy = VaLuE = verdi=  wert  ", k, v, true);
  EXPECT_EQ("kEy", k);
  EXPECT_EQ("VaLuE = verdi=  wert", v);

  SetupParser::splitKeyValue("kEy = \" VeRdI med luft   \" ", k, v, true);
  EXPECT_EQ("kEy", k);
  EXPECT_EQ(" VeRdI med luft   ", v);

  SetupParser::splitKeyValue("kEy=", k, v, true);
  EXPECT_EQ("kEy", k);
  EXPECT_EQ("", v);

  SetupParser::splitKeyValue("kEy =", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ("", v);

  SetupParser::splitKeyValue("kEy without  value  ", k, v, false);
  EXPECT_EQ("key without  value", k);
  EXPECT_EQ("", v);

  SetupParser::splitKeyValue("key=\"true || false\"", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ("true || false", v);

  SetupParser::splitKeyValue("key=\" true \" || \"false\"", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ(" true ", v);

  SetupParser::splitKeyValue("key=\"\" || false", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ("false", v);

  SetupParser::splitKeyValue("key=\"\" || \" false\"", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ(" false", v);

  SetupParser::splitKeyValue("key= || \"false \"", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ("false ", v);

  SetupParser::splitKeyValue("key= \" || nonsense ", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ("\" || nonsense", v);

  SetupParser::splitKeyValue("key=true ||", k, v, false);
  EXPECT_EQ("key", k);
  EXPECT_EQ("true", v);
}

TEST(miSetupParserTest, substitute)
{
  SetupParser::destroy();
  SetupParser::replaceUserVariables("VERY_LONG_SUBSTITUTION_NAME", "long");
  SetupParser::replaceUserVariables("BAD", "good");
  SetupParser::parse("no such file, as expected"); // only way to load user substitutions

  {
    std::string in_out = "this end is ${BAD}";
    EXPECT_TRUE(SetupParser::instance()->checkEnvironment(in_out));
    EXPECT_EQ("this end is good", in_out);
  }
  {
    std::string in_out = "${BAD} start";
    EXPECT_TRUE(SetupParser::instance()->checkEnvironment(in_out));
    EXPECT_EQ("good start", in_out);
  }
  {
    std::string in_out = "/${VERY_LONG_SUBSTITUTION_NAME}/is/${BAD}";
    EXPECT_TRUE(SetupParser::instance()->checkEnvironment(in_out));
    EXPECT_EQ("/long/is/good", in_out);
  }
}
