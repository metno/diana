/*
  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "qtProfetTimeSmoothDialog.h"
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>

ProfetTimeSmoothDialog::ProfetTimeSmoothDialog(QWidget* parent, vector<fetObject::TimeValues>& obj, vector<miTime>& tim) 
: QMainWindow(parent)
{
  
    setAttribute(Qt::WA_DeleteOnClose); 
  
  
  // actions: --------------------------

  // undo ========================
   undoAction = new QAction(  QIcon(), tr("&Undo"), this );
   undoAction->setShortcut(tr("Ctrl+Z"));
   undoAction->setStatusTip(tr("Undo"));
   connect( undoAction, SIGNAL( triggered() ) , this, SLOT( undo() ) );
   
   // redo ========================
   redoAction = new QAction(  QIcon(), tr("&Redo"), this );
   redoAction->setShortcut(tr("Ctrl+Y"));
   redoAction->setStatusTip(tr("redo"));
   connect( redoAction, SIGNAL( triggered() ) , this, SLOT( redo() ) );

   
  // run ========================
   runAction = new QAction(  QIcon(), tr("&Run"), this );
   runAction->setShortcut(tr("Ctrl+R"));
   runAction->setStatusTip(tr("Run the current objects"));
   connect( runAction, SIGNAL( triggered() ) , this, SLOT( run() ) );
 
   // quit ========================
   quitAction = new QAction(  QIcon(), tr("&Quit"), this );
   quitAction->setShortcut(tr("Ctrl+Q"));
   quitAction->setStatusTip(tr("Close this window"));
   connect( quitAction, SIGNAL( triggered() ) , this, SLOT( quit() ) );


   actionmenu = new QMenu(tr("Action"),this);
   actionmenu->addAction(undoAction);
   actionmenu->addAction(redoAction);
   actionmenu->addSeparator();
   actionmenu->addAction(runAction);
   actionmenu->addSeparator();
   actionmenu->addAction(quitAction);
   
   // Interpolation: --------------------------
   

   // Interpolation: None ========================
   interpolationCopyAction = new QAction(  QIcon(), tr("&Copy"), this );
   interpolationCopyAction->setShortcut(tr("F10"));
   interpolationCopyAction->setStatusTip(tr("Copy from the parent object "));
   interpolationCopyAction->setCheckable(true);
   connect( interpolationCopyAction , SIGNAL( triggered() ) , this, SLOT( setMethodCopy() ) );

   
   // Interpolation: Linear========================
   interpolationLinearAction = new QAction(  QIcon(), tr("&Linear"), this );
   interpolationLinearAction->setShortcut(tr("F11"));
   interpolationLinearAction->setStatusTip(tr("Linear Interpolation"));
   interpolationLinearAction->setCheckable(true);
   connect( interpolationLinearAction, SIGNAL( triggered() ) , this, SLOT( setMethodLinear() ) );
   
   
   // Interpolation: Gauss ========================
   interpolationGaussAction = new QAction(  QIcon(), tr("&Gauss"), this );
   interpolationGaussAction->setShortcut(tr("F12"));
   interpolationGaussAction->setStatusTip(tr("Interpolation by Gauss method"));
   interpolationGaussAction->setCheckable(true);
   connect( interpolationGaussAction, SIGNAL( triggered() ) , this, SLOT( setMethodGauss() ) );


   // Interpolation: ResetLine ========================
   interpolationLineResetAction = new QAction(  QIcon(), tr("&Reset Line"), this );
   interpolationLineResetAction->setShortcut(tr("F9"));
   interpolationLineResetAction->setStatusTip(tr("Reset from the parent to the choosen one "));
   interpolationLineResetAction->setCheckable(true);
   connect( interpolationLineResetAction , SIGNAL( triggered() ) , this, SLOT( setMethodLineReset() ) );

   // Interpolation: None ========================
   interpolationSingleResetAction = new QAction(  QIcon(), tr("Reset &Single"), this );
   interpolationSingleResetAction->setShortcut(tr("F8"));
   interpolationSingleResetAction->setStatusTip(tr("Reset single column "));
   interpolationSingleResetAction->setCheckable(true);
   connect( interpolationSingleResetAction , SIGNAL( triggered() ) , this, SLOT( setMethodSingleReset() ) );


   methodGroup = new QActionGroup(this);
   methodGroup->addAction(interpolationCopyAction);
   methodGroup->addAction(interpolationLinearAction);
   methodGroup->addAction(interpolationGaussAction);
   methodGroup->addAction(interpolationLineResetAction);
   methodGroup->addAction(interpolationSingleResetAction);

   interpolationCopyAction->setChecked(true);

   methodmenu = new QMenu(tr("Method"),this);
   methodmenu->addAction(interpolationCopyAction);
   methodmenu->addAction(interpolationLinearAction);
   methodmenu->addAction(interpolationGaussAction);
   methodmenu->addSeparator();
   methodmenu->addAction(interpolationLineResetAction);
   methodmenu->addAction(interpolationSingleResetAction);


   QToolBar * toolbar=new QToolBar(this);

   toolbar->addAction(interpolationCopyAction);
   toolbar->addAction(interpolationLinearAction);
   toolbar->addAction(interpolationGaussAction);
   toolbar->addSeparator();
   toolbar->addAction(interpolationLineResetAction);
   toolbar->addAction(interpolationSingleResetAction);
   toolbar->addSeparator();

   // Parameter Buttons - from data ================
   
   parameterSignalMapper = new QSignalMapper(this);
   parametermenu         = new QMenu(tr("Parameters"),this);

   if(!obj.empty()) {
     map<miString,float>::iterator itr=obj[0].parameters.begin();
     for(;itr!=obj[0].parameters.end();itr++) {
       QString s = itr->first.cStr();
       QAction * act = new QAction(s, this);
       act->setCheckable(true);
       act->setChecked(true);
       connect(act,SIGNAL(triggered()),parameterSignalMapper,SLOT(map()));
       parameterSignalMapper->setMapping(act,s);

       parametermenu->addAction(act);
       toolbar->addAction(act);
     }
   }
   connect(parameterSignalMapper,SIGNAL(mapped(const QString&)),
       this,SLOT(toggleParameters(const QString&)));


   // build menus: -------------------
   
   menuBar()->addMenu(actionmenu);
   menuBar()->addMenu(methodmenu);
   menuBar()->addMenu(parametermenu);

   addToolBar(Qt::TopToolBarArea,toolbar);

   // scrollarea: --------------------

   scrolla = new QScrollArea(this);

   control = new ProfetTimeControl(scrolla->viewport(),obj,tim);

   scrolla->setWidget(control);



   setCentralWidget(scrolla);  

   scrolla->ensureWidgetVisible(control->parentObject(),0,0);

}

void ProfetTimeSmoothDialog::toggleParameters(const QString& pname)
{
  miString p =pname.toStdString();
  control->toggleParameters(p);  
}

void ProfetTimeSmoothDialog::warn(miString w)
{
  statusBar()->showMessage(w.cStr(),6000);
}


void ProfetTimeSmoothDialog::undo()
{
  if(!control->undo())
    warn("Nothing to undo...");    
}

void ProfetTimeSmoothDialog::redo()
{
  if(!control->redo())
    warn("Nothing to redo...");    
}


void ProfetTimeSmoothDialog::run()
{
  vector<fetObject::TimeValues>  d = control->collect(true);
  emit runObjects(d);  
  control->setChanged(false);
}

void ProfetTimeSmoothDialog::quit()
{
  if(control->hasChanged()) {
    int ret= QMessageBox::warning(this, tr("timesmooth"),
        tr("There are changed and unprocessed objects.\n"
            "Do you want to run your changes first?"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
            QMessageBox::Yes);
      if(ret==QMessageBox::Cancel)
        return;
      if(ret==QMessageBox::Yes)
        run(); 
  }
  close();
}

void ProfetTimeSmoothDialog::setMethodLinear()
{
  control->setMethod(ProfetTimeControl::LINEAR);
}

void ProfetTimeSmoothDialog::setMethodCopy()
{
  control->setMethod(ProfetTimeControl::COPY);
}

void ProfetTimeSmoothDialog::setMethodGauss()
{
  control->setMethod(ProfetTimeControl::GAUSS);
}


void ProfetTimeSmoothDialog::setMethodSingleReset()
{
  control->setMethod(ProfetTimeControl::RESETSINGLE);
}

void ProfetTimeSmoothDialog::setMethodLineReset()
{
  control->setMethod(ProfetTimeControl::RESETLINE);
}
  
void ProfetTimeSmoothDialog::processed(miTime tim, miString obj_id)
{
  control->processed(tim,obj_id);  
}





