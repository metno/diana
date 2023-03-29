/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "qtEditText.h"
#include "qtUtility.h"
#include "qtToggleButton.h"
#include "diController.h"

#include <puTools/miStringFunctions.h>

#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextBlock>
#include <QString>

#include <fstream>

//#define DEBUGPRINT
#define MILOGGER_CATEGORY "diana.EditText"
#include <miLogger/miLogging.h>


QValidator::State EditText::complexValidator::validate(QString& inputString, int& pos) const
{
  //validator, only used for zero isoterm input !!!
  if (!inputString.contains("0°:")) {
    return QValidator::Invalid;
  }
  return QValidator::Acceptable;
}

// initialize static members
bool               EditText::initialized= false;
std::vector<Colour::ColourInfo> EditText::colourInfo; // all defined colours
int                EditText::nr_colors=0;
QColor*            EditText::pixcolor=0;

/*********************************************/
EditText::EditText(QWidget* parent, Controller* llctrl, std::vector<std::string>& symbolText, std::set<std::string> cList, bool useColour)
    : QDialog(parent)
    , m_ctrl(llctrl)
{
#ifdef DEBUGPRINT
  std::cout << "EditText::EditText called" << std::endl;
#endif

      setModal(false);

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

      setWindowTitle(tr("Write text for editing"));
      QString multitext;
      int ns = symbolText.size();
      METLIBS_LOG_DEBUG("?????????ns = "<< ns);
     // int nx = xText.size();
      //set <std::string> complexList = m_ctrl->getEditList();
      std::set<std::string> complexList = cList;
      QTextEdit *edittext = new QTextEdit(this);
      edittext->setLineWrapMode(QTextEdit::WidgetWidth);
      edittext->setFont(QFont("Arial", 10, QFont::Normal));
      edittext->setReadOnly(false);
      edittext->setMaximumHeight(150);
      if ( ns > 0 ) {
        for(int i=0; i<ns; i++ ){
           //multitext += symbolText[i].c_str();
           multitext.append(symbolText[i].c_str());
           multitext.append("\n");
        } 
      //edittext->setText("The QTextEdit class provides a widget that is used to edit and display both plain and rich text. More...");
      edittext->setText(multitext);
      } 
      //connect(edittext,SIGNAL(selectionChanged()),SLOT(textSelected()));
      //connect(edittext, SIGNAL(textChanged()), SLOT(textActivated()));
      vSymbolEdit =  edittext;  
      /*if (useColour){
            QGridLayout* glayout = new QGridLayout();
            QLabel* colourlabel= new QLabel(tr("Colour"), this) ;
            colourbox = ComboBox(this, pixcolor, nr_colors, false, 0 );
            colourbox->setMaximumWidth(50);
            colourbox->setMinimumWidth(50);
            colourbox->setEnabled(true);
            glayout->addWidget(colourlabel,1,2);
            glayout->addWidget(colourbox,1,3);
       }
       else
            colourbox=0;*/


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
      /*if (useColour){
         vlayout->addLayout(glayout,0);
      }*/
      vlayout->addWidget(edittext);
      vlayout->addLayout(hlayout,0);
      vlayout->activate();
      resize(350, 200);
 
      //important to avoid endless loop !
      startEdit=true;

} // End of Constructor


    EditText::~EditText(){
      delete vSymbolEdit;
      if (cv) delete cv;
      cv=0;
    }

    void EditText::getEditText(std::vector<std::string>& symbolText)
    {
#ifdef DEBUGPRINT
      std::cout << "EditText::getEditText called" << std::endl;
#endif
      //lines get when you enter return button
      //int lines = vSymbolEdit->document()->blockCount();
      QTextDocument *doc = vSymbolEdit->document();
      // get each line from textedit and put it into vector 
      // line length is max 50 char
      std::string line;
      symbolText.clear();
      for (QTextBlock it = doc->begin(); it!=doc->end(); it = it.next()) {
           line = it.text().toStdString();
           if (line.length()<55){
              symbolText.push_back(line);
           } else {
               std::vector<std::string> stokens = miutil::split(line, " ", true);
               std::string token, oldtoken;
               int slength = 0;
               int mtokens = stokens.size();
               for (int k = 0; k < mtokens; k++) {
                   int len = stokens[k].length();
                   slength += len;
                   token += stokens[k];
                   token += " ";
                   if (token.length() > 50 && token.length() < 55) {
                      symbolText.push_back(token);
                      token = "";
                      slength = 0;
                   }
                   else if (token.length() > 55){
                     token = oldtoken;
                     symbolText.push_back(token); 
                     token = stokens[k];
                     token += " ";
                     slength = token.length(); 
                   }
                   oldtoken = token; 
               } //end of for loop
              symbolText.push_back(oldtoken);  
          }  // end of else 
      }   // end of for 

    }


    void EditText::setColour(Colour::ColourInfo &colour){
#ifdef DEBUGPRINT
      std::cout << "EditText::setColour called" << std::endl;
#endif
      int index = getColourIndex(colourInfo,colour);
      colourbox-> setCurrentIndex(index);

    }

    void EditText::getColour(Colour::ColourInfo &colour){
#ifdef DEBUGPRINT
      std::cout << "EditText::getColour called" << std::endl;
#endif
      int index=colourbox->currentIndex();
      if (index>-1 && index<int(colourInfo.size()))
        colour=colourInfo[index];
    }

    int EditText::getColourIndex(std::vector<Colour::ColourInfo>& colourInfo, Colour::ColourInfo colour)
    {
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
        QIcon *icon = new QIcon(*pmap);
        colourbox->addItem ( *icon,"");
        index=nr_colors;
      }
      return index;
    }
