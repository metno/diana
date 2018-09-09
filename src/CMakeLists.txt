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

#################### miRaster

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  diField
)

SET (MIRASTER_SOURCES
  miRaster/satimg.cc
)

IF (ENABLE_HDF5)
  LIST (APPEND MIRASTER_SOURCES
    miRaster/satimgh5.cc
  )
ENDIF ()

SET (DIFIELD_SOURCES
  diField/DataReshape.cc
  diField/FimexIO.cc
  diField/FimexSource.cc
  diField/GridCollection.cc
  diField/GridIO.cc
  diField/GridInventoryTypes.cc
  diField/VcrossData.cc
  diField/VcrossSource.cc
  diField/VcrossUtil.cc
  diField/diArea.cc
  diField/diCommonFieldTypes.cc
  diField/diField.cc
  diField/diFieldCache.cc
  diField/diFieldCacheEntity.cc
  diField/diFieldCacheKeyset.cc
  diField/diFieldCalculations.cc
  diField/diFieldDefined.cc
  diField/diFieldFunctions.cc
  diField/diFieldManager.cc
  diField/diFieldUtil.cc
  diField/diFlightLevel.cc
  diField/diGridConverter.cc
  diField/diGridReprojection.cc
  diField/diMetConstants.cc
  diField/diPoint.cc
  diField/diProjection.cc
  diField/diRectangle.cc
)

SET (BUILDINFO_CC "${CMAKE_CURRENT_BINARY_DIR}/diBuild.cc")

ADD_CUSTOM_COMMAND(
  OUTPUT
    check_buildinfo ${BUILDINFO_CC}
  COMMAND
    ${CMAKE_COMMAND} -DGIT_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DBUILDINFO_CC=${BUILDINFO_CC} -P ${CMAKE_SOURCE_DIR}/cmake/GetGitCommit.cmake
)
ADD_CUSTOM_TARGET(buildinfo
  DEPENDS
    ${BUILDINFO_CC}
)

