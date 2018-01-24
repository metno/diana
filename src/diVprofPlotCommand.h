#ifndef VPROFPLOTCOMMAND_H
#define VPROFPLOTCOMMAND_H

#include "diKVListPlotCommand.h"

class VprofPlotCommand;

typedef std::shared_ptr<VprofPlotCommand> VprofPlotCommand_p;
typedef std::shared_ptr<const VprofPlotCommand> VprofPlotCommand_cp;

class VprofPlotCommand : public KVListPlotCommand
{
public:
  enum Type {
    OPTIONS,
    STATION,
    MODELS
  };

  VprofPlotCommand(Type type);

  const std::string toString() const override;
  static VprofPlotCommand_cp fromString(const std::string& text);

  Type type() const
    { return type_; }

  const std::vector<std::string>& items() const
    { return items_; }

  void setItems(const std::vector<std::string>& items)
    { items_ = items; }

private:
  Type type_;
  std::vector<std::string> items_; //! models or stations
};

#endif // VPROFPLOTCOMMAND_H
