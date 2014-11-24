
#ifndef QTVCROSSLAYERBUTTON_H
#define QTVCROSSLAYERBUTTON_H 1

#include <QToolButton>

class QAction;

class VcrossLayerButton : public QToolButton {
  Q_OBJECT;

public:
  VcrossLayerButton(const QString& model, const QString& field, int position, QWidget* parent=0);

  void setPosition(int position, bool last);

  enum { EDIT, REMOVE, SHOW_HIDE, UP, DOWN };

Q_SIGNALS:
  void triggered(int position, int what);

private Q_SLOTS:
  void onEdit();
  void onRemove();
  void onShowHide();
  void onUp();
  void onDown();

private:
  int position;
  QAction* actionShowHide;
  QAction* actionUp;
  QAction* actionDown;
};

#endif
