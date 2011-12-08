# $Id$

include $(top_srcdir)/diana.mk

bin_PROGRAMS = diana.bin bdiana 
noinst_LIBRARIES = libdiana.a

# automake requires one = before the first +=
common_cppflags = -I$(top_srcdir)/src/GL
common_ldflags =
common_ldadd = libdiana.a
CLEANFILES =

# convenience for developers using a non-standard $(prefix)
common_cppflags += -I$(prefix)/include
common_ldflags += -L$(prefix)/lib

# doesn't build cleanly with log4cxx
common_cppflags += -DNOLOG4CXX

# hard coded in old Makefile
# XXX should be optional
# XXX XXX should be in config.h
common_cppflags += -DMETNOFIELDFILE -DMETNOOBS

# C source code common to diana and bdiana
common_c_sources = \
        diTesselation.c \
        GL/libtess/dict.c \
        GL/libtess/geom.c \
        GL/libtess/memalloc.c \
        GL/libtess/mesh.c \
        GL/libtess/normal.c \
        GL/libtess/priorityq.c \
        GL/libtess/render.c \
        GL/libtess/sweep.c \
        GL/libtess/tess.c \
        GL/libtess/tessmono.c \
        GL/libutil/error.c \
        GL/libutil/glue.c

# C++ source code common to diana and bdiana
common_cxx_sources = \
	diAnnotationPlot.cc \
	diAreaBorder.cc \
	diAreaObjects.cc \
	diCommandParser.cc \
	diComplexSymbolPlot.cc \
	diContouring.cc \
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
	diGridArea.cc \
	diGridAreaManager.cc \
	diImageGallery.cc \
	diImageIO.cc \
	diLegendPlot.cc \
	diLocalSetupParser.cc \
	diLocationPlot.cc \
	diMItiff.cc \
	diMapManager.cc \
	diMapPlot.cc \
	diMeasurementsPlot.cc \
	diObjectManager.cc \
	diObjectPlot.cc \
	diObjectPoint.cc \
	diObsAireps.cc \
	diObsAscii.cc \
	diObsDribu.cc \
	diObsManager.cc \
	diObsMetar.cc \
	diObsOcea.cc \
	diObsPilot.cc \
	diObsPlot.cc \
	diObsSatob.cc \
	diObsSynop.cc \
	diObsTemp.cc \
	diObsTide.cc \
	diPlot.cc \
	diPlotModule.cc \
	diPrintOptions.cc \
	diQuickMenues.cc \
	diSat.cc \
	diSatManager.cc \
	diSatPlot.cc \
	diShapeObject.cc \
	diSpectrumFile.cc \
	diSpectrumManager.cc \
	diSpectrumOptions.cc \
	diSpectrumPlot.cc \
	diStationPlot.cc \
	diTimeFilter.cc \
	diTrajectoryPlot.cc \
	diUndoFront.cc \
	diVcrossField.cc \
	diVcrossFile.cc \
	diVcrossManager.cc \
	diVcrossOptions.cc \
	diVcrossPlot.cc \
	diVprofData.cc \
	diVprofDiagram.cc \
	diVprofManager.cc \
	diVprofOptions.cc \
	diVprofPilot.cc \
	diVprofPlot.cc \
	diVprofTables.cc \
	diVprofTemp.cc \
	diWeatherArea.cc \
	diWeatherFront.cc \
	diWeatherObjects.cc \
        diWeatherSymbol.cc \
        GL/paintgl.cc

# Qt sources
common_qt_sources =

