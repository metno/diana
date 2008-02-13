# Slave Makefile for DIANA

CCOBJ= \
diQuickMenues.o \
diController.o diPlotModule.o diPlot.o diSetupParser.o diTimeFilter.o \
diPrintOptions.o diFontManager.o diImageGallery.o diImageIO.o \
diMapManager.o diMapPlot.o diFilledMap.o diFieldPlot.o  \
diContouring.o diCommandParser.o \
diObsManager.o diObsPlot.o diObsSynop.o diObsDribu.o diObsTemp.o diObsPilot.o \
diObsAireps.o diObsSatob.o diObsMetar.o diObsTide.o diObsOcea.o diObsAscii.o \
diSatManager.o diSatPlot.o diSat.o diMItiff.o diLegendPlot.o \
diObjectManager.o diObjectPlot.o diWeatherArea.o diWeatherFront.o diUndoFront.o \
diWeatherSymbol.o diAreaObjects.o diAreaBorder.o diWeatherObjects.o \
diEditObjects.o diDisplayObjects.o diComplexSymbolPlot.o diObjectPoint.o \
diTrajectoryPlot.o diEditManager.o diFieldEdit.o \
diAnnotationPlot.o diStationPlot.o diLocationPlot.o \
diVprofManager.o diVprofPlot.o diVprofTables.o diVprofDiagram.o diVprofData.o \
diVprofTemp.o diVprofPilot.o diVprofOptions.o diFtnVfile.o \
diVcrossManager.o diVcrossPlot.o diVcrossOptions.o diVcrossFile.o \
diSpectrumManager.o diSpectrumPlot.o diSpectrumOptions.o diSpectrumFile.o \
diShapeObject.o diGridArea.o diGridAreaManager.o diProfetObjectFactory.o

ifndef NOBUFROBS
CCOBJ += diObsBufr.o 
endif


COBJ= \
diTesselation.o

QOBJ= \
qtMainWindow.o qtWorkArea.o qtGLwidget.o \
qtTimeSlider.o qtTimeControl.o qtTimeSpinbox.o \
qtTimeStepSpinbox.o qtStatusGeopos.o qtStatusPlotButtons.o \
qtShowSatValues.o \
qtQuickMenu.o qtTextDialog.o qtBrowserBox.o qtQuickAdmin.o \
qtQuickEditOptions.o qtAddtoMenu.o qtAddtoDialog.o \
qtButtonLayout.o qtToggleButton.o qtHelpDialog.o \
qtAdvancedButton.o \
qtMapDialog.o \
qtFieldDialog.o \
qtObsDialog.o qtObsWidget.o \
qtSatDialog.o qtSatDialogAdvanced.o \
qtObjectDialog.o \
qtTrajectoryDialog.o qtGeoPosLineEdit.o \
qtEditDialog.o qtEditNewDialog.o qtEditComment.o qtLoginDialog.o \
qtEditDefineField.o qtTextView.o qtComplexText.o \
qtAnnoText.o qtUffdaDialog.o \
qtVprofWindow.o qtVprofWidget.o \
qtVprofModelDialog.o qtVprofSetupDialog.o qtVprofSetup.o \
qtVcrossWindow.o qtVcrossWidget.o \
qtVcrossDialog.o qtVcrossSetupDialog.o qtVcrossSetup.o \
qtSpectrumWindow.o qtSpectrumWidget.o \
qtSpectrumModelDialog.o qtSpectrumSetupDialog.o qtSpectrumSetup.o \
qtPaintToolBar.o qtProfetSessionDialog.o qtProfetSessionTable.o \
qtProfetObjectDialog.o qtDianaProfetGUI.o qtProfetChatWidget.o

GOBJ= \
main_gui.o \
qtUtility.o qtPrintManager.o qtImageGallery.o

BOBJ= \
bdiana.o signalhelper.o

#----------------------------------------------------------------------------
CSRC=  $(COBJ:.o=.c)
CCSRC= $(CCOBJ:.o=.cc)
QSRC=  $(QOBJ:.o=.cc)
GSRC=  $(GOBJ:.o=.cc)
BSRC=  $(BOBJ:.o=.cc)

