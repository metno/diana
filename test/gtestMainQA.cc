
#include <QApplication>

#include <gtest/gtest.h>

#include "util/fimex_logging.h"
#include <miLogger/miLoggingSimple.h>

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  FimexLoggingAdapter fla;
  milogger::simple::SimpleSystemPtr sp = std::make_shared<milogger::simple::SimpleSystem>();
  // sp->setThreshold(milogger::DEBUG);
  sp->configure("");
  milogger::system::selectSystem(sp);

  QApplication app(argc, argv);
  setlocale(LC_NUMERIC, "C");

  return RUN_ALL_TESTS();
}
