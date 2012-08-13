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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>

#include <qcombobox.h>
#include <QListWidget>
#include <qlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlcdnumber.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qpixmap.h>
#include <QPainter>
#include <qimage.h>
#include <qbrush.h>
#include <QIcon>

#include "qtUtility.h"
#include <diField/diLinetype.h>
#include "diImageGallery.h"



int getIndex( vector<miutil::miString> vstr, miutil::miString def_str  ){
  for( unsigned int k=0; k<vstr.size(); k++){
    if( def_str == vstr[k] ){
      return k;
    }
  }
  return -1;
}


int getIndex( vector<Colour::ColourInfo> cInfo, miutil::miString def_str  ){
  for( unsigned int k=0; k<cInfo.size(); k++){
    if( def_str == cInfo[k].name ){
      return k;
    }
  }
  return -1;
}


/*********************************************/
QLabel* TitleLabel(const QString& name, QWidget* parent){
  QLabel* label= new QLabel( name, parent );

  QPalette pal(label->palette());
  pal.setColor(QPalette::WindowText, QColor(0,0,128));
  label->setPalette(pal);

  return label;
}


/*********************************************/
QPushButton* SmallPushButton(const QString& name, QWidget* parent){
  QPushButton* b = new QPushButton( name, parent);

  QString qstr=name;
  int height = int(b->fontMetrics().height()*1.4);
  int width  = int(b->fontMetrics().width(qstr)+ height*0.5);
  b->setMaximumSize( width, height );

  return b;
}


/*********************************************/
QPushButton* NormalPushButton(const QString& name, QWidget* parent){
  QPushButton* b = new QPushButton( name, parent);
  return b;
}


/*********************************************/
QPushButton* PixmapButton(const QPixmap& pixmap, QWidget* parent,
    int deltaWidth, int deltaHeight ) {

  QPushButton* b = new QPushButton( parent );

  b->setIcon(QIcon(pixmap));

  int width  = pixmap.width()  + deltaWidth;
  int height = pixmap.height() + deltaHeight;

  b->setMinimumSize( width, height );
  b->setMaximumSize( width, height );

  return b;
}



/*********************************************/
QComboBox* ComboBox( QWidget* parent, vector<miutil::miString> vstr,
    bool Enabled, int defItem  ){

  QComboBox* box = new QComboBox( parent );

  int nr_box = vstr.size();

  for( int i=0; i<nr_box; i++ ){
    box->addItem(QString(vstr[i].c_str()));
  }

  box->setEnabled( Enabled );

  box->setCurrentIndex(defItem);

  return box;
}



/*********************************************/
QComboBox* ComboBox( QWidget* parent, QColor* pixcolor, int nr_colors,
    bool Enabled, int defItem  ){
  int t;
  QPixmap** pmap = new QPixmap*[nr_colors];
  for( t=0; t<nr_colors; t++ )
    pmap[t] = new QPixmap( 20, 20 );

  for( t=0; t<nr_colors; t++ )
    pmap[t]->fill( pixcolor[t] );

  QComboBox* box = new QComboBox( parent );

  for( int i=0; i < nr_colors; i++){
    box->addItem ( *pmap[i], "");
  }

  box->setEnabled( Enabled );

  box->setCurrentIndex(defItem);

  for( t=0; t<nr_colors; t++ ){
    delete pmap[t];
    pmap[t]=0;
  }

  delete[] pmap;
  pmap=0;

  return box;
}

/*********************************************/
QComboBox* ColourBox( QWidget* parent,
    bool Enabled, int defItem,
    miutil::miString firstItem, bool name ){

  vector<Colour::ColourInfo> cInfo = Colour::getColourInfo();

  return ColourBox(parent, cInfo, Enabled, defItem, firstItem, name);

}