MOCOBJ=$(QOBJ:.o=_moc.o)

ALLSRC=  $(CCSRC) $(CSRC) $(QSRC) $(GSRC) $(BSRC)
ALLOBJ=  $(CCOBJ) $(COBJ) $(QOBJ) $(GOBJ) $(MOCOBJ)
BALLOBJ= $(CCOBJ) $(COBJ) $(BOBJ)

#----------------------------------------------------------------------------

.SUFFIXES: .cc .c .o

# From .cc to .o
.cc.o:
	@echo -e "\n[0;34mCOMPILING ......................... $< [0m"
	$(CXX) -c $(CCFLAGS) $(INCLUDE) -DQT3_SUPPORT   $<

# From .c to .o
.c.o:
	@echo -e "\n[0;34mCOMPILING ......................... $< [0m"
	$(CC) -c $(CFLAGS) $(INCLUDE) -DQT3_SUPPORT   $<

#----------------------------------------------------------------------------

all: build $(BINDIR)/diana $(BINDIR)/bdiana

mkdiana:  build $(BINDIR)/diana

mkbdiana: build $(BINDIR)/bdiana

#----------------------------------------------------------------------------

mocs:
	echo "#automatic generated moc targets " > $(MOCFILE)
	for ENTRY in $(QOBJ:.o=.h) ; \
	do \
	echo                                                                 >> $(MOCFILE) ; \
	echo $${ENTRY%.h}_moc.cc: $(INCDIR)/$${ENTRY}                        >> $(MOCFILE) ; \
	echo "	"$(MOC) $(INCDIR)/$${ENTRY} -o $${ENTRY%.h}_moc.cc >> $(MOCFILE) ; \
	done

-include $(MOCFILE)

#----------------------------------------------------------------------------

LANGUAGES= $(LANGDIR)/diana_no.ts


languages:
	$(QTDIR)/bin/lupdate  -verbose $(QSRC) -ts $(LANGUAGES)
	cd $(LANGDIR) ;
	$(QTDIR)/bin/lrelease -verbose $(LANGUAGES)

#----------------------------------------------------------------------------

depends:
	rm -f $(DEPENDSFILE)
	echo '# Automatically-generated dependencies list:' > $(DEPENDSFILE)
	$(CXX) $(CCFLAGS) $(INCLUDE) -DQT3_SUPPORT -M $(ALLSRC) >> $(DEPENDSFILE)

-include $(DEPENDSFILE)

#-----------------------------------------------------

$(BINDIR)/diana: $(ALLOBJ) $(MOCOBJ)
	$(CXX) $(LDFLAGS) -g -DQT3_SUPPORT $(ALLOBJ) $(LINKS) -o $@

#-----------------------------------------------------

$(BINDIR)/bdiana: $(BALLOBJ)
	$(CXX) $(LDFLAGS) -DQT3_SUPPORT $(BALLOBJ) $(BLINKS) -o $@

#-----------------------------------------------------

BUILDHEADER=diBuild.h

build:
	@if [ -s $(BUILDHEADER) ]; then \
	  olddate=`grep build_string $(BUILDHEADER) | cut -f2 -d\"`; \
	else \
	  olddate="firsttime"; \
	fi; \
	today=`date +%Y-%m-%d`; \
	if [ ! $$olddate = $$today ]; then \
	  rm -f $(BUILDHEADER); \
	  echo '#ifndef _build_h_' > $(BUILDHEADER); \
	  echo '#define _build_h_' >> $(BUILDHEADER); \
	  echo 'const string build_string= "'$$today'";' >> $(BUILDHEADER); \
	  echo '#endif' >> $(BUILDHEADER); \
	  chmod 0664 $(BUILDHEADER); \
	  echo "UPDATED $(BUILDHEADER) $$today"; \
	else \
	  echo "UNCHANGED $(BUILDHEADER) $$olddate"; \
	fi