diana_qt_sources = \
	qtAddtoDialog.cc \
	qtAddtoMenu.cc \
	qtAdvancedButton.cc \
	qtAnnoText.cc \
	qtBrowserBox.cc \
	qtButtonLayout.cc \
	qtComplexText.cc \
	qtEditComment.cc \
	qtEditDefineField.cc \
	qtEditDialog.cc \
	qtEditNewDialog.cc \
	qtEditText.cc \
	qtFieldDialog.cc \
	qtGLwidget.cc \
	qtGeoPosLineEdit.cc \
	qtLoginDialog.cc \
	qtMailDialog.cc \
	qtMainWindow.cc \
	qtMapDialog.cc \
	qtMeasurementsDialog.cc \
	qtObjectDialog.cc \
	qtObsDialog.cc \
	qtObsWidget.cc \
	qtPaintToolBar.cc \
	qtQuickAdmin.cc \
	qtQuickEditOptions.cc \
	qtQuickMenu.cc \
	qtSatDialog.cc \
	qtSatDialogAdvanced.cc \
	qtShowSatValues.cc \
	qtSpectrumModelDialog.cc \
	qtSpectrumSetupDialog.cc \
	qtSpectrumWidget.cc \
	qtSpectrumWindow.cc \
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
	qtVcrossDialog.cc \
	qtVcrossSetup.cc \
	qtVcrossSetupDialog.cc \
	qtVcrossWidget.cc \
	qtVcrossWindow.cc \
	qtVprofModelDialog.cc \
	qtVprofSetupDialog.cc \
	qtVprofWidget.cc \
	qtVprofWindow.cc \
	qtWorkArea.cc

bdiana_qt_sources = \
	diOrderBook.cc \
	diOrderClient.cc \
	diOrderListener.cc \
	diOrderQueue.cc \
	diWorkOrder.cc

noinst_HEADERS = \
	MovieMaker.h \
	diAnnotationPlot.h \
	diAreaBorder.h \
	diAreaObjects.h \
	diCommandParser.h \
	diCommonTypes.h \
	diComplexSymbolPlot.h \
	diContouring.h \
	diController.h \
	diDisplayObjects.h \
	diDrawingTypes.h \
	diEditManager.h \
	diEditObjects.h \
	diEditSpec.h \
	diFieldEdit.h \
	diFieldPlot.h \
	diFieldPlotManager.h \
	diFilledMap.h \
	diFontManager.h \
	diFtnVfile.h \
	diGridArea.h \
	diGridAreaManager.h \
	diHDF5.h \
	diImageGallery.h \
	diImageIO.h \
	diKeys.h \
	diLegendPlot.h \
	diLocalSetupParser.h \
	diLocationPlot.h \
	diMItiff.h \
	diMapManager.h \
	diMapMode.h \
	diMapPlot.h \
	diMeasurementsPlot.h \
	diObjectManager.h \
	diObjectPlot.h \
	diObjectPoint.h \
	diObsAireps.h \
	diObsAscii.h \
	diObsBufr.h \
	diObsData.h \
	diObsDribu.h \
	diObsManager.h \
	diObsMetar.h \
	diObsOcea.h \
	diObsPilot.h \
	diObsPlot.h \
	diObsSatob.h \
	diObsSynop.h \
	diObsTemp.h \
	diObsTide.h \
	diOrderBook.h \
	diOrderClient.h \
	diOrderListener.h \
	diOrderQueue.h \
	diPlot.h \
	diPlotModule.h \
	diPrintOptions.h \
	diQuickMenues.h \
	diSat.h \
	diSatManager.h \
	diSatPlot.h \
	diShapeObject.h \
	diSpectrumFile.h \
	diSpectrumManager.h \
	diSpectrumOptions.h \
	diSpectrumPlot.h \
	diStationPlot.h \
	diTesselation.h \
	diTimeFilter.h \
	diTrajectoryPlot.h \
	diUndoFront.h \
	diVcrossField.h \
	diVcrossFile.h \
	diVcrossManager.h \
	diVcrossOptions.h \
	diVcrossPlot.h \
	diVprofData.h \
	diVprofDiagram.h \
	diVprofManager.h \
	diVprofOptions.h \
	diVprofPilot.h \
	diVprofPlot.h \
	diVprofTables.h \
	diVprofTemp.h \
	diWeatherArea.h \
	diWeatherFront.h \
	diWeatherObjects.h \
	diWeatherSymbol.h \
	diWorkOrder.h \
	polyStipMasks.h \
	qtAddtoDialog.h \
	qtAddtoMenu.h \
	qtAdvancedButton.h \
	qtAnnoText.h \
	qtBrowserBox.h \
	qtButtonLayout.h \
	qtComplexText.h \
	qtEditComment.h \
	qtEditText.h \
	qtEditDefineField.h \
	qtEditDialog.h \
	qtEditNewDialog.h \
	qtFieldDialog.h \
	qtGLwidget.h \
	qtGeoPosLineEdit.h \
	qtImageGallery.h \
	qtLoginDialog.h \
	qtMailDialog.h \
	qtMainWindow.h \
	qtMapDialog.h \
	qtMeasurementsDialog.h \
	qtObjectDialog.h \
	qtObsDialog.h \
	qtObsWidget.h \
	qtPaintToolBar.h \
	qtPrintManager.h \
	qtQuickAdmin.h \
	qtQuickEditOptions.h \
	qtQuickMenu.h \
	qtSatDialog.h \
	qtSatDialogAdvanced.h \
	qtShowSatValues.h \
	qtSpectrumModelDialog.h \
	qtSpectrumSetupDialog.h \
	qtSpectrumWidget.h \
	qtSpectrumWindow.h \
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
	qtVcrossDialog.h \
	qtVcrossSetup.h \
	qtVcrossSetupDialog.h \
	qtVcrossWidget.h \
	qtVcrossWindow.h \
	qtVprofModelDialog.h \
	qtVprofSetupDialog.h \
	qtVprofWidget.h \
	qtVprofWindow.h \
	qtWorkArea.h \
        signalhelper.h \
        GL/gl.h \
        GL/glu.h \
        GL/gluint.h \
        GL/gluos.h \
        GL/paintgl.h \
        GL/libtess/dict.h \
        GL/libtess/dict-list.h \
        GL/libtess/geom.h \
        GL/libtess/memalloc.h \
        GL/libtess/mesh.h \
        GL/libtess/normal.h \
        GL/libtess/priorityq.h \
        GL/libtess/priorityq-sort.h \
        GL/libtess/render.h \
        GL/libtess/sweep.h \
        GL/libtess/tess.h \
        GL/libtess/tessmono.h

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
	$(DIMITIFF_CPPFLAGS) \
	$(PUCTOOLS_CPPFLAGS) \
	$(EMOS_CPPFLAGS) \
	$(PUDATATYPES_CPPFLAGS) \
	$(QUTILITIES_CPPFLAGS) \
	$(ROBS_CPPFLAGS) \
	$(SHP_CPPFLAGS) \
	$(CURL_CPPFLAGS) \
	$(PNG_CPPFLAGS) \
	$(UU_CPPFLAGS)