QComboBox* ColourBox( QWidget* parent, const vector<Colour::ColourInfo>& cInfo,
    bool Enabled, int defItem,
    miutil::miString firstItem, bool name ){

  QComboBox* box = new QComboBox( parent );

  if(firstItem.exists())
    box->addItem ( firstItem.cStr() );

  int nr_colors= cInfo.size();
  QPixmap* pmap = new QPixmap( 20, 20 );

  for( int t=0; t<nr_colors; t++ ){
    QColor pixcolor=QColor(cInfo[t].rgb[0],cInfo[t].rgb[1],cInfo[t].rgb[2] );
    pmap->fill( pixcolor );
    QIcon qicon( *pmap );
    QString qs;
    if(name) qs = QString(cInfo[t].name.cStr());
    box->addItem(qicon,qs);
  }

  box->setEnabled( Enabled );
  box->setCurrentIndex(defItem);

  delete pmap;
  pmap=0;

  return box;
}

void ExpandColourBox( QComboBox* box, const Colour::Colour& col )
{
  QPixmap* pmap = new QPixmap( 20, 20 );

  QColor pixcolor=QColor(col.R(),col.G(),col.B() );
  pmap->fill( pixcolor );
  QIcon qicon( *pmap );
  QString qs;
  //if(name)
  qs = QString(col.Name().cStr());
  box->addItem(qicon,qs);

  delete pmap;
  pmap=0;
}

/*********************************************/
QComboBox* PaletteBox( QWidget* parent,
    const vector<ColourShading::ColourShadingInfo>& csInfo,
    bool Enabled,
    int defItem,
    miutil::miString firstItem,
    bool name ){

  QComboBox* box = new QComboBox( parent );

  int nr_palettes= csInfo.size();

  if(firstItem.exists())
    box->addItem ( firstItem.cStr() );

  for( int i=0; i<nr_palettes; i++ ){
    int nr_colours = csInfo[i].colour.size();
    if ( nr_colours == 0 )
      continue;
    int maxwidth=20;
    int step = nr_colours/maxwidth+1;
    int factor = maxwidth/(nr_colours/step);
    int width = (nr_colours/step) * factor;
    QPixmap* pmap = new QPixmap( width, 20 );
    QPainter qp;
    qp.begin( pmap );
    for( int j=0; j<nr_colours; j+=step ){
      QColor pixcolor=QColor(csInfo[i].colour[j].R(),
          csInfo[i].colour[j].G(),
          csInfo[i].colour[j].B() );

      qp.fillRect( j*factor,0,factor,20,pixcolor);
    }

    qp.end();

    QIcon qicon( *pmap );
    QString qs;
    if(name) qs = QString(csInfo[i].name.cStr());
    box->addItem(qicon,qs);
    delete pmap;
    pmap=0;
  }

  box->setEnabled( Enabled );

  box->setCurrentIndex(defItem);

  return box;
}

void ExpandPaletteBox( QComboBox* box, const ColourShading& palette )
{
  vector<Colour> colours = palette.getColourShading();

  int nr_colours = colours.size();
  if ( nr_colours == 0 )
    return;
  int maxwidth=20;
  int step = nr_colours/maxwidth+1;
  int factor = maxwidth/(nr_colours/step);
  int width = (nr_colours/step) * factor;
  QPixmap* pmap = new QPixmap( width, 20 );
  QPainter qp;
  qp.begin( pmap );
  for( int j=0; j<nr_colours; j+=step ){
    QColor pixcolor=QColor(colours[j].R(),
        colours[j].G(),
        colours[j].B() );

    qp.fillRect( j*factor,0,factor,20,pixcolor);
  }

  qp.end();

  QIcon qicon( *pmap );
  QString qs = QString(palette.Name().cStr());
  box->addItem(qicon,qs);
  delete pmap;
  pmap=0;
}

