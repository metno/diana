
#ifndef QTVCROSSSELECTIONMANAGER_H
#define QTVCROSSSELECTIONMANAGER_H 1

#include "vcross_v2/VcrossQtManager.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <map>
#include <string>
#include <vector>

class CommandParser;

class VcrossSelectionManager : public QObject {
  Q_OBJECT;

public:
  VcrossSelectionManager(vcross::QtManager_p m);

  typedef std::vector<std::string> string_v;
  typedef std::map<std::string, std::string> string_string_m;

  // interface towards GUI

  bool addField(const std::string& model, const std::string& field, const std::string& fieldOpts, int position);
  bool updateField(const std::string& model, const std::string& field, const std::string& fieldOpts);
  bool removeField(const std::string& model, const std::string& field);
  void removeAllFields();

  void setVisibleAt(int position, bool visible);

  int countFields() const;
  const std::string& getFieldAt(int position) const;
  const std::string& getModelAt(int position) const;
  const std::string& getOptionsAt(int position) const;
  bool getVisibleAt(int position) const;

  QStringList allModels();
  QStringList availableFields(const QString& model);

  std::string defaultOptions(const QString& model, const QString& field, bool setupOptions);
  std::string defaultOptions(const std::string& model, const std::string& field, bool setupOptions);

  // interface towards quickmenu / ....

  std::string getShortname();
  string_v getOKString();
  void putOKString(const string_v& vstr, bool vcrossPrefix=true, bool checkOptions=true);

  string_v writeLog();
  void readLog(const string_v& vstr, const std::string& thisVersion, const std::string& logVersion);

Q_SIGNALS:
  void fieldAdded(const std::string& model, const std::string& field, int position);
  void fieldUpdated(const std::string& model, const std::string& field, int position);
  void fieldRemoved(const std::string& model, const std::string& field, int position);
  void fieldsRemoved();

private:
  std::string checkFieldOptions(const std::string& opts, bool vcrossPrefix);

private:
  vcross::QtManager_p vcrossm;
  std::auto_ptr<CommandParser> cp;

  struct SelectedField {
    std::string model;
    std::string field;
    std::string fieldOpts;
    int hourOffset;
    bool visible;
    SelectedField(const std::string& mdl, const std::string& fld, const std::string& opts)
      : model(mdl), field(fld), fieldOpts(opts), hourOffset(0), visible(true) { }
  };
  typedef std::vector<SelectedField> SelectedField_v;
  SelectedField_v selectedFields;

  // map<fieldname,fieldOpts>
  string_string_m setupFieldOptions;
  string_string_m fieldOptions;
};

#endif // QTVCROSSSELECTIONMANAGER_H
