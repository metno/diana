# $Id$

AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = . test/gtest test

if WITH_BATCH_ONLY
bin_PROGRAMS = bdiana
else
bin_PROGRAMS = diana.bin bdiana 
endif
noinst_LIBRARIES = libdiana.a

# automake requires one = before the first +=
common_cppflags =
common_ldflags =
common_ldadd = libdiana.a
CLEANFILES =

# convenience for developers using a non-standard $(prefix)
common_cppflags += -I$(prefix)/include
common_ldflags += -L$(prefix)/lib

# C source code common to diana and bdiana
common_c_sources = \
        diTesselation.c

# C++ source code common to diana and bdiana
common_cxx_sources = \
	diAnnotationPlot.cc \
	diAreaBorder.cc \
	diAreaObjects.cc \
	diColour.cc \
	diColourShading.cc \
	diCommandParser.cc \
	diComplexSymbolPlot.cc \
	diContouring.cc \
	diPolyContouring.cc \
	diController.cc \
	diDisplayObjects.cc \
	diEditManager.cc \
	diEditObjects.cc \
	diFieldEdit.cc \
	diFieldPlot.cc \
	diFieldPlotManager.cc \
	diFilledMap.cc \
	diFontManager.cc \
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
	diPrintOptions.cc \
	diQuickMenues.cc \
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
	poly_contouring.cc

common_cppflags += -Ivcross_v2 -Ivcross_qt
common_cxx_sources += \
	vcross_v2/VcrossSetup.cc \
	vcross_v2/VcrossComputer.cc \
	vcross_v2/VcrossCollector.cc \
	vcross_v2/VcrossEvaluate.cc \
	vcross_v2/VcrossResolver.cc \
	vcross_v2/VcrossOptions.cc

common_ldadd += \
	$(BOOST_REGEX_LIBS) $(BOOST_SYSTEM_LIBS)

# Qt sources
common_qt_sources = diManager.cc \
	diDrawingManager.cc

common_qt_sources += \
	vcross_v2/VcrossQtContour.cc \
	vcross_v2/VcrossQtManager.cc \
	vcross_v2/VcrossQtPaint.cc \
	vcross_v2/VcrossQtPlot.cc \
	vcross_v2/VcrossQtUtil.cc \
	vcross_v2/VcrossQtAxis.cc \
	vcross_v2/VcrossQuickmenues.cc

common_qt_headers = \
	diVcrossInterface.h

diana_qt_forms =

if WITH_BATCH_ONLY
diana_qt_sources =
else
diana_qt_sources = \
	diEditItemManager.cc \
	EditItems/editcomposite.cc \
	EditItems/edititembase.cc \
	EditItems/editpolyline.cc \
	EditItems/editsymbol.cc \
	EditItems/edittext.cc \
	EditItems/properties.cc \
	EditItems/style.cc \
	EditItems/toolbar.cc \
	EditItems/drawingdialog.cc \
	EditItems/editdrawingdialog.cc \
	EditItems/eimtestdialog.cc \
	EditItems/layergroupspane.cc \
	EditItems/layerspanebase.cc \
	EditItems/drawinglayerspane.cc \
	EditItems/editdrawinglayerspane.cc \
	EditItems/dialogcommon.cc \
	EditItems/modifylayerscommand.cc \
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

diana_qt_sources += \
	vcross_qt/qtVcrossSetup.cc \
	vcross_qt/qtVcrossInterface.cc \
	vcross_qt/qtVcrossLayerBar.cc \
	vcross_qt/qtVcrossLayerButton.cc \
	vcross_qt/qtVcrossSetupDialog.cc \
	vcross_qt/qtVcrossWindow.cc \
	vcross_qt/qtVcrossAddPlotDialog.cc \
	vcross_qt/qtVcrossStyleDialog.cc \
	vcross_qt/qtVcrossStyleWidget.cc \
	vcross_qt/VcrossQtWidget.cc 

diana_qt_forms += \
	vcross_qt/vcross_plot_add_dialog.ui \
	vcross_qt/vcross_style_dialog.ui \
	vcross_qt/vcross_style_widget.ui \
	vcross_qt/vcross_window.ui

common_cppflags += \
	$(QUTILITIES_CPPFLAGS)

common_ldflags += \
	$(QUTILITIES_LDFLAGS)

