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

#ifndef ExportImageDialog_h
#define ExportImageDialog_h 1

#include <QDialog>
#include <memory>

class DianaMainWindow;

class Ui_ExportImageDialog;
struct P_ExportImageDialog;

class ExportImageDialog : public QDialog
{
  Q_OBJECT

public:
  ExportImageDialog(DianaMainWindow *parent);
  ~ExportImageDialog();

public Q_SLOTS:
  void onDianaResized(int w, int h);

private Q_SLOTS:
  void onProductChanged(int);
  void onSaveToChanged(int);
  void onFilenameChanged(QString);
  void onFileChooser();
  void onSizeComboChanged(int);
  void onSizeWidthChanged(int);
  void onSizeHeightChanged(int);
  void onPreview();

  void onStart();

private:
  void setupUi();

  //! check if the filename is allowed for the product; if yes, return true; if no, fix it and return false
  bool checkFilename(QString& filename);

  void updateFilenameHint();

  void enableStartButton();
  void updateComboSize();
  bool isAnimation() const;

  QSize exportSize() const;

private:
  std::unique_ptr<Ui_ExportImageDialog> ui;
  std::unique_ptr<P_ExportImageDialog> p;
  DianaMainWindow* mw;
};

#endif // ExportImageDialog_h
