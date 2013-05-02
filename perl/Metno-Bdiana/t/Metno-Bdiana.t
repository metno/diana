# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl Metno-Bdiana.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;

use Test::More tests => 4;
BEGIN { use_ok('Metno::Bdiana', ':all') };

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.
Metno::Bdiana::init();
#my $setup = "/disk1/WMS/usr/share/metno-wmsserver/etc/diana.setup-WMS";
#my $setup = "/etc/diana/3.28/diana.setup-COMMON";
my $setup = "/disk1/WMS/usr/share/metno-wmsservice/verportal/bdiana/diana.setup";
SKIP: {

    skip "no test-data installed", 3 unless -r $setup; 

    is(DI_OK(), readSetupFile($setup), "reading setup");

    my $plot =<<'EOT';
output = PNG
colour = COLOUR
buffersize = 256x256
settime = 2012-09-13 13:00:00
addhour = 00
filename = /tmp/test.png
PLOT
MAP area=EPSG:3575 backcolour=255:255:255 xylimit=-640000,-320000,-2880000,-2560000

FIELD Proff_default NEDBOR.1T alpha=160 base=0 colour=off colour_2=off extreme.radius=1 extreme.size=1 extreme.type=Ingen field.smooth=0 grid.lines=0 grid.lines.max=0 label.size=1 line.interval=40 line.smooth=0 value.label=0  line.values=0.1,0.2,0.5,1,2,4,6,10,15,20,25,30,...100 linetype=solid linewidth=1 maxvalue=off minvalue=off palettecolours=vp_nedbor patterns=off recursive=0 table=0 undef.colour=white undef.linetype=solid undef.linewidth=1 undef.masking=0 value.label=1


ENDPLOT
EOT
    is(DI_OK(), parseAndProcessString($plot), "creating plot");
    ok(-f "/tmp/test.png", "plot created");
}

