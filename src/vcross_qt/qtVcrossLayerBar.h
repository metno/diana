
#ifndef QTVCROSSLAYERBAR_H
#define QTVCROSSLAYERBAR_H 1

#include "vcross_v2/VcrossQtManager.h"
#include <QWidget>

class QAction;
class QMimeData;

class VcrossLayerBar : public QWidget {
  Q_OBJECT;

public:
  VcrossLayerBar(QWidget* parent=0);
  void setManager(vcross::QtManager_p vcrossm);

protected:
  void dragEnterEvent(QDragEnterEvent* event);
  void dragMoveEvent(QDragMoveEvent* event);
  void dropEvent(QDropEvent *event);

Q_SIGNALS:
  void requestStyleEditor(int position);

private:
  bool acceptableDrop(const QDropEvent* event,
      int& drag_position, int& drop_position);
  int fieldIndex(const QPoint& pos);

private Q_SLOTS:
  // from buttons
  void onFieldAction(int position, int action);
  void onStartDrag(int position);

  // from vcross manager
  void onFieldAdded(int position);
  void onFieldRemoved(int position);

private:
  vcross::QtManager_p vcrossm;
};

#endif
