
AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = miRaster diField .

# libexecdiana contains "exec" so that automake uses install-exec
libexecdiana_LTLIBRARIES = libdiana.la

bin_PROGRAMS = bdiana

if WITH_GUI
bin_PROGRAMS += diana.bin
libexecdianadir= $(pkglibdir)/@PVERSION@
else
libexecdianadir= $(libdir)
endif # WITH_GUI

# convenience for developers using a non-standard $(prefix)
CPPFLAGS += -I$(prefix)/include
LDFLAGS  += -L$(prefix)/lib

BUILT_SOURCES =
CLEANFILES =

libdiana_la_SOURCES = \
	diAnnotationPlot.cc \
	diAreaBorder.cc \
	diAreaObjects.cc \
	diColour.cc \
	diColourShading.cc \
	diCommandParser.cc \
	diController.cc \
	diComplexSymbolPlot.cc \
	diContouring.cc \
	diDisplayObjects.cc \
	diEditManager.cc \
	diEditObjects.cc \
	diFieldEdit.cc \
	diFieldPlot.cc \
	diFieldPlotManager.cc \
	diFilledMap.cc \
	diFtnVfile.cc \
	diGlUtilities.cc \
	diImageGallery.cc \
	diImageIO.cc \
	diLegendPlot.cc \
	diLinetype.cc \
	diLocalSetupParser.cc \
	diLocationPlot.cc \
	diLogFile.cc \
	diMItiff.cc \
	diMapManager.cc \
	diMapPlot.cc \
	diMeasurementsPlot.cc \
	diObjectManager.cc \
	diObjectPlot.cc \
	diObjectPoint.cc \
	diObsAscii.cc \
	diObsManager.cc \
	diObsMetaData.cc \
	diObsPlot.cc \
	diPattern.cc \
	diPlot.cc \
	diPlotModule.cc \
	diPlotOptions.cc \
	diPoint.cc \
	diPolyContouring.cc \
	diPrintOptions.cc \
	diQuickMenues.cc \
	diRasterPlot.cc \
	diSat.cc \
	diSatManager.cc \
	diSatPlot.cc \
	diShapeObject.cc \
	diSpectrumData.cc \
	diSpectrumFile.cc \
	diSpectrumManager.cc \
	diSpectrumOptions.cc \
	diSpectrumPlot.cc \
	diStationManager.cc \
	diStationPlot.cc \
	diTrajectoryGenerator.cc \
	diTrajectoryPlot.cc \
	diUndoFront.cc \
	diUtilities.cc \
	diVprofData.cc \
	diVprofDiagram.cc \
	diVprofManager.cc \
	diVprofOptions.cc \
	diVprofPlot.cc \
	diVprofTables.cc \
	diWeatherArea.cc \
	diWeatherFront.cc \
	diWeatherObjects.cc \
	diWeatherSymbol.cc \
	EditItems/drawingcomposite.cc \
	EditItems/drawingitembase.cc \
	EditItems/drawingpolyline.cc \
	EditItems/drawingstylemanager.cc \
	EditItems/drawingsymbol.cc \
	EditItems/drawingtext.cc \
	EditItems/kml.cc \
	EditItems/timefilesextractor.cc \
	poly_contouring.cc \
	wmsclient/WebMapPainting.cc \
	wmsclient/WebMapUtilities.cc \
	diPainter.cc \
	diGLPainter.cc \
	diPaintGLPainter.cc

libdiana_la_SOURCES += \
	vcross_v2/VcrossSetup.cc \
	vcross_v2/VcrossComputer.cc \
	vcross_v2/VcrossCollector.cc \
	vcross_v2/VcrossEvaluate.cc \
	vcross_v2/VcrossOptions.cc \
	vcross_v2/VcrossQtAxis.cc \
	vcross_v2/VcrossQtContour.cc \
	vcross_v2/VcrossQtPaint.cc \
	vcross_v2/VcrossQtPlot.cc \
	vcross_v2/VcrossQtUtil.cc \
	vcross_v2/VcrossResolver.cc

libdiana_la_CPPFLAGS = -Ivcross_v2 -Ivcross_qt
libdiana_la_LIBADD = \
	$(BOOST_DATE_TIME_LIBS) \
	$(BOOST_REGEX_LIBS) $(BOOST_SYSTEM_LIBS)

