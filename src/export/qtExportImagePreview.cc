#include "qtExportImagePreview.h"

#include <QEvent>

#include "export/export_image_preview.ui.h"

ExportImagePreview::ExportImagePreview(QImage image, QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::ExportImagePreview)
  , image_(image)
  , zoom_(100)
{
  ui->setupUi(this);
  ui->buttonZoomIn->setDefaultAction(ui->actionZoomIn);
  ui->buttonZoomOut->setDefaultAction(ui->actionZoomOut);
  ui->buttonZoom100->setDefaultAction(ui->actionZoom100);

  updatePreview();
}

ExportImagePreview::~ExportImagePreview()
{
  delete ui;
}

void ExportImagePreview::onZoomOut()
{
  if (zoom_ > 10) {
    zoom_ -= 10;
    updatePreview();
  }
}

void ExportImagePreview::onZoomIn()
{
  zoom_ += 10;
  updatePreview();
}

void ExportImagePreview::onZoom100()
{
  zoom_ = 100;
  updatePreview();
}

void ExportImagePreview::updatePreview()
{
  QImage scaled = image_.scaled(image_.size() * (zoom_ / 100.0),
                                Qt::KeepAspectRatio, Qt::SmoothTransformation);
  ui->preview->setPixmap(QPixmap::fromImage(scaled));

  ui->labelZoom->setText(tr("Zoom: %1 %").arg(zoom_));

  ui->actionZoomIn->setEnabled(zoom_ > 10);
  ui->actionZoom100->setEnabled(zoom_ != 100);
}

void ExportImagePreview::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  if (e->type() == QEvent::LanguageChange)
    ui->retranslateUi(this);
}
