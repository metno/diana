/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no
  
  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <diFontManager.h>
#include <glTextX.h>
#include <glTextTT.h>
#include <GLP.h>

miString FontManager::fontpath;
miString FontManager::display_name;
map<miString,miString> FontManager::defaults;

static const miString key_bitmap=        "bitmap";
static const miString key_scaleable=     "scaleable";

static const miString key_bitmapfont=    "bitmapfont";
static const miString key_scalefont=     "scalefont";
static const miString key_metsymbolfont= "metsymbolfont";
  

FontManager::FontManager() :
  xfonts(0), ttfonts(0), usexf(false)
{
  if (display_name.exists()) // do not use environment-var DISPLAY
    xfonts= new glTextX(display_name);
  else
    xfonts= new glTextX();

  ttfonts= new glTextTT();
}

FontManager::~FontManager()
{
  if (xfonts) delete xfonts;
  if (ttfonts) delete ttfonts;
}


void FontManager::startHardcopy(GLPcontext* gc)
{
  if (xfonts) xfonts->startHardcopy(gc);
  if (ttfonts) ttfonts->startHardcopy(gc);
}

void FontManager::endHardcopy()
{
  if (xfonts) xfonts->endHardcopy();
  if (ttfonts) ttfonts->endHardcopy();
}


// fill fontpack for testing
bool FontManager::testDefineFonts(miString path)
{
  bool res= true;
  res= xfonts->testDefineFonts(path);
  xfam.clear();
  int n= xfonts->getNumFonts();
  cerr << "-- TEST Defined X-fonts:" << endl; 
  for (int i=0; i<n; i++){
    miString s = xfonts->getFontName(i);
    xfam.insert(s);
    cerr << i << " " << s << endl;
  }
  
  res= res && ttfonts->testDefineFonts(path);
  n= ttfonts->getNumFonts();
  cerr << "-- TEST Defined TT-fonts:" << endl; 
  for (int i=0; i<n; i++){
    miString s = ttfonts->getFontName(i);
    ttfam.insert(s);
    cerr << i << " " << s << endl;
  }
  
  return res;
}


glText::FontFace FontManager::fontFace( const miString& s)
{
  miString suface= s.upcase();
  suface.trim();
  glText::FontFace face= glText::F_NORMAL;
  if (suface!="NORMAL"){
    if (suface=="ITALIC")
      face= glText::F_ITALIC;
    else if (suface=="BOLD")
      face= glText::F_BOLD;
    else if (suface=="BOLD_ITALIC")
      face= glText::F_BOLD_ITALIC;
  }

  return face;
}


bool FontManager::parseSetup(SetupParser& sp){
  const miString sf_name=  "FONTS";
  vector<miString> sect_fonts;
  
  const miString key_font=       "font";
  const miString key_fonttype=   "type";
  const miString key_fontface=   "face";
  const miString key_fontname=   "name";
  const miString key_postscript= "postscript";
  const miString key_fontpath=   "fontpath";

  defaults[key_bitmapfont]    = "Helvetica";
  defaults[key_scalefont]     = "Arial";
  defaults[key_metsymbolfont] = "Symbol";

  xfam.clear();
  ttfam.clear();

  if (!fontpath.exists()){
    fontpath= sp.basicValue("fontpath");
    if (!fontpath.exists())
      fontpath="fonts/";
  }

  if (!sp.getSection(sf_name,sect_fonts)){
    //cerr << "Missing section " << sf_name << " in setupfile." << endl;
    testDefineFonts(fontpath);
    return false;
  }

  int n= sect_fonts.size();
  for (int i=0; i<n; i++){
    miString fontfam   = "";
    miString fontname  = "";
    miString fonttype  = "";
    miString fontface  = "";
    miString postscript= "";

    vector<miString> stokens = sect_fonts[i].split(" ");
    for (int j=0; j<stokens.size(); j++){
      miString key;
      miString val;
      sp.splitKeyValue(stokens[j], key, val);
      //cerr << "Key:" << key << " Value:" << val << endl;
      
      if ( key == key_font )
	fontfam = val;
      else if ( key == key_fonttype )
	fonttype = val;
      else if ( key == key_fontface )
	fontface = val;
      else if ( key == key_fontname )
	fontname = val;
      else if ( key == key_postscript )
	postscript = val;
      else if ( key == key_fontpath )
	fontpath = val;
      else
	defaults[key] = val;
    }

//     cerr << " Fonttype:" << fonttype
// 	 << " Fontfam:"  << fontfam
// 	 << " Fontname:" << fontname
// 	 << " Fontface:" << fontface
// 	 << " Postscript:" << postscript << endl;
      
    if ( !fonttype.exists() || !fontfam.exists() || !fontname.exists() )
      continue;
    
    if ( fonttype.downcase() == key_bitmap ){
      xfam.insert(fontfam);
      if (xfonts) xfonts->defineFonts( fontname, fontfam, postscript );

    } else if ( fonttype.downcase() == key_scaleable ){
      ttfam.insert(fontfam);
      if (ttfonts) ttfonts->defineFont( fontfam,
					fontpath + "/" + fontname,
					fontFace( fontface ),
					20);
    }
  }

//   std::set<miString>::iterator fitr;
//   int i;
//   cerr << "-- Defined X-fonts:" << endl; 
//   for (i=0, fitr=xfam.begin(); fitr!=xfam.end(); fitr++,i++){
//     cerr << i << " " << *fitr << endl;
//   }
  
//   cerr << "-- Defined TT-fonts:" << endl; 
//   for (i=0, fitr=ttfam.begin(); fitr!=ttfam.end(); fitr++,i++){
//     cerr << i << " " << *fitr << endl;
//   }

  return true;
}


