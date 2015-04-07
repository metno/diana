/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diController.h"
#include "qtComplexPressureText.h"
#include "qtUtility.h"
#include "qtToggleButton.h"

#include <puTools/miStringFunctions.h>

#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QString>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.ComplexPressureText"
#include <miLogger/miLogging.h>

QValidator::State ComplexPressureText::complexValidator::validate(QString& input, int& pos) const
{
  //validator, only used for zero isoterm input !!!
  std::string inputString = input.toStdString();
  if (!miutil::contains(inputString,"0�:")){
    return QValidator::Invalid;
  }
  return QValidator::Acceptable;
}

// initialize static members
bool               ComplexPressureText::initialized= false;
std::vector<Colour::ColourInfo> ComplexPressureText::colourInfo; // all defined colours
int                ComplexPressureText::nr_colors=0;
QColor*            ComplexPressureText::pixcolor=0;

/*********************************************/
ComplexPressureText::ComplexPressureText( QWidget* parent,
    Controller* llctrl,
    std::vector <std::string> & symbolText,
    std::vector <std::string>  & xText,
    std::set <std::string> cList,
    const std::string & currentTool,
    bool useColour)
: QDialog(parent), m_ctrl(llctrl) , tool(currentTool) , colorIndex(0)
{
  METLIBS_LOG_SCOPE();
  setModal(true);

  if (!initialized) {
    colourInfo = Colour::getColourInfo();
    
    nr_colors= colourInfo.size();
    pixcolor = new QColor[nr_colors];// this must exist in the objectlifetime
    for (int i=0; i<nr_colors; i++) {
      pixcolor[i]=QColor( colourInfo[i].rgb[0], colourInfo[i].rgb[1], colourInfo[i].rgb[2] );
    }
  }

  QPalette *tPalette = new QPalette();
  std::cerr << "Tool: " << tool << std::endl;
  if (miutil::contains(tool,TOOL_DECREASING)) {
    preText = "F";
    tPalette->setColor(QPalette::Text,Qt::red);
    colorIndex = 2;
  } else if (miutil::contains(tool,TOOL_INCREASING)) {
    preText = "S";
    tPalette->setColor(QPalette::Text,Qt::blue);
    colorIndex = 15;
  }
  setWindowTitle(tr("Write text"));

  //  //horizontal layout for holding grid layouts
  QHBoxLayout * hglayout = new QHBoxLayout();
  //grid layouts
  
  int nx = xText.size();
  
  QGridLayout* glayout = new QGridLayout();
  
  hglayout->addLayout(glayout, 0);
  
  //set <std::string> complexList = m_ctrl->getComplexList();
  std::set <std::string> complexList = cList;
  
  QLineEdit *text  = new QLineEdit(this);
  text->setCompleter(NULL);
  cv = new complexValidator(this);
  
  text->setPalette(*tPalette);
  text->setFont(QFont( "Helvetica", 14, QFont::Bold, false ));
  text->setMinimumWidth(30);
  text->setMaximumWidth(40);
  text->setFrame(false);
  text->setText(preText.c_str());
  glayout->addWidget(text, 1,1);
  
  connect(text, SIGNAL(editingFinished()),
      SLOT(textSelected()));
//          connect(text,
//        		  SIGNAL(activated(const QString &)),
//        		  SLOT(textActivated(const QString &)));

  text->setEnabled(false);

  colourbox=0;
  
  if (nx) {
    QGridLayout* glayout = new QGridLayout();
    hglayout->addLayout(glayout, 0);
    for (int i=0;i<nx;i++) {
      METLIBS_LOG_DEBUG("xText["<<i<<"]='"<<xText[i] <<"'");

      QString labeltext = "X" + QString::number(i+1);
      QLabel* namelabel= new QLabel(labeltext, this) ;
      
      QLineEdit *x = new QLineEdit(this);
      x->setMinimumWidth(100);
      x->setText(QString::fromStdString(xText[i]));
      glayout->addWidget(namelabel, i+1, 0);
      glayout->addWidget(x, i+1, 1);
      vXEdit.push_back(x);
    }
  }
  
  QPushButton * okb= new QPushButton(tr("OK"),this);
  connect(okb, SIGNAL(clicked()), SLOT(accept()));
  QPushButton* quitb= new QPushButton(tr("Cancel"),this);
  connect(quitb, SIGNAL(clicked()), SLOT(reject()));

  int width  = quitb->sizeHint().width();
  int height = quitb->sizeHint().height();
  //set button size
  okb->setMinimumSize( width, height );
  okb->setMaximumSize( width, height );
  quitb->setMinimumSize( width, height );
  quitb->setMaximumSize( width, height );
  
  // buttons layout
  QHBoxLayout * hlayout = new QHBoxLayout();
  hlayout->addWidget(okb, 10);
  hlayout->addWidget(quitb, 10);
  
  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout(this);
  vlayout->addLayout(hglayout, 0);
  vlayout->addLayout(hlayout,0);
  
  //important to avoid endless loop !
  startEdit=true;
}

