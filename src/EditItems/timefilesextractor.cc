/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2013 met.no

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

#include <EditItems/timefilesextractor.h>
#include <QRegExp>
#include <QDir>
#include <QDirIterator>

// Returns a datetime pattern where occurrences of 'MM' are replaced with 'mm' and vice versa.
static QString convertDateTimePattern(const QString &s)
{
  QString r(s);
  for (int i = 0; i < r.size(); ++i) {
    if (i > 0) {
      if ((r[i] == 'M') && (r[i - 1] == 'M'))
        r[i - 1] = r[i] = 'm';
      else if ((r[i] == 'm') && (r[i - 1] == 'm'))
        r[i - 1] = r[i] = 'M';
    }
  }
  return r;
}

static bool extractComponents(
    const QString &filePattern, QString &prefix, QString &dtPattern, QString &suffix, QDir &dir)
{
  QRegExp rx("^([^\\[\\]]+)\\[([^\\[\\]]+)\\]([^\\[\\]]*)$");
  if (rx.indexIn(filePattern) < 0)
    return false;
  prefix = rx.cap(1);
  dtPattern = convertDateTimePattern(rx.cap(2));
  suffix = rx.cap(3);
  dir = QFileInfo(filePattern).dir();
  return true;
}

static void extractMatchingFiles(
    QString &prefix, QString &dtPattern, QString &suffix, const QDir &dir,
    QList<QPair<QFileInfo, QDateTime> > &result)
{
  QDirIterator dit(dir);
  QRegExp rx(QString("^%1(.+)%2$").arg(prefix).arg(suffix));
  while (dit.hasNext()) {
    const QString fileName = dit.next();
    if (rx.indexIn(fileName) >= 0) {
      const QDateTime dateTime = QDateTime::fromString(rx.cap(1), dtPattern);
      if (dateTime.isValid())
        result.append(qMakePair(QFileInfo(fileName), dateTime));
    }
  }
}

static bool lessThan(const QPair<QFileInfo, QDateTime> &p1, const QPair<QFileInfo, QDateTime> &p2)
{
  return p1.second < p2.second;
}

// Returns a list of (fileInfo, dateTime) pairs for files matching a file pattern
// of the form <part1>[<part2>]<part3> where none of <part1-3> contain '[' or ']'
// and <part2> is a format string similar to the second argument of
// QDateTime::fromString(const QString &, const QString &), except that the minute
// is 'MM' and the month number is 'mm'.
// Example of use:
//
//     QList<QPair<QFileInfo, QDateTime> > tfiles = TimeFilesExtractor::getFiles("/disk1/gale_warnings/gale_[yyyymmddtHHMM].kml");
//     for (int i = 0; i < tfiles.size(); ++i)
//         qDebug() << tfiles.at(i).first.filePath() << " / " << tfiles.at(i).second;
//
QList<QPair<QFileInfo, QDateTime> > TimeFilesExtractor::getFiles(const QString &filePattern)
{
  // extract prefix, converted datetime pattern, suffix, and directory
  QString prefix;
  QString dtPattern;
  QString suffix;
  QDir dir;
  if (!extractComponents(filePattern, prefix, dtPattern, suffix, dir)) {
    qWarning("getDateTimeFiles(): invalid file pattern");
    return QList<QPair<QFileInfo, QDateTime> >();
  }

  // extract matching files
  QList<QPair<QFileInfo, QDateTime> > result;
  extractMatchingFiles(prefix, dtPattern, suffix, dir, result);

  // sort chronologically so that the oldest dateTime appears first
  qSort(result.begin(), result.end(), lessThan);

  return result;
}