/*********************************************/
QComboBox* PatternBox( QWidget* parent,
    const vector<Pattern::PatternInfo>& patternInfo,
    bool Enabled,
    int defItem,
    miutil::miString firstItem,
    bool name ){

  QComboBox* box = new QComboBox( parent );

  ImageGallery ig;
  QColor pixcolor=QColor("black");

  if(firstItem.exists())
    box->addItem ( firstItem.cStr() );

  int nr_patterns= patternInfo.size();
  for( int i=0; i<nr_patterns; i++ ){
    int index = patternInfo[i].pattern.size()-1;
    if(index<0) continue;
    miutil::miString filename = ig.getFilename(patternInfo[i].pattern[index],true);
    QIcon qicon(QString(filename.cStr()));
    QString qs;
    if(name) qs = QString(patternInfo[i].name.cStr());
    box->addItem(qicon,qs);
  }

  box->setEnabled( Enabled );

  box->setCurrentIndex(defItem);

  return box;
}

/*********************************************/
QComboBox* LinetypeBox( QWidget* parent, bool Enabled, int defItem  ) {

  vector<miutil::miString> slinetypes = Linetype::getLinetypeInfo();
  int nr_linetypes= slinetypes.size();

  QPixmap** pmapLinetypes = new QPixmap*[nr_linetypes];

  for (int i=0; i<nr_linetypes; i++) {
    size_t k1= slinetypes[i].find_first_of('[',0);
    size_t k2= slinetypes[i].find_first_of(']',0);
    if (k2-k1-1>=16) {
      pmapLinetypes[i]= linePixmap(slinetypes[i].substr(k1+1,16),3);
    } else {
      pmapLinetypes[i]= linePixmap("- - - - - - - - ",3);
    }
  }



  QComboBox* box = new QComboBox(parent);
  for(int i=0; i < nr_linetypes; i++){
    box->addItem ( *pmapLinetypes[i], "" );
    delete pmapLinetypes[i];
    pmapLinetypes[i] = NULL;
  }

  box->setEnabled( Enabled );

  delete [] pmapLinetypes;

  return box;
}

/*********************************************/
QComboBox* LinewidthBox( QWidget* parent,
    bool Enabled,
    int nr_linewidths,
    int defItem  ) {

  QComboBox* box = new QComboBox( parent );

  for( int i=0; i < nr_linewidths; i++){
    QPixmap*  pmapLinewidth = new QPixmap;
    pmapLinewidth= linePixmap("x",i+1);
    miutil::miString ss = "  " + miutil::miString(i+1);
    box->addItem ( *pmapLinewidth, ss.cStr() );
    delete pmapLinewidth;
    pmapLinewidth = NULL;
  }
  box->setEnabled(Enabled);

  return box;
}

/*********************************************/
void ExpandLinewidthBox( QComboBox* box,
    int new_nr_linewidths)
{

  int current_nr_linewidths = box->count();

  for( int i=current_nr_linewidths; i < new_nr_linewidths; i++){
    QPixmap*  pmapLinewidth = new QPixmap;
    pmapLinewidth= linePixmap("x",i+1);
    miutil::miString ss = "  " + miutil::miString(i+1);
    box->addItem ( *pmapLinewidth, ss.cStr() );
    delete pmapLinewidth;
    pmapLinewidth = NULL;
  }

}

/*********************************************/
QComboBox* PixmapBox( QWidget* parent, vector<miutil::miString>& markerName){

  /* Image support in Qt
BMP Windows Bitmap Read/write 
GIF Graphic Interchange Format (optional) Read 
JPG Joint Photographic Experts Group Read/write 
JPEG Joint Photographic Experts Group Read/write 
PNG Portable Network Graphics Read/write 
PBM Portable Bitmap Read 
PGM Portable Graymap Read 
PPM Portable Pixmap Read/write 
XBM X11 Bitmap Read/write 
XPM X11 Pixmap Read/write
   */

  QComboBox* box = new QComboBox( parent );

  ImageGallery ig;

  vector<miutil::miString> name;
  ig.ImageNames(name,ImageGallery::marker);

  int n=name.size();
  for( int i=0;i<n; i++){
    miutil::miString filename = ig.getFilename(name[i]);
    markerName.push_back(name[i]);

    miutil::miString format;
    // sometimes Qt doesnt understand the format
    if(filename.contains(".xpm"))
      format = "XPM";
    else if(filename.contains(".png"))
      format = "PNG";
    else if(filename.contains(".jpg"))
      format = "JPG";
    else if(filename.contains(".jpeg"))
      format = "JPEG";
    else if(filename.contains(".gif"))
      format = "GIF";
    else if(filename.contains(".pbm"))
      format = "PBM";
    else if(filename.contains(".pgm"))
      format = "PGM";
    else if(filename.contains(".ppm"))
      format = "PPM";
    else if(filename.contains(".xbm"))
      format = "XBM";
    else if(filename.contains(".bmp"))
      format = "BMP";
    QImage image;
    if (!format.empty())
      image.load(filename.c_str(),format.c_str());
    else
      image.load(filename.c_str(),NULL);
    if (image.isNull())
    {
      cerr << "PixmapBox: problem loading image: " << filename << endl;
      continue;
    }
    QPixmap p = QPixmap::fromImage(image);
    if (p.isNull())
    {
      cerr << "PixmapBox: problem converting from QImage to QPixmap" << filename << endl;
      continue;
    }
    box->addItem (p, "" );
  }

  return box;
}

