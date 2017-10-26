/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2016 MET Norway

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "qtExportImageDialog.h"

#include "miSetupParser.h"
#include "qtMainWindow.h"
#include "export/MovieMaker.h"
#include "export/qtExportImagePreview.h"
#include "export/qtTempDir.h"
#include "util/subprocess.h"

#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStringListModel>

#include <cmath>

#include "export/export_image_dialog.ui.h"

#define MILOGGER_CATEGORY "diana.ExportImageDialog"
#include <miLogger/miLogging.h>

namespace {
enum SizeType { Size_Any, Size_Fixed, Size_AspectRatio };

struct SizeSpec {
  QString label;
  SizeType type;
  QSize size;
  SizeSpec(const QString& l, SizeType t, QSize s)
    : label(l), type(t), size(s) { }
};
typedef QList<SizeSpec> SizeSpecs;

const SizeSpecs movieSizes = SizeSpecs()
    << SizeSpec("1280x720 (HD)", Size_Fixed, QSize(1280,720))
    << SizeSpec("800x600", Size_Fixed, QSize(800,600));

const SizeSpecs imageSizes = SizeSpecs()
    << SizeSpec("as Diana", Size_Fixed, QSize(-1,-1))
    << movieSizes
    << SizeSpec("4:3 (TV)", Size_AspectRatio, QSize(4,3))
    << SizeSpec("16:9 (wide TV)", Size_AspectRatio, QSize(16,9))
    << SizeSpec("Custom", Size_Any, QSize(600,400));

static const char * const dummyFilenameHints[] = {
  QT_TRANSLATE_NOOP("ExportImageDialog", "E.g. diana.png or diana.pdf"),
  QT_TRANSLATE_NOOP("ExportImageDialog", "E.g. diana_%1.png or diana_%1.pdf"),
  QT_TRANSLATE_NOOP("ExportImageDialog", "E.g. diana.gif"),
  QT_TRANSLATE_NOOP("ExportImageDialog", "E.g. diana.avi"),
  0
};

enum Product {
  PRODUCT_IMAGE,
  PRODUCT_IMAGE_SERIES,
  PRODUCT_IMAGE_ANIMATION,
  PRODUCT_MOVIE
};

typedef QList<Product> Products;

bool isValidProduct(int p)
{
  return p >= PRODUCT_IMAGE && p <= PRODUCT_MOVIE;
}

struct ExportCommand {
  QString label;
  QString tooltip;
  QString command;
  Products supportedProducts;
  SizeSpecs sizes;

  bool addProduct(const std::string& ps);
  bool addSize(const std::string& sz)
    { return addSize(QString::fromStdString(sz)); }
  bool addSize(const QString& sz);
};

bool ExportCommand::addProduct(const std::string& ps)
{
  Product p = PRODUCT_IMAGE;
  if (ps == "IMAGE_ANIMATION")
    p = PRODUCT_IMAGE_ANIMATION;
  else if (ps == "IMAGE_SERIES")
    p = PRODUCT_IMAGE_SERIES;
  else if (ps == "MOVIE")
    p = PRODUCT_MOVIE;
  else if (ps != "IMAGE")
    return false;
  supportedProducts << p;
  return true;
};

bool ExportCommand::addSize(const QString& sis)
{
  static const QRegExp reFixedAspect("^(\\d+)([x:])(\\d+)");

  if (sis == "DIANA")
    sizes << imageSizes.front();
  else if (sis == "ANY")
    sizes << imageSizes.back();
  else if (reFixedAspect.indexIn(sis) >= 0) {
    int w = reFixedAspect.cap(1).toInt();
    int h = reFixedAspect.cap(3).toInt();
    bool aspect = (reFixedAspect.cap(2) == ":");
    sizes << SizeSpec(sis, aspect ? Size_AspectRatio : Size_Fixed, QSize(w, h));
  } else {
    return false;
  }
  return true;
}

typedef QList<ExportCommand> ExportCommands;

ExportCommands parseCommands(QStringList commands)
{
  ExportCommands ecs;
  foreach(const QString& c, commands) {
    if (!c.startsWith("SAVETO "))
      continue;
    ExportCommand ec;
    QString products, sizes;
    for (const miutil::KeyValue& kv : miutil::splitKeyValue(c.mid(7).toStdString())) {
      const std::string& key = kv.key();
      const QString value = QString::fromStdString(kv.value());
      if (key == "title")
        ec.label = value;
      else if (key == "tooltip")
        ec.tooltip = value;
      else if (key == "command")
        ec.command = value;
      else if (key == "products") {
        products = value;
      } else if (key == "sizes") {
        sizes = value;
      } else if (key == "product") {
        ec.addProduct(kv.value());
      } else if (key == "size") {
        ec.addSize(kv.value());
      }
    }
    if (!products.isEmpty()) {
      for (const QString& ps : products.split(","))
        ec.addProduct(ps.toStdString());
    }
    if (!sizes.isEmpty()) {
      for (const QString& sis : sizes.split(","))
        ec.addSize(sis);
    }
    ecs << ec;
  }
  return ecs;
}

const char SECTION[] = "EXPORT_COMMANDS";
const char* IMAGE_FILE_SUFFIXES[] = { "png", "jpeg", "jpg", "xpm", "bmp", "svg" };

class ExportCommandsModel : public QAbstractListModel {
public:
  ExportCommandsModel(const ExportCommands* commands, QObject* parent)
    : QAbstractListModel(parent), commands_(commands) { }

