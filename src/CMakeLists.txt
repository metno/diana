#  Diana - A Free Meteorological Visualisation Tool
#
#  Copyright (C) 2017-2022 met.no
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
  diField/CachedGridIO.cc
  diField/DataReshape.cc
  diField/FimexIO.cc
  diField/FimexSource.cc
  diField/GridCollection.cc
  diField/GridIO.cc
  diField/GridIOBase.cc
  diField/GridIOsetup.cc
  diField/GridInventoryTypes.cc
  diField/VcrossData.cc
  diField/VcrossSource.cc
  diField/VcrossUtil.cc
  diField/diArea.cc
  diField/diCommonFieldTypes.cc
  diField/diField.cc
  diField/diFieldFunctions.cc
  diField/diFieldFwd.h
  diField/diFieldManager.cc
  diField/diFieldUtil.cc
  diField/diFieldVerticalAxes.cc
  diField/diFlightLevel.cc
  diField/diGridConverter.cc
  diField/diGridReprojection.cc
  diField/diPoint.cc
  diField/diProjection.cc
  diField/diRectangle.cc
  diField/diValues.cc
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
  diComplexSymbolPlot.cc
  diContouring.cc
  diDisplayObjects.cc
  diEditManager.cc
  diEditObjects.cc
  diFieldEdit.cc
  diFieldPlot.cc
  diFieldPlotCluster.cc
  diFieldPlotCommand.cc
  diFieldPlotManager.cc
  diFieldRenderer.cc
  diFieldRendererBase.cc
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
  diLocationPlotCluster.cc
  diLogFile.cc
  diMItiff.cc
  diMainPaintable.cc
  diMapAreaNavigator.cc
  diMapAreaSetup.cc
  diMapInfo.cc
  diMapManager.cc
  diMapPlot.cc
  diMapPlotCluster.cc
  diMeasurementsPlot.cc
  diObjectManager.cc
  diObjectPlot.cc
  diObjectPlotCluster.cc
  diObjectsPlotCommand.cc
  diObjectPoint.cc
  diObsAscii.cc
  diObsData.cc
  diObsDataContainer.cc
  diObsDataRotated.cc
  diObsDataUnion.cc
  diObsDataVector.cc
  diObsDialogInfo.cc
  diObsManager.cc
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
  diObsReaderMutex.cc
  diObsReaderRoad.cc
  diObsReaderTimeInterval.cc
  diPaintGLPainter.cc
  diPaintable.cc
  diPainter.cc
  diPattern.cc
  diPlot.cc
  diPlotArea.cc
  diPlotCluster.cc
  diPlotCommand.cc
  diPlotCommandFactory.cc
  diPlotElement.cc
  diPlotModule.cc
  diPlotOptions.cc
  diPlotOptionsPlot.cc
  diPlotStatus.cc
  diPolyContouring.cc
  diPrintOptions.cc
  diQuickMenues.cc
  diRasterAlarmBox.cc
  diRasterAlpha.cc
  diRasterFillCell.cc
  diRasterGrid.cc
  diRasterPlot.cc
  diRasterRGB.cc
  diRasterSat.cc
  diRasterUndef.cc
  diRasterUtil.cc
  diSat.cc
  diSatManager.cc
  diSatManagerBase.cc
  diSatPlot.cc
  diSatPlotBase.cc
  diSatPlotCluster.cc
  diSatPlotCommand.cc
  diShapeObject.cc
  diStaticPlot.cc
  diStationManager.cc
  diStationPlot.cc
  diStationPlotCluster.cc
  diStationPlotCommand.cc
  diStringPlotCommand.cc

  diRandomStreamlineSeed.cc
  diStreamlineGenerator.cc
  diStreamlineLengthPainter.cc
  diStreamlinePainter.cc
  diStreamlinePalettePainter.cc
  diStreamlinePlot.cc
  diStreamlineSeed.cc

  diTrajectoryGenerator.cc
  diTrajectoryPlot.cc
  diTrajectoryPlotCluster.cc
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
  util/diUnitsConverter.cc
  util/fimex_logging.cc
  util/format_int.cc
  util/geo_util.cc
  util/plotoptions_util.cc
  util/polygon_util.cc
  util/qstring_util.cc
  util/string_util.cc
  util/subprocess.cc
  util/time_util.cc
  util/was_enabled.cc
  util/qtComboBoxItemDelegate.cc

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
  diController.cc
  diDrawingManager.cc
  diManager.cc
  EditItems/itemgroup.cc

  export/ImageSource.cc

  util/qtAnyDoubleSpinBox.cc

  vcross_v2/VcrossQtManager.cc
  vcross_v2/VcrossQuickmenues.cc

  wmsclient/WebMapDimension.cc
  wmsclient/WebMapImage.cc
  wmsclient/WebMapLayer.cc
  wmsclient/WebMapManager.cc
  wmsclient/WebMapPlot.cc
  wmsclient/WebMapRequest.cc
  wmsclient/WebMapService.cc
  wmsclient/WebMapSlippyOSM.cc
  wmsclient/WebMapSlippyOSMLayer.cc
  wmsclient/WebMapSlippyOSMRequest.cc
  wmsclient/WebMapTile.cc
  wmsclient/WebMapTilesRequest.cc

  wmsclient/WebMapWMS.cc
  wmsclient/WebMapWMSLayer.cc
  wmsclient/WebMapWMSRequest.cc

  wmsclient/WebMapWMTS.cc
  wmsclient/WebMapWMTSLayer.cc
  wmsclient/WebMapWMTSRequest.cc
  wmsclient/WebMapWMTSTileMatrix.cc
  wmsclient/WebMapWMTSTileMatrixSet.cc
)

