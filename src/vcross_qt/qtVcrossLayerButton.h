
#ifndef QTVCROSSLAYERBUTTON_H
#define QTVCROSSLAYERBUTTON_H 1

#include "vcross_v2/VcrossQtManager.h"
#include <QToolButton>

class QAction;

class VcrossLayerButton : public QToolButton {
  Q_OBJECT;

public:
  VcrossLayerButton(vcross::QtManager_p vcm, int position, QWidget* parent=0);

  enum { EDIT, REMOVE, SHOW_HIDE, UP, DOWN };

Q_SIGNALS:
  void triggered(int position, int what);

private Q_SLOTS:
  void onEdit();
  void onRemove();
  void onShowHide();
  void onUp();
  void onDown();

  void onFieldAdded(int position);
  void onFieldRemoved(int position);
  void onFieldOptionsChanged(int position);
  void onFieldVisibilityChanged(int position);

private:
  void enableUpDown();
  void updateStyle();

private:
  vcross::QtManager_p vcrossm;
  int position;
  QAction* actionShowHide;
  QAction* actionUp;
  QAction* actionDown;
};

#endif
