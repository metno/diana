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

#include "diana_config.h"

#include "qtAnnoText.h"

#include "qtToggleButton.h"
#include "diController.h"

#include <puTools/miStringFunctions.h>

#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QKeyEvent>
#include <qstring.h>

#include <set>
#include <fstream>

/*********************************************/
AnnoText::AnnoText( QWidget* parent, Controller* llctrl, std::string prodname,
    std::vector<std::string>& symbolText, std::vector<std::string>& xText)
: QDialog(parent), m_ctrl(llctrl)
{
#ifdef DEBUGPRINT
      std::cout<<"AnnoText::AnnoText called"<<std::endl;
#endif

      setModal(true);
      productname=prodname;
      std::string caption=productname+tr(":Write text").toStdString();
      setWindowTitle(caption.c_str());

      //horizontal layout for holding grid layouts
      QHBoxLayout * hglayout = new QHBoxLayout();
      //grid layouts
      int ns = symbolText.size();
      if (ns){
        QGridLayout* glayout = new QGridLayout();
        hglayout->addLayout(glayout, 0);

        for (int i=0;i<ns;i++){
          std::string ltext="Text"+miutil::from_number(i+1);
          QString labeltext=ltext.c_str();
          QLabel* namelabel= new QLabel(labeltext, this) ;

          QComboBox * text = new QComboBox(this);
          text->setEditable(true);

          text->setMinimumWidth(150);

          text->addItem(symbolText[i].c_str());
          text->lineEdit()->deselect();
          connect(text->lineEdit(),
              SIGNAL(selectionChanged()),SLOT(textSelected()));
          connect(text->lineEdit(),SIGNAL(textChanged(const QString &))
              ,SLOT(textChanged(const QString &)));

          glayout->addWidget(namelabel, i+1,1);
          glayout->addWidget(text, i+1,2);
          vSymbolEdit.push_back(text);
        }
      }

      quitb= new QPushButton(tr("Exit"),this);
      connect(quitb, SIGNAL(clicked()), SLOT(stop()));

      int width  = quitb->sizeHint().width();
      int height = quitb->sizeHint().height();
      //set button size
      quitb->setMinimumSize( width, height );
      quitb->setMaximumSize( width, height );

      // buttons layout
      QHBoxLayout * hlayout = new QHBoxLayout();
      hlayout->addWidget(quitb, 10);

      //now create a vertical layout to put all the other layouts in
      QVBoxLayout * vlayout = new QVBoxLayout( this);
      vlayout->addLayout(hglayout, 0);
      vlayout->addLayout(hlayout,0);



}


AnnoText::~AnnoText(){
  int i,n;
  n= vSymbolEdit.size();
  for (i = 0;i<n;i++)
    delete vSymbolEdit[i];
  vSymbolEdit.clear();
}


void AnnoText::getAnnoText(std::vector<std::string>& symbolText, std::vector<std::string>& xText)
{
  symbolText.clear();
  int ns=vSymbolEdit.size();
  for (int i =0; i<ns;i++)
    symbolText.push_back(vSymbolEdit[i]->currentText().toStdString());
}


    void AnnoText::textChanged(const QString &textstring){
      int cursor;
      int sel1=0,sel2=0;
      if (vSymbolEdit.size()){
        cursor = vSymbolEdit[0]->lineEdit()->cursorPosition();
        m_ctrl->changeMarkedAnnotation(textstring.toStdString(),cursor,sel1,sel2);
      }
      emit editUpdate();
    }

    void AnnoText::textSelected(){
      grabMouse();
    }

    void AnnoText::keyReleaseEvent(QKeyEvent* e){
      int cursor;
      int sel1=0,sel2=0;
      if (e->key()==Qt::Key_End){
        stop();
        return;
      } else if(e->key()==Qt::Key_PageDown){
        m_ctrl->editNextAnnoElement();
        std::string text=m_ctrl->getMarkedAnnotation();
        if (vSymbolEdit.size()) vSymbolEdit[0]->setItemText(0,text.c_str());
        for (unsigned int i =0;i<vSymbolEdit.size();i++){
          vSymbolEdit[i]->lineEdit()->selectAll();
        }
      } else if(e->key()==Qt::Key_PageUp){
        m_ctrl->editLastAnnoElement();
        std::string text=m_ctrl->getMarkedAnnotation();
        if (vSymbolEdit.size()) vSymbolEdit[0]->setItemText(0,text.c_str());
        for (unsigned int i =0;i<vSymbolEdit.size();i++){
          vSymbolEdit[i]->lineEdit()->selectAll();
        }
      } else if (vSymbolEdit.size()){
        const QString & textstring=vSymbolEdit[0]->currentText();
        cursor = vSymbolEdit[0]->lineEdit()->cursorPosition();
        m_ctrl->changeMarkedAnnotation(textstring.toStdString(),cursor,sel1,sel2);
      }
      emit editUpdate();
    }



    void AnnoText::mouseReleaseEvent(QMouseEvent *m){
      //cerr << "AnnoText::mouseReleaseEvent" << endl;
      for (unsigned int i =0;i<vSymbolEdit.size();i++){
        vSymbolEdit[i]->lineEdit()->deselect();

      }
      releaseMouse();
      m->ignore();
    }


    void AnnoText::mousePressEvent(QMouseEvent *m){
      //cerr << "AnnoText::mousePressEvent" << endl;
      if (m->x()>quitb->x() && m->x()<(quitb->x()+quitb->width()) &&
          m->y()>quitb->y() && m->y()<(quitb->y()+quitb->height())){
        releaseMouse();
        stop();
      } else
        m->ignore();
    }



    void AnnoText::stop(){
      //save text !
      m_ctrl->stopEditAnnotation(productname);
      emit editUpdate();
      hide();
    }