common_ldadd += \
	$(QUTILITIES_LIBS)

common_cppflags += \
	$(COSERVER_CPPFLAGS)

common_ldflags += \
	$(COSERVER_LDFLAGS)

common_ldadd += \
	$(COSERVER_LIBS)
endif

bdiana_qt_sources = \
	diOrderBook.cc \
	diOrderClient.cc \
	diOrderListener.cc \
	diOrderQueue.cc \
	diWorkOrder.cc \
	EditItems/layer.cc \
	EditItems/layergroup.cc \
	EditItems/layermanager.cc \
	EditItems/timefilesextractor.cc


if WITH_EMBEDDED
bdiana_qt_sources += \
	PaintGL/paintgl.cc
endif

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
	diEditItemManager.h \
	EditItems/drawingcomposite.h \
	EditItems/drawingitembase.h \
	EditItems/drawingpolyline.h \
	EditItems/drawingstylemanager.h \
	EditItems/drawingsymbol.h \
	EditItems/drawingtext.h \
	EditItems/editcomposite.h \
	EditItems/edititembase.h \
	EditItems/editpolyline.h \
	EditItems/editsymbol.h \
	EditItems/edittext.h \
	EditItems/layer.h \
	EditItems/layergroup.h \
	EditItems/layermanager.h \
	EditItems/kml.h \
	EditItems/properties.h \
	EditItems/style.h \
	EditItems/timefilesextractor.h \
	EditItems/toolbar.h \
	EditItems/drawingdialog.h \
	EditItems/editdrawingdialog.h \
	EditItems/eimtestdialog.h \
	EditItems/layergroupspane.h \
	EditItems/layerspanebase.h \
	EditItems/drawinglayerspane.h \
	EditItems/editdrawinglayerspane.h \
	EditItems/dialogcommon.h \
	EditItems/modifylayerscommand.h \
	diDrawingManager.h \
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
	vcross_qt/qtVcrossSetupDialog.h \
	vcross_qt/VcrossQtWidget.h \
	vcross_qt/qtVcrossWindow.h \
	vcross_qt/qtVcrossSetup.h 

if WITH_EMBEDDED
if WITH_BATCH_ONLY
# Headers
include_HEADERS = \
        diController.h \
        diLocalSetupParser.h
else
noinst_HEADERS += \
        diController.h \
        diLocalSetupParser.h
endif
else
noinst_HEADERS += \
        diController.h \
        diLocalSetupParser.h
endif

# xpm files needed to build
common_xpm_sources= active_object.xpm \
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

common_cppflags += \
	$(DIFIELD_CPPFLAGS) \
	$(MIRASTER_CPPFLAGS) \
	$(PUCTOOLS_CPPFLAGS) \
	$(EMOS_CPPFLAGS) \
	$(PUDATATYPES_CPPFLAGS) \
	$(SHP_CPPFLAGS) \
	$(CURL_CPPFLAGS) \
	$(PNG_CPPFLAGS) \
	$(UU_CPPFLAGS)

common_ldflags += \
	$(DIFIELD_LDFLAGS) \
	$(MIRASTER_LDFLAGS) \
	$(PUCTOOLS_LDFLAGS) \
	$(EMOS_LDFLAGS) \
	$(PUDATATYPES_LDFLAGS) \
	$(SHP_LDFLAGS) \
	$(CURL_LDFLAGS) \
	$(PNG_LDFLAGS) \
	$(UU_LDFLAGS)

common_ldadd += \
	$(DIFIELD_LIBS) \
	$(MIRASTER_LIBS) \
	$(PUCTOOLS_LIBS) \
	$(EMOS_LIBS) $(FORTRAN_LIBS) \
	$(PUDATATYPES_LIBS) \
	$(SHP_LIBS) \
	$(CURL_LIBS) \
	$(PNG_LIBS) \
	$(UU_LIBS)

# Additional source code for SMHI support
if WITH_SMHI
common_cppflags += -DSMHI
endif

# Additional source code for mora support
if WITH_NEWARKOBS
common_cppflags += -DROADOBS -DNEWARK_INC
common_cxx_sources += \
	diObsRoad.cc \
	diVprofRTemp.cc
common_ldflags += \
	${NEWARKAPI_LDFLAGS}
common_ldadd += \
	${NEWARKAPI_LIBS}
endif

