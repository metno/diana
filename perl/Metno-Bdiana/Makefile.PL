use 5.008008;
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    NAME              => 'Metno::Bdiana',
    VERSION_FROM      => 'lib/Metno/Bdiana.pm', # finds $VERSION
    PREREQ_PM         => {}, # e.g., Module::Name => 1.1
    ($] >= 5.005 ?     ## Add these new keywords supported since 5.005
      (ABSTRACT_FROM  => 'lib/Metno/Bdiana.pm', # retrieve abstract from module
       AUTHOR         => 'Heiko Klein <heiko.klein@met.no>') : ()),
    LIBS              => ['-L../../src/ -L/disk1/heiko/local/metlibs/lib -L/usr/lib -ldiana -L/disk1/heiko/local/metlibs/lib/ -ldiField -lfimex -lmic -lpropoly -lmi -lgfortran -ldiMItiff -lproj -ltiff -lpuDatatypes -lrobs -lparameter -lpuTools -lpuCtools -lmiLogger -llog4cpp -lshp -lcurl-gnutls -lpng -L/opt/qt4-qws/lib -lQtSvg -lQtGui -lQtNetwork -lQtCore -lpthread'], # e.g., '-lm'
    DEFINE            => '', # e.g., '-DHAVE_SOMETHING'
    INC               => '-I. -I../../src/', # e.g., '-I. -I/usr/include/other'
	# Un-comment this if you add C files to link with later:
    # OBJECT            => '$(O_FILES)', # link all the C files too
);
