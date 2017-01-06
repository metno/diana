/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2015 met.no

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

#ifndef MOVIEMAKER_H_
#define MOVIEMAKER_H_

#include "qtTempDir.h"

#include <QSize>
#include <QStringList>

#include <string>

class QImage;

class MovieMaker {
public:
  /**
   * Constructor. Sets filename to save the finished animation to, and
   * which quality it will be saved in.
   */
  MovieMaker(const QString &filename, const QString &format,
      double framerate, const QSize& frameSize);
  ~MovieMaker();

  const QString& outputFile() const
    { return mOutputFile; }
  const QString& outputFormat() const
    { return mOutputFormat; }
  QSize frameSize() const
    { return mFrameSize; }

  bool addImage(const QImage &image);
  bool finish();

  const QStringList& outputFiles() const
    { return mOutputFiles; }

private:
  bool isImageSeries() const;

  //! filename for the given frame number (relative to mOutputDir)
  QString frameFile(int frameNumber) const;

  //! file path for the given frame number
  QString framePath(int frameNumber) const;

  bool createVideo();
  bool createAnimatedGif();

private:
  QString mOutputFile;
  QString mOutputFormat;
  double mFrameRate;
  QSize mFrameSize;

  TempDir mOutputDir;
  int mFrameCount;
  QStringList mOutputFiles;
};

#endif /*MOVIEMAKER_H_*/