libdiana_la_QTSOURCES = diManager.cc \
	diDrawingManager.cc \
	diOrderBook.cc \
	diOrderClient.cc \
	diOrderListener.cc \
	diOrderQueue.cc \
	diWorkOrder.cc \
	EditItems/itemgroup.cc \
	vcross_v2/VcrossQtManager.cc \
	vcross_v2/VcrossQuickmenues.cc \
	wmsclient/WebMapManager.cc \
	wmsclient/WebMapPlot.cc \
	wmsclient/WebMapService.cc \
	wmsclient/WebMapSlippyOSM.cc \
	wmsclient/WebMapTile.cc \
	wmsclient/WebMapWMS.cc \
	wmsclient/WebMapWMTS.cc

libdiana_la_QTHEADERS = \
	diVcrossInterface.h

diana_bin_SOURCES = \
	diFontManager.cc \
	diTesselation.c \
	diOpenGLPainter.cc \
	diOpenGLWidget.cc \
	diPaintable.cc

diana_bin_QTSOURCES = \
	diEditItemManager.cc \
	diPaintGLWidget.cc \
	EditItems/dialogcommon.cc \
	EditItems/drawingdialog.cc \
	EditItems/editcomposite.cc \
	EditItems/filterdrawingdialog.cc \
	EditItems/edititembase.cc \
	EditItems/editpolyline.cc \
	EditItems/editsymbol.cc \
	EditItems/edittext.cc \
	EditItems/properties.cc \
	EditItems/style.cc \
	EditItems/toolbar.cc \
	qtActionButton.cc \
	qtAddtoDialog.cc \
	qtAddtoMenu.cc \
	qtAnnotationDialog.cc \
	qtAnnoText.cc \
	qtBrowserBox.cc \
	qtButtonLayout.cc \
	qtComplexPressureText.cc \
	qtComplexText.cc \
	qtDataDialog.cc \
	qtEditComment.cc \
	qtEditDefineField.cc \
	qtEditDialog.cc \
	qtEditNewDialog.cc \
	qtEditText.cc \
	qtFieldDialog.cc \
	qtGLwidget.cc \
	qtGeoPosLineEdit.cc \
	qtMailDialog.cc \
	qtMainWindow.cc \
	qtMapDialog.cc \
	qtMeasurementsDialog.cc \
	qtObjectDialog.cc \
	qtObsDialog.cc \
	qtObsWidget.cc \
	qtQuickAdmin.cc \
	qtQuickEditOptions.cc \
	qtQuickMenu.cc \
	qtSatDialog.cc \
	qtSatDialogAdvanced.cc \
	qtSetupDialog.cc \
	qtShowSatValues.cc \
	qtSpectrumModelDialog.cc \
	qtSpectrumSetupDialog.cc \
	qtSpectrumWidget.cc \
	qtSpectrumWindow.cc \
	qtStationDialog.cc \
	qtStatusGeopos.cc \
	qtStatusPlotButtons.cc \
	qtTextDialog.cc \
	qtTextView.cc \
	qtTimeControl.cc \
	qtTimeSlider.cc \
	qtTimeSpinbox.cc \
	qtTimeStepSpinbox.cc \
	qtToggleButton.cc \
	qtTrajectoryDialog.cc \
	qtUffdaDialog.cc \
	qtVprofModelDialog.cc \
	qtVprofSetupDialog.cc \
	qtVprofWidget.cc \
	qtVprofWindow.cc \
	qtWorkArea.cc 

diana_bin_QTSOURCES += \
	vcross_qt/qtVcrossSetup.cc \
	vcross_qt/qtVcrossInterface.cc \
	vcross_qt/qtVcrossLayerBar.cc \
	vcross_qt/qtVcrossLayerButton.cc \
	vcross_qt/qtVcrossLayerList.cc \
	vcross_qt/qtVcrossSetupDialog.cc \
	vcross_qt/qtVcrossWindow.cc \
	vcross_qt/qtVcrossAddPlotDialog.cc \
	vcross_qt/qtVcrossModelPage.cc \
	vcross_qt/qtVcrossReftimePage.cc \
	vcross_qt/qtVcrossReplaceModelDialog.cc \
	vcross_qt/qtVcrossStyleDialog.cc \
	vcross_qt/qtVcrossStyleWidget.cc \
	vcross_qt/VcrossQtWidget.cc \
	wmsclient/WebMapDialog.cc

