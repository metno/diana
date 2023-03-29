/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2020 met.no

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

#include "testinghelpers.h"

#include <miLogger/miLoggingMemory.h>

#include <fstream>
#include <iostream>
#include <stdexcept>

using std::string;

namespace {
const std::string test_src(TEST_SRCDIR "/");
const std::string extra_data_dir(TEST_EXTRADATA_DIR "/");
} // namespace

namespace ditest {

void setupMemoryLog()
{
  milogger::system::selectSystem(std::make_shared<milogger::memory::MemorySystem>());
}

void clearMemoryLog()
{
  milogger::memory::MemorySystem::instance()->clear();
}

const milogger::memory::MemorySystem::messages_v& getMemoryLogMessages()
{
  return milogger::memory::MemorySystem::instance()->messages();
}

const std::string& testSrcDir()
{
    return test_src;
}

bool exists(const std::string& path)
{
    return std::ifstream(path.c_str()).is_open();
}

void remove(const std::string& path)
{
    ::remove(path.c_str());
}

std::string require(const std::string& path)
{
    if (!exists(path))
        throw std::runtime_error("no such file: '" + path + "'");
    return path;
}

std::string pathTest(const std::string& filename)
{
    return require(test_src + filename);
}

std::string pathTestExtra(const std::string& filename)
{
    return require(extra_data_dir + filename);
}

bool hasTestExtra()
{
    return exists(extra_data_dir + "VERSION");
}

} // namespace ditest
