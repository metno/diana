
#ifndef diPolyContouring_hh
#define diPolyContouring_hh 1

#include <string>

class Area;
class FontManager;
class GLPfile;
class PlotOptions;

bool poly_contour(int nx, int ny, float z[], float xz[], float yz[],
    const int ipart[], int icxy, float cxy[], float xylim[],
    int idraw, float zrange[], float zstep, float zoff,
    int nlines, float rlines[],
    int ncol, int icol[], int ntyp, int ityp[],
    int nwid, int iwid[], int nlim, float rlim[],
    int idraw2, float zrange2[], float zstep2, float zoff2,
    int nlines2, float rlines2[],
    int ncol2, int icol2[], int ntyp2, int ityp2[],
    int nwid2, int iwid2[], int nlim2, float rlim2[],
    int ismooth, const int labfmt[], float chxlab, float chylab,
    int ibcol,
    int ibmap, int lbmap, int kbmap[],
    int nxbmap, int nybmap, float rbmap[],
    FontManager* fp, const PlotOptions& poptions, GLPfile* psoutput,
    const Area& fieldArea, const float& fieldUndef,
    const std::string& modelName, const std::string& paramName,
    const int& fhour);

#endif // diPolyContouring_hh