diana_bin_QTFORMS = \
	vcross_qt/vcross_plot_add_dialog.ui \
	vcross_qt/vcross_modelpage.ui \
	vcross_qt/vcross_reftimepage.ui \
	vcross_qt/vcross_replace_model_dialog.ui \
	vcross_qt/vcross_style_dialog.ui \
	vcross_qt/vcross_style_widget.ui \
	vcross_qt/vcross_window.ui \
	wmsclient/webmap_dialog.ui

noinst_HEADERS = \
	MovieMaker.h \
	diAnnotationPlot.h \
	diAreaBorder.h \
	diAreaObjects.h \
	diColour.h \
	diColourShading.h \
	diCommandParser.h \
	diCommonTypes.h \
	diComplexSymbolPlot.h \
	diContouring.h \
	diDisplayObjects.h \
	diDrawingTypes.h \
	diDrawingManager.h \
	diEditItemManager.h \
	diEditManager.h \
	diEditObjects.h \
	diEditSpec.h \
	diFieldEdit.h \
	diFieldPlot.h \
	diFieldPlotManager.h \
	diFilledMap.h \
	diFontManager.h \
	diFtnVfile.h \
	diGlUtilities.h \
	diHDF5.h \
	diImageGallery.h \
	diImageIO.h \
	diKeys.h \
	diLegendPlot.h \
	diLinetype.h \
	diLocationData.h \
	diLocationPlot.h \
	diLogFile.h \
	diMItiff.h \
	diManager.h \
	diMapManager.h \
	diMapMode.h \
	diMapPlot.h \
	diMeasurementsPlot.h \
	diObjectManager.h \
	diObjectPlot.h \
	diObjectPoint.h \
	diObsAscii.h \
	diObsBufr.h \
	diObsData.h \
	diObsManager.h \
	diObsMetaData.h \
	diObsPlot.h \
	diOrderBook.h \
	diOrderClient.h \
	diOrderListener.h \
	diOrderQueue.h \
	diPattern.h \
	diPlot.h \
	diPlotModule.h \
	diPlotOptions.h \
	diPoint.h \
	diPrintOptions.h \
	diQuickMenues.h \
	diRasterPlot.h \
	diSat.h \
	diSatManager.h \
	diSatPlot.h \
	diShapeObject.h \
	diSpectrumData.h \
	diSpectrumFile.h \
	diSpectrumManager.h \
	diSpectrumOptions.h \
	diSpectrumPlot.h \
	diStationManager.h \
	diStationPlot.h \
	diTesselation.h \
	diTrajectoryGenerator.h \
	diTrajectoryPlot.h \
	diUndoFront.h \
	diUtilities.h \
	diVprofData.h \
	diVprofDiagram.h \
	diVprofManager.h \
	diVprofOptions.h \
	diVprofPlot.h \
	diVprofTables.h \
	diWeatherArea.h \
	diWeatherFront.h \
	diWeatherObjects.h \
	diWeatherSymbol.h \
	diWorkOrder.h \
	EditItems/dialogcommon.h \
	EditItems/drawingcomposite.h \
	EditItems/drawingdialog.h \
	EditItems/drawingitembase.h \
	EditItems/drawingpolyline.h \
	EditItems/drawingstylemanager.h \
	EditItems/drawingsymbol.h \
	EditItems/drawingtext.h \
	EditItems/editcomposite.h \
	EditItems/filterdrawingdialog.h \
	EditItems/edititembase.h \
	EditItems/editpolyline.h \
	EditItems/editsymbol.h \
	EditItems/edittext.h \
	EditItems/kml.h \
	EditItems/itemgroup.h \
	EditItems/properties.h \
	EditItems/style.h \
	EditItems/timefilesextractor.h \
	EditItems/toolbar.h \
	poly_contouring.hh \
	reversible_list.hh \
	polyStipMasks.h \
	qtAddtoDialog.h \
	qtAddtoMenu.h \
	qtAnnotationDialog.h \
	qtAnnoText.h \
	qtBrowserBox.h \
	qtButtonLayout.h \
	qtComplexPressureText.h \
	qtComplexText.h \
	qtDataDialog.h \
	qtEditComment.h \
	qtEditText.h \
	qtEditDefineField.h \
	qtEditDialog.h \
	qtEditNewDialog.h \
	qtFieldDialog.h \
	qtGLwidget.h \
	qtGeoPosLineEdit.h \
	qtImageGallery.h \
	qtMailDialog.h \
	qtMainWindow.h \
	qtMapDialog.h \
	qtMeasurementsDialog.h \
	qtObjectDialog.h \
	qtObsDialog.h \
	qtObsWidget.h \
	qtPrintManager.h \
	qtQuickAdmin.h \
	qtQuickEditOptions.h \
	qtQuickMenu.h \
	qtSatDialog.h \
	qtSatDialogAdvanced.h \
	qtSetupDialog.h \
	qtShowSatValues.h \
	qtSpectrumModelDialog.h \
	qtSpectrumSetupDialog.h \
	qtSpectrumWidget.h \
	qtSpectrumWindow.h \
	qtStationDialog.h \
	qtStatusGeopos.h \
	qtStatusPlotButtons.h \
	qtTextDialog.h \
	qtTextView.h \
	qtTimeControl.h \
	qtTimeSlider.h \
	qtTimeSpinbox.h \
	qtTimeStepSpinbox.h \
	qtToggleButton.h \
	qtTrajectoryDialog.h \
	qtUffdaDialog.h \
	qtUtility.h \
	qtVprofModelDialog.h \
	qtVprofSetupDialog.h \
	qtVprofWidget.h \
	qtVprofWindow.h \
	qtWorkArea.h \
	signalhelper.h \
	bdiana_capi.h 

