#ifndef DILABELPLOTCOMMAND_H
#define DILABELPLOTCOMMAND_H

#include "diKVListPlotCommand.h"

class LabelPlotCommand : public KVListPlotCommand
{
public:
  LabelPlotCommand();

  //! text without "LABEL" prefix
  LabelPlotCommand(const std::string& text);

  virtual LabelPlotCommand& add(const std::string& key, const std::string& value);

  virtual LabelPlotCommand& add(const miutil::KeyValue& kv);

  virtual LabelPlotCommand& add(const miutil::KeyValue_v& kvs);
};

typedef std::shared_ptr<LabelPlotCommand> LabelPlotCommand_p;
typedef std::shared_ptr<const LabelPlotCommand> LabelPlotCommand_cp;

#endif // DILABELPLOTCOMMAND_H