  int rowCount(const QModelIndex&) const
    { return commands_->size(); }

  QVariant data(const QModelIndex& index, int role) const;

private:
  const ExportCommands* commands_;
};

QVariant ExportCommandsModel::data(const QModelIndex& index, int role) const
{
  const int i = index.row();
  if (i >= 0 && i < commands_->size()) {
    const ExportCommand& ec = commands_->at(i);
    if (role == Qt::DisplayRole)
      return ec.label;
    else if (role == Qt::ToolTipRole)
      return ec.tooltip;
  }
  return QVariant();
}

class SizesModel : public QAbstractListModel {
public:
  SizesModel(const SizeSpecs* sizes, QObject* parent)
    : QAbstractListModel(parent), sizes_(sizes) { }

  int rowCount(const QModelIndex&) const
    { return sizes_->size(); }

  QVariant data(const QModelIndex& index, int role) const;

private:
  const SizeSpecs* sizes_;
};

QVariant SizesModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::DisplayRole) {
    const int i = index.row();
    return sizes_->at(i).label;
  }
  return QVariant();
}

} // namespace


struct P_ExportImageDialog {
  SizeSpecs sizes;
  ExportCommands ecs;
  int dianaWidth;
  int dianaHeight;
};

ExportImageDialog::ExportImageDialog(DianaMainWindow *parent)
  : QDialog(parent)
  , ui(new Ui_ExportImageDialog)
  , p(new P_ExportImageDialog)
  , mw(parent)
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> lines;
  if (miutil::SetupParser::getSection(SECTION, lines)) {
    QStringList cmds;
    Q_FOREACH(const std::string& line, lines) {
      cmds << QString::fromStdString(line);
    }
    p->ecs = parseCommands(cmds);
  }
  if (p->ecs.isEmpty()) {
    ExportCommand ec;
    ec.label = tr("File(s)");
    ec.sizes = imageSizes;
    ec.supportedProducts = Products()
        << PRODUCT_IMAGE
        << PRODUCT_IMAGE_SERIES
        << PRODUCT_IMAGE_ANIMATION
        << PRODUCT_MOVIE;
    p->ecs << ec;
  }

  setupUi();
}

ExportImageDialog::~ExportImageDialog()
{
}

void ExportImageDialog::setupUi()
{
  METLIBS_LOG_SCOPE();
  ui->setupUi(this);
  // ui->comboProduct->model() must match 'Products' enum; done via ui file

  ExportCommandsModel* modelExports = new ExportCommandsModel(&p->ecs, this);
  ui->comboSaveTo->setModel(modelExports);
  updateFilenameHint();
}

void ExportImageDialog::onProductChanged(int)
{
  bool animation = isAnimation();
  ui->spinFrameRate->setEnabled(animation);
  updateComboSize();
  enableStartButton();
  updateFilenameHint();
}

