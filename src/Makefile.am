# $Id$

bin_PROGRAMS = diana.bin bdiana 

# automake requires one = before the first +=
common_cppflags =
common_ldflags =
common_ldadd =
CLEANFILES =

# convenience for developers using a non-standard $(prefix)
common_cppflags += -I$(prefix)/include
common_ldflags += -L$(prefix)/lib


# doesn't build cleanly with log4cxx
common_cppflags += -DNOLOG4CXX


# hard coded in old Makefile
common_cppflags += -DMETNOFIELDFILE  -DMETNOPRODDB -DMETNOOBS

# C source code common to diana and bdiana
common_c_sources = \
	diTesselation.c

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
	diLocationPlot.cc \
	diMItiff.cc \
	diMapManager.cc \
	diMapPlot.cc \
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
	diRadarEchoPlot.cc \
	diSat.cc \
	diSatManager.cc \
	diSatPlot.cc \
	diSetupParser.cc \
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
	diWeatherSymbol.cc

# Qt sources - diana only
qt_sources = \
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
	qtFieldDialog.cc \
	qtGLwidget.cc \
	qtGeoPosLineEdit.cc \
	qtLoginDialog.cc \
	qtMailDialog.cc \
	qtMainWindow.cc \
	qtMapDialog.cc \
	qtObjectDialog.cc \
	qtObsDialog.cc \
	qtObsWidget.cc \
	qtPaintToolBar.cc \
	qtQuickAdmin.cc \
	qtQuickEditOptions.cc \
	qtQuickMenu.cc \
	qtRadarEchoDialog.cc \
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



# header files needed to build
common_header_sources= diAnnotationPlot.h\
	diAreaBorder.h\
	diAreaObjects.h\
	diCommandParser.h\
	diCommonTypes.h\
	diComplexSymbolPlot.h\
	diContouring.h\
	diController.h\
	diDisplayObjects.h\
	diDrawingTypes.h\
	diEditManager.h\
	diEditObjects.h\
	diEditSpec.h\
	diFieldEdit.h\
	diFieldPlot.h\
	diFieldPlotManager.h\
	diFilledMap.h\
	diFontManager.h\
	diFtnVfile.h\
	diGridArea.h\
	diGridAreaManager.h\
	diHDF5.h\
	diImageGallery.h\
	diImageIO.h\
	diKeys.h\
	diLegendPlot.h\
	diLocationPlot.h\
	diMapManager.h\
	diMapMode.h\
	diMapPlot.h\
	diMItiff.h\
	diObjectManager.h\
	diObjectPlot.h\
	diObjectPoint.h\
	diObsAireps.h\
	diObsAscii.h\
	diObsBufr.h\
	diObsData.h\
	diObsDribu.h\
	diObsManager.h\
	diObsMetar.h\
	diObsOcea.h\
	diObsPilot.h\
	diObsPlot.h\
	diObsSatob.h\
	diObsSynop.h\
	diObsTemp.h\
	diObsTide.h\
	diPlot.h\
	diPlotModule.h\
	diPrintOptions.h\
	diProfetObjectFactory.h\
	diQuickMenues.h\
	diRadarEchoPlot.h\
	diSat.h\
	diSatManager.h\
	diSatPlot.h\
	diSetupParser.h\
	diShapeObject.h\
	diSpectrumFile.h\
	diSpectrumManager.h\
	diSpectrumOptions.h\
	diSpectrumPlot.h\
	diStationPlot.h\
	diTesselation.h\
	diTimeFilter.h\
	diTrajectoryPlot.h\
	diUndoFront.h\
	diVcrossField.h\
	diVcrossFile.h\
	diVcrossManager.h\
	diVcrossOptions.h\
	diVcrossPlot.h\
	diVprofData.h\
	diVprofDiagram.h\
	diVprofManager.h\
	diVprofOptions.h\
	diVprofPilot.h\
	diVprofPlot.h\
	diVprofTables.h\
	diVprofTemp.h\
	diWeatherArea.h\
	diWeatherFront.h\
	diWeatherObjects.h\
	diWeatherSymbol.h\
	MovieMaker.h\
	polyStipMasks.h\
	qtAddtoDialog.h\
	qtAddtoMenu.h\
	qtAdvancedButton.h\
	qtAnnoText.h\
	qtBrowserBox.h\
	qtButtonLayout.h\
	qtComplexText.h\
	qtDianaProfetGUI.h\
	qtEditComment.h\
	qtEditDefineField.h\
	qtEditDialog.h\
	qtEditNewDialog.h\
	qtFieldDialog.h\
	qtGeoPosLineEdit.h\
	qtGLwidget.h\
	qtImageGallery.h\
	qtLoginDialog.h\
	qtMailDialog.h\
	qtMainWindow.h\
	qtMapDialog.h\
	qtObjectDialog.h\
	qtObsDialog.h\
	qtObsWidget.h\
	qtPaintToolBar.h\
	qtPolygonBookmarkDialog.h\
	qtPolygonBookmarkModel.h\
	qtPrintManager.h\
	qtProfetChatWidget.h\
	qtProfetDataModels.h\
	qtProfetEvents.h\
	qtProfetObjectDialog.h\
	qtProfetSessionDialog.h\
	qtProfetSingleControl.h\
	qtProfetTimeControl.h\
	qtProfetTimeSmoothDialog.h\
	qtProfetWaitDialog.h\
	qtQuickAdmin.h\
	qtQuickEditOptions.h\
	qtQuickMenu.h\
	qtRadarEchoDialog.h\
	qtSatDialogAdvanced.h\
	qtSatDialog.h\
	qtShowSatValues.h\
	qtSpectrumModelDialog.h\
	qtSpectrumSetupDialog.h\
	qtSpectrumWidget.h\
	qtSpectrumWindow.h\
	qtStatusGeopos.h\
	qtStatusPlotButtons.h\
	qtTextDialog.h\
	qtTextView.h\
	qtTimeControl.h\
	qtTimeSlider.h\
	qtTimeSpinbox.h\
	qtTimeStepSpinbox.h\
	qtToggleButton.h\
	qtTrajectoryDialog.h\
	qtUffdaDialog.h\
	qtUtility.h\
	qtVcrossDialog.h\
	qtVcrossSetupDialog.h\
	qtVcrossSetup.h\
	qtVcrossWidget.h\
	qtVcrossWindow.h\
	qtVprofModelDialog.h\
	qtVprofSetupDialog.h\
	qtVprofWidget.h\
	qtVprofWindow.h\
	qtWorkArea.h\
	signalhelper.h