common_ldflags += \
	$(DIFIELD_LDFLAGS) \
	$(DIMITIFF_LDFLAGS) \
	$(PUCTOOLS_LDFLAGS) \
	$(EMOS_LDFLAGS) \
	$(PUDATATYPES_LDFLAGS) \
	$(QUTILITIES_LDFLAGS) \
	$(ROBS_LDFLAGS) \
	$(SHP_LDFLAGS) \
	$(CURL_LDFLAGS) \
	$(PNG_LDFLAGS) \
	$(UU_LDFLAGS)

common_ldadd += \
	$(DIFIELD_LIBS) \
	$(DIMITIFF_LIBS) \
	$(PUCTOOLS_LIBS) \
	$(EMOS_LIBS) \
	$(PUDATATYPES_LIBS) \
	$(QUTILITIES_LIBS) \
	$(ROBS_LIBS) \
	$(SHP_LIBS) \
	$(CURL_LIBS) \
	$(PNG_LIBS) \
	$(UU_LIBS)

# Additional source code for SMHI support
if WITH_SMHI
common_cppflags += -DSMHI
endif

# Additional source code for NetCDF support
if WITH_NETCDF
common_cppflags += -DNETCDF
common_ldflags += \
	${NETCDF_LDFLAGS}
common_ldadd += \
	${NETCDF_LIBS}
endif

# Additional source code for RoadOBS support
if WITH_ROADOBS
common_cppflags += -DROADOBS
common_cxx_sources += \
	diObsRoad.cc \
	diVprofRTemp.cc
common_ldflags += \
	${ROADAPI_LDFLAGS}
common_ldadd += \
	${ROADAPI_LIBS}
endif

# Additional source code for RoadOBS support
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

if WITH_GRIBFILE
common_ldflags += \
	${GRIBAPI_LDFLAGS}
common_ldadd += \
	${GRIBAPI_LIBS} ${GRIBAPI_LIBS}
endif

# Additional source code for RoadOBS support
if WITH_GEOTIFF
common_cppflags += -DGEOTIFF
common_cxx_sources += \
	diGEOtiff.cc
noinst_HEADERS += \
	diGEOtiff.h
