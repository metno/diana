#include "fimex_logging.h"

// required to tell fimex to use log4cpp
#include <fimex/Logger.h>
#include <fimex/CDMconstants.h>

#include <miLogger/miLoggingLogger.h>

#if MIFI_VERSION_CURRENT_INT >= MIFI_VERSION_INT(0,63,4)
#define FIMEX_LOGGING_0_63_4 1
#endif

namespace {

#ifdef FIMEX_LOGGING_0_63_4
milogger::Severity levelFimexToMiLogger(MetNoFimex::Logger::LogLevel level)
{
  using namespace MetNoFimex;
  switch (level) {
  case Logger::OFF: return milogger::INVALID;
  case Logger::FATAL: return milogger::FATAL;
  case Logger::ERROR: return milogger::ERROR;
  case Logger::WARN: return milogger::WARN;
  case Logger::INFO: return milogger::INFO;
  case Logger::DEBUG: return milogger::DEBUG;
  }
  return milogger::INVALID;
}

// ========================================================================

class FimexLoggerImpl : public MetNoFimex::LoggerImpl {
public:
  FimexLoggerImpl(const std::string& className)
    : className_(className), tag_(className_.c_str()) { }

  bool isEnabledFor(MetNoFimex::Logger::LogLevel level) /* override */
    { return tag_.isEnabledFor(levelFimexToMiLogger(level)); }

  void log(MetNoFimex::Logger::LogLevel level, const std::string& message, const char* filename, unsigned int lineNumber) /* override */;

private:
  const std::string className_;
  milogger::LoggerTag tag_;
};

void FimexLoggerImpl::log(MetNoFimex::Logger::LogLevel level, const std::string& message, const char* filename, unsigned int lineNumber)
{
  if (milogger::RecordPtr r = tag_.createRecord(levelFimexToMiLogger(level))) {
    r->stream() << message
                << " in " << filename
                << " at line " << lineNumber;
    tag_.submitRecord(r);
  }
}

// ========================================================================

class FimexLoggerClass : public MetNoFimex::LoggerClass {
public:
  MetNoFimex::LoggerImpl* loggerFor(MetNoFimex::Logger* logger, const std::string& className) /* override */
    { remember(logger); return new FimexLoggerImpl(className); }
};
#endif // FIMEX_LOGGING_0_63_4

} // namespace

// ========================================================================

void init_fimex_logging()
{
#ifdef FIMEX_LOGGING_0_63_4
  MetNoFimex::Logger::setClass(new FimexLoggerClass);
#else
  // tell fimex to use log4cpp
  MetNoFimex::Logger::setClass(MetNoFimex::Logger::LOG4CPP);
#endif
}

void finish_fimex_logging()
{
#ifdef FIMEX_LOGGING_0_63_4
  MetNoFimex::Logger::setClass(0);
#endif
}