# xpm files needed to build
common_xpm_sources= active_object.xpm\
	autoupdate.xpm\
	back.xpm\
	bakover.xpm\
	balloon.xpm\
	blocks.xpm\
	bookmark.xpm\
	clock.xpm\
	convert.xpm\
	custom_user.xpm\
	dialoger.xpm\
	diana_icon.xpm\
	directory.xpm\
	dnmi.xpm\
	down12x12.xpm\
	down12x14.xpm\
	down20x20.xpm\
	down32x24.xpm\
	earth3.xpm\
	editcopy.xpm\
	editcut.xpm\
	edit_lock_value.xpm\
	editmode.xpm\
	edit_open_value.xpm\
	editpaste.xpm\
	editundo.xpm\
	exit.xpm\
	face_grin.xpm\
	felt.xpm\
	fet_object_fog.xpm\
	fet_object_normal.xpm\
	fet_object_p.xpm\
	fet_object_rain.xpm\
	fet_object_sky.xpm\
	fet_object_tmp.xpm\
	fet_object_wave.xpm\
	fet_object_wind.xpm\
	filenew.xpm\
	fileopen.xpm\
	fileprint.xpm\
	filesave.xpm\
	forover.xpm\
	forward.xpm\
	front.xpm\
	idnumDown.xpm\
	idnumUp.xpm\
	inactive_object.xpm\
	info.xpm\
	kill.xpm\
	levelDown.xpm\
	levelUp.xpm\
	linear_copy.xpm\
	linear_down.xpm\
	linear_remove.xpm\
	locked_bookmark.xpm\
	locked_directory.xpm\
	loop.xpm\
	magnify2.xpm\
	metno_sort_txt.xpm\
	minus12x12.xpm\
	multiple_users.xpm\
	no_object.xpm\
	paint_add_crusor.xpm\
	paint_add_point.xpm\
	paint_color.xpm\
	paint_cursor.xpm\
	paint_cut.xpm\
	paint_delete.xpm\
	paint_draw.xpm\
	paint_forbidden_crusor.xpm\
	paint_help.xpm\
	paint_hide.xpm\
	paint_include.xpm\
	paint_mode.xpm\
	paint_move_point.xpm\
	paint_move.xpm\
	paint_new.xpm\
	paint_redo.xpm\
	paint_remove_crusor.xpm\
	paint_remove_point.xpm\
	paint_select.xpm\
	paint_spatial.xpm\
	paint_undo.xpm\
	parent_object.xpm\
	pencil2.xpm\
	pick.xpm\
	preferences.xpm\
	private_directory.xpm\
	profet.xpm\
	question.xpm\
	rade.xpm\
	redo.xpm\
	revert.xpm\
	Robot.xpm\
	run_smooth.xpm\
	sat.xpm\
	session_deployed.xpm\
	session_finalize.xpm\
	session_lock.xpm\
	session_open.xpm\
	session_operational.xpm\
	shuttle.xpm\
	single_remove.xpm\
	slutt.xpm\
	small_metno_sort.xpm\
	spectrum.xpm\
	start.xpm\
	stopp.xpm\
	sun2.xpm\
	synop_red.xpm\
	synop.xpm\
	tb_close.xpm\
	tb_left_arrow.xpm\
	tb_print.xpm\
	tb_right_arrow.xpm\
	thumbs_down.xpm\
	thumbs_up.xpm\
	Tool_32_draw.xpm\
	traj.xpm\
	trashcan.xpm\
	undo.xpm\
	up12x12.xpm\
	up12x14.xpm\
	up20x20.xpm\
	up32x24.xpm\
	user_admin.xpm\
	user.xpm\
	vcross.xpm\
	vprof_normal.xpm\
	vprof_selected.xpm\
	weather_rain.xpm\
	weather_storm.xpm


