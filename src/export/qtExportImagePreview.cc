#include "qtExportImagePreview.h"

#include <QEvent>

#include "ui_export_image_preview.h"

ExportImagePreview::ExportImagePreview(QImage image, QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::ExportImagePreview)
  , image_(image)
{
  ui->setupUi(this);

  updatePreview();
}

ExportImagePreview::~ExportImagePreview()
{
  delete ui;
}

void ExportImagePreview::onZoomFit()
{
  float scaleX = ui->scroll->viewport()->width() / (float)image_.width();
  float scaleY = ui->scroll->viewport()->height() / (float)image_.height();
  int zoom = (int)(100 * std::min(scaleX, scaleY));
  if (zoom < 10)
    zoom = 10;
  else if (zoom > 100)
    zoom = 100;
  ui->sliderZoom->setValue(zoom);
  updatePreview();
}

void ExportImagePreview::onZoomSliderChanged(int)
{
  updatePreview();
}

void ExportImagePreview::updatePreview()
{
  int zoom = ui->sliderZoom->value();
  QImage scaled = image_.scaled(image_.size() * (zoom / 100.0), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  ui->preview->setPixmap(QPixmap::fromImage(scaled));
  ui->labelZoomValue->setText(QString("%1 %").arg(zoom));
}

void ExportImagePreview::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  if (e->type() == QEvent::LanguageChange)
    ui->retranslateUi(this);
}
