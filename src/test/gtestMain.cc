
#include <gtest/gtest.h>

//#define DEBUG_MESSAGES

#ifdef DEBUG_MESSAGES
#include <log4cpp/Category.hh>
// required to tell fimex to use log4cpp
#include <fimex/Logger.h>
#endif // DEBUG_MESSAGES

#define MILOGGER_CATEGORY "diana.test.main"
#include <miLogger/miLogging.h>

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  milogger::LoggingConfig log4cpp("-.!!=-:");
#ifdef DEBUG_MESSAGES
  MetNoFimex::Logger::setClass(MetNoFimex::Logger::LOG4CPP);
  log4cpp::Category::getRoot().setPriority(log4cpp::Priority::DEBUG);
#endif

  setlocale(LC_NUMERIC, "C");

  return RUN_ALL_TESTS();
}
