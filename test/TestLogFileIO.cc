/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2018 met.no

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

#include <diLogFile.h>
#include <sstream>

#include <gtest/gtest.h>


#define HEI_UTF8 "hei blåbær"

static const char log_1[] = "# -*- coding: utf-8 -*-\n"
    "[MAIN.LOG]\n"
    "hello world\n"
    "[/MAIN.LOG]\n"
    "[SUB.LOG]\n"
    HEI_UTF8 "\n"
    "[/SUB.LOG]\n";

TEST(TestLogFileIO, Read)
{
  std::istringstream input(log_1);
  LogFileIO logfile_r;
  EXPECT_TRUE(logfile_r.read(input));

  const LogFileIO& logfile = logfile_r;
  ASSERT_TRUE(logfile.hasSection("MAIN.LOG"));
  const LogFileIO::Section& main_log = logfile.getSection("MAIN.LOG");
  ASSERT_EQ(1, main_log.size());
  EXPECT_EQ("hello world", main_log[0]);
}

TEST(TestLogFileIO, Write)
{
  LogFileIO logfile;
  LogFileIO::Section& main_log = logfile.getSection("MAIN.LOG");
  main_log.addLine("hello world");
  LogFileIO::Section& sub_log = logfile.getSection("SUB.LOG");
  sub_log.addLine(HEI_UTF8);

  std::ostringstream output;
  logfile.write(output);

  EXPECT_EQ(log_1, output.str());
}

TEST(TestLogFileIO, ReadVector)
{
  std::istringstream input(log_1);
  LogFileIO logfile_r;
  EXPECT_TRUE(logfile_r.read(input));

  const LogFileIO& logfile = logfile_r;

  ASSERT_EQ(1, logfile.getSection("SUB.LOG").lines().size());
  EXPECT_EQ(HEI_UTF8, logfile.getSection("SUB.LOG").lines().at(0));

  ASSERT_EQ(1, logfile.getSection("MAIN.LOG").lines().size());
  EXPECT_EQ("hello world", logfile.getSection("MAIN.LOG").lines().at(0));
}

TEST(TestLogFileIO, WriteVector)
{
  LogFileIO logfile;

  { std::vector<std::string> lines;
    lines.push_back("hello world");
    logfile.getSection("MAIN.LOG").addLines(lines);
  }

  { std::vector<std::string> lines;
    lines.push_back(HEI_UTF8);
    logfile.getSection("SUB.LOG").addLines(lines);
  }

  std::ostringstream output;
  logfile.write(output);

  EXPECT_EQ(log_1, output.str());
}