SET (LIBDIANA_SOURCES
  QImagePNGWriter.cc
  diAnnotationPlot.cc
  diAreaBorder.cc
  diAreaObjects.cc
  diAreaObjectsCluster.cc
  diColour.cc
  diColourShading.cc
  diCommandParser.cc
  diComplexSymbolPlot.cc
  diContouring.cc
  diController.cc
  diDisplayObjects.cc
  diEditManager.cc
  diEditObjects.cc
  diFieldEdit.cc
  diFieldPlot.cc
  diFieldPlotCluster.cc
  diFieldPlotManager.cc
  diFieldUtil.cc
  diFilledMap.cc
  diFont.cc
  diFtnVfile.cc
  diGLPainter.cc
  diGlUtilities.cc
  diImageGallery.cc
  diImageIO.cc
  diKVListPlotCommand.cc
  diLabelPlotCommand.cc
  diLegendPlot.cc
  diLinestyle.cc
  diLinetype.cc
  diLocalSetupParser.cc
  diLocationPlot.cc
  diLogFile.cc
  diMItiff.cc
  diMainPaintable.cc
  diMapAreaNavigator.cc
  diMapAreaSetup.cc
  diMapInfo.cc
  diMapManager.cc
  diMapPlot.cc
  diMeasurementsPlot.cc
  diObjectManager.cc
  diObjectPlot.cc
  diObjectPoint.cc
  diObsAscii.cc
  diObsData.cc
  diObsDialogInfo.cc
  diObsManager.cc
  diObsMetaData.cc
  diObsPlot.cc
  diObsPlotCluster.cc
  diObsPlotType.cc
  diObsPositions.cc
  diObsReader.cc
  diObsReaderAscii.cc
  diObsReaderBufr.cc
  diObsReaderFactory.cc
  diObsReaderFile.cc
  diObsReaderMetnoUrl.cc
  diObsReaderRoad.cc
  diPaintGLPainter.cc
  diPaintable.cc
  diPainter.cc
  diPattern.cc
  diPlot.cc
  diPlotArea.cc
  diPlotCluster.cc
  diPlotCommand.cc
  diPlotCommandFactory.cc
  diPlotModule.cc
  diPlotOptions.cc
  diPolyContouring.cc
  diPrintOptions.cc
  diQuickMenues.cc
  diRasterAlarmBox.cc
  diRasterAlpha.cc
  diRasterFillCell.cc
  diRasterGrid.cc
  diRasterPlot.cc
  diRasterRGB.cc
  diRasterUndef.cc
  diRasterUtil.cc
  diSat.cc
  diSatManager.cc
  diSatPlot.cc
  diShapeObject.cc
  diStaticPlot.cc
  diStationManager.cc
  diStationPlot.cc
  diStationPlotCommand.cc
  diStringPlotCommand.cc
  diTrajectoryGenerator.cc
  diTrajectoryPlot.cc
  diUndoFront.cc
  diUtilities.cc
  diWeatherArea.cc
  diWeatherFront.cc
  diWeatherObjects.cc
  diWeatherSymbol.cc
  miSetupParser.cc
  poly_contouring.cc

  EditItems/drawingcomposite.cc
  EditItems/drawingitembase.cc
  EditItems/drawingpolyline.cc
  EditItems/drawingstylemanager.cc
  EditItems/drawingsymbol.cc
  EditItems/drawingtext.cc
  EditItems/kml.cc
  EditItems/timefilesextractor.cc

  export/DianaImageSource.cc
  export/ExportUtil.cc
  export/ImageSink.cc
  export/MovieMaker.cc
  export/PaintGLImageSource.cc
  export/PaintableImageSource.cc
  export/PdfSink.cc
  export/QPictureSink.cc
  export/RasterFileSink.cc
  export/SvgFileSink.cc
  export/VcrossImageSource.cc
  export/qtTempDir.cc

  util/charsets.cc
  util/debug_timer.cc
  util/diKeyValue.cc
  util/diLineMerger.cc
  util/fimex_logging.cc
  util/format_int.cc
  util/math_util.cc
  util/openmp_tools.cc
  util/plotoptions_util.cc
  util/polygon_util.cc
  util/qstring_util.cc
  util/string_util.cc
  util/subprocess.cc
  util/time_util.cc
  util/was_enabled.cc

  vcross_v2/VcrossCollector.cc
  vcross_v2/VcrossComputer.cc
  vcross_v2/VcrossEvaluate.cc
  vcross_v2/VcrossOptions.cc
  vcross_v2/VcrossPlotCommand.cc
  vcross_v2/VcrossQtAxis.cc
  vcross_v2/VcrossQtContour.cc
  vcross_v2/VcrossQtPaint.cc
  vcross_v2/VcrossQtPlot.cc
  vcross_v2/VcrossQtUtil.cc
  vcross_v2/VcrossResolver.cc
  vcross_v2/VcrossSetup.cc
  vcross_v2/VcrossVerticalTicks.cc

  diSpectrumData.cc
  diSpectrumFile.cc
  diSpectrumManager.cc
  diSpectrumOptions.cc
  diSpectrumPaintable.cc
  diSpectrumPlot.cc

  vprof/diVprofAxes.cc
  vprof/diVprofAxesPT.cc
  vprof/diVprofAxesStandard.cc
  vprof/diVprofBox.cc
  vprof/diVprofBoxFactory.cc
  vprof/diVprofBoxFL.cc
  vprof/diVprofBoxLine.cc
  vprof/diVprofBoxPT.cc
  vprof/diVprofBoxSigWind.cc
  vprof/diVprofBoxVerticalWind.cc
  vprof/diVprofBoxWind.cc
  vprof/diVprofBoxZ.cc
  vprof/diVprofCalculations.cc
  vprof/diVprofData.cc
  vprof/diVprofDiagram.cc
  vprof/diVprofGraphData.cc
  vprof/diVprofManager.cc
  vprof/diVprofModelSettings.cc
  vprof/diVprofOptions.cc
  vprof/diVprofPaintable.cc
  vprof/diVprofPainter.cc
  vprof/diVprofPlotCommand.cc
  vprof/diVprofReader.cc
  vprof/diVprofReaderBufr.cc
  vprof/diVprofReaderFimex.cc
  vprof/diVprofReaderRoadobs.cc
  vprof/diVprofSimpleData.cc
  vprof/diVprofSimpleValues.cc
  vprof/diVprofUtils.cc
  vprof/diVprofValues.cc

  wmsclient/WebMapPainting.cc
  wmsclient/WebMapUtilities.cc
)

