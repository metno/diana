#ifndef EXPORTIMAGEPREVIEW_H
#define EXPORTIMAGEPREVIEW_H

#include <QDialog>
#include <QImage>

namespace Ui {
class ExportImagePreview;
}

class ExportImagePreview : public QDialog
{
  Q_OBJECT

public:
  explicit ExportImagePreview(QImage preview, QWidget *parent = 0);
  ~ExportImagePreview();

protected:
  void changeEvent(QEvent *e);

private Q_SLOTS:
  void onZoomSliderChanged(int);
  void onZoomFit();

private:
  void updatePreview();

private:
  Ui::ExportImagePreview *ui;
  QImage image_;
};

#endif // EXPORTIMAGEPREVIEW_H