bool FontManager::check_family( const miString& fam,
				miString& family )
{
  if ( defaults.count( fam.downcase() ) > 0 )
    family = defaults[fam.downcase()];
  else
    family = fam;

  // return true if bitmapfont
  return ( fam.downcase() == key_bitmapfont ||
	   find(xfam.begin(),xfam.end(),fam) != xfam.end() );
}



// choose font, size and face
bool FontManager::set(const miString fam,
		      const glText::FontFace face,
		      const float size)
{
  bool res= true;
  miString family;

  usexf = check_family(fam, family);

  if ( usexf )
    res= xfonts->set(family,face,size);
  else
    res= ttfonts->set(family,face,size);
  return res;
}


// choose font, size and face
bool FontManager::set(const miString fam,
		      const miString sface,
		      const float size)
{
//   static miString cfam="";
//   static miString csface="";
//   static float csize=0.0;

//   if ( fam==cfam && sface==csface && fabsf(size-csize)<0.05 )
//     return true;
  
//   cfam=fam;
//   csface=sface;
//   csize=size;

  //cerr << "Really set fam=" << fam << " sface=" << sface << " size=" << size << endl;

  bool res= true;
  miString family;
  glText::FontFace face= fontFace( sface );
  usexf= check_family(fam, family);

  if ( usexf )
    res= xfonts->set(family,face,size);
  else
    res= ttfonts->set(family,face,size);
  return res;
}


bool FontManager::setFont(const miString fam)
{
  bool res= true;
  miString family;
  
  usexf= check_family(fam, family);

  if ( usexf )
    res= xfonts->setFont(family);
  else
    res= ttfonts->setFont(family);
  return res;
}



bool FontManager::setFontFace(const glText::FontFace face)
{
  bool res= true;
  if (usexf)
    res= xfonts->setFontFace(face);
  else
    res= ttfonts->setFontFace(face);
  return res;
}


bool FontManager::setFontFace(const miString sface)
{
  bool res= true;
  glText::FontFace face= fontFace( sface );

  if (usexf)
    res= xfonts->setFontFace(face);
  else
    res= ttfonts->setFontFace(face);
  return res;
}


bool FontManager::setFontSize(const float size)
{
  bool res= true;
  if (usexf)
    res= xfonts->setFontSize(size);
  else
    res= ttfonts->setFontSize(size);
  return res;
}


// printing commands
bool FontManager::drawChar(const int c, const float x,
			   const float y, const float a)
{
  bool res= true;
  if (usexf)
    res= xfonts->drawChar(c,x,y,a);
  else
    res= ttfonts->drawChar(c,x,y,a);
  return res;
}

bool FontManager::drawStr(const char* s, const float x,
			  const float y, const float a)
{
  bool res= true;
  if (usexf)
    res= xfonts->drawStr(s,x,y,a);
  else
    res= ttfonts->drawStr(s,x,y,a);
  return res;
}

// Metric commands
void FontManager::adjustSize(const int sa)
{
  xfonts->adjustSize(sa);
  ttfonts->adjustSize(sa);
}

void FontManager::setScalingType(const glText::FontScaling fs)
{
  xfonts->setScalingType(fs);
  ttfonts->setScalingType(fs);
}

// set viewport size in GL coordinates
void FontManager::setGlSize(const float glw, const float glh)
{
  xfonts->setGlSize(glw,glh);
  ttfonts->setGlSize(glw,glh);
}

// set viewport size in physical coordinates (pixels)
void FontManager::setVpSize(const float vpw, const float vph)
{
  xfonts->setVpSize(vpw,vph);
  ttfonts->setVpSize(vpw,vph);
}

void FontManager::setPixSize(const float pw, const float ph)
{
  xfonts->setPixSize(pw,ph);
  ttfonts->setPixSize(pw,ph);
}

bool FontManager::getCharSize(const int c, float& w, float& h)
{
  bool res= true;
  if (usexf)
    res= xfonts->getCharSize(c,w,h);
  else
    res= ttfonts->getCharSize(c,w,h);
  return res;
}

bool FontManager::getMaxCharSize(float& w, float& h)
{
  bool res= true;
  if (usexf)
    res= xfonts->getMaxCharSize(w,h);
  else
    res= ttfonts->getMaxCharSize(w,h);
  return res;
}

bool FontManager::getStringSize(const char* s, float& w, float& h)
{
  bool res= true;
  if (usexf)
    res= xfonts->getStringSize(s,w,h);
  else
    res= ttfonts->getStringSize(s,w,h);
  return res;
}

// return info
glText::FontScaling FontManager::getFontScaletype()
{
  if (usexf)
    return xfonts->getFontScaletype();
  else
    return ttfonts->getFontScaletype();
}

int FontManager::getNumFonts()
{
  if (usexf)
    return xfonts->getNumFonts();
  else
    return ttfonts->getNumFonts();
}

int FontManager::getNumSizes()
{
  if (usexf)
    return xfonts->getNumSizes();
  else
    return ttfonts->getNumSizes();
}

glText::FontFace FontManager::getFontFace()
{
  if (usexf)
    return xfonts->getFontFace();
  else
    return ttfonts->getFontFace();
}

float FontManager::getFontSize()
{
  if (usexf)
    return xfonts->getFontSize();
  else
    return ttfonts->getFontSize();
}

int FontManager::getFontSizeIndex()
{
  if (usexf)
    return xfonts->getFontSizeIndex();
  else
    return ttfonts->getFontSizeIndex();
}

miString FontManager::getFontName(const int index)
{
  if (usexf)
    return xfonts->getFontName(index);
  else
    return ttfonts->getFontName(index);
}

float FontManager::getSizeDiv()
{
  if (usexf)
    return xfonts->getSizeDiv();
  else
    return 1.0;
}

