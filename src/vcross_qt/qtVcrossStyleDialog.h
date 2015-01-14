
#ifndef VcrossStyleDialog_h
#define VcrossStyleDialog_h 1

#include "vcross_v2/VcrossQtManager.h"

#include <QDialog>
#include <QStandardItemModel>

class Ui_VcrossStyleDialog;

class VcrossStyleDialog : public QDialog {
  Q_OBJECT;

public:
  VcrossStyleDialog(QWidget* parent);
  void setManager(vcross::QtManager_p vcrossm);

  void showModelField(const QString& mdl, const QString& fld);

private:
  void setupUi();
  QString modelName(int index);
  QString fieldName(int index);
  void enableWidgets();

private Q_SLOTS:
  void onFieldAdded(const std::string& model, const std::string& field, int position);
  void onFieldUpdated(const std::string& model, const std::string& field, int position);
  void onFieldRemoved(const std::string& model, const std::string& field, int position);

  void slotSelectedPlotChanged(int index);
  void slotResetPlotOptions();
  void slotApply();

private:
  vcross::QtManager_p vcrossm;
  std::auto_ptr<Ui_VcrossStyleDialog> ui;
  QStandardItemModel* mPlots;
};

#endif // VcrossStyleDialog_h