#common_ldflags += \
#	${ROADAPI_LDFLAGS}
common_ldadd += \
	${ROADAPI_LIBS} ${GRIBAPI_LIBS}

endif

# Additional source code for HDF5 support
if WITH_HDF5
common_cppflags += -DHDF5FILE
common_cxx_sources += \
	diHDF5.cc
endif

# Additional source code for met.no prod db
if WITH_PRODDB
common_cppflags += \
	$(PUSQL_CPPFLAGS) \
	$(DISQL_CPPFLAGS) \
	-DMETNOPRODDB
common_ldflags += \
	$(PUSQL_LDFLAGS) \
	$(DISQL_LDFLAGS)
common_ldadd += \
	$(PUSQL_LIBS) \
	$(DISQL_LIBS)
endif

# Additional source code for observation buffers
if WITH_OBS_BUFR
common_cxx_sources += \
	diObsBufr.cc
common_cppflags += -DBUFROBS
endif

# Additional source code for Profet
if WITH_PROFET
common_cxx_sources += \
	diProfetObjectFactory.cc
noinst_HEADERS += \
	qtPolygonBookmarkDialog.h \
	qtPolygonBookmarkModel.h \
	diProfetObjectFactory.h \
	qtDianaProfetGUI.h \
	qtProfetChatWidget.h \
	qtProfetDataModels.h \
	qtProfetEvents.h \
	qtProfetObjectDialog.h \
	qtProfetSessionDialog.h \
	qtProfetSingleControl.h \
	qtProfetTimeControl.h \
	qtProfetTimeSmoothDialog.h \
	qtProfetWaitDialog.h
common_xpm_sources += \
	profet.xpm
diana_qt_sources += \
	qtProfetSessionDialog.cc \
	qtProfetObjectDialog.cc \
	qtDianaProfetGUI.cc \
	qtProfetChatWidget.cc \
	qtProfetDataModels.cc \
	qtProfetSingleControl.cc \
	qtProfetTimeControl.cc \
	qtProfetTimeSmoothDialog.cc \
	qtPolygonBookmarkDialog.cc \
	qtPolygonBookmarkModel.cc \
	qtProfetWaitDialog.cc 
common_cppflags += \
	$(PROFET_CPPFLAGS) \
	-DPROFET
common_ldflags += \
	$(PROFET_LDFLAGS)
common_ldadd += \
	$(PROFET_LIBS)
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
endif

# Generated Qt source code
common_moc_sources = $(common_qt_sources:.cc=.moc.cc)
diana_moc_sources = $(diana_qt_sources:.cc=.moc.cc)
bdiana_moc_sources = $(bdiana_qt_sources:.cc=.moc.cc)
CLEANFILES += $(common_moc_sources) $(diana_moc_sources) $(bdiana_moc_sources)
%.moc.cc: %.h
	$(MOC4) -o $@ $<

language_sources=$(top_srcdir)/share/diana/lang/diana_no.ts \
	$(top_srcdir)/share/diana/lang/diana_es.ts \
	$(top_srcdir)/share/diana/lang/diana_sv.ts

languagedir= $(datarootdir)/$(APP)/lang
nodist_language_DATA=$(language_sources:.ts=.qm)

CLEANFILES += $(nodist_language_DATA)

%.qm: %.ts  $(diana_qt_sources)
	$(LUPDATE4)  $(diana_qt_sources) -ts $< ; \
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
	$(common_cxx_sources)

nodist_libdiana_a_SOURCES = \
	$(common_moc_sources)

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
	$(bdiana_qt_sources) \
	bdiana.cc \
	signalhelper.cc

nodist_bdiana_SOURCES = \
	$(bdiana_moc_sources)

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

noinst_PROGRAMS = \
	test-diOrderBook

test_diOrderBook_qt_sources = \
	diOrderBook.cc \
	diOrderClient.cc \
	diOrderListener.cc \
	diOrderQueue.cc \
	diWorkOrder.cc
test_diOrderBook_SOURCES = \
	$(test_diOrderBook_qt_sources) \
	$(test_diOrderBook_qt_sources:.cc=.moc.cc) \
	test-diOrderBook.cc
test_diOrderBook_CPPFLAGS = $(QT4_CPPFLAGS)
test_diOrderBook_LDFLAGS = $(QT4_LDFLAGS)
test_diOrderBook_LDADD = $(QT4_LIBS)
