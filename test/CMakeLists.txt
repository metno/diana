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

QT5_WRAP_CPP(dianaUnitTests_MOC_SRCS ${dianaUnitTests_QTHEADERS})

SET (dianaUnitTests_SOURCES
  ${dianaUnitTests_QTSOURCES}
  TestDianaLevels.cc
  TestVcrossComputer.cc
  TestVprofData.cc
  TestCommandParser.cc
  TestLogFileIO.cc
  TestPlotCommands.cc
  TestPlotOptions.cc
  TestPoint.cc
  TestQuickMenues.cc
  TestSatImg.cc
  TestSetupParser.cc
  TestUtilities.cc
  TestWebMap.cc
  gtestMainQCA.cc

  ${dianaUnitTests_MOC_SRCS}
)

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
  TestVcrossGui.cc
  TestGLPainter.cc
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