noinst_HEADERS += \
	diVcrossInterface.h \
	vcross_v2/VcrossSetup.h \
	vcross_v2/VcrossComputer.h \
	vcross_v2/VcrossCollector.h \
	vcross_v2/VcrossEvaluate.h \
	vcross_v2/VcrossResolver.h \
	vcross_v2/VcrossOptions.h \
	vcross_v2/VcrossQtContour.h \
	vcross_v2/VcrossQtManager.h \
	vcross_v2/VcrossQtPaint.h \
	vcross_v2/VcrossQtPlot.h \
	vcross_v2/VcrossQtUtil.h \
	vcross_v2/VcrossQtAxis.h \
	vcross_qt/qtVcrossInterface.h \
	vcross_qt/qtVcrossLayerBar.h \
	vcross_qt/qtVcrossLayerButton.h
	src/vcross_qt/qtVcrossLayerList.h \
	vcross_qt/qtVcrossModelPage.h \
	vcross_qt/qtVcrossReftimePage.h \
	src/vcross_qt/qtVcrossReplaceModelDialog.h \
	vcross_qt/qtVcrossSetup.h \
	vcross_qt/qtVcrossSetupDialog.h \
	vcross_qt/qtVcrossStyleDialog.h \
	vcross_qt/qtVcrossStyleWidget.h \
	vcross_qt/VcrossQtWidget.h \
	vcross_qt/qtVcrossWindow.h \
	vcross_qt/qtVcrossSetup.h 

noinst_HEADERS += \
	wmsclient/WebMapDialog.h \
	wmsclient/WebMapManager.h \
	wmsclient/WebMapPainting.h \
	wmsclient/WebMapPlot.h \
	wmsclient/WebMapService.h \
	wmsclient/WebMapSlippyOSM.h \
	wmsclient/WebMapTile.h \
	wmsclient/WebMapUtilities.h \
	wmsclient/WebMapWMS.h \
	wmsclient/WebMapWMTS.h

include_HEADERS = \
        diController.h \
        diLocalSetupParser.h

