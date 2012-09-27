package Metno::Bdiana;

use 5.008008;
use strict;
use warnings;

require Exporter;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Metno::Bdiana ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	readSetupFile
    parseAndProcessString
    DI_OK
    DI_ERROR
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.01';

use constant DI_OK => 1;
use constant OK => DI_OK(); # compatibility with old version
use constant DI_ERROR => 99;
use constant ERROR => DI_ERROR(); # compatibility with old version

require XSLoader;
XSLoader::load('Metno::Bdiana', $VERSION);

# Preloaded methods go here.

my $isInit = 0;
sub init {
    deInit_();
    my $ret = init_();
    if ($ret == DI_OK) {
         $isInit++;
    }
    return $ret;
}

sub deInit_ {
    if ($isInit) {
        free();
        $isInit = 0;
    }
}

END {
    deInit_();
}

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

Metno::Bdiana - Perl extension for blah blah blah

=head1 SYNOPSIS

  use Metno::Bdiana;
  blah blah blah

=head1 DESCRIPTION

Stub documentation for Metno::Bdiana, created by h2xs. It looks like the
author of the extension was negligent enough to leave the stub
unedited.

Blah blah blah.

=head2 EXPORT

None by default.



=head1 SEE ALSO

Mention other useful documentation such as the documentation of
related modules or operating system documentation (such as man pages
in UNIX), or any relevant external documentation such as RFCs or
standards.

If you have a mailing list set up for your module, mention it here.

If you have a web site set up for your module, mention it here.

=head1 AUTHOR

Heiko Klein, E<lt>heikok@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Heiko Klein

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.


=cut
