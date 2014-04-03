
#include "diVcrossHardcopy.h"

#include "diFontManager.h"
#include "diPrintOptions.h"

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glText/glText.h>
#include <glp/glpfile.h>
#endif

#include <map>
#include <stdlib.h>

#define MILOGGER_CATEGORY "diana.VcrossHardcopy"
#include <miLogger/miLogging.h>

VcrossHardcopy::VcrossHardcopy(FontManager* fm)
  : fp(fm)
{
}

bool VcrossHardcopy::start(const printOptions& po)
{
  if (psoutput)
    return false;

  printOptions pro = po;
  printerManager printman;

  int feedsize = 10000000;
  int print_options = 0;

  // Fit output to page
  if (pro.fittopage)
    print_options = print_options | GLP_FIT_TO_PAGE;

  // set colour mode
  if (pro.colop == d_print::greyscale) {
    print_options = print_options | GLP_GREYSCALE;

    if (pro.drawbackground)
      print_options = print_options | GLP_DRAW_BACKGROUND;

  } else if (pro.colop == d_print::blackwhite) {
    print_options = print_options | GLP_BLACKWHITE;

  } else {
    if (pro.drawbackground)
      print_options = print_options | GLP_DRAW_BACKGROUND;
  }

  // set orientation
  if (pro.orientation == d_print::ori_landscape)
    print_options = print_options | GLP_LANDSCAPE;
  else if (pro.orientation == d_print::ori_portrait)
    print_options = print_options | GLP_PORTRAIT;
  else
    print_options = print_options | GLP_AUTO_ORIENT;

  // calculate line, point (and font?) scale
  if (!pro.usecustomsize)
    pro.papersize = printman.getSize(pro.pagesize);
  d_print::PaperSize a4size;
  float scale = 1.0;
  if (abs(pro.papersize.vsize) > 0)
    scale = a4size.vsize / pro.papersize.vsize;

  // check if extra output-commands
  std::map<std::string, std::string> extra;
  printman.checkSpecial(pro, extra);

  // make GLPfile object
  psoutput = new GLPfile(const_cast<char*> (pro.fname.c_str()), print_options, feedsize, &extra, pro.doEPS);

  // set line and point scale
  psoutput->setScales(0.5 * scale, 0.5 * scale);

  psoutput->StartPage();
  // set viewport
  psoutput->setViewport(pro.viewport_x0, pro.viewport_y0, pro.viewport_width, pro.viewport_height);

  // inform fontpack
  if (fp)
    fp->startHardcopy(psoutput);

  return true;
}

void VcrossHardcopy::addHCStencil(const int& size, const float* x, const float* y)
{
  if (psoutput)
    psoutput->addStencil(size, x, y);
}

void VcrossHardcopy::addHCScissor(const int x0, const int y0, const int w, const int h)
{
  if (psoutput)
    psoutput->addScissor(x0, y0, w, h);
}

void VcrossHardcopy::removeHCClipping()
{
  if (psoutput)
    psoutput->removeClipping();
}

void VcrossHardcopy::UpdateOutput()
{
  if (psoutput)
    psoutput->UpdatePage(true);
}

void VcrossHardcopy::resetPage()
{
  if (psoutput)
    psoutput->addReset();
}

bool VcrossHardcopy::startPSnewpage()
{
  if (not psoutput)
    return false;

  glFlush();
  if (psoutput->EndPage() != 0)
    METLIBS_LOG_ERROR("startPSnewpage: EndPage BAD!!!");
  psoutput->StartPage();
  return true;
}

bool VcrossHardcopy::end()
{
  if (not psoutput)
    return false;

  glFlush();
  if (psoutput->EndPage() == 0) {
    delete psoutput;
    psoutput = 0;
  }

  if (fp)
    fp->endHardcopy();

  return true;
}