SET(LIBDIANA_QTSOURCES
  diDrawingManager.cc
  diManager.cc
  EditItems/itemgroup.cc

  export/ImageSource.cc

  vcross_v2/VcrossQtManager.cc
  vcross_v2/VcrossQuickmenues.cc

  wmsclient/WebMapManager.cc
  wmsclient/WebMapPlot.cc
  wmsclient/WebMapService.cc
  wmsclient/WebMapSlippyOSM.cc
  wmsclient/WebMapTile.cc
  wmsclient/WebMapWMS.cc
  wmsclient/WebMapWMTS.cc
)

SET(LIBBDIANA_SOURCES
  bdiana_capi.cc
  bdiana_main.cc
  bdiana_graphics.cc
  bdiana_source.cc
  bdiana_spectrum.cc
  bdiana_vcross.cc
  bdiana_vprof.cc
  signalhelper.cc

  diOrderBook.cc
  diOrderClient.cc
  diOrderListener.cc
  diOrderQueue.cc
  diWorkOrder.cc
)

SET (LIBDIANAUI_SOURCES
  diFontFamily.cc
  diFontManager.cc
  diOpenGLPainter.cc
  diOpenGLWidget.cc
  diPaintableWidget.cc
  diTesselation.cc

  qtImageGallery.cc
  qtPrintManager.cc
  qtUtility.cc
)

SET(LIBDIANAUI_QTSOURCES
  diEditItemManager.cc
  diPaintGLWidget.cc

  qtActionButton.cc
  qtAddtoDialog.cc
  qtAddtoMenu.cc
  qtAnnoText.cc
  qtAnnotationDialog.cc
  qtBrowserBox.cc
  qtButtonLayout.cc
  qtComplexText.cc
  qtDataDialog.cc
  qtDianaApplication.cc
  qtEditComment.cc
  qtEditDefineField.cc
  qtEditDialog.cc
  qtEditNewDialog.cc
  qtEditText.cc
  qtFieldDialog.cc
  qtGeoPosLineEdit.cc
  qtMainUiEventHandler.cc
  qtMainWindow.cc
  qtMapDialog.cc
  qtMeasurementsDialog.cc
  qtObjectDialog.cc
  qtObsDialog.cc
  qtObsWidget.cc
  qtQuickAdmin.cc
  qtQuickEditOptions.cc
  qtQuickMenu.cc
  qtSatDialog.cc
  qtSatDialogAdvanced.cc
  qtSetupDialog.cc
  qtShowSatValues.cc
  qtSpectrumModelDialog.cc
  qtSpectrumSetupDialog.cc
  qtSpectrumUiEventHandler.cc
  qtSpectrumWindow.cc
  qtStationDialog.cc
  qtStationDialogModel.cc
  qtStatusGeopos.cc
  qtStatusPlotButtons.cc
  qtStyleButtons.cc
  qtTextDialog.cc
  qtTextView.cc
  qtTimeControl.cc
  qtTimeNavigator.cc
  qtTimeSlider.cc
  qtTimeSpinbox.cc
  qtTimeStepSpinbox.cc
  qtToggleButton.cc
  qtTrajectoryDialog.cc
  qtTreeFilterProxyModel.cc
  qtUiEventHandler.cc
  qtWorkArea.cc

  EditItems/dialogcommon.cc
  EditItems/drawingdialog.cc
  EditItems/editcomposite.cc
  EditItems/edititembase.cc
  EditItems/editpolyline.cc
  EditItems/editsymbol.cc
  EditItems/edittext.cc
  EditItems/filterdrawingdialog.cc
  EditItems/properties.cc
  EditItems/toolbar.cc

  export/qtExportImageDialog.cc
  export/qtExportImagePreview.cc
  export/PrinterDialog.cc

  vcross_qt/VcrossQtWidget.cc
  vcross_qt/qtVcrossAddPlotDialog.cc
  vcross_qt/qtVcrossInterface.cc
  vcross_qt/qtVcrossLayerBar.cc
  vcross_qt/qtVcrossLayerButton.cc
  vcross_qt/qtVcrossLayerList.cc
  vcross_qt/qtVcrossModelPage.cc
  vcross_qt/qtVcrossReftimePage.cc
  vcross_qt/qtVcrossReplaceModelDialog.cc
  vcross_qt/qtVcrossSetup.cc
  vcross_qt/qtVcrossSetupDialog.cc
  vcross_qt/qtVcrossStyleDialog.cc
  vcross_qt/qtVcrossStyleWidget.cc
  vcross_qt/qtVcrossWindow.cc

  vprof/qtVprofModelDialog.cc
  vprof/qtVprofSetupDialog.cc
  vprof/qtVprofUiEventHandler.cc
  vprof/qtVprofWindow.cc

  wmsclient/WebMapDialog.cc
)

