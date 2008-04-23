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
/*
  Input for adding complex text
*/  

#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QVBoxLayout>

#include <fstream>
#include <qtComplexText.h>
#include <qtUtility.h>

#include <miString.h>
#include <qstring.h>
#include <qtToggleButton.h>
#include <iostream>
#include <diController.h>


QValidator::State ComplexText::complexValidator::validate(QString& input,
							 int& pos) const
{
  //validator, only used for zero isoterm input !!!
  miString inputString = input.toStdString();
  if (!inputString.contains("0°:")){
    return QValidator::Invalid;
  }
  return QValidator::Acceptable;
}

// initialize static members
bool               ComplexText::initialized= false;
vector<Colour::ColourInfo> ComplexText::colourInfo; // all defined colours
int                ComplexText::nr_colors=0;
QColor*            ComplexText::pixcolor=0;

/*********************************************/
ComplexText::ComplexText( QWidget* parent, Controller* llctrl, 
vector <miString> & symbolText, vector <miString>  & xText, 
			  set <miString> cList,bool useColour)
: QDialog(parent,"complex",true), m_ctrl(llctrl)
{
#ifdef DEBUGPRINT 
  cout<<"ComplexText::ComplexText called"<<endl;
#endif

  //------------------------
  if (!initialized) {
  //------------------------


    colourInfo = Colour::getColourInfo();


    nr_colors= colourInfo.size();
    pixcolor = new QColor[nr_colors];// this must exist in the objectlifetime
    for(int i=0; i<nr_colors; i++ ){
      pixcolor[i]=QColor( colourInfo[i].rgb[0], colourInfo[i].rgb[1],
			  colourInfo[i].rgb[2] );
      
    
    }
  }
  cv = 0;

  setWindowTitle(tr("Write text"));

 //  //horizontal layout for holding grid layouts
  QHBoxLayout * hglayout = new QHBoxLayout(20, "hglayout");
  //grid layouts
  int ns = symbolText.size();
  int nx = xText.size();
  if (ns){
    QGridLayout* glayout = new QGridLayout(ns,2,5,"symbol");
    hglayout->addLayout(glayout, 0);

    //set <miString> complexList = m_ctrl->getComplexList();
    set <miString> complexList = cList;

    for (int i=0;i<ns;i++){
      miString ltext="Text"+miString(i+1);
      QString labeltext=ltext.c_str();
      QLabel* namelabel= new QLabel(labeltext, this,"textlabel") ;
    
      QComboBox *text = new QComboBox(TRUE,this,"text");
      if (!cv) cv = new complexValidator(this);
      set <miString>::iterator p = complexList.begin();  
      for (; p!=complexList.end(); p++) {
	miString temp=*p;
	text->insertItem(temp.c_str());
      }
      text->setCurrentText(symbolText[i].c_str()); 
      connect(text->lineEdit(),
	      SIGNAL(selectionChanged()),SLOT(textSelected()));
      connect(text,SIGNAL(activated(const QString &)),
	      SLOT(textActivated(const QString &)));

      glayout->addWidget(namelabel, i+1,0);
      glayout->addWidget(text, i+1,1);
      vSymbolEdit.push_back(text);

      if (useColour){

	QLabel* colourlabel= new QLabel(tr("Colour"), this,"clabel") ;
	colourbox = ComboBox(this, pixcolor, nr_colors, false, 0 );
	colourbox->setMaximumWidth(50);
	colourbox->setMinimumWidth(50);
	colourbox->setEnabled(true);
	glayout->addWidget(colourlabel,i+1,2);
	glayout->addWidget(colourbox,i+1,3);
	glayout->addColSpacing(4,10);
      }
      else 
	colourbox=0;

    }

    glayout->setColStretch(1,10);


  }

  
  if (nx){
    QGridLayout* glayout = new QGridLayout(nx,2,5,"xtext");
    hglayout->addLayout(glayout, 0);
    for (int i=0;i<nx;i++){
      miString ltext="X"+miString(i+1);
      QString labeltext=ltext.c_str();

      QLabel* namelabel= new QLabel(labeltext, this,"xlabel") ;

      QLineEdit *x  = new QLineEdit(this,"x");
      x->setMinimumWidth(100);
      x->setText(xText[i].c_str());
      glayout->addWidget(namelabel, i+1,0);
      glayout->addWidget(x, i+1,1);
      vXEdit.push_back(x);
    }
    
  }



  QPushButton * okb= new QPushButton(tr("OK"),this, "okb");
  connect(okb, SIGNAL(clicked()), SLOT(accept()));
  QPushButton* quitb= new QPushButton(tr("Cancel"),this, "quitb");
  connect(quitb, SIGNAL(clicked()), SLOT(reject()));

  int width  = quitb->sizeHint().width();
  int height = quitb->sizeHint().height();
  //set button size
  okb->setMinimumSize( width, height );
  okb->setMaximumSize( width, height );  
  quitb->setMinimumSize( width, height );
  quitb->setMaximumSize( width, height );

  // buttons layout
  QHBoxLayout * hlayout = new QHBoxLayout(20, "hlayout");
  hlayout->addWidget(okb, 10);
  hlayout->addWidget(quitb, 10);

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout( this, 10, 10 );                            
  vlayout->addLayout(hglayout, 0);
  vlayout->addLayout(hlayout,0);

  //important to avoid endless loop !
  startEdit=true;

}


