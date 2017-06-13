#ifndef DIKVLISTPLOTCOMMAND_H
#define DIKVLISTPLOTCOMMAND_H

#include "diPlotCommand.h"

#include "util/diKeyValue.h"

class KVListPlotCommand : public PlotCommand
{
public:
  KVListPlotCommand(const std::string& commandKey);

  // FIXME parsing should be done somewhere else
  KVListPlotCommand(const std::string& commandKey, const std::string& command);

  const std::string& commandKey() const override
    { return commandKey_; }

  const std::string toString() const override;

  virtual KVListPlotCommand& add(const std::string& key, const std::string& value);

  virtual KVListPlotCommand& add(const miutil::KeyValue& kv);

  virtual KVListPlotCommand& add(const miutil::KeyValue_v& kvs);

  size_t size() const
    { return keyValueList_.size(); }

  size_t find(const std::string& key, size_t start=0) const;

  size_t rfind(const std::string& key) const;

  size_t rfind(const std::string& key, size_t start) const;

  const std::string& value(size_t idx) const
    { return get(idx).value(); }

  const miutil::KeyValue& get(size_t idx) const
    { return keyValueList_.at(idx); }

  const miutil::KeyValue_v& all() const
    { return keyValueList_; }

  static const size_t npos = static_cast<size_t>(-1);

private:
  std::string commandKey_;

  miutil::KeyValue_v keyValueList_;
};

typedef std::shared_ptr<KVListPlotCommand> KVListPlotCommand_p;
typedef std::shared_ptr<const KVListPlotCommand> KVListPlotCommand_cp;

#endif // DIKVLISTPLOTCOMMAND_H
