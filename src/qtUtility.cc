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

#include <qtUtility.h>
#include <diLinetype.h>
#include <diImageGallery.h>



int getIndex( vector<miString> vstr, miString def_str  ){
  for( int k=0; k<vstr.size(); k++){
    if( def_str == vstr[k] ){ 
      return k; 
    }
  }
  return -1;
}


int getIndex( vector<Colour::ColourInfo> cInfo, miString def_str  ){
  for( int k=0; k<cInfo.size(); k++){
    if( def_str == cInfo[k].name ){ 
      return k; 
    }
  }
  return -1;
}


/*********************************************/
QLabel* TitleLabel(const QString& name, QWidget* parent){
  QLabel* label= new QLabel( name, parent );
  label->setPaletteForegroundColor ( QColor(0,0,128) );

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

  b->setIconSet(QIcon(pixmap));
  
  int width  = pixmap.width()  + deltaWidth;
  int height = pixmap.height() + deltaHeight;

  b->setMinimumSize( width, height );
  b->setMaximumSize( width, height );

  return b;
}



/*********************************************/
QComboBox* ComboBox( QWidget* parent, vector<miString> vstr, 
		     bool Enabled, int defItem  ){

  QComboBox* box = new QComboBox( false, parent );
 
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

  QComboBox* box = new QComboBox( false, parent );
  
  for( int i=0; i < nr_colors; i++){
    box->insertItem ( *pmap[i] );
  }

  box->setEnabled( Enabled );
  
  box->setCurrentItem(defItem);

  for( t=0; t<nr_colors; t++ ){
    delete pmap[t];
    pmap[t]=0;
  }

  delete[] pmap;
  pmap=0;

  return box;
}

/*********************************************/
QComboBox* ColourBox( QWidget* parent, const vector<Colour::ColourInfo>& cInfo,
		      bool Enabled, int defItem,
		      miString firstItem, bool name ){

  QComboBox* box = new QComboBox( false, parent );

  if(firstItem.exists())
    box->insertItem ( firstItem.cStr() );

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
  box->setCurrentItem(defItem);

  delete pmap;
  pmap=0;

  return box;
}

/*********************************************/
QComboBox* PaletteBox( QWidget* parent, 
		       const vector<ColourShading::ColourShadingInfo>& csInfo,
		       bool Enabled, 
		       int defItem,
		       miString firstItem,
		       bool name ){

  QComboBox* box = new QComboBox( false, parent );

  int nr_palettes= csInfo.size();

  if(firstItem.exists())
    box->insertItem ( firstItem.cStr() );
    
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

  box->setCurrentItem(defItem);

  return box;
}

/*********************************************/
QComboBox* PatternBox( QWidget* parent, 
		       const vector<Pattern::PatternInfo>& patternInfo,
		       bool Enabled, 
		       int defItem,
		       miString firstItem,
		       bool name ){

  QComboBox* box = new QComboBox( false, parent );

  ImageGallery ig;
  QColor pixcolor=QColor("black");

  if(firstItem.exists())
    box->insertItem ( firstItem.cStr() );
    
  int nr_patterns= patternInfo.size();
  for( int i=0; i<nr_patterns; i++ ){
    int index = patternInfo[i].pattern.size()-1;
    if(index<0) continue;
    miString filename = ig.getFilename(patternInfo[i].pattern[index],true);
    QIcon qicon(QString(filename.cStr()));
    QString qs;
    if(name) qs = QString(patternInfo[i].name.cStr());
    box->addItem(qicon,qs);
  }

  box->setEnabled( Enabled );

  box->setCurrentItem(defItem);

  return box;
}

/*********************************************/
QComboBox* LinetypeBox( QWidget* parent, bool Enabled, int defItem  )
{

  vector<miString> slinetypes = Linetype::getLinetypeInfo();
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
  for(int i=0; i < nr_linetypes; i++)
    box->insertItem ( *pmapLinetypes[i] );

  box->setEnabled( Enabled );

  return box;
}

/*********************************************/
QComboBox* LinewidthBox( QWidget* parent, 
			 bool Enabled, 
			 int nr_linewidths,
			 int defItem  )
{

  QComboBox* box = new QComboBox( false, parent );

  QPixmap**  pmapLinewidths = new QPixmap*[nr_linewidths];
  vector<miString> linewidths;
  
  for (int i=0; i<nr_linewidths; i++) {
    ostringstream ostr;
    ostr << i+1;
    linewidths.push_back(ostr.str());
    pmapLinewidths[i]= linePixmap("x",i+1);
  }
  
  for( int i=0; i < nr_linewidths; i++){
    miString ss = "  " + miString(i+1);
    box->insertItem ( *pmapLinewidths[i], ss.cStr() );
  }
  box->setEnabled(true);

  return box;
}

/*********************************************/
QComboBox* PixmapBox( QWidget* parent, vector<miString>& markerName){

  QComboBox* box = new QComboBox( false, parent );

  ImageGallery ig;

  vector<miString> name;
  ig.ImageNames(name,ImageGallery::marker);

  int n=name.size();
  for( int i=0;i<n; i++){
    miString filename = ig.getFilename(name[i]);
    markerName.push_back(name[i]);
    QImage image(filename.c_str());
    QPixmap p(image);
    box->insertItem ( p );
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
  QSlider* slider = new QSlider( minValue, maxValue, pageStep, value, 
				 orient, parent);
  slider->setMinimumSize( slider->sizeHint() );
  slider->setMaximumWidth( width );
  return slider;
}

/*********************************************/
QSlider* Slider( int minValue, int maxValue, int pageStep, int value,  
		 Qt::Orientation orient, QWidget* parent ){
  QSlider* slider = new QSlider( minValue, maxValue, pageStep, value, 
				 orient, parent);
  slider->setMinimumSize( slider->sizeHint() );
  slider->setMaximumSize( slider->sizeHint() );
  return slider;
}


/*********************************************/
void listWidget( QListWidget* listwidget, vector<miString> vstr, int defItem  ){

  if( listwidget->count() )
    listwidget->clear();

  for( int i=0; i<vstr.size(); i++ ){
    listwidget->addItem( QString(vstr[i].cStr()) );
  }

  if( defItem> -1 ) listwidget->setCurrentRow( defItem );

}

/*********************************************/
QPixmap* linePixmap(const miutil::miString& pattern, 
				      int linewidth) 
{

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
