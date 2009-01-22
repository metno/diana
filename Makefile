#--------------------------------
# Toplevel Makefile for DIANA
#--------------------------------

# includefile contains Compiler definitions
include ../conf/${OSTYPE}.mk

SRCDIR=src
LIBDIR=lib$(PLT)
OBJDIR=obj$(PLT)
BINDIR=bin$(PLT)
INCDIR=../include
LOCALINC=$(LOCALDIR)/include
LANGDIR=share/diana/lang

DEPENDSFILE=make.depends
MOCFILE=make.moc
DEFINES+=-DMETNOFIELDFILE  -DMETNOPRODDB -DMETNOOBS $(PROFETDEF)\
	$(NETCDFDEF) $(BUFROBSDEF) $(LOGGDEF) $(CORBADEF) $(ANIMDEF)

ifdef WDB
DEFINES += -DWDB
WDB_LIB=-ldiWdb
WDB_EXTRA_LIB=	-lboost_thread \
                $(shell pkg-config --libs libpqxx) \
                -L$(shell pg_config --libdir)  -lpq \
                -lboost_date_time
endif

ifdef OMNIORB_INST
PROFETLIBS+= -lpods 
endif

INCLUDE= -I. \
	 -I$(INCDIR) \
	 -I$(LOCALINC)/propoly \
	 -I$(LOCALINC)/qUtilities \
	 -I$(LOCALINC)/puDatatypes \
	 -I$(LOCALINC)/glp \
	 -I$(LOCALINC)/glText \
	 -I$(LOCALINC)/robs \
	 -I$(LOCALINC)/diMItiff \
	 -I$(LOCALINC)/diField \
	 -I$(LOCALINC)/milib \
	 -I$(LOCALINC)/diSQL \
	 -I$(LOCALINC)/puSQL \
	 -I$(LOCALINC)/puTools \
	 -I$(LOCALINC)/puCtools \
	 -I$(LOCALINC)          \
	 $(PNGINCLUDE) \
	 $(QTINCLUDE) \
	 $(GLINCLUDE) \
	 $(MYSQLINCLUDE) \
	 $(TIFFINCLUDE) \
	 $(FTGLINCLUDE) \
	 $(FTINCLUDE) \
	 $(SHAPEINCLUDE) \
	 $(XINCLUDE) \
	 $(OMNI_INCLUDE)


# Note: PNG library included in the Qt library (also used in batch version)

# WARNING: library sequence may be very important due to path (-L) sequence

LINKS = -L$(LOCALDIR)/$(LIBDIR) $(PROFETLIBS) \
	-lqUtilities -lpuDatatypes \
	-lglp -lglText -lrobs -ldiMItiff -ldiField -lpropoly  -lmic -ldiSQL -lpuSQL \
	-lpuTools \
	$(QTLIBDIR) $(QT_LIBS) \
	$(GLLIBDIR) -lGL -lGLU $(GLXTRALIBS) \
	$(MYSQLLIBDIR) -lmysqlclient \
	$(TIFFLIBDIR) -ltiff \
	$(FTGLLIBDIR) -lftgl $(FTLIBDIR) -lfreetype \
	$(SHAPELIBDIR) -lshp \
	$(XLIBDIR) -lXext -lXmu -lXt -lX11 \
	-L$(LOCALDIR)/$(LIBDIR) -lmi \
	-L$(EMOSLIBDIR) -lemos \
	$(LOGGLIBS) \
	$(F2CLIB) -lm \
	$(UDUNITSLIB) \
	$(NETCDFLIB) \
	$(WDB_LIB) \
	$(WDB_EXTRA_LIB) \
	$(OMNI_LIBS) \
	$(MAGICK_LIBS) 


BLINKS= $(LINKS)

OPTIONS="CXX=${CXX}" "CCFLAGS=${CXXFLAGS} ${DEFINES}" \
	"CC=${CC}" "CFLAGS=${CFLAGS} ${DEFINES}" \
	"LDFLAGS=${CXXLDFLAGS}" "INCLUDE=${INCLUDE}" \
	"DEPENDSFILE=${DEPENDSFILE}" "BINDIR=../${BINDIR}" \
	"LOCALDIR=${LOCALDIR}" "INCDIR=${INCDIR}" \
	"LINKS=${LINKS}" "BLINKS=${BLINKS}" \
	"QTDIR=${QTDIR}" "MOC=${MOC}" "MOCFILE=../${MOCFILE}" \
	 "LANGDIR=../${LANGDIR}"


all: directories mocs depends mark diana bmark bdiana


nodep: mark diana bmark bdiana

directories:
	if [ ! -d $(OBJDIR) ] ; then mkdir $(OBJDIR) ; fi
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR) ; fi
	cd $(OBJDIR) ; ln -sf ../$(SRCDIR)/*.cc .
	cd $(OBJDIR) ; ln -sf ../$(SRCDIR)/*.c .
	cd $(OBJDIR) ; ln -sf ../$(SRCDIR)/Makefile .
	if [ ! -f $(OBJDIR)/$(DEPENDSFILE) ] ; \
	then touch $(OBJDIR)/$(DEPENDSFILE) ; fi
	if [ ! -f $(OBJDIR)/$(MOCFILE) ] ; \
	then touch $(OBJDIR)/$(MOCFILE) ; fi
	cd $(OBJDIR); make $(OPTIONS) build

mocs:
	cd $(OBJDIR); make $(OPTIONS) mocs

languages:
	if [ ! -d $(LANGDIR) ] ; then mkdir $(LANGDIR) ; fi
	cd $(SRCDIR); make $(OPTIONS) languages

depends:
	cd $(OBJDIR); make $(OPTIONS) depends

mark:
	@echo  'Making diana ---------- '

diana:
	cd $(OBJDIR); make $(OPTIONS) mkdiana

bmark:
	@echo 'Making bdiana -----------------'

bdiana:
	cd $(OBJDIR); make $(OPTIONS) mkbdiana

pretty:
	find . \( -name 'core' -o -name '*~' \) -exec rm -f {} \;

binclean:
	rm $(BINDIR)/diana.bin
	rm $(BINDIR)/bdiana

clean:
	@make pretty
	cd $(OBJDIR); rm -f *.o

veryclean:
#	@make pretty
	rm -rf $(OBJDIR)

# install
DESTNAME=diana-3.6.2
COPYFILES=bin/diana bin/dianaTEST bin/bdiana bin/diana.bin
COPYDIRS=
COPYTREES= share etc
BINLINKS=
#

#CONFDESTNAME=diana_conf

CONFFILES=
CONFTREES=
TESTDESTNAME=dianaTEST
TESTBINLINKS=dianaTEST.sh 
#
PROFFDESTNAME=dianaPROFF
PROFFBINLINKS=dianaPROFF.sh 

include ../conf/install.mk