IF (ENABLE_GEOTIFF)
  LIST (APPEND MIRASTER_SOURCES miRaster/satgeotiff.cc miRaster/ImageCache.cpp)
  LIST (APPEND LIBDIANA_SOURCES diGEOtiff.cc)
ENDIF ()

IF (ENABLE_NEWARKOBS)
  LIST (APPEND LIBDIANA_SOURCES diObsRoad.cc)
ENDIF ()

IF (ENABLE_HDF5)
  LIST (APPEND LIBDIANA_SOURCES diHDF5.cc)
ENDIF ()

if (ENABLE_OBS_BUFR)
  LIST (APPEND LIBDIANA_SOURCES diObsBufr.cc)
ENDIF ()


METNO_HEADERS(MIRASTER_HEADERS     "(cc|cpp)" "h" ${MIRASTER_SOURCES})
METNO_HEADERS(DIFIELD_HEADERS      "cc" "h" ${DIFIELD_SOURCES})
METNO_HEADERS(LIBDIANA_HEADERS     "cc" "h" ${LIBDIANA_SOURCES})
METNO_HEADERS(LIBDIANA_QTHEADERS   "cc" "h" ${LIBDIANA_QTSOURCES})
METNO_HEADERS(LIBDIANAUI_HEADERS   "cc" "h" ${LIBDIANAUI_SOURCES})
METNO_HEADERS(LIBDIANAUI_QTHEADERS "cc" "h" ${LIBDIANAUI_QTSOURCES})
METNO_HEADERS(LIBBDIANA_HEADERS    "cc" "h" ${LIBBDIANA_SOURCES})

LIST (APPEND LIBDIANA_HEADERS
  diAreaTypes.h
  diBuild.h
  diCommonTypes.h
  diEditTypes.h
  diEventResult.h
  diObjTypes.h
  diPlotElement.h
  diPlotOrder.h
  diQuickMenuTypes.h
  diSatTypes.h
  diSliderValues.h
  diStationTypes.h
  diTimeTypes.h

  vprof/diVprofText.h

  util/misc_util.h
  util/nearest_element.h
)
LIST (APPEND LIBDIANA_SOURCES
  ${BUILDINFO_CC}
)
LIST (APPEND LIBDIANA_QTHEADERS
  diVcrossInterface.h
)

SET (LIBDIANAUI_QTFORMS
  linestyledialog.ui
  mapdialog.ui
  export/export_image_dialog.ui
  export/export_image_preview.ui
  vcross_qt/vcross_plot_add_dialog.ui
  vcross_qt/vcross_modelpage.ui
  vcross_qt/vcross_reftimepage.ui
  vcross_qt/vcross_replace_model_dialog.ui
  vcross_qt/vcross_style_dialog.ui
  vcross_qt/vcross_style_widget.ui
  vcross_qt/vcross_window.ui
  wmsclient/webmap_dialog.ui
)