# Additional source code for Geotiff support
if WITH_GEOTIFF
common_cppflags += -DGEOTIFF
common_cxx_sources += \
	diGEOtiff.cc
noinst_HEADERS += \
	diGEOtiff.h
endif

# Additional source code for HDF5 support
if WITH_HDF5
common_cppflags += -DHDF5FILE
common_cxx_sources += \
	diHDF5.cc
noinst_HEADERS += \
	diHDF5.h
endif

# Additional source code for observation buffers
if WITH_OBS_BUFR
common_cxx_sources += \
	diObsBufr.cc
common_cppflags += -DBUFROBS
endif

# Additional source code for video export feature
if WITH_VIDEO_EXPORT
common_cxx_sources += MovieMaker.cc
common_cppflags += -DVIDEO_EXPORT $(AVFORMAT_CPPFLAGS)
common_ldflags += $(AVFORMAT_LDFLAGS)
common_ldadd += $(AVFORMAT_LIBS)
endif

# Use Xlib-specific routines
if WITH_XLIB
common_cppflags += -DUSE_XLIB
common_ldadd += -lX11
endif

# Embedded build support - remove X and OpenGL dependencies
if WITH_EMBEDDED
common_cppflags += \
        -I$(top_srcdir)/src/PaintGL -I$(top_srcdir)/src/PaintGL/GL \
        -DUSE_PAINTGL

common_c_sources += \
        PaintGL/GL/libtess/dict.c \
        PaintGL/GL/libtess/geom.c \
        PaintGL/GL/libtess/memalloc.c \
        PaintGL/GL/libtess/mesh.c \
        PaintGL/GL/libtess/normal.c \
        PaintGL/GL/libtess/priorityq.c \
        PaintGL/GL/libtess/render.c \
        PaintGL/GL/libtess/sweep.c \
        PaintGL/GL/libtess/tess.c \
        PaintGL/GL/libtess/tessmono.c \
        PaintGL/GL/libutil/error.c \
        PaintGL/GL/libutil/glue.c

noinst_HEADERS +=  \
        PaintGL/GL/gl.h \
        PaintGL/GL/glu.h \
        PaintGL/GL/gluos.h \
        PaintGL/GL/libtess/dict.h \
        PaintGL/GL/libtess/dict-list.h \
        PaintGL/GL/libtess/geom.h \
        PaintGL/GL/libtess/memalloc.h \
        PaintGL/GL/libtess/mesh.h \
        PaintGL/GL/libtess/normal.h \
        PaintGL/GL/libtess/priorityq.h \
        PaintGL/GL/libtess/priorityq-sort.h \
        PaintGL/GL/libtess/render.h \
        PaintGL/GL/libtess/sweep.h \
        PaintGL/GL/libtess/tess.h \
        PaintGL/GL/libtess/tessmono.h \
        PaintGL/GL/libutil/gluint.h

if WITH_BATCH_ONLY
include_HEADERS += PaintGL/paintgl.h
else
noinst_HEADERS += PaintGL/paintgl.h
endif

else
common_cppflags += \
        $(FTGL_CPPFLAGS) \
        $(GLP_CPPFLAGS) \
        $(GLTEXT_CPPFLAGS) \
        $(GLU_CPPFLAGS)

common_ldflags += \
        $(FTGL_LDFLAGS) \
        $(GLP_LDFLAGS) \
        $(GLTEXT_LDFLAGS) \
        $(GLU_LDFLAGS)

common_ldadd += \
        $(FTGL_LIBS) \
        $(GLP_LIBS) \
        $(GLTEXT_LIBS) \
        $(GLU_LIBS)
endif

# Generated Qt source code
common_moc_sources = $(common_qt_sources:.cc=.moc.cc) $(common_qt_headers:.h=.moc.cc)
diana_moc_sources = $(diana_qt_sources:.cc=.moc.cc)
bdiana_moc_sources = $(bdiana_qt_sources:.cc=.moc.cc)
diana_uic_headers = $(diana_qt_forms:.ui=.ui.h)
CLEANFILES += $(common_moc_sources) $(diana_moc_sources) $(bdiana_moc_sources) $(diana_uic_headers)
%.moc.cc: %.h
	@mkdir -p $(shell dirname $@)
	$(MOC4) $(DEFAULT_INCLUDES) -o $@ $<

