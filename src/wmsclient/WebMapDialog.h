
#ifndef WebMapDialog_h
#define WebMapDialog_h 1

#include "qtDataDialog.h"
#include <memory>

class QSortFilterProxyModel;
class QStringListModel;
class Ui_WebMapDialog;

class WebMapLayer;
class WebMapService;

class WebMapDialog : public DataDialog
{
  Q_OBJECT;

public:
  WebMapDialog(QWidget *parent, Controller *ctrl);
  ~WebMapDialog();

  std::string name() const;
  void updateDialog();
  std::vector<std::string> getOKString();
  void putOKString(const std::vector<std::string>& vstr);

public /*Q_SLOTS*/:
  void updateTimes();

private Q_SLOTS:
  void onServiceRefreshStarting();
  void onServiceRefreshFinished();

  void onAddServicesFilter(const QString&);
  void onAddLayersFilter(const QString&);

  void checkAddComplete();
  void onAddNext();
  void onAddBack();
  void onAddRestart();

  void onModifyApply();
  void onModifyLayerSelected();

private:
  void setupUi();

  void initializeAddServicePage(bool forward);
  bool isAddServiceComplete();
  WebMapService* selectedAddService() const;

  void initializeAddLayerPage(bool forward);
  bool isAddLayerComplete();
  const WebMapLayer* selectedAddLayer() const;

  void updateAddLayers();
  void addSelectedLayer();

private:
  enum { AddServicePage, AddLayerPage };

  std::auto_ptr<Ui_WebMapDialog> ui;

  QStringListModel* mServicesModel;
  QSortFilterProxyModel* mServicesFilter;

  QStringListModel* mLayersModel;
  QSortFilterProxyModel* mLayersFilter;

  WebMapService* mAddSelectedService;

  std::vector<std::string> mOk;
};

#endif // WebMapDialog_h