SET(LIBBDIANA_SOURCES
  bdiana_capi.cc
  bdiana_main.cc
  bdiana_graphics.cc
  bdiana_source.cc
  bdiana_spectrum.cc
  bdiana_vcross.cc
  bdiana_vprof.cc

  diCommandlineOptions.cc
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
  qtStringSliderControl.cc
)

SET(LIBDIANAUI_QTSOURCES
  diEditItemManager.cc
  diFieldDialogData.cc
  diPaintGLWidget.cc
  diSatDialogData.cc

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
  qtFieldDialogAdd.cc
  qtFieldDialogData.cc
  qtFieldDialogStyle.cc
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
  qtSatDialogData.cc
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
  wmsclient/WebMapPlotListModel.cc
)

IF (ENABLE_GEOTIFF)
  LIST (APPEND MIRASTER_SOURCES miRaster/satgeotiff.cc)
  LIST (APPEND LIBDIANA_SOURCES diGEOtiff.cc)
ENDIF ()

IF (ENABLE_GEOTIFF OR ENABLE_HDF5)
  LIST (APPEND MIRASTER_SOURCES
    miRaster/ImageCache.cpp
  )
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
  diPlotOrder.h
  diQuickMenuTypes.h
  diSatDialogInfo.h
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
  autoupdate_off.xpm
  autoupdate_on.xpm
  autoupdate_warn.xpm
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

QT5_WRAP_UI(LIBDIANAUI_UIS ${LIBDIANAUI_QTFORMS})

SET_PROPERTY(SOURCE ${LIBDIANAUI_UIS} diBuild.cc PROPERTY SKIP_AUTOMOC ON)

SET(TS_DIR "${CMAKE_SOURCE_DIR}/share/diana/lang")
METNO_QT_CREATE_TRANSLATION(LIBDIANAUI_QM
  TSFILES
   ${TS_DIR}/diana_bn.ts
   ${TS_DIR}/diana_en.ts
   ${TS_DIR}/diana_es.ts
   ${TS_DIR}/diana_my.ts
   ${TS_DIR}/diana_pt.ts
   ${TS_DIR}/diana_no.ts
   ${TS_DIR}/diana_ru.ts
   ${TS_DIR}/diana_sv.ts

  UPDATE_SOURCES
   ${LIBDIANAUI_SOURCES}
   ${LIBDIANAUI_QTSOURCES}
   ${DIANA_SOURCES}

   ${LIBDIANAUI_QTFORMS}

  UPDATE_OPTIONS
   -locations relative
   #-no-obsolete

  UPDATE_TARGET
   lupdate
)

