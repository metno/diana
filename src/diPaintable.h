/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017 met.no

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

#ifndef DIPAINTABLE_H
#define DIPAINTABLE_H 1

#include <QObject>
#include <QSize>
#include <memory>

class DiCanvas;
class DiPainter;
class ImageSource;

class Paintable : public QObject
{
  Q_OBJECT

public:
  Paintable();
  virtual ~Paintable();

  virtual void setCanvas(DiCanvas* canvas);
  DiCanvas* canvas() const
    { return mCanvas; }

  const QSize& size() const { return mSize; }
  virtual void resize(const QSize& size);

  virtual void paintUnderlay(DiPainter* painter) = 0;
  virtual void paintOverlay(DiPainter* painter) = 0;
  void requestBackgroundBufferUpdate()
    { update_background_buffer = true; }

  virtual ImageSource* imageSource();

Q_SIGNALS:
  void resized(const QSize& size);

public:
  bool enable_background_buffer;
  bool update_background_buffer;

protected:
  std::unique_ptr<ImageSource> imageSource_;

private:
  DiCanvas* mCanvas;
  QSize mSize;
};

#endif // DIPAINTABLE_H
