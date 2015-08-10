
#ifndef QTVCROSSLAYERLIST_H
#define QTVCROSSLAYERLIST_H 1

#include "vcross_v2/VcrossQtManager.h"
#include <QWidget>
#include <QList>

class QStringListModel;
class QListView;

class VcrossLayerList : public QWidget {
  Q_OBJECT;

public:
  VcrossLayerList(QWidget* parent=0);
  void setManager(vcross::QtManager_p vcrossm);

  void selectAll();
  QList<int> selected() const;

Q_SIGNALS:
  void selectionChanged();

private Q_SLOTS:
  // from vcross manager
  void onFieldAdded(int position);
  void onFieldRemoved(int position);

  // from list
  void onPlotSelectionChanged();

private:
  QString fieldText(int position);

private:
  vcross::QtManager_p vcrossm;
  QStringListModel* plots;
  QListView* plotsList;
};

#endif