########################################################################

IF(ENABLE_GRPC)
  SET (diana_protos_dir "${CMAKE_CURRENT_SOURCE_DIR}/service")

  FUNCTION (protoc_generated out bindir)
    SET (_out ${${out}})
    FOREACH (f IN LISTS ARGN)
      LIST (APPEND _out "${bindir}/${f}.pb.cc" "${bindir}/${f}.pb.h")
      SET_PROPERTY(SOURCE "${bindir}/${f}.pb.cc" "${bindir}/${f}.pb.h" PROPERTY SKIP_AUTOGEN ON)
    ENDFOREACH()
    SET("${out}" "${_out}" PARENT_SCOPE)
  ENDFUNCTION ()

  FUNCTION (list_prepend_dir lst dir)
    SET (_lst ${${lst}})
    FOREACH (p IN LISTS ARGN)
      LIST (APPEND _lst "${dir}/${p}")
    ENDFOREACH ()
    SET("${lst}" "${_lst}" PARENT_SCOPE)
  ENDFUNCTION ()

  FUNCTION (protoc_generate gen protos)
    list_prepend_dir(protos_with_dir "${diana_protos_dir}" ${protos})
    ADD_CUSTOM_COMMAND(
      OUTPUT ${gen}
      COMMAND "${_PROTOBUF_PROTOC}"
      ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
           ${protos}
      DEPENDS ${protos_with_dir}
      WORKING_DIRECTORY "${diana_protos_dir}"
    )
  ENDFUNCTION()

  FUNCTION (grpc_generate gen protos)
    list_prepend_dir(protos_with_dir "${diana_protos_dir}" ${protos})
    ADD_CUSTOM_COMMAND(
      OUTPUT ${gen}
      COMMAND "${_PROTOBUF_PROTOC}"
      ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
           --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
           --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}
           ${protos}
      DEPENDS ${protos_with_dir}
      WORKING_DIRECTORY "${diana_protos_dir}"
    )
  ENDFUNCTION()


  SET (diana_obs_protos diana_obs_v0.proto diana_obs_v0_svc.proto)
  protoc_generated(diana_obs_proto_gen "${CMAKE_CURRENT_BINARY_DIR}" diana_obs_v0)
  protoc_generate("${diana_obs_proto_gen}" diana_obs_v0.proto)
  protoc_generated(diana_obs_grpc_gen "${CMAKE_CURRENT_BINARY_DIR}" diana_obs_v0_svc diana_obs_v0_svc.grpc)
  grpc_generate("${diana_obs_grpc_gen}" diana_obs_v0_svc.proto)

  SET (DIANA_OBS_SOURCES
    diCommandlineOptions.cc

    service/diObsService.cc
  )
  METNO_HEADERS(DIANA_OBS_HEADERS "cc" "h" ${DIANA_OBS_SOURCES})
  list_prepend_dir(LIBDIANA_SOURCES "${diana_protos_dir}" ${diana_obs_protos})

  SET (DDIANA_OBS_EXECUTABLE ddiana_obs)
  ADD_EXECUTABLE(${DDIANA_OBS_EXECUTABLE}
    ${DIANA_OBS_SOURCES}
    ${DIANA_OBS_HEADERS}
    service/main_obs.cc
  )
  SET_TARGET_PROPERTIES(${DDIANA_OBS_EXECUTABLE} PROPERTIES
    OUTPUT_NAME "ddiana-obs-${PVERSION}"
  )
  TARGET_LINK_LIBRARIES(${DDIANA_OBS_EXECUTABLE}
    libdiana_core
    mi-programoptions
  )

  LIST (APPEND LIBDIANA_SOURCES
    ${diana_obs_proto_gen}
    ${diana_obs_grpc_gen}
    diObsReaderGRPC.cc
    service/diObsGRPCServiceUtils.cc
    service/diObsGRPCUtils.cc
    service/diGRPCUtils.cc
  )
  LIST (APPEND LIBDIANA_HEADERS
    service/diObsGRPCServiceDecls.h
  )