ComplexText::~ComplexText(){
  int i,n;
  n= vSymbolEdit.size();
  for (i = 0;i<n;i++)
    delete vSymbolEdit[i];
  vSymbolEdit.clear();
  n= vXEdit.size();
  for (i = 0;i<n;i++)
    delete vXEdit[i];
  vXEdit.clear();
  if (cv) delete cv;
  cv=0;
}


void ComplexText::getComplexText(vector <miString> & symbolText, vector <miString>  & xText){
  symbolText.clear();
  int ns=vSymbolEdit.size();
  for (int i =0; i<ns;i++)
    symbolText.push_back(vSymbolEdit[i]->currentText().toStdString());
  xText.clear();
  int nx=vXEdit.size();
  for (int i =0; i<nx;i++)
    xText.push_back(vXEdit[i]->text().toStdString());

}


void ComplexText::setColour(Colour::ColourInfo &colour){
  int index = getColourIndex(colourInfo,colour);
  colourbox-> setCurrentItem(index);

}

void ComplexText::getColour(Colour::ColourInfo &colour){
  int index=colourbox->currentItem();
  if (index>-1 && index<colourInfo.size())
    colour=colourInfo[index];
}


int ComplexText::getColourIndex(vector <Colour::ColourInfo> & colourInfo,
				Colour::ColourInfo colour){
  int i,index=-1;
  int nr_colors= colourInfo.size();
  for(i=0; i<nr_colors; i++ ){
    if (colourInfo[i].rgb[0]== colour.rgb[0] && 
	colourInfo[i].rgb[1]==colour.rgb[1] && 
	colourInfo[i].rgb[2]==colour.rgb[2] )
      index=i;
  }
  if (index==-1){
    colourInfo.push_back(colour);
    QColor * pcolor=new QColor( colourInfo[i].rgb[0], colourInfo[i].rgb[1],
			colourInfo[i].rgb[2] );
    QPixmap* pmap = new QPixmap( 20, 20 );
    pmap->fill(*pcolor);
    colourbox->insertItem ( *pmap);
    index=nr_colors;
  }
  return index;
}




void ComplexText::textActivated(const QString &textstring){
  for (int i =0;i<vSymbolEdit.size();i++){
    if (!vSymbolEdit[i]->hasFocus()) continue;
    startEdit=true;
    selectText(i);
  }
}


void ComplexText::selectText(int i){
  // Special routine to facilitate editing strings with "0°:"
  //
 if (startEdit){
    startEdit=false;
    miString text = vSymbolEdit[i]->currentText().toStdString();
    if (!text.contains("0°:")){
      vSymbolEdit[i]->lineEdit()->clearValidator();
      return;
    } else {
      vSymbolEdit[i]->lineEdit()->setValidator(cv);
      vSymbolEdit[i]->lineEdit()->setCursorPosition(3);
      int length=vSymbolEdit[i]->currentText().length()-3;
      vSymbolEdit[i]->lineEdit()->setSelection(3,length);
    }
  }
}




void ComplexText::textSelected(){
  for (int i =0;i<vSymbolEdit.size();i++){
    selectText(i);
  }
}