# xpm files needed to build
diana_bin_xpm_SOURCES= active_object.xpm \
	autoupdate.xpm \
	back.xpm \
	bakover.xpm \
	balloon.xpm \
	blocks.xpm \
	bookmark.xpm \
	clock.xpm \
	convert.xpm \
	custom_user.xpm \
	dialoger.xpm \
	diana_icon.xpm \
	directory.xpm \
	dnmi.xpm \
	down12x12.xpm \
	down12x14.xpm \
	down20x20.xpm \
	down32x24.xpm \
	earth3.xpm \
	editcopy.xpm \
	editcut.xpm \
	edit_lock_value.xpm \
	editmode.xpm \
	edit_open_value.xpm \
	editpaste.xpm \
	editundo.xpm \
	exit.xpm \
	face_grin.xpm \
	felt.xpm \
	fet_object_fog.xpm \
	fet_object_normal.xpm \
	fet_object_p.xpm \
	fet_object_rain.xpm \
	fet_object_sky.xpm \
	fet_object_tmp.xpm \
	fet_object_wave.xpm \
	fet_object_wind.xpm \
	filenew.xpm \
	fileopen.xpm \
	fileprint.xpm \
	filesave.xpm \
	forover.xpm \
	forward.xpm \
	front.xpm \
	idnumDown.xpm \
	idnumUp.xpm \
	inactive_object.xpm \
	info.xpm \
	kill.xpm \
	levelDown.xpm \
	levelUp.xpm \
	linear_copy.xpm \
	linear_down.xpm \
	linear_remove.xpm \
	locked_bookmark.xpm \
	locked_directory.xpm \
	loop.xpm \
	magnify2.xpm \
	metno_sort_txt.xpm \
	minus12x12.xpm \
	multiple_users.xpm \
	no_object.xpm \
	paint_add_crusor.xpm \
	paint_add_point.xpm \
	paint_color.xpm \
	paint_create_polyline.xpm \
	paint_create_symbol.xpm \
	paint_cursor.xpm \
	paint_cut.xpm \
	paint_delete.xpm \
	paint_draw.xpm \
	paint_forbidden_crusor.xpm \
	paint_help.xpm \
	paint_hide.xpm \
	paint_include.xpm \
	paint_mode.xpm \
	paint_move_point.xpm \
	paint_move.xpm \
	paint_new.xpm \
	paint_redo.xpm \
	paint_remove_crusor.xpm \
	paint_remove_point.xpm \
	paint_select.xpm \
	paint_select2.xpm \
	paint_spatial.xpm \
	paint_undo.xpm \
	parent_object.xpm \
	pencil2.xpm \
	pick.xpm \
	preferences.xpm \
	private_directory.xpm \
	question.xpm \
	ruler.xpm \
	redo.xpm \
	revert.xpm \
	Robot.xpm \
	run_smooth.xpm \
	sat.xpm \
	session_deployed.xpm \
	session_finalize.xpm \
	session_lock.xpm \
	session_open.xpm \
	session_operational.xpm \
	shuttle.xpm \
	single_remove.xpm \
	slutt.xpm \
	small_metno_sort.xpm \
	spectrum.xpm \
	start.xpm \
	stopp.xpm \
	station.xpm \
	sun2.xpm \
	synop_red.xpm \
	synop.xpm \
	tb_close.xpm \
	tb_left_arrow.xpm \
	tb_print.xpm \
	tb_right_arrow.xpm \
	thumbs_down.xpm \
	thumbs_up.xpm \
	Tool_32_draw.xpm \
	traj.xpm \
	trashcan.xpm \
	undo.xpm \
	up12x12.xpm \
	up12x14.xpm \
	up20x20.xpm \
	up32x24.xpm \
	user_admin.xpm \
	user.xpm \
	vcross.xpm \
	vprof_normal.xpm \
	vprof_selected.xpm \
	weather_rain.xpm \
	weather_storm.xpm

libdiana_la_CPPFLAGS += \
	${FIMEX_CPPFLAGS} \
	$(METLIBS_CPPFLAGS) \
	$(EMOS_CPPFLAGS) \
	$(SHP_CPPFLAGS) \
	$(CURL_CPPFLAGS) \
	$(PNG_CPPFLAGS) \
	${GEOTIFF_CPPFLAGS} \
	$(TIFF_CPPFLAGS) \
	$(UDUNITS_CPPFLAGS)

