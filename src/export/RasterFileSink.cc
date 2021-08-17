/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2021 met.no

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

#include "RasterFileSink.h"

#include "QImagePNGWriter.h"
#include <QFileInfo>

namespace {
bool save(const QImage& image, const QString& filename, const QString format)
{
  const QString suffix = QFileInfo(filename).suffix().toLower();
  if (format == "png" || (format.isEmpty() && suffix == "png")) {
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QImagePNGWriter writer(&file);
    return writer.write(image);
  } else if (!format.isEmpty()) {
    return image.save(filename, format.toStdString().c_str());
  } else {
    return image.save(filename);
  }
}
} // namespace

RasterFileSink::RasterFileSink(const QSize& size, const QString& filename, const QString format)
    : image_(size, QImage::Format_ARGB32_Premultiplied)
    , filename_(filename)
    , format_(format)
{
}

bool RasterFileSink::isPrinting()
{
  return false;
}

bool RasterFileSink::beginPage()
{
  image_.fill(Qt::transparent);
  painter_.begin(&image_);
  return true;
}

QPainter& RasterFileSink::paintPage()
{
  return painter_;
}

bool RasterFileSink::endPage()
{
  painter_.end();
  return true;
}

bool RasterFileSink::finish()
{
  if (!filename_.isEmpty())
    return saveTo(filename_);
  else
    return true;
}

bool RasterFileSink::saveTo(const QString& filename)
{
  if (!filename.isEmpty())
    return save(image_, filename, format_);
  else
    return false;
}