SET (LIBDIANAUI_XPM
  active_object.xpm
  apply.xpm
  autoupdate.xpm
  back.xpm
  bakover.xpm
  balloon.xpm
  blocks.xpm
  bookmark.xpm
  clock.xpm
  convert.xpm
  custom_user.xpm
  dialoger.xpm
  diana_icon.xpm
  directory.xpm
  dnmi.xpm
  down12x12.xpm
  down12x14.xpm
  down20x20.xpm
  down32x24.xpm
  earth3.xpm
  editcopy.xpm
  editcut.xpm
  edit_lock_value.xpm
  editmode.xpm
  edit_open_value.xpm
  editpaste.xpm
  editundo.xpm
  exit.xpm
  face_grin.xpm
  felt.xpm
  fet_object_fog.xpm
  fet_object_normal.xpm
  fet_object_p.xpm
  fet_object_rain.xpm
  fet_object_sky.xpm
  fet_object_tmp.xpm
  fet_object_wave.xpm
  fet_object_wind.xpm
  filenew.xpm
  fileopen.xpm
  fileprint.xpm
  filesave.xpm
  forover.xpm
  forward.xpm
  front.xpm
  help.xpm
  hideapply.xpm
  idnumDown.xpm
  idnumUp.xpm
  inactive_object.xpm
  info.xpm
  kill.xpm
  levelDown.xpm
  levelUp.xpm
  linear_copy.xpm
  linear_down.xpm
  linear_remove.xpm
  locked_bookmark.xpm
  locked_directory.xpm
  loop.xpm
  magnify2.xpm
  metno_sort_txt.xpm
  minus12x12.xpm
  multiple_users.xpm
  no_object.xpm
  paint_add_cursor.xpm
  paint_add_point.xpm
  paint_color.xpm
  paint_create_polyline.xpm
  paint_create_symbol.xpm
  paint_cursor.xpm
  paint_cut.xpm
  paint_delete.xpm
  paint_draw.xpm
  paint_forbidden_cursor.xpm
  paint_help.xpm
  paint_hide.xpm
  paint_include.xpm
  paint_mode.xpm
  paint_move_point.xpm
  paint_move.xpm
  paint_new.xpm
  paint_redo.xpm
  paint_remove_cursor.xpm
  paint_remove_point.xpm
  paint_select.xpm
  paint_select2.xpm
  paint_spatial.xpm
  paint_undo.xpm
  parent_object.xpm
  pencil2.xpm
  pick.xpm
  preferences.xpm
  private_directory.xpm
  question.xpm
  ruler.xpm
  redo.xpm
  revert.xpm
  Robot.xpm
  run_smooth.xpm
  sat.xpm
  session_deployed.xpm
  session_finalize.xpm
  session_lock.xpm
  session_open.xpm
  session_operational.xpm
  shuttle.xpm
  single_remove.xpm
  slutt.xpm
  small_metno_sort.xpm
  spectrum.xpm
  start.xpm
  stopp.xpm
  station.xpm
  sun2.xpm
  synop_red.xpm
  synop.xpm
  tb_close.xpm
  tb_left_arrow.xpm
  tb_print.xpm
  tb_right_arrow.xpm
  thumbs_down.xpm
  thumbs_up.xpm
  Tool_32_draw.xpm
  traj.xpm
  trashcan.xpm
  undo.xpm
  up12x12.xpm
  up12x14.xpm
  up20x20.xpm
  up32x24.xpm
  user_admin.xpm
  user.xpm
  vcross.xpm
  vprof_normal.xpm
  vprof_selected.xpm
  weather_rain.xpm
  weather_storm.xpm
)

#QT5_WRAP_CPP(LIBDIANA_MOC_SRCS   ${LIBDIANA_QTHEADERS})
#QT5_WRAP_CPP(LIBDIANAUI_MOC_SRCS ${LIBDIANAUI_QTHEADERS})
#QT5_WRAP_CPP(DIANA_MOC_SRCS      ${DIANA_QTHEADERS})

QT5_WRAP_UI(LIBDIANAUI_UIS ${LIBDIANAUI_QTFORMS})
QT5_WRAP_UI(DIANA_UIS      ${DIANA_QTFORMS})

SET_PROPERTY(SOURCE ${LIBDIANAUI_UIS} ${DIANA_UIS} diBuild.cc PROPERTY SKIP_AUTOMOC ON)

SET(TS_DIR "${CMAKE_SOURCE_DIR}/share/diana/lang")
METNO_QT_CREATE_TRANSLATION(LIBDIANAUI_QM
  TSFILES
   ${TS_DIR}/diana_en.ts
   ${TS_DIR}/diana_es.ts
   ${TS_DIR}/diana_no.ts
   ${TS_DIR}/diana_ru.ts
   ${TS_DIR}/diana_sv.ts

  UPDATE_SOURCES
   ${LIBDIANAUI_SOURCES}
   ${LIBDIANAUI_QTSOURCES}
   ${DIANA_SOURCES}

   ${LIBDIANAUI_QTFORMS}
   ${DIANA_QTFORMS}

  UPDATE_OPTIONS
   -locations relative

  UPDATE_TARGET
   lupdate
)

LINK_DIRECTORIES(${PC_LIBRARY_DIRS} ${PCUI_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})