void ExportImageDialog::onSaveToChanged(int current)
{
  bool saveToFile = false;
  if (current >= 0) {
    const ExportCommand& ec = p->ecs[current];
    saveToFile = ec.command.isEmpty();
    ui->comboSaveTo->setToolTip(ec.tooltip);
  } else {
    ui->comboSaveTo->setToolTip(QString());
  }
  ui->editFilename->setEnabled(saveToFile);
  ui->buttonFilename->setEnabled(saveToFile);
  updateComboSize();
  enableStartButton();
}

void ExportImageDialog::onFilenameChanged(QString)
{
  enableStartButton();
}

void ExportImageDialog::onFileChooser()
{
  const Product product = static_cast<Product>(ui->comboProduct->currentIndex());
  QString filetypes, title;
  switch (product) {
  case PRODUCT_IMAGE:
  case PRODUCT_IMAGE_SERIES:
    if (product == PRODUCT_IMAGE)
      title = tr("Select image filename");
    else
      title = tr("Select image filename pattern");
    filetypes = tr("Images") + " (";
    for (size_t i=0; i<sizeof(IMAGE_FILE_SUFFIXES)/sizeof(IMAGE_FILE_SUFFIXES[0]); ++i) {
      if (i>0)
        filetypes += " ";
      filetypes += "*.";
      filetypes += IMAGE_FILE_SUFFIXES[i];
    }
    filetypes += ");;" + tr("PDF Files") + " (*.pdf)";
    break;
  case PRODUCT_IMAGE_ANIMATION:
    title = tr("Select animated image filename");
    filetypes = tr("Images") + " (*.gif)";
    break;
  case PRODUCT_MOVIE:
    title = tr("Select movie filename");
    filetypes = tr("Movies") + " (*.mp4 *.mpg *.avi)";
    break;
  }
  filetypes += ";;" + tr("All") + " (*.*)";

  const QString fn = QFileDialog::getSaveFileName(this, title, ui->editFilename->text(), filetypes);
  if (!fn.isEmpty())
    ui->editFilename->setText(fn);
}

void ExportImageDialog::onSizeComboChanged(int current)
{
  const SizeSpec& size = p->sizes.at(current);
  bool editing = (size.type != Size_Fixed);
  int nw=0, nh=0;
  const int sw = size.size.width(), sh = size.size.height();
  if (size.type == Size_Fixed) {
    if (size.size.isValid()) {
      nw = sw;
      nh = sh;
    } else {
      nw = p->dianaWidth;
      nh = p->dianaHeight;
    }
  } else if (size.type == Size_AspectRatio) {
    nw = 1280;
    nh = static_cast<int>(round(nw * static_cast<double>(sh) / static_cast<double>(sw)));
    ui->spinWidth->setSingleStep(sw);
    ui->spinHeight->setSingleStep(sh);
  } else {
    ui->spinWidth->setSingleStep(10);
    ui->spinHeight->setSingleStep(10);
  }

  ui->spinWidth->setValue(nw);
  ui->spinHeight->setValue(nh);
  ui->spinWidth->setEnabled(editing);
  ui->spinHeight->setEnabled(editing);

  onSizeWidthChanged(0);
}

void ExportImageDialog::onSizeWidthChanged(int)
{
  const SizeSpec& size = p->sizes.at(ui->comboSize->currentIndex());
  if (size.type == Size_AspectRatio) {
    double h = ui->spinWidth->value()
        * static_cast<double>(size.size.height()) / static_cast<double>(size.size.width());
    ui->spinHeight->setValue(static_cast<int>(round(h)));
  }
}

void ExportImageDialog::onSizeHeightChanged(int)
{
  const SizeSpec& size = p->sizes.at(ui->comboSize->currentIndex());
  if (size.type == Size_AspectRatio) {
    double w = ui->spinHeight->value()
        * static_cast<double>(size.size.width()) / static_cast<double>(size.size.height());
    ui->spinWidth->setValue(static_cast<int>(round(w)));
  }
}