libdiana_la_LDFLAGS = \
	${FIMEX_LDFLAGS} \
	$(METLIBS_LDFLAGS) \
	$(EMOS_LDFLAGS) \
	$(SHP_LDFLAGS) \
	$(CURL_LDFLAGS) \
	$(PNG_LDFLAGS) \
	${GEOTIFF_LDFLAGS} \
	$(TIFF_LDFLAGS) \
	$(UDUNITS_LDFLAGS)

libdiana_la_LIBADD += \
	miRaster/libmiRaster.la \
	diField/libdiField.la \
	${FIMEX_LIBS} \
	$(METLIBS_LIBS) \
	$(EMOS_LIBS) $(FORTRAN_LIBS) \
	$(SHP_LIBS) \
	$(CURL_LIBS) \
	$(PNG_LIBS) \
	${GEOTIFF_LIBS} \
	$(TIFF_LIBS) \
	$(UDUNITS_LIBS)

# Additional source code for SMHI support
if WITH_SMHI
libdiana_la_CPPFLAGS += -DSMHI
endif

# Additional source code for mora support
if WITH_NEWARKOBS
libdiana_la_CPPFLAGS += -DROADOBS -DNEWARK_INC
libdiana_la_SOURCES += \
	diObsRoad.cc \
	diVprofRTemp.cc
libdiana_la_LDFLAGS += \
	${NEWARKAPI_LDFLAGS}
libdiana_la_LIBADD += \
	${NEWARKAPI_LIBS}
endif

# Additional source code for Geotiff support
if WITH_GEOTIFF
libdiana_la_CPPFLAGS += -DGEOTIFF
libdiana_la_SOURCES += \
	diGEOtiff.cc
noinst_HEADERS += \
	diGEOtiff.h
libdiana_la_LDFLAGS += ${GEOTIFF_LDFLAGS} ${BOOST_THREAD_LDFLAGS}
libdiana_la_LIBADD += ${GEOTIFF_LIBS} ${BOOST_THREAD_LIBS}
endif

# Additional source code for HDF5 support
if WITH_HDF5
libdiana_la_CPPFLAGS += -DHDF5FILE
libdiana_la_SOURCES += \
	diHDF5.cc
noinst_HEADERS += \
	diHDF5.h
libdiana_la_LDFLAGS += ${HDF5_LDFLAGS}
libdiana_la_LIBADD += ${HDF5_LIBS}
endif

# Additional source code for observation buffers
if WITH_OBS_BUFR
libdiana_la_SOURCES += \
	diObsBufr.cc
libdiana_la_CPPFLAGS += -DBUFROBS
endif

# Additional source code for video export feature
if WITH_VIDEO_EXPORT
libdiana_la_SOURCES  += MovieMaker.cc
libdiana_la_CPPFLAGS += -DVIDEO_EXPORT $(AVFORMAT_CPPFLAGS)
libdiana_la_LDFLAGS  += $(AVFORMAT_LDFLAGS)
libdiana_la_LIBADD   += $(AVFORMAT_LIBS)
endif

libdiana_la_SOURCES += $(libdiana_la_QTSOURCES)
nodist_libdiana_la_SOURCES = $(libdiana_la_QTSOURCES:.cc=.moc.cc) $(libdiana_la_QTHEADERS:.h=.moc.cc)
BUILT_SOURCES += $(nodist_libdiana_la_SOURCES)
CLEANFILES    += $(nodist_libdiana_la_SOURCES)

libdiana_la_CPPFLAGS += $(QT4_CPPFLAGS)
libdiana_la_LDFLAGS  += $(QT4_LDFLAGS)
libdiana_la_LIBADD   += $(QT4_LIBS)

libdiana_la_CPPFLAGS += $(WIN32_CPPFLAGS)
libdiana_la_LDFLAGS  += $(WIN32_LDFLAGS)
libdiana_la_LIBADD   += $(WIN32_LIBS)

libdiana_la_SOURCES += \
	bdiana_capi.cc \
	signalhelper.cc

libdiana_la_LDFLAGS += -version-number @PVERSION_MAJOR@:@PVERSION_MINOR@:@PVERSION_PATCH@

%.moc.cc: %.h
	@mkdir -p $(shell dirname $@)
	$(MOC4) $(DEFAULT_INCLUDES) -o $@ $<

########################################################################
########################################################################
########################################################################