ComplexPressureText::~ComplexPressureText()
{
  int n = vSymbolEdit.size();
  for (int i = 0; i<n; i++)
    delete vSymbolEdit[i];
  
  n= vXEdit.size();
  for (int i = 0; i<n; i++)
    delete vXEdit[i];
  
  delete cv;
}

void ComplexPressureText::getComplexText(std::vector <std::string> & symbolText, std::vector <std::string>  & xText)
{
  METLIBS_LOG_SCOPE();

  symbolText.clear();
  symbolText.push_back(preText);
  xText.clear();
  int nx=vXEdit.size();
  for (int i =0; i<nx;i++)
    xText.push_back(vXEdit[i]->text().toStdString());
}

void ComplexPressureText::setColour(Colour::ColourInfo &colour)
{
  METLIBS_LOG_SCOPE();
  //This function does nothing at the moment. It's still here since it's called p� the manager object
//      int index = 0;//getColourIndex(colourInfo,colour);
//      colourbox-> setCurrentIndex(index);
}

void ComplexPressureText::getColour(Colour::ColourInfo &colour)
{
  //This function just send the hardcoded color values for "Fall" and "Stig"
  METLIBS_LOG_SCOPE();
//      int index=0;//colourbox->currentIndex();
////      if (index>-1 && index<int(colourInfo.size()))
//      if (tool.contains("Fall")) index = 15;
//      else if (tool.contains("Stig")) index = 2;
        colour=colourInfo[colorIndex];
//	  if (tool.contains("Fall")) colour=colourInfo[index];
//      else if (tool.contains("Stig")) colour=colourInfo[index];
}

int ComplexPressureText::getColourIndex(std::vector <Colour::ColourInfo> & colourInfo,
    Colour::ColourInfo colour)
{
  //Maybe needs a cleanup. Code is commented out for future use.
  int index=-1;
//      int nr_colors= colourInfo.size();
//      for(i=0; i<nr_colors; i++ ){
//        if (colourInfo[i].rgb[0]== colour.rgb[0] &&
//            colourInfo[i].rgb[1]==colour.rgb[1] &&
//            colourInfo[i].rgb[2]==colour.rgb[2] )
//          index=i;
//      }
//      if (index==-1){
//        colourInfo.push_back(colour);
//        QColor * pcolor=new QColor( colourInfo[i].rgb[0], colourInfo[i].rgb[1],
//            colourInfo[i].rgb[2] );
//        QPixmap* pmap = new QPixmap( 20, 20 );
//        pmap->fill(*pcolor);
//        QIcon *icon = new QIcon(*pmap);
//        colourbox->addItem ( *icon,"");
//        index=nr_colors;
//      }
  return index;
}

void ComplexPressureText::textActivated(const QString &textstring)
{
  METLIBS_LOG_SCOPE();
  for (unsigned int i =0;i<vSymbolEdit.size();i++){
    if (!vSymbolEdit[i]->hasFocus()) continue;
    startEdit=true;
    selectText(i);
  }
}

void ComplexPressureText::selectText(int i)
{
  METLIBS_LOG_SCOPE();
  // Special routine to facilitate editing strings with "0�:"
  //
  if (startEdit){
    startEdit=false;
    std::string text = vSymbolEdit[i]->currentText().toStdString();
    METLIBS_LOG_DEBUG(LOGVAL(text));
    if (!miutil::contains(text,"0�:")){
      vSymbolEdit[i]->lineEdit()->setValidator(0);
      return;
    } else {
      vSymbolEdit[i]->lineEdit()->setValidator(cv);
      vSymbolEdit[i]->lineEdit()->setCursorPosition(3);
      int length=vSymbolEdit[i]->currentText().length()-3;
      vSymbolEdit[i]->lineEdit()->setSelection(3,length);
    }
  }
}

void ComplexPressureText::textSelected()
{
  METLIBS_LOG_SCOPE();
  for (unsigned int i =0;i<vSymbolEdit.size();i++)
    selectText(i);
}