common_cppflags += \
	$(DIFIELD_CPPFLAGS) \
	$(DIMITIFF_CPPFLAGS) \
	$(DISQL_CPPFLAGS) \
	$(MYSQL_CPPFLAGS) \
	$(EMOS_CPPFLAGS) \
	$(FTGL_CPPFLAGS) \
	$(GLP_CPPFLAGS) \
	$(GLTEXT_CPPFLAGS) \
	$(PUDATATYPES_CPPFLAGS) \
	$(QT4_CPPFLAGS) \
	$(QUTILITIES_CPPFLAGS) \
	$(ROBS_CPPFLAGS) \
	$(SHP_CPPFLAGS) \
	$(UU_CPPFLAGS)

common_ldflags += \
	$(DIFIELD_LDFLAGS) \
	$(DIMITIFF_LDFLAGS) \
	$(DISQL_LDFLAGS) \
	$(MYSQL_LDFLAGS) \
	$(EMOS_LDFLAGS) \
	$(FTGL_LDFLAGS) \
	$(GLP_LDFLAGS) \
	$(GLTEXT_LDFLAGS) \
	$(PUDATATYPES_LDFLAGS) \
	$(QT4_LDFLAGS) \
	$(QUTILITIES_LDFLAGS) \
	$(ROBS_LDFLAGS) \
	$(SHP_LDFLAGS) \
	$(UU_LDFLAGS)

common_ldadd += \
	$(DIFIELD_LIBS) \
	$(DIMITIFF_LIBS) \
	$(DISQL_LIBS) \
	$(MYSQL_LIBS) \
	$(EMOS_LIBS) \
	$(FTGL_LIBS) \
	$(GLP_LIBS) \
	$(GLTEXT_LIBS) \
	$(PUDATATYPES_LIBS) \
	$(QT4_LIBS) \
	$(QUTILITIES_LIBS) \
	$(ROBS_LIBS) \
	$(SHP_LIBS) \
	$(UU_LIBS)

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
	diObsRoad.cc\
	diVprofRTemp.cc
common_ldflags += \
	${ROADAPI_LDFLAGS}
common_ldadd += \
	${ROADAPI_LIBS}
endif

if WITH_GRIBAPI
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
qt_sources += \
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
common_cppflags += -DVIDEO_EXPORT
common_ldflags += $(AVFORMAT_LDFLAGS)
common_ldadd += $(AVFORMAT_LIBS)
endif

# Use Xlib-specific routines
if WITH_XLIB
common_cppflags += -DUSE_XLIB
endif

# Generated Qt source code - diana
moc_sources = $(qt_sources:.cc=.moc.cc)
CLEANFILES += $(moc_sources)
%.moc.cc: %.h
	$(MOC4) -o $@ $<

language_sources=$(top_srcdir)/share/diana/lang/diana_no.ts

languagedir= $(datarootdir)/$(APP)/lang
nodist_language_DATA=$(language_sources:.ts=.qm)

CLEANFILES += $(nodist_language_DATA)

%.qm: %.ts  $(qt_sources)
	$(LUPDATE4)  $(qt_sources) -ts $< ; \
	$(LRELEASE4)  $<


# Full set of source code for diana
diana_bin_SOURCES = \
	$(common_c_sources) \
	$(common_cxx_sources) \
	$(qt_sources) \
	$(common_header_sources) \
	$(common_xpm_sources) \
	$(language_sources) \
	main_gui.cc \
	qtImageGallery.cc \
	qtPrintManager.cc \
	qtUtility.cc

# moc files are not going into make dist
nodist_diana_bin_SOURCES = $(moc_sources)


diana_bin_CPPFLAGS = $(common_cppflags)
diana_bin_LDFLAGS = $(common_ldflags)
diana_bin_LDADD = $(common_ldadd)

# Full set of source code for bdiana
bdiana_SOURCES = \
	$(common_c_sources) \
	$(common_cxx_sources) \
	bdiana.cc \
	signalhelper.cc
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

# This is broken.  On systems that have the BSD "install" utility,
# autoconf defines INSTALL to e.g. "/usr/bin/install -c"; on systems
# that don't, it uses something like "$(top_srcdir)/install-sh -c".
# We really want -p, not -c, but there is no good way to do this
# without writing our own version of AC_PROG_INSTALL, so we will
# simply assume that a working version of the BSD "install" utility is
# in PATH.
INSTALL = install -p