%.ui.h: %.ui
	@mkdir -p $(shell dirname $@)
	$(UIC4) -o $@ $<

BUILT_SOURCES = $(diana_uic_headers)

language_sources=$(top_srcdir)/share/diana/lang/diana_no.ts \
	$(top_srcdir)/share/diana/lang/diana_es.ts \
	$(top_srcdir)/share/diana/lang/diana_ru.ts \
	$(top_srcdir)/share/diana/lang/diana_sv.ts

languagedir= $(datarootdir)/diana/@PVERSION@/lang
nodist_language_DATA=$(language_sources:.ts=.qm)

CLEANFILES += $(nodist_language_DATA)

lupdate:
	$(LUPDATE4) $(diana_qt_sources:%=$(srcdir)/%) \
	    $(diana_qt_forms:%=$(srcdir)/%) \
	    -ts $(language_sources)
.PHONY: lupdate

%.qm: %.ts  $(diana_qt_sources)
	$(LRELEASE4)  $<

# Problem:
#
#  - zlib is used by several other libraries, so it must come last
#  - QtCore contains a complete copy of zlib
#  - hence, we can not link with *both* Qt *and* zlib
#  - therefore, we link with Qt only, and place it at the end.
#
common_cppflags += $(QT4_CPPFLAGS)
common_ldflags += $(QT4_LDFLAGS)
common_ldadd += $(QT4_LIBS)

# Win32 support
common_cppflags += $(WIN32_CPPFLAGS)
common_ldflags += $(WIN32_LDFLAGS)
common_ldadd += $(WIN32_LIBS)

# Full set of source code for libdiana
libdiana_a_SOURCES = \
	$(common_c_sources) \
	$(common_cxx_sources) \
	$(common_qt_sources) \
	$(bdiana_qt_sources) \
	bdiana_capi.cc \
	signalhelper.cc

nodist_libdiana_a_SOURCES = \
        $(common_moc_sources) \
        $(bdiana_moc_sources)

libdiana_a_CPPFLAGS = \
	$(common_cppflags)
# no LDFLAGS / LDADD needed for libdiana

# diana-specific sources (the rest is in libdiana)
diana_bin_SOURCES = \
	$(diana_qt_sources) \
	$(common_xpm_sources) \
	$(language_sources) \
	main_gui.cc \
	qtImageGallery.cc \
	qtPrintManager.cc \
	qtUtility.cc

nodist_diana_bin_SOURCES = \
	$(diana_moc_sources)

diana_bin_CPPFLAGS = $(common_cppflags)
diana_bin_LDFLAGS = $(common_ldflags)
diana_bin_LDADD = $(common_ldadd)

# bdiana-specific sources (the rest is in libdiana)
bdiana_SOURCES = \
	bdiana.cc

nodist_bdiana_SOURCES =

bdiana_CPPFLAGS = $(common_cppflags)
bdiana_LDFLAGS = $(common_ldflags)
bdiana_LDADD = $(common_ldadd)

# Generated header
main_gui.cc: diBuild.h
diBuild.h:
	@if [ -s $@ ]; then \
		olddate=`grep build_string $@ | cut -f2 -d\"`; \
	else \
		olddate="firsttime"; \
	fi; \
	today=`date +%Y-%m-%d`; \
	if [ ! x$$olddate = x$$today ]; then \
	  	echo '#define build_string "'$$today'"' >$@ ; \
	  	echo "UPDATED $@ $$today"; \
	else \
	  	echo "UNCHANGED $@ $$olddate"; \
	fi

CLEANFILES += diBuild.h

if WITH_EMBEDDED
# A shared library target for language binding experiments
lib_LTLIBRARIES = libdiana.la
if WITH_BATCH_ONLY
libdiana_la_SOURCES = $(libdiana_a_SOURCES)
else
libdiana_la_SOURCES = $(diana_bin_SOURCES) $(diana_moc_sources)
libdiana_la_SOURCES -= main_gui.cc
endif
libdiana_la_CPPFLAGS = $(libdiana_a_CPPFLAGS)
libdiana_la_LDFLAGS = $(common_ldflags) -version-number @PVERSION_MAJOR@:@PVERSION_MINOR@:@PVERSION_PATCH@
libdiana_la_LIBADD = @LTLIBOBJS@ $(common_ldadd)
endif
