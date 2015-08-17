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
    DEFINE            => '-Wall', # e.g., '-DHAVE_SOMETHING'
    INC               => '',   # set from commandline in perl/Makefile.am
    LIBS              => [''], # set from commandline in perl/Makefile.am
    MYEXTLIB          => '', # set from perl/Makefile.am
);


sub MY::postamble {
    return <<'MAKE_FRAG';
$(MYEXTLIB):
	make -C ../../src all
MAKE_FRAG
}