ADD_LIBRARY(libdiana SHARED
  ${LIBDIANA_SOURCES}
  ${LIBDIANA_HEADERS}

  ${LIBDIANA_QTHEADERS}
  ${LIBDIANA_QTSOURCES}

  ${LIBDIANA_MOC_SRCS}

  ${MIRASTER_SOURCES}
  ${MIRASTER_HEADERS}

  ${DIFIELD_SOURCES}
  ${DIFIELD_HEADERS}
)
SET_TARGET_PROPERTIES(libdiana PROPERTIES
  VERSION 0
  SOVERSION 0
  OUTPUT_NAME "diana-${PVERSION}"
)
ADD_DEPENDENCIES(libdiana buildinfo)

ADD_LIBRARY(libdianaui SHARED
  ${LIBDIANAUI_SOURCES}
  ${LIBDIANAUI_HEADERS}

  ${LIBDIANAUI_QTSOURCES}
  ${LIBDIANAUI_QTHEADERS}

  ${LIBDIANAUI_MOC_SRCS}
  ${LIBDIANAUI_UIS}
  ${LIBDIANAUI_QM}

  ${LIBDIANAUI_XPM}
)
SET_TARGET_PROPERTIES(libdianaui PROPERTIES
  VERSION 0
  SOVERSION 0
  OUTPUT_NAME "dianaui-${PVERSION}"
)

ADD_LIBRARY(libbdiana SHARED
  ${LIBBDIANA_SOURCES}
  ${LIBBDIANA_HEADERS}
)

SET_TARGET_PROPERTIES(libbdiana PROPERTIES
  VERSION 0
  SOVERSION 0
  OUTPUT_NAME "bdiana-${PVERSION}"
)

ADD_EXECUTABLE(diana
  main_gui.cc

  ${DIANA_MOC_SRCS}
  ${DIANA_UIS}
)
SET_TARGET_PROPERTIES(diana PROPERTIES
  OUTPUT_NAME "diana.bin-${PVERSION}"
)

ADD_EXECUTABLE(bdiana
  bdiana.cc
)
SET_TARGET_PROPERTIES(bdiana PROPERTIES
  OUTPUT_NAME "bdiana-${PVERSION}"
)

########################################################################

INSTALL (TARGETS libdiana libdianaui libbdiana
  DESTINATION ${LIBDIANA_DIR}
)
INSTALL (TARGETS diana bdiana
  DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
INSTALL(FILES ${LIBDIANAUI_QM}
  DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME}/${PVERSION}/lang"
)

########################################################################

INCLUDE_DIRECTORIES(
  ${PC_INCLUDE_DIRS}
  ${PCUI_INCLUDE_DIRS}
  diField
)

TARGET_LINK_LIBRARIES(libdiana
  ${PC_LIBRARIES}
  ${geotiff_LIB}
  ${hdf5_LIB}
  ${proj_LIB}
  ${shp_LIB}
  ${udunits2_LIB}
  ${QT_LIBRARIES} # includes Qt5::Gui :-(
  ${Boost_LIBRARIES}
)
TARGET_LINK_LIBRARIES(libdianaui libdiana ${PCUI_LIBRARIES})
TARGET_LINK_LIBRARIES(libbdiana libdiana)

TARGET_LINK_LIBRARIES(bdiana libbdiana)
TARGET_LINK_LIBRARIES(diana  libdianaui)

IF(ENABLE_SMHI)
  SET(SMHI TRUE)
ENDIF()

IF (ENABLE_NEWARKOBS)
  SET(ROADOBS TRUE)
  SET(NEWARK_INC TRUE)
ENDIF ()

IF (ENABLE_GEOTIFF)
  SET(GEOTIFF TRUE)
  TARGET_INCLUDE_DIRECTORIES(libdiana PUBLIC "${geotiff_INC_DIR}")
  TARGET_COMPILE_OPTIONS(libdiana PUBLIC "${geotiff_CFLAGS_OTHER}")
  TARGET_LINK_LIBRARIES(libdiana ${geotiff_LIB})
ENDIF ()

IF (ENABLE_HDF5)
  SET(HDF5FILE TRUE)
  TARGET_INCLUDE_DIRECTORIES(libdiana PUBLIC "${hdf5_INC_DIR}")
  TARGET_COMPILE_OPTIONS(libdiana PUBLIC "${hdf5_CFLAGS_OTHER}")
  TARGET_LINK_LIBRARIES(libdiana ${hdf5_LIB})
ENDIF ()

if (ENABLE_OBS_BUFR)
  SET(BUFROBS TRUE)
ENDIF ()

CONFIGURE_FILE(diana_config.h.in diana_config.h)
