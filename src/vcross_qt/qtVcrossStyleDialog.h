
#ifndef VcrossStyleDialog_h
#define VcrossStyleDialog_h 1

#include "diVcrossSelectionManager.h"

#include <QDialog>
#include <QStandardItemModel>

class Ui_VcrossStyleDialog;

class VcrossStyleDialog : public QDialog {
  Q_OBJECT;

public:
  VcrossStyleDialog(QWidget* parent);
  void setSelectionManager(VcrossSelectionManager* vsm);

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
  void onFieldsRemoved();

  void slotSelectedPlotChanged(int index);
  void slotResetPlotOptions();
  void slotApply();

private:
  VcrossSelectionManager* selectionManager;
  std::auto_ptr<Ui_VcrossStyleDialog> ui;
  QStandardItemModel* mPlots;
};

#endif // VcrossStyleDialog_h
