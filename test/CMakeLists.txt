#  Diana - A Free Meteorological Visualisation Tool
#
#  Copyright (C) 2017-2018 met.no
#
#  Contact information:
#  Norwegian Meteorological Institute
#  Box 43 Blindern
#  0313 OSLO
#  NORWAY
#  email: diana@met.no
#
#  This file is part of Diana
#
#  Diana is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  Diana is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Diana; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

INCLUDE_DIRECTORIES(
  ${PC_INCLUDE_DIRS}
  ${PCUI_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/src
)

ADD_DEFINITIONS(
  -DTEST_SRCDIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
  -DTEST_BUILDDIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)

LINK_LIBRARIES(
  ${GTEST_LIBRARY}
  ${GTEST_MAIN_LIBRARY}
)

LINK_DIRECTORIES(${PC_LIBRARY_DIRS} ${PCUI_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})

########################################################################

SET (dianaUnitTests_QTSOURCES
  TestVcrossQtManager.cc
  TestVcrossQuickmenues.cc
)

METNO_HEADERS(dianaUnitTests_QTHEADERS   "cc" "h" ${dianaUnitTests_QTSOURCES})

#QT5_WRAP_CPP(dianaUnitTests_MOC_SRCS ${dianaUnitTests_QTHEADERS})

SET (dianaUnitTests_SOURCES
  ${dianaUnitTests_QTSOURCES}
  TestCommandParser.cc
  TestDianaLevels.cc
  TestFieldUtil.cc
  TestImageIO.cc
  TestLogFileIO.cc
  TestObsReaderAscii.cc
  TestObsReaderFile.cc
  TestPlotCommands.cc
  TestPlotOptions.cc
  TestPoint.cc
  TestQuickMenues.cc
  TestSatImg.cc
  TestSetupParser.cc
  TestUtilities.cc
  TestVcrossComputer.cc
  TestVprofData.cc
  TestWebMap.cc
  gtestMainQCA.cc

  ${dianaUnitTests_MOC_SRCS}
)

if (ENABLE_OBS_BUFR)
  LIST (APPEND dianaUnitTests_SOURCES TestObsReaderBufr.cc)
ENDIF ()

ADD_EXECUTABLE(dianaUnitTests
  ${dianaUnitTests_SOURCES}
)

TARGET_LINK_LIBRARIES(dianaUnitTests
  libdiana
  ${Boost_SYSTEM_LIBRARY}
)

ADD_TEST(NAME dianaUnitTests
  COMMAND dianaUnitTests --gtest_color=yes
)

########################################################################

SET (diFieldTest_SOURCES
  FieldFunctionsTest.cc
  GridConverterTest.cc
  ProjectionTest.cc
  gtestMain.cc

  DataReshapeTest.cc
  FieldManagerTest.cc
  FimexSourceTest.cc
  MetConstantsTest.cc
  RectangleTest.cc
  ShapeTest.cc
  VcrossDataTest.cc
  VcrossUtilTest.cc
)

ADD_EXECUTABLE(diFieldTest
  ${diFieldTest_SOURCES}
)

TARGET_INCLUDE_DIRECTORIES(diFieldTest PRIVATE
  ${CMAKE_SOURCE_DIR}/src/diField
)

TARGET_LINK_LIBRARIES(diFieldTest
  libdiana
)

ADD_TEST(NAME diFieldTest
  COMMAND diFieldTest --gtest_color=yes
)

########################################################################

SET (dianaGuiTests_SOURCES
  TestFieldDialog.cc
  TestGLPainter.cc
  TestPdfSink.cc
  TestSatDialog.cc
  TestVcrossGui.cc
  gtestMainQA.cc
)

ADD_EXECUTABLE(dianaGuiTests
  ${dianaGuiTests_SOURCES}
)

TARGET_LINK_LIBRARIES(dianaGuiTests PRIVATE
  Qt5::Gui
  libdianaui
)

ADD_TEST(NAME dianaGuiTests
  COMMAND dianaGuiTests --gtest_color=yes
)
