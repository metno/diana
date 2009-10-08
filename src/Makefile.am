# $Id$

bin_PROGRAMS = diana bdiana

# automake requires one = before the first +=
common_cppflags =
common_ldflags =
common_ldadd =
CLEANFILES =

# convenience for developers using a non-standard $(prefix)
common_cppflags += -I$(prefix)/include
common_ldflags += -L$(prefix)/lib

# our own headers are here
common_cppflags += -I$(top_srcdir)/include

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

common_cppflags += \
	$(DIFIELD_CPPFLAGS) \
	$(DIMITIFF_CPPFLAGS) \
	$(DISQL_CPPFLAGS) \
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

# Additional source code for HDF5 support
if WITH_HDF5
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
	qtProfetSessionTable.cc \
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
	$(PROFET_CPPFLAGS)
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

# Generated Qt source code - diana
moc_sources = $(qt_sources:.cc=.moc.cc)
CLEANFILES += $(moc_sources)
%.moc.cc: $(top_srcdir)/include/%.h
	$(MOC4) -o $@ $<

# Full set of source code for diana
diana_SOURCES = \
	$(common_c_sources) \
	$(common_cxx_sources) \
	$(moc_sources) \
	$(qt_sources) \
	main_gui.cc \
	qtImageGallery.cc \
	qtPrintManager.cc \
	qtUtility.cc
diana_CPPFLAGS = $(common_cppflags)
diana_LDFLAGS = $(common_ldflags)
diana_LDADD = $(common_ldadd)

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