/*********************************************/
QLCDNumber* LCDNumber( uint numDigits, QWidget * parent ){
  QLCDNumber* lcdnum = new QLCDNumber( numDigits, parent );
  lcdnum->setSegmentStyle ( QLCDNumber::Flat );
  //   lcdnum->setMinimumSize( lcdnum->sizeHint() );
  //   lcdnum->setMaximumSize( lcdnum->sizeHint() );
  return lcdnum;
}


/*********************************************/
QSlider* Slider( int minValue, int maxValue, int pageStep, int value,
    Qt::Orientation orient, QWidget* parent, int width ){
  QSlider* slider = new QSlider(orient, parent);
  slider->setMinimum(minValue);
  slider->setMaximum(maxValue);
  slider->setSingleStep(pageStep);
  slider->setValue(value);
  slider->setMinimumSize( slider->sizeHint() );
  slider->setMaximumWidth( width );
  return slider;
}

/*********************************************/
QSlider* Slider( int minValue, int maxValue, int pageStep, int value,
    Qt::Orientation orient, QWidget* parent ){
  QSlider* slider = new QSlider(orient, parent);
  slider->setMinimum(minValue);
  slider->setMaximum(maxValue);
  slider->setSingleStep(pageStep);
  slider->setValue(value);
  slider->setMinimumSize( slider->sizeHint() );
  slider->setMaximumSize( slider->sizeHint() );
  return slider;
}


/*********************************************/
void listWidget( QListWidget* listwidget, vector<miutil::miString> vstr, int defItem  ){

  if( listwidget->count() )
    listwidget->clear();

  for( unsigned int i=0; i<vstr.size(); i++ ){
    listwidget->addItem( QString(vstr[i].cStr()) );
  }

  if( defItem> -1 ) listwidget->setCurrentRow( defItem );

}

/*********************************************/
QPixmap* linePixmap(const miutil::miString& pattern,
    int linewidth) {
  // make a 32x20 pixmap of a linepattern of length 16 (where ' ' is empty)

  miutil::miString xpmEmpty= "################################";
  miutil::miString xpmLine=  "................................";
  int i;
  int lw= linewidth;
  if (lw<1)  lw=1;
  if (lw>20) lw=20;

  if (pattern.length()>=16) {
    for (i=0; i<16; i++)
      if (pattern[i]==' ') xpmLine[16+i]= xpmLine[i]= '#';
  }

  int l1= 10 - lw/2;
  int l2= l1 + lw;

  const char** xpmData= new const char*[3+20];
  xpmData[0]= "32 20 2 1";
  xpmData[1]= ". c #000000";
  xpmData[2]= "# c None";

  for (i=0;  i<l1; i++) xpmData[3+i]= xpmEmpty.c_str();
  for (i=l1; i<l2; i++) xpmData[3+i]= xpmLine.c_str();
  for (i=l2; i<20; i++) xpmData[3+i]= xpmEmpty.c_str();

  QPixmap* pmap= new QPixmap(xpmData);

  delete[] xpmData;

  return pmap;
}
