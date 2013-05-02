#include <bdiana_capi.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    char* input = "output = PNG\ncolour = COLOUR\nbuffersize = 256x256\nsettime = 2012-09-13 13:00:00\naddhour = 00\nfilename = /tmp/test.png\nPLOT\nMAP area=EPSG:3575 backcolour=255:255:255 xylimit=-640000,-320000,-2880000,-2560000\n\nFIELD Proff_default NEDBOR.1T alpha=160 base=0 colour=off colour_2=off extreme.radius=1 extreme.size=1 extreme.type=Ingen field.smooth=0 grid.lines=0 grid.lines.max=0 label.size=1 line.interval=40 line.smooth=0 value.label=0  line.values=0.1,0.2,0.5,1,2,4,6,10,15,20,25,30,...100 linetype=solid linewidth=1 maxvalue=off minvalue=off palettecolours=vp_nedbor patterns=off recursive=0 table=0 undef.colour=white undef.linetype=solid undef.linewidth=1 undef.masking=0 value.label=1\n\nENDPLOT\n";
    char* argv_[] =  {"-v", "-use_qimage", "-use_singlebuffer"};
    int retVal = -1;
    retVal = mi_di_init(3 , argv_);
    printf("diana initialized: %d\n", retVal);

    //retVal = mi_di_readSetupFile("/disk1/WMS/usr/share/metno-wmsserver/etc/diana.setup-WMS");
    retVal = mi_di_readSetupFile("/disk1/WMS/usr/share/metno-wmsservice/verportal/bdiana/diana.setup");
    printf("setupfile read: %d\n", retVal);

    retVal = mi_di_parseAndProcessString(input);
    printf("input read: %d\n", retVal);

    return 0;
}