language_sources=$(top_srcdir)/share/diana/lang/diana_no.ts \
	$(top_srcdir)/share/diana/lang/diana_es.ts \
	$(top_srcdir)/share/diana/lang/diana_ru.ts \
	$(top_srcdir)/share/diana/lang/diana_sv.ts \
	$(top_srcdir)/share/diana/lang/diana_en.ts

languagedir= $(datarootdir)/diana/@PVERSION@/lang
nodist_language_DATA=$(language_sources:.ts=.qm)

CLEANFILES += $(nodist_language_DATA)

lupdate:
	$(LUPDATE4) $(diana_bin_QTSOURCES:%=$(srcdir)/%) \
	    $(srcdir)/qtUtility.cc \
	    $(diana_bin_QTFORMS:%=$(srcdir)/%) \
	    -ts $(language_sources)
.PHONY: lupdate

%.qm: %.ts  $(diana_bin_QTSOURCES)
	$(LRELEASE4)  $<

diana_uic_headers = $(diana_bin_QTFORMS:.ui=.ui.h)

BUILT_SOURCES  += $(diana_uic_headers)
CLEANFILES     += $(diana_uic_headers)
noinst_HEADERS += $(diana_uic_headers)

%.ui.h: %.ui
	@mkdir -p $(shell dirname $@)
	$(UIC4) -o $@ $<

diana_bin_SOURCES += $(diana_bin_QTSOURCES)
nodist_diana_bin_SOURCES = $(diana_bin_QTSOURCES:.cc=.moc.cc)
BUILT_SOURCES += $(nodist_diana_bin_SOURCES)
CLEANFILES    += $(nodist_diana_bin_SOURCES)

# diana-specific sources (the rest is in libdiana)
diana_bin_SOURCES += \
	$(diana_bin_xpm_SOURCES) \
	$(language_sources) \
	main_gui.cc \
	qtImageGallery.cc \
	qtPrintManager.cc \
	qtUtility.cc

diana_bin_CPPFLAGS = \
	-I $(builddir)/vcross_qt \
	${FIMEX_CPPFLAGS} \
	$(METLIBSUI_CPPFLAGS) \
	$(QT4_CPPFLAGS)

diana_bin_LDFLAGS = \
	${FIMEX_LDFLAGS} \
	$(METLIBSUI_LDFLAGS) \
	$(QT4_LDFLAGS)

diana_bin_LDADD = \
	${FIMEX_LIBS} \
	$(METLIBSUI_LIBS) \
	$(QT4_LIBS) \
	libdiana.la

if WITH_SMHI
diana_bin_CPPFLAGS += -DSMHI
endif

if WITH_VIDEO_EXPORT
diana_bin_CPPFLAGS += -DVIDEO_EXPORT
endif

if WITH_XLIB
diana_bin_CPPFLAGS += -DUSE_XLIB
diana_bin_LDADD += -lX11
endif # WITH_XLIB

# generate version info header
BUILT_SOURCES += diBuild.cc
CLEANFILES += diBuild.cc diBuild.cc.tmp

libdiana_la_SOURCES += diBuild.cc
diBuild.cc: diBuild.cc.tmp
	@if cmp -s $@ $< ; then \
	    echo "UNCHANGED $@"; \
	else \
	    cp -f $< $@ ; \
	    echo "UPDATED $@:"; \
	    cat $@ ; \
	fi

diBuild.cc.tmp:
	@today=`date +%Y-%m-%d` ; \
	if git --git-dir="$(top_srcdir)/.git" --work-tree="$(top_srcdir)" status > /dev/null; then \
	    commit=`git --git-dir="$(top_srcdir)/.git" --work-tree="$(top_srcdir)" describe --abbrev=40 --dirty --always`; \
	else \
	    commit="unknown"; \
	fi; \
	echo "#include <diBuild.h>" > $@ ; \
	echo 'const char diana_build_string[] = "'$$today'";'  >> $@ ; \
	echo 'const char diana_build_commit[] = "'$$commit'";' >> $@
.PHONY: diBuild.cc.tmp

########################################################################
########################################################################
########################################################################

bdiana_SOURCES = bdiana.cc
bdiana_LDADD = libdiana.la
