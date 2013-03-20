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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtProfetTimeSmoothDialog.h"
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>
#include <QPixmap>
#include <linear_copy.xpm>
#include <linear_down.xpm>
#include <linear_remove.xpm>
#include <single_remove.xpm>
#include <run_smooth.xpm>
#include <undo.xpm>
#include <exit.xpm>
#include <redo.xpm>





ProfetTimeSmoothDialog::ProfetTimeSmoothDialog(QWidget* parent, vector<fetObject::TimeValues>& obj, vector<miutil::miTime>& tim) 
: QMainWindow(parent)
{
  
    setAttribute(Qt::WA_DeleteOnClose); 
  
  
  // actions: --------------------------

  // undo ========================
   undoAction = new QAction(  QPixmap(undo_xpm), tr("&Undo"), this );
   undoAction->setShortcut(tr("Ctrl+Z"));
   undoAction->setStatusTip(tr("Undo"));
   connect( undoAction, SIGNAL( triggered() ) , this, SLOT( undo() ) );
   
   // redo ========================
   redoAction = new QAction(  QPixmap(redo_xpm), tr("&Redo"), this );
   redoAction->setShortcut(tr("Ctrl+Y"));
   redoAction->setStatusTip(tr("redo"));
   connect( redoAction, SIGNAL( triggered() ) , this, SLOT( redo() ) );

   
  // run ========================
   runAction = new QAction(  QPixmap(run_smooth_xpm), tr("&Run"), this );
   runAction->setShortcut(tr("Ctrl+R"));
   runAction->setStatusTip(tr("Run the current objects"));
   connect( runAction, SIGNAL( triggered() ) , this, SLOT( run() ) );
 
   // quit ========================
   quitAction = new QAction(  QPixmap(exit_xpm), tr("&Quit"), this );
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
   interpolationCopyAction = new QAction(  QPixmap(linear_copy_xpm), tr("&Copy"), this );
   interpolationCopyAction->setShortcut(tr("F10"));
   interpolationCopyAction->setStatusTip(tr("Copy from the parent object "));
   interpolationCopyAction->setCheckable(true);
   connect( interpolationCopyAction , SIGNAL( triggered() ) , this, SLOT( setMethodCopy() ) );

   
   // Interpolation: Linear========================
   interpolationLinearAction = new QAction(  QPixmap(linear_down_xpm), tr("&Linear"), this );
   interpolationLinearAction->setShortcut(tr("F11"));
   interpolationLinearAction->setStatusTip(tr("Linear Interpolation"));
   interpolationLinearAction->setCheckable(true);
   connect( interpolationLinearAction, SIGNAL( triggered() ) , this, SLOT( setMethodLinear() ) );
   
   // Interpolation: ResetLine ========================
   interpolationLineResetAction = new QAction(  QPixmap(linear_remove_xpm), tr("&Reset Line"), this );
   interpolationLineResetAction->setShortcut(tr("F9"));
   interpolationLineResetAction->setStatusTip(tr("Reset from the parent to the choosen one "));
   interpolationLineResetAction->setCheckable(true);
   connect( interpolationLineResetAction , SIGNAL( triggered() ) , this, SLOT( setMethodLineReset() ) );

   // Interpolation: None ========================
   interpolationSingleResetAction = new QAction(  QPixmap(single_remove_xpm), tr("Reset &Single"), this );
   interpolationSingleResetAction->setShortcut(tr("F8"));
   interpolationSingleResetAction->setStatusTip(tr("Reset single column "));
   interpolationSingleResetAction->setCheckable(true);
   connect( interpolationSingleResetAction , SIGNAL( triggered() ) , this, SLOT( setMethodSingleReset() ) );


   methodGroup = new QActionGroup(this);
   methodGroup->addAction(interpolationCopyAction);
   methodGroup->addAction(interpolationLinearAction);
   methodGroup->addAction(interpolationLineResetAction);
   methodGroup->addAction(interpolationSingleResetAction);

   interpolationLinearAction->setChecked(true);

   methodmenu = new QMenu(tr("Method"),this);
   methodmenu->addAction(interpolationCopyAction);
   methodmenu->addAction(interpolationLinearAction);
   methodmenu->addSeparator();
   methodmenu->addAction(interpolationLineResetAction);
   methodmenu->addAction(interpolationSingleResetAction);


   QToolBar * toolbar=new QToolBar(this);

   
   toolbar->addAction(quitAction);
   toolbar->addAction(runAction);
   toolbar->addAction(undoAction);
   toolbar->addAction(redoAction);
   toolbar->addSeparator();
   toolbar->addAction(interpolationCopyAction  );
   toolbar->addAction(interpolationLinearAction);
   toolbar->addSeparator();
   toolbar->addAction(interpolationLineResetAction  );
   toolbar->addAction(interpolationSingleResetAction);
   toolbar->addSeparator();

   // Parameter Buttons - from data ================
   
   parameterSignalMapper = new QSignalMapper(this);
   parametermenu         = new QMenu(tr("Parameters"),this);

   if(!obj.empty()) {
     map<miutil::miString,float>::iterator itr=obj[0].parameters.begin();
     for(;itr!=obj[0].parameters.end();itr++) {
       QString s = itr->first.c_str();
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
   
   int h=control->height()+menuBar()->height()+toolbar->height()+20;
   resize(800,h);
   


   setCentralWidget(scrolla);  

   scrolla->ensureWidgetVisible(control->focusObject(),0,0);

   
   setWindowModality(Qt::ApplicationModal);
   show();
   
}

void ProfetTimeSmoothDialog::closeEvent(QCloseEvent * e)
{
  vector<fetObject::TimeValues>  d = control->collect(false);
  emit endTimesmooth(d);  
  
}


void ProfetTimeSmoothDialog::toggleParameters(const QString& pname)
{
  miutil::miString p =pname.toStdString();
  control->toggleParameters(p);  
}

void ProfetTimeSmoothDialog::warn(miutil::miString w)
{
  statusBar()->showMessage(w.c_str(),6000);
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
  
void ProfetTimeSmoothDialog::processed(miutil::miTime tim, miutil::miString obj_id)
{
  control->processed(tim,obj_id);  
}