void ExportImageDialog::onPreview()
{
  QImage image(exportSize(), QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  mw->paintOnDevice(&image, false);

  ExportImagePreview preview(image, this);
  preview.exec();
}

void ExportImageDialog::onStart()
{
  const Product product = static_cast<Product>(ui->comboProduct->currentIndex());
  const int saveTo = ui->comboSaveTo->currentIndex();
  if (saveTo < 0)
    return;
  const ExportCommand& ec = p->ecs[saveTo];
  const bool saveToFile = (ec.command.isEmpty());
  const QSize size = exportSize();

  TempDir tmp;
  QString filename;
  QStringList filenames;
  if (saveToFile) {
    filename = ui->editFilename->text();
    if (!checkFilename(filename)) {
      ui->editFilename->setText(filename);
      QMessageBox::warning(this, tr("Warning"), tr("Invalid filename has been changed, please check and press start again."));
      return;
    }
  } else {
    if (!tmp.create()) {
      QMessageBox::warning(this, tr("Error"), tr("Could not create temporary directory."));
      return;
    }
    // TODO create temp filename
    switch (product) {
    case PRODUCT_MOVIE: {
      filename = tmp.filePath("diana.avi");
      break; }
    case PRODUCT_IMAGE_ANIMATION: {
      filename = tmp.filePath("diana.gif");
     break; }
    case PRODUCT_IMAGE_SERIES: {
      filename = tmp.filePath("diana_%1.png");
      break; }
    case PRODUCT_IMAGE: {
      filename = tmp.filePath("diana.png");
      break; }
    }
  }

  if (product == PRODUCT_IMAGE) {
    // check or add image/pdf extension
    mw->saveRasterImage(filename, size);
    QMessageBox::information(this, tr("Done"), tr("Image saved."));
    filenames << filename;
  } else {
    const QFileInfo fi(filename);
    const QString suffix = fi.suffix().toLower();
    QString format;
    switch (product) {
    case PRODUCT_MOVIE: {
      format = "mpg";
      if (suffix == "avi") {
        format = "avi";
      } else if (suffix == "mp4") {
        format = "mp4";
      }
      break; }
    case PRODUCT_IMAGE_ANIMATION:
      format = "animated_gif";
      break;
    case PRODUCT_IMAGE_SERIES:
      format = "png_series";
      break;
    case PRODUCT_IMAGE:
      break; // handled in "if" before switch statement
    }
    MovieMaker mm(filename, format, ui->spinFrameRate->value(), size);
    mw->saveAnimation(mm);
    filenames = mm.outputFiles();
  }

  if (!saveToFile) {
    QStringList args;

    switch (product) {
    case PRODUCT_MOVIE: {
      args << "-movie";
      break; }
    case PRODUCT_IMAGE_ANIMATION: {
      args << "-image-animation";
     break; }
    case PRODUCT_IMAGE_SERIES: {
      args << "-image-series";
      break; }
    case PRODUCT_IMAGE: {
      args << "-image";
      break; }
    }

    args << "-tmpdir" << tmp.dir().absolutePath();

    args << "--" << filenames;

    if (diutil::execute(ec.command, args) == 0)
      tmp.nodestroy();
  }

  // ~TempDir will remove temporary dir if necessary
}

bool ExportImageDialog::checkFilename(QString& filename)
{
  if (filename.isEmpty())
    return false;

  const QFileInfo fi(filename);
  if (!fi.dir().exists())
    return false;

  const QString suffix = fi.suffix().toLower();
  bool filename_ok = true;

  const Product product = static_cast<Product>(ui->comboProduct->currentIndex());
  switch (product) {
    case PRODUCT_MOVIE:
      if (suffix != "avi" && suffix != "mp4" && suffix != "mpg") {
        filename += ".mpg";
        filename_ok = false;
      }
      break;
    case PRODUCT_IMAGE_ANIMATION:
      // check or add gif extension
      if (suffix != "gif") {
        filename += ".gif";
        filename_ok = false;
      }
      break;
    case PRODUCT_IMAGE_SERIES:
      // check for pattern; if missing: add, dialog, return and ask for new start
      if (filename.indexOf("%1") == -1) {
        filename  = fi.fileName() + "_%1" + fi.suffix();
        filename_ok = false;
      }
      // fallthrough
    case PRODUCT_IMAGE: {
      bool found = (suffix == "pdf");
      for (size_t i=0; !found && i<sizeof(IMAGE_FILE_SUFFIXES)/sizeof(IMAGE_FILE_SUFFIXES[0]); ++i) {
        if (suffix == IMAGE_FILE_SUFFIXES[i])
          found = true;
      }
      if (!found) {
        filename += ".png";
        filename_ok = false;
      }
      break; }
  }
  return filename_ok;
}

void ExportImageDialog::updateFilenameHint()
{
  const int saveTo = ui->comboSaveTo->currentIndex();
  const ExportCommand& ec = p->ecs[saveTo];
  const bool saveToFile = ec.command.isEmpty();
  const int product = ui->comboProduct->currentIndex();
  if (saveToFile && isValidProduct(product)) {
    ui->labelFilenameHint->setText(tr(dummyFilenameHints[product]));
    ui->labelFilenameHint->show();
  } else {
    ui->labelFilenameHint->hide();
  }
}

void ExportImageDialog::enableStartButton()
{
  bool enable = false;
  const Product product = static_cast<Product>(ui->comboProduct->currentIndex());
  const int saveTo = ui->comboSaveTo->currentIndex();
  if (product >= 0 && saveTo >= 0) {
    const ExportCommand& ec = p->ecs[saveTo];
    enable = ec.supportedProducts.isEmpty()
        || ec.supportedProducts.contains(product);
    const bool saveToFile = ec.command.isEmpty();
    if (saveToFile) {
      QString filename = ui->editFilename->text();
      if (!checkFilename(filename))
        enable = false;
    }
  }
  ui->buttonStart->setEnabled(enable);
}

void ExportImageDialog::updateComboSize()
{
  p->sizes.clear();
  const int saveTo = ui->comboSaveTo->currentIndex();
  if (saveTo >= 0) {
    const SizeSpecs& sizes = (ui->comboProduct->currentIndex() == PRODUCT_MOVIE)
        ? movieSizes : imageSizes;
    const ExportCommand& ec = p->ecs[saveTo];
    if (ec.sizes.isEmpty()) {
      p->sizes = sizes;
    } else {
      foreach (const SizeSpec& es, ec.sizes) {
        bool accept = false;
        foreach (const SizeSpec& sis, sizes) {
          if (sis.type == Size_Any)
            accept = true;
          else if (es.type == Size_Fixed && sis.type == Size_Fixed && es.size == sis.size)
            accept = true;
          else if (es.type == Size_AspectRatio || sis.type == Size_AspectRatio) {
            const double rw = es.size.width() / static_cast<double>(sis.size.width());
            const double rh = es.size.height() / static_cast<double>(sis.size.height());
            if (std::abs(rw - rh) < 1e-6)
              accept = true;
          }
          if (accept)
            break;
        }
        if (accept)
          p->sizes << es;
      }
    }
  }
  SizesModel* modelSizes = new SizesModel(&p->sizes, ui->comboSize);
  ui->comboSize->setModel(modelSizes);
  if (!p->sizes.isEmpty())
    ui->comboSize->setCurrentIndex(0);

  ui->spinWidth->setEnabled(false);
  ui->spinHeight->setEnabled(false);
}

bool ExportImageDialog::isAnimation() const
{
  const int product = ui->comboProduct->currentIndex();
  return (product == PRODUCT_IMAGE_ANIMATION || product == PRODUCT_MOVIE);
}

QSize ExportImageDialog::exportSize() const
{
  return QSize(ui->spinWidth->value(), ui->spinHeight->value());
}

void ExportImageDialog::onDianaResized(int w, int h)
{
  p->dianaWidth = w;
  p->dianaHeight = h;

  // if (running) return; ?
  const int sizeidx = ui->comboSize->currentIndex();
  if (sizeidx < 0)
    return;

  const SizeSpec& size = p->sizes.at(sizeidx);
  if (size.type == Size_Fixed && !size.size.isValid()) {
    ui->spinWidth->setValue(p->dianaWidth);
    ui->spinHeight->setValue(p->dianaHeight);
  }
}
