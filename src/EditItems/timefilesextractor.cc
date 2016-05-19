/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2016 met.no

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

#include "EditItems/timefilesextractor.h"

#include "diUtilities.h"

#include <puTools/miTime.h>
#include <puTools/TimeFilter.h>

#define MILOGGER_CATEGORY "diana.timefilesextractor"
#include <miLogger/miLogging.h>

namespace {

typedef QPair<QFileInfo, QDateTime> file_time;

static bool lessThan(const file_time& p1, const file_time& p2)
{
  return p1.second < p2.second;
}

static QDateTime fromMiTime(const miutil::miTime& time)
{
  return QDateTime(QDate(time.year(), time.month(), time.day()),
      QTime(time.hour(), time.min(), time.sec()));
}

} // anonymous namespace


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
QList<file_time> TimeFilesExtractor::getFiles(const QString &filePattern)
{
  // FIXME char encoding when converting between QString and std::string

  QList<file_time> result;

  std::string pattern = filePattern.toStdString();
  miutil::TimeFilter tf(pattern);
  if (tf.ok()) {
    const diutil::string_v matches = diutil::glob(pattern);
    for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
      miutil::miTime time;
      if (tf.getTime(*it, time)) {
        result.append(qMakePair(QFileInfo(QString::fromStdString(*it)), fromMiTime(time)));
      } else {
        METLIBS_LOG_WARN("getDateTimeFiles(): cannot extract time from '" << *it << "'");
      }
    }
  } else {
    METLIBS_LOG_WARN("getDateTimeFiles(): invalid pattern '" << filePattern.toStdString() << "'");
  }

  // sort chronologically so that the oldest dateTime appears first
  qSort(result.begin(), result.end(), lessThan);

  return result;
}