ENDIF (ENABLE_GRPC)

########################################################################

ADD_LIBRARY(libdiana_core SHARED
  ${LIBDIANA_SOURCES}
  ${LIBDIANA_HEADERS}

  ${LIBDIANA_QTHEADERS}
  ${LIBDIANA_QTSOURCES}

  ${MIRASTER_SOURCES}
  ${MIRASTER_HEADERS}

  ${DIFIELD_SOURCES}
  ${DIFIELD_HEADERS}
)
SET_TARGET_PROPERTIES(libdiana_core PROPERTIES
  VERSION 0
  SOVERSION 0
  OUTPUT_NAME "diana-core-${PVERSION}"
)
ADD_DEPENDENCIES(libdiana_core buildinfo)

IF(ENABLE_GRPC)
  TARGET_LINK_LIBRARIES(libdiana_core
    ${GRPC_LIBRARIES}
  )
ENDIF (ENABLE_GRPC)

ADD_LIBRARY(libdiana_ui SHARED
  ${LIBDIANAUI_SOURCES}
  ${LIBDIANAUI_HEADERS}

  ${LIBDIANAUI_QTSOURCES}
  ${LIBDIANAUI_QTHEADERS}

  ${LIBDIANAUI_MOC_SRCS}
  ${LIBDIANAUI_UIS}
  ${LIBDIANAUI_QM}

  ${LIBDIANAUI_XPM}
)
SET_TARGET_PROPERTIES(libdiana_ui PROPERTIES
  VERSION 0
  SOVERSION 0
  OUTPUT_NAME "diana-ui-${PVERSION}"
)

ADD_LIBRARY(libdiana_batch SHARED
  ${LIBBDIANA_SOURCES}
  ${LIBBDIANA_HEADERS}
)

SET_TARGET_PROPERTIES(libdiana_batch PROPERTIES
  VERSION 0
  SOVERSION 0
  OUTPUT_NAME "diana-batch-${PVERSION}"
)

ADD_EXECUTABLE(diana_ui
  diCommandlineOptions.cc
  main_gui.cc
)
SET_TARGET_PROPERTIES(diana_ui PROPERTIES
  OUTPUT_NAME "diana.bin-${PVERSION}"
)

ADD_EXECUTABLE(diana_batch
  bdiana.cc
)
SET_TARGET_PROPERTIES(diana_batch PROPERTIES
  OUTPUT_NAME "bdiana-${PVERSION}"
)

########################################################################

INSTALL(
  TARGETS
    libdiana_core
    libdiana_ui
    libdiana_batch
  DESTINATION
    ${LIBDIANA_DIR}
)
INSTALL(
  TARGETS
    diana_ui
    diana_batch
    ${DDIANA_OBS_EXECUTABLE}
  DESTINATION
    ${CMAKE_INSTALL_FULL_BINDIR}
)
INSTALL(
  FILES
    ${LIBDIANAUI_QM}
  DESTINATION
    "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME}/${PVERSION}/lang"
)

########################################################################

TARGET_LINK_LIBRARIES(libdiana_core
  ${PKGS_CORE}
)
TARGET_LINK_LIBRARIES(libdiana_ui
  libdiana_core
  ${PKGS_UI}
)
TARGET_LINK_LIBRARIES(libdiana_batch
  libdiana_core
  mi-programoptions
)

TARGET_LINK_LIBRARIES(diana_batch
  libdiana_batch
)
TARGET_LINK_LIBRARIES(diana_ui
  libdiana_ui
  mi-programoptions
)

CONFIGURE_FILE(diana_config.h.in diana_config.h)
