#ifndef VCROSSPLOTCOMMAND_H
#define VCROSSPLOTCOMMAND_H

#include "diKVListPlotCommand.h"

class VcrossPlotCommand;

typedef std::shared_ptr<VcrossPlotCommand> VcrossPlotCommand_p;
typedef std::shared_ptr<const VcrossPlotCommand> VcrossPlotCommand_cp;

class VcrossPlotCommand : public KVListPlotCommand
{
public:
  enum Type {
    FIELD,
    OPTIONS,
    CROSSECTION,
    CROSSECTION_LONLAT,
    TIMEGRAPH,
    TIMEGRAPH_LONLAT
  };

  VcrossPlotCommand(Type type);

  const std::string toString() const override;
  static VcrossPlotCommand_cp fromString(const std::string& text);

  Type type() const
    { return type_; }

private:
  Type type_;
};

#endif // VCROSSPLOTCOMMAND_H
