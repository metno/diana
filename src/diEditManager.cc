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
//#define DEBUGREDRAW

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>

#include <diEditManager.h>
#include <diPlotModule.h>
#include <diObjectManager.h>
#include <diFieldPlotManager.h>
#include <diWeatherFront.h>
#include <diWeatherSymbol.h>
#include <diWeatherArea.h>
#include <diMapMode.h>
#include <diFieldPlot.h>
#include <diUndoFront.h>
#include <diFieldEdit.h>
#include <diAnnotationPlot.h>
#include "diUtilities.h"
#include <puTools/miDirtools.h>
#include <puTools/miSetupParser.h>

#include <iomanip>
#include <set>
#include <cmath>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QFile>
#include <QDir>

#define MILOGGER_CATEGORY "diana.EditManager"
#include <miLogger/miLogging.h>

//#define DEBUGPRINT
using namespace miutil;
using namespace std;

EditManager::EditManager(PlotModule* pm, ObjectManager* om, FieldPlotManager* fm)
: plotm(pm), objm(om), fieldPlotManager(fm), mapmode(normal_mode), edittool(0), editpause(false),
combinematrix(0),numregs(0), hiddenObjects(false),
hiddenCombining(false), hiddenCombineObjects(false), showRegion(-1)
, apEditmessage(0)
, inEdit(false)
, producttimedefined(false)
{
  if (plotm==0 || objm==0){
    METLIBS_LOG_WARN("Catastrophic error: plotm or objm == 0");
  }

  initEditTools();
  ObjectPlot::defineTranslations();
  unsentProduct = false;
  moved=false;

}

EditManager::~EditManager()
{
  delete apEditmessage;
  apEditmessage = 0;
}

bool EditManager::parseSetup()
{
  METLIBS_LOG_SCOPE();

  std::string section="EDIT";
  vector<std::string> vstr;

  if (!SetupParser::getSection(section,vstr)){
    METLIBS_LOG_ERROR("No " << section << " section in setupfile, ok.");
    return true;
  }

  int i,n,nval, nv=0, nvstr=vstr.size();
  std::string key,error;
  vector<std::string> values, vsub;
  bool ok= true;

  while (ok && nv<nvstr) {

    SetupParser::splitKeyValue(vstr[nv],key,values);
    nval= values.size();

    // yet only products in this setup section...
    EditProduct ep;
    ep.areaminimize= false;
    ep.standardSymbolSize=60;
    ep.complexSymbolSize=6;
    ep.frontLineWidth=8;
    ep.areaLineWidth=4;
    ep.startEarly= false;
    ep.startLate=  false;
    ep.minutesStartEarly= 0;
    ep.minutesStartLate=  0;
    ep.combineBorders="./ANAborders.";  // default as old version
    ep.winX=  0;
    ep.winY=  0;
    ep.autoremove= -1;

    if (key=="product" && nval==1) {
      ep.name= values[0];
      nv++;
    } else {
      ok= false;
    }

    while (ok && nv<nvstr) {

      SetupParser::splitKeyValue(vstr[nv],key,values);
      nval= values.size();
      if (key=="end.product") {
        nv++;
        break;
      } else if (key=="drawtools"){
        for (int j=0; j<nval; j++)
          ep.drawtools.push_back(values[j]);
      } else if (nval==0) {
        // keywords without any values
        if (key=="grid.minimize")
          ep.areaminimize= true;
        else
          ok= false;
      } else if (key=="local_save_dir" && nval==1) {
        ep.local_savedir = values[0];

      } else if (key=="prod_save_dir" && nval==1) {
        ep.prod_savedir = values[0];

      } else if (key=="input_dir") {
        ep.inputdirs.insert(ep.inputdirs.end(),
            values.begin(),values.end());

      } else if (key=="input_field_format") {
        ep.inputFieldFormat = values[0];

      } else if (key=="input_field_config") {
        ep.inputFieldConfig = values[0];

      } else if (key=="combine_input_dir") {
        ep.combinedirs.insert(ep.combinedirs.end(),
            values.begin(),values.end());

      } else if (key=="combine_borders") {
        ep.combineBorders= values[0];

      } else if (key=="field") {
        EditProductField epf;
        epf.fromfield= false;
        epf.minValue= fieldUndef;
        epf.maxValue= fieldUndef;
        epf.editTools.push_back("standard");
        for (int j=0; j<nval; j++) {
          vsub= miutil::split(values[j], 0, ":");
          if (vsub.size()==2) {
            if (vsub[0]=="filenamepart") {
              epf.filenamePart= vsub[1];
            } else if (vsub[0]=="plot") {
              epf.name= vsub[1];
            } else if (vsub[0]=="min") {
              epf.minValue= atof(vsub[1].c_str());
            } else if (vsub[0]=="max") {
              epf.maxValue= atof(vsub[1].c_str());
            } else if (vsub[0]=="tool") {
              epf.editTools= miutil::split(miutil::to_lower(vsub[1]), "+", true);
            } else if (vsub[0]=="vcoord") {
              epf.vcoord_cdm= vsub[1];
            } else if (vsub[0]=="vlevel") {
              epf.vlevel_cdm= vsub[1];
            } else if (vsub[0]=="unit") {
              epf.unit_cdm= vsub[1];
            }
          }
        }
        ep.fields.push_back(epf);

      } else if (key=="objects" && nval==1) {
        ep.objectsFilenamePart= values[0];
        //....................................... + typer ??????

      } else if (key=="comments" && nval==1) {
        ep.commentFilenamePart= values[0];

      } else if (key=="local_idents") {
        for (i=0; i<nval; i++) {
          EditProductId pid;
          pid.name= values[i];
          pid.sendable= false;
          pid.combinable= false;
          ep.pids.push_back(pid);
        }

      } else if (key=="prod_idents") {
        for (i=0; i<nval; i++) {
          EditProductId pid;
          pid.name= values[i];
          pid.sendable= true;
          pid.combinable= false;
          ep.pids.push_back(pid);
        }

      } else if (key=="combine_ident" && nval>=3) {
        n= ep.pids.size();
        i= 0;
        while (i<n && ep.pids[i].name!=values[0]) i++;
        if (i<n) {
          ep.pids[i].combinable= true;
          ep.pids[i].combineids.clear();
          for (int j=1; j<nval; j++)
            ep.pids[i].combineids.push_back(values[j]);
        } else {
          ok= false;
        }

      } else if (key=="commandfile" && nval==1) {
        ep.commandFilename= values[0];
      } else if (key=="standard_symbolsize" && nval==1){
        ep.standardSymbolSize=atoi(values[0].c_str());
      } else if (key=="complex_symbolsize" && nval==1){
        ep.complexSymbolSize=atoi(values[0].c_str());
      } else if (key=="frontlinewidth" && nval==1){
        ep.frontLineWidth = atoi(values[0].c_str());
      } else if (key=="arealinewidth" && nval==1){
        ep.areaLineWidth = atoi(values[0].c_str());
      } else if (key=="window_x" && nval==1){
        ep.winX = atoi(values[0].c_str());
      } else if (key=="window_y" && nval==1){
        ep.winY = atoi(values[0].c_str());
      } else if (key=="time_start_early" && nval==1){
        vector<std::string> vs= miutil::split(values[0], 0, ":", true);
        if (vs.size()==2) {
          int hr= atoi(vs[0].c_str());
          int mn= atoi(vs[1].c_str());
          if (hr<0 && mn>0) mn= -mn;
          ep.minutesStartEarly= hr*60 + mn;
          ep.startEarly= true;
          if (ep.startLate && ep.minutesStartEarly>=ep.minutesStartLate)
            ok= false;
        } else {
          ok= false;
        }
      } else if (key=="time_start_late" && nval==1){
        vector<std::string> vs= miutil::split(values[0], 0, ":", true);
        if (vs.size()==2) {
          int hr= atoi(vs[0].c_str());
          int mn= atoi(vs[1].c_str());
          if (hr<0 && mn>0) mn= -mn;
          ep.minutesStartLate= hr*60 + mn;
          ep.startLate= true;
          if (ep.startEarly && ep.minutesStartEarly>=ep.minutesStartLate)
            ok= false;
        } else {
          ok= false;
        }
      } else if (key=="template_file" && nval==1 ){
        ep.templateFilename = values[0];
      } else if (key=="autoremove" && nval==1 ){
        ep.autoremove = atoi(values[0].c_str());
      } else {
        ok= false;
      }

      if (ok) nv++;
    }

    if (ok) {
      //
      if (ep.local_savedir.empty()) ep.local_savedir= ".";
      // insert savedir as the last inputdir and the last combinedir,
      // this sequence is also kept when timesorting
      ep.inputdirs.push_back(ep.local_savedir);
      ep.combinedirs.push_back(ep.local_savedir);
      //HK !!! important ! default drawtools if not specified in setup
      if (ep.drawtools.empty()) {
        ep.drawtools.push_back(OBJECTS_ANALYSIS);
        ep.drawtools.push_back(OBJECTS_SIGMAPS);
      }
      //read commands(OKstrings) from commandfile
      if (not ep.commandFilename.empty())
        readCommandFile(ep);
      // find duplicate

      unsigned int q;
      for ( q=0; q<editproducts.size(); q++ )
        if ( editproducts[q].name == ep.name )
          break;
      if ( q != editproducts.size() )
        editproducts[q] = ep;
      else
        editproducts.push_back(ep);
    }
  }

  if (!ok) {
    error="Error in edit product definition";
    SetupParser::errorMsg(section,nv,error);
    return true;
  }

  return true;
}



void EditManager::readCommandFile(EditProduct & ep)
{
  METLIBS_LOG_DEBUG("++ EditManager::readCommandFile");
  // the commands OKstrings to be exectuted when we start an
  // edit session, for the time being called from parseSeup
  // and the OKstrings stored for each product
  std::string s;
  bool merge= false, newmerge;
  int n,linenum=0;
  vector<std::string> tmplines;

  // open filestream
  ifstream file(ep.commandFilename.c_str());
  if (!file){
    METLIBS_LOG_ERROR("ERROR OPEN (READ) " << ep.commandFilename);
    return;
  }
  while (getline(file,s)){
    linenum++;
    n= s.length();
    if (n>0) {
      newmerge= false;
      if (s[n-1] == '\\'){
        newmerge= true;
        s= s.substr(0,s.length()-1);
      }
      if (merge)
        tmplines[tmplines.size()-1]+= s;
      else
        tmplines.push_back(s);
      merge= newmerge;
    }
  }
  //split up in LABEL and OTHER info...
  vector <std::string> labcom,commands;
  n=tmplines.size();
  for (int i=0; i<n; i++){
    std::string s= tmplines[i];
    miutil::trim(s);
    if (s.empty())
      continue;
    vector<std::string> vs= miutil::split(s, " ");
    std::string pre= miutil::to_upper(vs[0]);
    if (pre=="LABEL")
      labcom.push_back(s);
    else
      commands.push_back(s);
  }
  ep.labels = labcom;
  METLIBS_LOG_DEBUG("++ EditManager::readCommandFile start reading --------");
  for (size_t ari=0; ari<ep.labels.size(); ari++)
       METLIBS_LOG_DEBUG("   " << ep.labels[ari ] << "  ");
  METLIBS_LOG_DEBUG("++ EditManager::readCommandFile finish reading ------------");

  ep.OKstrings = commands;
}

/*----------------------------------------------------------------------
----------------------------  edit Dialog methods ----------------------
 -----------------------------------------------------------------------*/


EditDialogInfo EditManager::getEditDialogInfo(){

  //info about edit objects to dialog

  EditDialogInfo EditDI;
  EditDI.mapmodeinfo=mapmodeinfo;

  return EditDI;
}


// set and get mapmode, editmode and edittool
void EditManager::setEditMode(const std::string mmode,  // mapmode
    const std::string emode,  // editmode
    const std::string etool){ // edittool

  const mapMode oldMapMode = mapmode;
  if (mmode=="fedit_mode")
    mapmode= fedit_mode;
  else if (mmode=="draw_mode")
    mapmode= draw_mode;
  else if (mmode=="combine_mode")
    mapmode= combine_mode;
  else if (mmode=="normal_mode")
    mapmode= normal_mode;
  else {
    METLIBS_LOG_ERROR("diEditManager::setEditMode  unknown mapmode:" << mmode);
    return;
  }

  bool modeChange= (oldMapMode!=mapmode);

  if (mapmode==normal_mode){
    editmode= edittool= 0;
    inEdit = false;
    return;
  } else {
    inEdit = true;
  }

  int n= mapmodeinfo.size();
  int mmidx=0;
  while (mmidx<n && mmode!=mapmodeinfo[mmidx].mapmode) mmidx++;
  if (mmidx==n){
    METLIBS_LOG_ERROR("diEditManager::setEditMode  no info for mapmode:"
    << mmode);
    editmode=edittool=0;
    return;
  }

  n= mapmodeinfo[mmidx].editmodeinfo.size();
  if (n==0){ // no defined modes or tools for this mapmode
    editmode=edittool=0;
    return;
  }
  int emidx=0;
  while (emidx<n &&
      emode!=mapmodeinfo[mmidx].editmodeinfo[emidx].editmode) emidx++;
  if (emidx==n){
    METLIBS_LOG_ERROR("diEditManager::setEditMode  unknown editmode:" << emode);
    editmode=edittool=0;
    return;
  }
  editmode= emidx;

  if (modeChange) showAllObjects();

  objm->setEditMode(mapmode, editmode, etool);
}


mapMode EditManager::getMapMode()
{
  return mapmode;
}

/*----------------------------------------------------------------------
----------------------------  end of edit Dialog methods ----------------
 -----------------------------------------------------------------------*/


/*----------------------------------------------------------------------
----------------------------  keyboard/mouse event ----------------------
 -----------------------------------------------------------------------*/

void EditManager::sendMouseEvent(QMouseEvent* me, EventResult& res)
{
  //  METLIBS_LOG_DEBUG("EditManager::sendMouseEvent");
  res.savebackground= true;
  res.background= false;
  res.repaint= false;
  res.newcursor= edit_cursor;

  if (showRegion>=0 && mapmode!=combine_mode)
    return;

  plotm->PhysToMap(me->x(),me->y(),newx,newy);
  objm->getEditObjects().setMouseCoordinates(newx,newy);

  if (mapmode== fedit_mode){

    //field editing
    if (me->type() == QEvent::MouseButtonPress){
      if (me->button() == Qt::LeftButton){         // LEFT MOUSE-BUTTON
        EditEvent ee;                     // send an editevent
        ee.type= edit_pos;                // ..type edit_pos
        ee.order= start_event;
        ee.x= newx;
        ee.y= newy;
        res.repaint= notifyEditEvent(ee);
      } else if (me->button() == Qt::MidButton){
        EditEvent ee;                     // send an edit-event
        ee.type= edit_inspection;         // ..type edit_inspection
        ee.order= start_event;
        ee.x= newx;
        ee.y= newy;
        res.repaint= notifyEditEvent(ee);
      } else if (me->button() == Qt::RightButton){ // RIGHT MOUSE-BUTTON
        EditEvent ee;                     // send an editevent
        ee.type= edit_size;               // ..type edit_size
        ee.order= start_event;
        ee.x= newx;
        ee.y= newy;
        res.repaint= notifyEditEvent(ee);
      }
    } else if (me->type() == QEvent::MouseMove){
      res.action = quick_browsing;
      if (me->buttons() == Qt::NoButton){
        //HK ??? edittool=0, alltid
        if (edittool != 0)                // ..just set correct cursor
          res.newcursor= edit_move_cursor;
        else
          res.newcursor= edit_value_cursor;
        res.action = browsing;
      } else if (me->buttons() & Qt::LeftButton){  // LEFT MOUSE-BUTTON
        EditEvent ee;                     // send an edit-event
        ee.type= edit_pos;                // ...type edit_pos
        ee.order= normal_event;
        ee.x= newx;
        ee.y= newy;
        res.repaint= notifyEditEvent(ee);
      } else if (me->buttons() & Qt::MidButton){
        EditEvent ee;                     // send an edit-event
        ee.type= edit_inspection;         // ..type edit_inspection
        ee.order= normal_event;
        ee.x= newx;
        ee.y= newy;
        res.repaint= notifyEditEvent(ee);
      } else if (me->buttons() & Qt::RightButton){ // RIGHT MOUSE-BUTTON
        EditEvent ee;                     // send an edit-event
        ee.type= edit_size;               // ..type edit_size
        ee.order= normal_event;
        ee.x= newx;
        ee.y= newy;
        res.repaint= notifyEditEvent(ee);
      }
    }
    else if (me->type() == QEvent::MouseButtonRelease){
      if (me->button() == Qt::LeftButton){         // LEFT MOUSE-BUTTON
        EditEvent ee;                     // send an edit-event
        ee.type= edit_pos;                // ..type edit_pos
        ee.order= stop_event;
        ee.x= newx;
        ee.y= newy;
        res.repaint= notifyEditEvent(ee);
        if (res.repaint)
          res.action = fields_changed;
      }
    } else if (me->type() == QEvent::MouseButtonDblClick){
      if (me->button() == Qt::LeftButton) {
        EditEvent ee;                     // send an editevent
        ee.type= edit_pos;                // ..type edit_pos
        ee.order= start_event;
        ee.x= newx;
        ee.y= newy;
        res.repaint= notifyEditEvent(ee);
      }
    }

  } else {

    //draw_mode or combine mode
    if (me->type() == QEvent::MouseButtonPress){
      if (me->button() == Qt::LeftButton){         // LEFT MOUSE-BUTTON
        if (objm->inDrawing()){
          // add point to active objects
          objm->editAddPoint(newx,newy);
          if (objm->toDoCombine()) editCombine();
          res.action = objects_changed;
          res.repaint= true;
        } else {
          objm->editPrepareChange(MoveMarkedPoints);
          first_x= newx;                    // remember current position
          first_y= newy;
          moved = false;
        }
      } else if (me->button() == Qt::MidButton){         // MIDDLE MOUSE-BUTTON
        objm->editPrepareChange(RotateLine);
        first_x= newx;
        first_y= newy;
        moved = false;
      } else if (me->button() == Qt::RightButton){ // RIGHT MOUSE-BUTTON
        objm->editStopDrawing();
        res.newcursor= edit_cursor;
        res.repaint= true;
      }
    } else if (me->type() == QEvent::MouseMove){
      //METLIBS_LOG_DEBUG("mousemove ");
      res.action = quick_browsing;
      if (me->buttons() == Qt::NoButton){
        if (objm->inDrawing()){
          if (objm->setRubber(true,newx,newy))
            res.repaint= true;
          res.newcursor= draw_cursor;
        } else
          if (objm->editCheckPosition(newx,newy))
            res.repaint= true;
        res.action = browsing;
      } else if (me->buttons() & Qt::LeftButton){  // LEFT MOUSE-BUTTON
        if (!objm->inDrawing()) { // move marked points
          moved = objm->editMoveMarkedPoints(newx-first_x,newy-first_y);
          first_x= newx;
          first_y= newy;
          if (moved){
            res.repaint= true;
            if (mapmode==combine_mode)
              editCombine();
          }
        }
      } else if (me->buttons() & Qt::MidButton){  // MIDDLE MOUSE-BUTTON
        if (!objm->inDrawing()) {  // rotate "line"
          moved = objm->editRotateLine(newx-first_x,newy-first_y);
          first_x= newx;
          first_y= newy;
          if (moved){
            res.repaint= true;
            if (mapmode==combine_mode)
              editCombine();
          }
        }
      }
    } else if (me->type() == QEvent::MouseButtonRelease){
      if (me->button() == Qt::LeftButton || me->button() == Qt::MidButton){
        if (!objm->inDrawing())
          objm->editMouseRelease(moved);
        if(mapmode==combine_mode){
          objm->setAllPassive();
          if (objm->toDoCombine())
            editCombine();
        }
        if (moved) {
          res.action= objects_changed;
          res.repaint= true;
          moved = false;
        }
      }
    } else if (me->type() == QEvent::MouseButtonDblClick){
      objm->editStopDrawing();
      res.newcursor= edit_cursor;
      res.repaint= true;
    }
  }

//  METLIBS_LOG_DEBUG("EditManager::sendMouseEvent return res.repaint= "<<res.repaint);
}


void EditManager::sendKeyboardEvent(QKeyEvent* ke, EventResult& res)
{
//  METLIBS_LOG_DEBUG("EditManager::sendKeyboardEvent");
  res.savebackground= true;
  res.background= false;
  res.repaint= false;

  // numregs==0 if not combine (and then edit)
  int keyRegMin= Qt::Key_1;
  int keyRegMax= (numregs<10) ? Qt::Key_0 + numregs : Qt::Key_9;

  if (showRegion>=0 && mapmode!=combine_mode) {
    if (ke->type() != QEvent::KeyPress || ke->key() < keyRegMin || ke->key() > keyRegMax)
      return;
  }

  if (ke->type() == QEvent::KeyPress){
    if (ke->key() == Qt::Key_Shift) {
      res.newcursor= normal_cursor; // show user possible mode-change
      return;
    } else if (ke->key() == Qt::Key_G) {
      //hide/show all objects.
      if (hiddenObjects || hiddenCombineObjects) {
        objm->editUnHideAll();
        hiddenObjects= false;
      } else {
        objm->editHideAll();
        hiddenObjects= true;
      }
      hiddenCombineObjects= false;
      res.background= (mapmode==fedit_mode);
      setEditMessage("");
    }
    else if (ke->key() == Qt::Key_L) {
      //hide/show all combining objects.
      if (hiddenCombining)
        objm->editUnHideCombining();
      else
        objm->editHideCombining();
      hiddenCombining= !hiddenCombining;
      res.background= true;
    }
    else if (ke->key() >= keyRegMin && ke->key() <= keyRegMax){
      // show objects and fields from one region (during and after combine)
      std::string msg;
      int reg= ke->key() - keyRegMin;
      if (reg!=showRegion) {
        showRegion= reg;
        msg=regnames[showRegion];
      } else {
        showRegion= -1;
      }
      res.background= true;
      setEditMessage(msg);
    }
    res.repaint= true;
  }

  if (mapmode!= fedit_mode){
    // first keypress events
    if (ke->type() == QEvent::KeyPress){
      if (ke->key() == Qt::Key_N){
        objm->createNewObject();
        res.newcursor= draw_cursor;
      }
      else if (ke->key() == Qt::Key_Delete){
        objm->editDeleteMarkedPoints();
      }
      else if (ke->key() == Qt::Key_P){
        objm->editStopDrawing();
        res.newcursor= edit_cursor;
      }
      else if (ke->key() == Qt::Key_V && ke->modifiers() & Qt::ControlModifier)
        objm->editPasteObjects();
      else if (ke->key() == Qt::Key_C && ke->modifiers() & Qt::ControlModifier)
        objm->editCopyObjects();
      else if (ke->key() == Qt::Key_V){
        objm->editFlipObjects();
      }
      else if (ke->key() == Qt::Key_U)
        objm->editUnmarkAllPoints();
      else if (ke->key()==Qt::Key_Plus && ke->modifiers() & Qt::ControlModifier){
        objm->editRotateObjects(+10.0);
      }
      else if (ke->key()==Qt::Key_Minus && ke->modifiers() & Qt::ControlModifier){
        objm->editRotateObjects(-10.0);
      }
      else if (ke->key() == Qt::Key_Plus){
        objm->editIncreaseSize(1);
      }
      else if (ke->key() == Qt::Key_Minus){
        objm->editIncreaseSize(-1);
      }
      else if (ke->key() == Qt::Key_O){
        objm->editHideBox();
      }
      else if (ke->key() == Qt::Key_T){
        if (ke->modifiers() & Qt::ShiftModifier){
          objm->editChangeObjectType(-1);
        }
        else{
          objm->editChangeObjectType(1);
        }
      } else if (ke->key() == Qt::Key_F){
        // resume drawing of marked front
        objm->editResumeDrawing(newx,newy);
      }
      else if (ke->key() == Qt::Key_K){
        // split front
        objm->editSplitFront(newx,newy);
      }
      else if (ke->key()==Qt::Key_J){
        //join marked fronts...
        objm->editCommandJoinFronts(false,true,true);
      }
      else if (ke->key()==Qt::Key_M)
        //mark objects permanently...
        objm->editStayMarked();
      else if (ke->key()==Qt::Key_C)
        //unmark objects permanently...
        objm->editNotMarked();
      else if (ke->key()==Qt::Key_Q)
        //unjoin points...
        objm->editUnJoinPoints();
      else if (ke->key()==Qt::Key_E)
        //merge fronts...
        objm->editMergeFronts(false);
      else if (ke->key()==Qt::Key_B && ke->modifiers() & Qt::ShiftModifier)
        //set to default size...
        objm->editDefaultSizeAll();
      else if (ke->key()==Qt::Key_B)
        //set marked objects to default size...
        objm->editDefaultSize();
      else {
        return;
      }
    }
    res.repaint= true;
    if(mapmode==combine_mode){
      objm->setAllPassive();
      if (objm->toDoCombine())
        editCombine();
    } else { //OK?
      if (objm->haveObjectsChanged())
        res.action = objects_changed;
    }
  }


  // then key release events
  if (ke->type() == QEvent::KeyRelease){

    if (ke->key() == Qt::Key_Shift){ // reset cursor
      if (mapmode!=fedit_mode){
        res.newcursor= (objm->inDrawing()?draw_cursor:edit_cursor);
      } else {                            // field-editing
        if (edittool != 0)                // ..just set correct cursor
          res.newcursor= edit_move_cursor;
        else
          res.newcursor= edit_value_cursor;
      }
    }
  }
}


bool EditManager::notifyEditEvent(const EditEvent& ee){

  int nf= fedits.size();
  for (int i=0; i<nf; i++) {
    if (fedits[i]->activated())
      return fedits[i]->notifyEditEvent(ee);
  }

  // no active object, probably only setting static members
  if (nf>0) return fedits[0]->notifyEditEvent(ee);

  // no objects, probably only setting static members (not often)
  FieldEdit* fed= new FieldEdit( fieldPlotManager );
  bool res= fed->notifyEditEvent(ee);
  delete fed;

  return res;
}


void EditManager::activateField(int index) {
  int nf= fedits.size();
  for (int i=0; i<nf; i++)
    fedits[i]->deactivate();
  if (index>=0 && index<nf)
    fedits[index]->activate();
}



/*----------------------------------------------------------------------
------------------------ end keyboard/mouse event ----------------------
 -----------------------------------------------------------------------*/




/*----------------------------------------------------------------------
------------------------ various useful functions ---------------------
 -----------------------------------------------------------------------*/


bool EditManager::showAllObjects() {
  if (hiddenObjects || hiddenCombineObjects || showRegion>=0) {
    // show all objects
    objm->editUnHideAll();
    hiddenObjects= hiddenCombineObjects= false;
    showRegion= -1;
    setEditMessage("");
    return true;
  } else {
    return false;
  }
}


bool EditManager::unsavedEditChanges(){

  //returns true if objects or fields changed since last save
  bool editc= false;
  for (unsigned int i=0; i<fedits.size(); i++)
    if (fedits[i]->changedEditField()) editc= true;

  bool drawc = !objm->areObjectsSaved();

  bool commentc = !objm->getEditObjects().areCommentsSaved();

  bool labelc = !objm->getEditObjects().areLabelsSaved();



  return (editc || drawc || commentc || labelc);
}


bool EditManager::unsentEditChanges(){
  //returns true if objects or fields changed since last send
  return unsentProduct;
}




bool EditManager::getProductTime(miTime& t){
  //METLIBS_LOG_DEBUG("EditManager::getProductTime");
  //returns the current product time
  if (producttimedefined){
    t = producttime;
    return true;
  } else
    return false;
}


std::string EditManager::getProductName(){
  //METLIBS_LOG_DEBUG("EditManager::getProductName");
  //returns the current product time
  return EdProd.name;
}


void EditManager::saveProductLabels(vector <std::string> labels)
{
  objm->getEditObjects().saveEditLabels(labels);
}


std::string EditManager::editFileName(const std::string directory,
    const std::string region,
    const std::string name,
    const miTime& t){

  //constructs a filename

  int yyyy= t.year();
  int mm  = t.month();
  int dd  = t.day();
  int hh  = t.hour();
  int min = t.min();

  ostringstream ostr;
  ostr << setw(4) << setfill('0') << yyyy
  << setw(2) << setfill('0') << mm
  << setw(2) << setfill('0') << dd
  << setw(2) << setfill('0') << hh
  << setw(2) << setfill('0') << min;

  std::string filename;

  if (directory.length()>0)
    filename+= (directory + "/");
  if (region.length()>0)
    filename+= (region + "_");

  filename+= (name + "." + ostr.str());

  return filename;
}



/*----------------------------------------------------------------------
------------------------ end  various useful functions -----------------
 -----------------------------------------------------------------------*/






/*----------------------------------------------------------------------
functions to start and end editing
 -----------------------------------------------------------------------*/

bool EditManager::fileExists(const EditProduct& ep, const EditProductId& ci,
    const miutil::miTime& time, QString& message)
{
  METLIBS_LOG_SCOPE(LOGVAL(EdProd.name) <<LOGVAL(EdProdId.name));

  for (size_t j=0; j<ep.fields.size(); j++) {

    std::string outputFilename;

    std::string time_string;
    if ( !time.undef() ) {
      time_string = "_" + time.format("%Y%m%dt%H%M%S");
    }

    if (ci.sendable ) {
      outputFilename = ep.prod_savedir + "/work/";
      std::string filename = ci.name + "_" + ep.fields[j].filenamePart + time_string + ".nc";
      outputFilename += filename;
      QString qs(outputFilename.c_str());
      QFile qfile(qs);
      if ( qfile.exists() ) {
        message = qs + " already exists, do you want to continue?";
        return false;
      }
    }

    outputFilename = ep.local_savedir + "/";
    std::string filename = ci.name + "_" + ep.fields[j].filenamePart + time_string + ".nc";
    outputFilename += filename;
    QString qs(outputFilename.c_str());
    QFile qfile(qs);
    if ( qfile.exists() ) {
      message = qs + " already exists, do you want to continue?";
      return false;
    }

  }


  if (ci.sendable ) {
    std::string outputFilename = ep.prod_savedir + "/work/";
    std::string objectsFilename= editFileName(outputFilename,ci.name,
        ep.objectsFilenamePart,time);
    QString qstr(objectsFilename.c_str());
    QFile qfile(qstr);
    if ( qfile.exists() ) {
      message = qstr + " already exists, do you want to continue?";
      return false;
    }
  }
  std::string objectsFilename= editFileName(ep.local_savedir,ci.name,
      ep.objectsFilenamePart,time);
  QString qstr(objectsFilename.c_str());
  QFile qfile(qstr);
  if ( qfile.exists() ) {
    message = qstr + " already exists, do you want to continue?";
    return false;
  }

  return true;
}

//Read template file
bool EditManager::makeNewFile(int fnum, bool local, QString& message)
{
  METLIBS_LOG_SCOPE();

  QString templateFilename = EdProd.templateFilename.c_str();
  if ( !QFile::exists ( templateFilename ) ) {
    message = "Template file: " + templateFilename + " do not exist";
    return false;
  }
  QFile qfile(EdProd.templateFilename.c_str());

  std::string outputFilename;

  std::string time_string;
  if ( producttimedefined )
    time_string= "_" + producttime.format("%Y%m%dt%H%M%S");

  if ( local ) {
    outputFilename = EdProd.local_savedir + "/";
  } else {
    QDir qdir(EdProd.prod_savedir.c_str());
    if ( !qdir.exists() ) {
      if ( !qdir.mkpath(EdProd.prod_savedir.c_str())) {
        METLIBS_LOG_WARN("could not make:" <<EdProd.prod_savedir);
      }
    }
      if ( !qdir.mkpath("work")) {
        METLIBS_LOG_WARN("could not make dir:" <<EdProd.prod_savedir<<"work");
      }
      if ( !qdir.mkpath("products")) {
        METLIBS_LOG_WARN("could not make dir:" <<EdProd.prod_savedir<<"products");
      }

    outputFilename = EdProd.prod_savedir + "/work/";
  }

  EdProd.fields[fnum].filename = EdProdId.name + "_" + EdProd.fields[fnum].filenamePart + time_string + ".nc";
  outputFilename += EdProd.fields[fnum].filename;
  if ( qfile.exists(outputFilename.c_str()) && !qfile.remove(outputFilename.c_str()) ){
    message = "Copy from " + QString(EdProd.templateFilename.c_str()) + " to " + QString(outputFilename.c_str()) + "  failed. (File exsists, but can't be overwritten.)";
    return false;
  }
  if (!qfile.copy(outputFilename.c_str())){
    message = "Copy from "  + QString(EdProd.templateFilename.c_str()) + " to " + QString(outputFilename.c_str()) + "  failed";
    return false;
  }
  if ( local ) {
    EdProd.fields[fnum].localFilename = outputFilename;
  } else {
    EdProd.fields[fnum].prodFilename = outputFilename;
  }

  std::string fileType = "fimex";

  std::vector<std::string> filenames;
  std::string modelName = outputFilename;
  filenames.push_back(outputFilename);

  std::vector<std::string> format;
  format.push_back("netcdf");

  std::vector<std::string> config;

  std::vector<std::string> option;
  std::string opt = "writeable=true";
  option.push_back(opt);

  fieldPlotManager->addGridCollection(fileType, modelName, filenames,
      format,config, option);

  vector<FieldGroupInfo> fgi;
  std::string reftime = fieldPlotManager->getBestFieldReferenceTime(modelName,0,-1 );
  fieldPlotManager->getFieldGroups(modelName,reftime,true,fgi);

  return true;

}

//todo: if prod, make another copy



bool EditManager::startEdit(const EditProduct& ep,
    const EditProductId& ei,
    miTime& valid,
    QString& message)
{
  METLIBS_LOG_SCOPE();

  //this routine starts an Edit session
  //If EditProductId is sendable check that OK to start
  // then startProduction.


  //EdProd and EdProdId contains information about production
  EdProd = ep;
  EdProdId = ei;

  //get edit tools for this product
  setMapmodeinfo();

  // just in case
  combineprods.clear();
  cleanCombineData(true);
  numregs= 0;

  diutil::delete_all_and_clear(fedits);

  producttime = valid;
  producttimedefined = !valid.undef();

  for (unsigned int j=0; j<EdProd.fields.size(); j++) {
    // spec. used when reading and writing field
    FieldEdit *fed= new FieldEdit( fieldPlotManager );
    fed->setSpec(EdProd, j);

    const std::string& fieldname= EdProd.fields[j].name;

    if (EdProd.fields[j].fromfield) {

      // edit field from existing field, find correct fieldplot

      const std::vector<FieldPlot*>& vfp = plotm->getFieldPlots();
      vector<Field*> vf;
      size_t i=0;
      for (; i<vfp.size(); i++){
        vf = vfp[i]->getFields();
        // for now, only accept scalar fields
        if (vf.size()==1 && vf[0]->fieldText==EdProd.fields[j].fromfname)
          break;
      }
      if (i==vfp.size())
        return false;
      if ( vf[0]->validFieldTime.undef() ){
        valid= producttime=vf[0]->validFieldTime;
        producttimedefined = false;

      }
      fed->setData(vf, fieldname, producttime);
      fedits.push_back(fed);

    } else {

      // get field from a saved product
      std::string filename = EdProd.fields[j].fromprod.filename;
      METLIBS_LOG_DEBUG("filename for saved field file to open:" << filename);
      if(!fed->readEditFieldFile(filename, fieldname, producttime))
        return false;
      fedits.push_back(fed);

    }

    //make new local file from template
    if (!makeNewFile(j,true, message)){
      return false;
    }
    if (EdProdId.sendable) {
      //make new prod file from template
      if (!makeNewFile(j,false, message)){
        return false;
      }
    }

  }

  // delete all previous objects
  objm->getEditObjects().init();
  objm->getEditObjects().setPrefix(EdProdId.name);
  if(producttimedefined)
    objm->getEditObjects().setTime(producttime);

  bool newProduct=true;

  //objectproducts to read
  int nprods=EdProd.objectprods.size();
  std::string commentstring;

  for (int i=0;i<nprods;i++){

    savedProduct objectProd = EdProd.objectprods[i];
    std::string filename =  objectProd.filename;
    if (!filename.empty()){
      //METLIBS_LOG_INFO("filename for saved objects file to open:" << filename);
      objm->getEditObjects().setSelectedObjectTypes(objectProd.selectObjectTypes);
      objm->editCommandReadDrawFile(filename);
      commentstring += "Objects from:\n" + savedProductString(objectProd) + "\n";
      //open the comments file
      miutil::replace(filename, "draw", "comm");
      objm->editCommandReadCommentFile(filename);


      if (objectProd.productName==EdProd.name && objectProd.ptime==valid)
        newProduct=false;
    }
  }
  objm->putCommentStartLines(EdProd.name,EdProdId.name, commentstring);

  // set correct time for labels
  for (vector<std::string>::iterator p=EdProd.labels.begin(); p!=EdProd.labels.end(); p++)
    *p=insertTime(*p,valid);
  //Merge labels from EdProd  with object label input strings
  plotm->updateEditLabels(EdProd.labels,EdProd.name,newProduct);
  //save merged labels in editobjects
  vector <std::string> labels = plotm->writeAnnotations(EdProd.name);
  saveProductLabels(labels);
  objm->getEditObjects().labelsAreSaved();

  if (fedits.size()>0) fedits[0]->activate();

  return true;
}


bool EditManager::writeEditProduct(QString&  message,
    const bool wfield,
    const bool wobjects,
    const bool send,
    const bool isapproved){

  METLIBS_LOG_DEBUG("EditManager::writeEditProduct");
message.clear();
  //
  QDir qdir(EdProd.local_savedir.c_str());
  if ( !qdir.exists() ) {
    if ( !qdir.mkpath(EdProd.local_savedir.c_str())) {
      METLIBS_LOG_WARN("could not make:" <<EdProd.local_savedir);
    }
  }
  if( EdProdId.sendable && send ) {
  QDir qdir(EdProd.prod_savedir.c_str());
  if ( !qdir.exists() ) {
    if ( !qdir.mkpath(EdProd.prod_savedir.c_str())) {
      METLIBS_LOG_WARN("could not make:" <<EdProd.prod_savedir);
    }
  }
    if ( !qdir.mkpath("work")) {
      METLIBS_LOG_WARN("could not make dir:" <<EdProd.prod_savedir<<"/work");
    }
    if ( !qdir.mkpath("products")) {
      METLIBS_LOG_WARN("could not make dir:" <<EdProd.prod_savedir<<"/products");
    }
  }

  bool res= true;
  miTime t = producttime;

  if (wfield) {
    for (unsigned int i=0; i<fedits.size(); i++) {
      if(fedits[i]->writeEditFieldFile(EdProd.fields[i].localFilename) ) {
      METLIBS_LOG_INFO("Writing field:" << EdProd.fields[i].localFilename);
      } else {
        res= false;
        message += QString("Could not write field to file:") + QString(EdProd.fields[i].localFilename.c_str()) + "\n";
      }
      if(EdProdId.sendable ) {
        if ( send ) {
          if(fedits[i]->writeEditFieldFile(EdProd.fields[i].prodFilename) ) {
          METLIBS_LOG_INFO("Writing field:" << EdProd.fields[i].prodFilename);
          } else {
            res= false;
            message += "Could not write field to file:" + QString(EdProd.fields[i].prodFilename.c_str()) + "\n";
          }
        }
        if( isapproved ) {
          QString workFile = EdProd.fields[i].prodFilename.c_str();
          QFile qfile(workFile);
          QString prodFile = workFile.replace("work","products");
          METLIBS_LOG_INFO("Writing field:" << prodFile.toStdString());
          if ( qfile.exists(prodFile) && !qfile.remove(prodFile) ) {
            METLIBS_LOG_WARN("Could not save file: "<<prodFile.toStdString()<<"(File already exists and could not be removed)");
            res = false;
            message += "Could not write field to file:" + prodFile + "\n";
          } else if (!qfile.copy(prodFile) ) {
            METLIBS_LOG_WARN("Could not copy file: "<<prodFile.toStdString());
            makeNewFile(i,false,message);
            res = false;
            message += "Could not write field to file:" + prodFile + "\n";
          }
        }
      }
    }
  }

  if (wobjects){
    bool saveok = true;
    //get object string from objm to put in database and local files
    std::string objectsFilename, objectsFilenamePart,editObjectsString;
    editObjectsString = objm->writeEditDrawString(t,objm->getEditObjects());

    if (not editObjectsString.empty()) {
      //first save to local file
      objectsFilenamePart= EdProd.objectsFilenamePart;
      objectsFilename= editFileName(EdProd.local_savedir,EdProdId.name,
          objectsFilenamePart,t);

      if (!objm->writeEditDrawFile(objectsFilename,editObjectsString)){
        res= false;
        saveok= false;
        message += "Could not write objects to file:" + QString(objectsFilename.c_str()) + "\n";
      }

      if (EdProdId.sendable ) {
        if ( send ) {
          objectsFilename = editFileName(EdProd.prod_savedir + "/work",EdProdId.name,
              objectsFilenamePart,t);

          if (!objm->writeEditDrawFile(objectsFilename,editObjectsString)){
            res= false;
            saveok= false;
            message += "Could not write objects to file:" + QString(objectsFilename.c_str()) + "\n";
          }
        }
        if( isapproved ) {
          QString workFile = objectsFilename.c_str();
          QFile qfile(workFile);
          QString prodFile = workFile.replace("work","products");
          if ( qfile.exists(prodFile) && !qfile.remove(prodFile) ) {
            METLIBS_LOG_WARN("Could not save file: "<<prodFile.toStdString()<<"(File already exists and could not be removed)");
            res = false;
            message += "Could not write objects to file:" + prodFile + "\n";
          } else if (!qfile.copy(prodFile) ) {
            METLIBS_LOG_WARN("Could not copy file: "<<prodFile.toStdString());
            res = false;
            message += "Could not write objects to file:" +  prodFile+ "\n";
          }
        }
      }

    }

    objm->setObjectsSaved(saveok);
    if (saveok) objm->getEditObjects().labelsAreSaved();
  }

  if ( objm->getEditObjects().hasComments() ) {
    bool saveok= true;
    //get comment string from objm to put in database and local files
    //only do this if comments have changed !

    std::string commentFilename, commentFilenamePart,editCommentString;
    editCommentString = objm->getComments();

    if (not editCommentString.empty()) {
      //first save to local file
      commentFilenamePart= EdProd.commentFilenamePart;
      commentFilename= editFileName(EdProd.local_savedir,EdProdId.name,
          commentFilenamePart,t);

      if (!objm->writeEditDrawFile(commentFilename,editCommentString)){
        res= false;
        saveok= false;
        message += "Could not write comments to file:" + QString(commentFilename.c_str()) + "\n";
      }

      if (EdProdId.sendable ) {
        if ( send ) {

          commentFilename= editFileName(EdProd.prod_savedir + "/work",EdProdId.name,
              commentFilenamePart,t);

          if (!objm->writeEditDrawFile(commentFilename,editCommentString)){
            res= false;
            saveok= false;
            message += "Could not write comments to file:" + QString(commentFilename.c_str()) + "\n";
          }
        }
        if( isapproved ) {
          QString workFile = commentFilename.c_str();
          QFile qfile(workFile);
          QString prodFile = workFile.replace("work","products");
          if ( qfile.exists(prodFile) && !qfile.remove(prodFile) ) {
            METLIBS_LOG_WARN("Could not save file: "<<prodFile.toStdString()<<"(File already exists and could not be removed)");
            res = false;
            message += "Could not write field to file:" + prodFile + "\n";
          }

          if (!qfile.copy(prodFile) ) {
            METLIBS_LOG_WARN("Could not copy file: "<<prodFile.toStdString());
            res = false;
            message += "Could not write field to file:" +  prodFile+ "\n";
          }
          qfile.copy(prodFile);
        }
      }
    }
    if (saveok) objm->getEditObjects().commentsAreSaved();
  }


  if ( EdProdId.sendable ) {
    std::string text =t.isoTime("t") + "\n" +miutil::miTime::nowTime().isoTime("t");
    if( send ) {
      std::string filename = EdProd.prod_savedir + "/lastsaved." + EdProdId.name;
      QFile lastsaved(filename.c_str());
      lastsaved.open(QIODevice::WriteOnly);
      lastsaved.write(text.c_str());
      lastsaved.close();
    }
    if ( isapproved ) {
      std::string filename = EdProd.prod_savedir + "/lastfinished." + EdProdId.name;
      QFile lastfinnished(filename.c_str());
      lastfinnished.open(QIODevice::WriteOnly);
      lastfinnished.write(text.c_str());
      lastfinnished.close();
    }
  }
  return res;
}


bool EditManager::findProduct(EditProduct& ep, std::string pname){

  //find product with name = pname
  int n= editproducts.size();
  int i= 0;
  while (i<n && editproducts[i].name!=pname) i++;

  if (i<n) {
    //METLIBS_LOG_INFO("Found correct product for " << pname);
    ep= editproducts[i];
    return true;
  } else {
    METLIBS_LOG_ERROR("ERROR: No product found for " << pname);
    return false;
  }
}



vector<savedProduct> EditManager::getSavedProducts(const EditProduct& ep,
    std::string fieldname){
  METLIBS_LOG_SCOPE();
  int num=-1,n=ep.fields.size();
  for (int i=0;i<n;i++){
    if (fieldname==ep.fields[i].name){
      num=i;
      break;
    }
  }
  return getSavedProducts(ep,num);
}


vector<savedProduct> EditManager::getSavedProducts(const EditProduct& ep,
    int element)
{
  vector<savedProduct> prods;

  std::string fileString,filenamePart, dir,pid;
  dataSource dsource;

  // element objects or fields
  if (element==-1)
    filenamePart= ep.objectsFilenamePart;
  else if (element>-1 && element <int(ep.fields.size()))
    filenamePart= ep.fields[element].filenamePart;
  else
    return prods;

  // file prefix, find all files
  pid= "*";

  int n= ep.inputdirs.size();
  for (int i=0; i<n; i++){
    // directory to search
    dir= ep.inputdirs[i];
    // filestring to search
    fileString = dir + "/" + pid + "_" + filenamePart+ "*";
    //datasource info for dialog (inputdirs[n-1]==savedir)
    if (i==n-1) dsource= data_local;
    else        dsource= data_server;
    findSavedProducts(prods,fileString,dsource,element,ep.autoremove);
  }

  //give correct product name
  int m = prods.size();
  for (int i = 0;i<m;i++)
    prods[i].productName=ep.name;

  return prods;
}


vector<miTime> EditManager::getCombineProducts(const EditProduct& ep,
    const EditProductId& ei)
{
  METLIBS_LOG_DEBUG("getCombineProducts");

  vector<miTime> ctime;

  int ncombdirs= ep.combinedirs.size();
  // disable the last (savedir) if a sendable ProductId
  if (ei.sendable) ncombdirs--;

  if (!ei.combinable || ei.combineids.size()<2 ||
      ncombdirs<1 || ep.fields.size()<1)
    return ctime;

  std::string fileString,filenamePart, dir,pid;

  combineprods.clear();

  int numfields= ep.fields.size();

  pid= "*";

  // loop over directories
  for (int i=0; i<ncombdirs; i++) {
    dir = ep.combinedirs[i];
    dataSource dsource= (dir==ep.local_savedir) ? data_local : data_server;
    METLIBS_LOG_DEBUG("Looking in directory " << dir);
    for (int j=-1; j<numfields; j++) {
      if (j == -1) filenamePart= ep.objectsFilenamePart;
      else         filenamePart= ep.fields[j].filenamePart;
      fileString = dir + "/" + pid + "_" + filenamePart+ "*";
      METLIBS_LOG_DEBUG("    find " << fileString);
      findSavedProducts(combineprods,fileString,dsource,j,ep.autoremove);
    }
  }

  // combineprods are timesorted, newest first
  int ncp= combineprods.size();

  if (ncp==0) return ctime;

  // some test on combinations (pids and objects/fields)
  // before adding legal time

  vector<std::string> okpids;

  int i=0;
  while (i<ncp) {
    miTime ptime= combineprods[i].ptime;
    int ibegin= i;
    i++;
    while (i<ncp && combineprods[i].ptime==ptime) i++;

    okpids= findAcceptedCombine(ibegin,i,ep,ei);

    if (okpids.size()) ctime.push_back(ptime);
  }

  return ctime;
}


vector<std::string> EditManager::findAcceptedCombine(int ibegin, int iend,
    const EditProduct& ep,
    const EditProductId& ei){

  METLIBS_LOG_SCOPE();

  vector<std::string> okpids;

  // a "table" of found pids and object/fields
  int nf= ep.fields.size();
  int mf= 1 + nf;
  int np= ei.combineids.size();
  int j,ip;

  bool *table= new bool[mf*np];
  for (j=0; j<mf*np; j++) table[j]= false;

  for (int i=ibegin; i<iend; i++) {
    ip=0;
    while (ip<np && combineprods[i].pid!=ei.combineids[ip]) ip++;
    if (ip<np) {
      j= combineprods[i].element + 1;
    if (j>=0 && j<mf) table[ip*mf+j]= true;
    }
  }

  // pid accepted if all objects/fields exist
  for (ip=0; ip<np; ip++) {
    j= 0;
    while (j<mf && table[ip*mf+j]) j++;
    if (j==mf) okpids.push_back(ei.combineids[ip]);
  }

  delete[] table;

  // and finally: one pid is nothing to combine!
  if (okpids.size()==1) okpids.clear();

  return okpids;
}


void EditManager::findSavedProducts(vector <savedProduct> & prods,
    const std::string fileString,
    dataSource dsource, int element, int autoremove){

  miTime now = miTime::nowTime();

  diutil::string_v matches = diutil::glob(fileString);
  for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
    const std::string& name = *it;
    METLIBS_LOG_DEBUG("Found a file " << name);
    savedProduct savedprod;
    savedprod.ptime= objm->timeFileName(name);
    // remove files older than autoremove
    if (autoremove > 0 && !savedprod.ptime.undef() && miTime::hourDiff(now,savedprod.ptime) > autoremove ) {
      METLIBS_LOG_DEBUG("Removing file: "<<name );
      QFile::remove(name.c_str());
      continue;
    }
    savedprod.pid= objm->prefixFileName(name);
    savedprod.filename = name;
    savedprod.source = dsource;
    savedprod.element= element;
    // sort files with the newest files first !
    if (prods.empty()) {
      prods.push_back(savedprod);
    } else {
      vector <savedProduct>::iterator p =  prods.begin();
      // test >= to keep directory sequence for each time too
      while (p!=prods.end() && p->ptime>=savedprod.ptime) p++;
      prods.insert(p,savedprod);
    }
  }

  // remove files older than autoremove from the products dir
  std::string prod_str = fileString;
  miutil::replace(prod_str,"work","products");
  matches = diutil::glob(prod_str);
  for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
    const std::string& name = *it;
    METLIBS_LOG_DEBUG("Found a file " << name);
    miTime ptime= objm->timeFileName(name);
    if (autoremove > 0 && !ptime.undef() && miTime::hourDiff(now,ptime) > autoremove ) {
      METLIBS_LOG_DEBUG("Removing file: "<<name );
      QFile::remove(name.c_str());
    }
  }
}


vector<std::string> EditManager::getValidEditFields(const EditProduct& ep,
    const int element){
  METLIBS_LOG_SCOPE();

  // return names of existing fields valid for editing
  vector<std::string> vstr;
  std::string fname= miutil::to_lower(ep.fields[element].name);

  const std::vector<FieldPlot*>& vfp = plotm->getFieldPlots();
  for (size_t i=0; i<vfp.size(); i++){
    vector<Field*> vf = vfp[i]->getFields();
    // for now, only accept scalar fields
    if (vf.size() == 1) {
      std::string s= miutil::to_lower(vf[0]->name);
      if (s.find(fname)!=string::npos) {
        vstr.push_back(vf[0]->fieldText);
      }
    }
  }

  return vstr;
}



// close edit-session
void EditManager::stopEdit()
{
  METLIBS_LOG_SCOPE();

  producttimedefined = false;

  if (!inEdit)
    return;

  cleanCombineData(true);

  diutil::delete_all_and_clear(fedits);
  plotm->deleteAllEditAnnotations();
  objm->getEditObjects().clear();
  objm->getCombiningObjects().clear();

  objm->setObjectsSaved(true);
  objm->undofrontClear();

  unsentProduct = false;
}

vector<std::string> EditManager::getEditProductNames(){
  METLIBS_LOG_SCOPE();

  vector<std::string> names;
  for ( size_t i = 0; i<editproducts.size(); ++i ) {
    names.push_back(editproducts[i].name);
  }
  return names;
}

vector<EditProduct> EditManager::getEditProducts(){
  return editproducts;
}


std::string EditManager::savedProductString(savedProduct sp)
{
  if (sp.ptime.undef()){
    return sp.pid + " " + sp.productName + " "
         + " " + sp.selectObjectTypes + " " + sp.filename;
  } else {
    return sp.pid + " " + sp.productName + " "
        + sp.ptime.isoTime() + " " + sp.selectObjectTypes + " " + sp.filename;
  }
}


/*----------------------------------------------------------------------
---------------------------- combine functions --------------------------
 -----------------------------------------------------------------------*/


void EditManager::cleanCombineData(bool cleanData){

  if (cleanData) {
    int kf= combinefields.size();
    for (int i=0; i<kf; i++) {
      int nf= combinefields[i].size();
      for (int n=0; n<nf; n++)
        if (combinefields[i][n]) delete combinefields[i][n];
      combinefields[i].clear();
    }
    combinefields.clear();

    int ko= combineobjects.size();
    for (int i=0; i<ko; i++)
      combineobjects[i].clear();
    combineobjects.clear();
  }

  //delete all combiningobjects except borders
  vector <ObjectPlot*>::iterator p = objm->getCombiningObjects().objects.begin();
  while (p!=objm->getCombiningObjects().objects.end()){
    ObjectPlot * pobject = *p;
    if (!pobject->objectIs(Border)){
      p = objm->getCombiningObjects().objects.erase(p);
      delete pobject;
    }
    else p++;
  }

}


vector <std::string> EditManager::getCombineIds(const miTime & valid,
    const EditProduct& ep,
    const EditProductId& ei){
  METLIBS_LOG_SCOPE();

  vector <std::string> pids;
  int ipc=0, npc=combineprods.size();
  while (ipc<npc && combineprods[ipc].ptime!=valid) ipc++;
  if (ipc==npc) return pids;

  int ipcbegin= ipc;
  ipc++;
  while (ipc<npc && combineprods[ipc].ptime==valid) ipc++;
  int ipcend= ipc;

  pids = findAcceptedCombine(ipcbegin,ipcend,ep,ei);
  return pids;
}



bool EditManager::startCombineEdit(const EditProduct& ep,
    const EditProductId& ei,
    const miTime& valid,
    vector<std::string>& pids,
    QString& message)
{
  METLIBS_LOG_SCOPE("Time = " << valid);

  int nfe = fedits.size();
  const Area& newarea = ( nfe > 0 ? fedits[0]->editfield->area : plotm->getMapArea() );

  fieldsCombined= false;

  // erase combine fields and objects
  cleanCombineData(true);

  //EdProd and EdProdId contains information about production
  EdProd = ep;
  EdProdId = ei;

  // delete editfields
  diutil::delete_all_and_clear(fedits);

  // delete all previous objects
  objm->getEditObjects().init();
  objm->getCombiningObjects().init();

  int ipc=0, npc=combineprods.size();
  while (ipc<npc && combineprods[ipc].ptime!=valid)
    ipc++;
  if (ipc==npc)
    return false;

  int ipcbegin= ipc;
  ipc++;
  while (ipc<npc && combineprods[ipc].ptime==valid)
    ipc++;
  int ipcend= ipc;

  regnames= findAcceptedCombine(ipcbegin,ipcend,EdProd,EdProdId);

  if (regnames.size()<2)
    return false;

  // pids is returned to dialog!
  pids= regnames;

  numregs= regnames.size();

  //get edit tools for this product (updates "region" stuff )
  setMapmodeinfo();

  producttime = valid;
  producttimedefined = true;


  std::string filename = EdProd.combineBorders + EdProdId.name;
  //read AreaBorders
  if(!objm->getCombiningObjects().readAreaBorders(filename,plotm->getMapArea())){
    message = "EditManager::startCombineEdit  error reading borders";
    return false;
  }


  objm->getEditObjects().setPrefix(EdProdId.name);

  // read fields

  int nf= EdProd.fields.size();

  combinefields.resize(nf);

  matrix_nx= matrix_ny= 0;

  bool ok= true;
  int j=0;

  while (ok && j<nf) {

    std::string fieldname= EdProd.fields[j].name;

    int i= 0;
    while (ok && i<numregs) {
      ipc= ipcbegin;
      while (ipc<ipcend && (combineprods[ipc].pid!=regnames[i] ||
          combineprods[ipc].element!=j)) ipc++;
      if (ipc<ipcend) {
        FieldEdit *fed= new FieldEdit( fieldPlotManager );
        // spec. used when reading field
        if (!makeNewFile(j, true, message)){
          return false;
        }
        fed->setSpec(EdProd, j);
        std::string filename = combineprods[ipc].filename;
        METLIBS_LOG_DEBUG("Read field file " << filename);
        if(fed->readEditFieldFile(filename, fieldname, producttime)){
          int nx,ny;
          fed->getFieldSize(nx,ny);
          if (matrix_nx==0 && matrix_ny==0) {
            matrix_nx= nx;
            matrix_ny= ny;
            gridResolutionX = fed->gridResolutionX;
            gridResolutionY = fed->gridResolutionY;
          } else if (nx!=matrix_nx || ny!=matrix_ny
              || gridResolutionX != fed->gridResolutionX
              || gridResolutionY != fed->gridResolutionY) {
            ok= false;
          }
        } else {
          ok= false;
        }
        if (ok) combinefields[j].push_back(fed);
        else    delete fed;
      } else {
        ok= false;
      }
      i++;
    }
    j++;
  }

  if (!ok) {
    METLIBS_LOG_ERROR("EditManager::startCombineEdit  error reading fields");
    cleanCombineData(true);
    combineprods.clear(); // not needed to keep this, as dialog works now
    return false;
  }

  // init editfield(s)
  for (j=0; j<nf; j++) {
    FieldEdit *fed= new FieldEdit( fieldPlotManager );
    *(fed)= *(combinefields[j][0]);
    fed->setConstantValue(fieldUndef);
    fedits.push_back(fed);
    if (EdProdId.sendable) {
      //make new prd file from template
      if (!makeNewFile(j, false, message)){
        return false;
      }
    }

  }

  objm->getCombiningObjects().changeProjection(newarea);

  combineobjects.clear();

  int i= 0;
  while (ok && i<numregs) {
    ipc= ipcbegin;
    while (ipc<ipcend && (combineprods[ipc].pid!=regnames[i] ||
        combineprods[ipc].element!=-1)) ipc++;
    if (ipc<ipcend) {
      std::string filename = combineprods[ipc].filename;
      //METLIBS_LOG_DEBUG("Read object file " << filename);
      EditObjects wo;
      //init weather objects with correct prefix (region name)
      wo.setPrefix(combineprods[ipc].pid);
      objm->readEditDrawFile(filename, newarea, wo);
      combineobjects.push_back(wo);
      //open the comments file, which should have same path and
      //extension as the object file
      miutil::replace(filename, EdProd.objectsFilenamePart,
          EdProd.commentFilenamePart);
      objm->editCommandReadCommentFile(filename);
    }
    i++;
  }

  objm->getEditObjects().setTime(producttime);
  
  std::string lines;
  objm->putCommentStartLines(EdProd.name,EdProdId.name,lines);

  // the list is needed later (editmanager or editdialog)
  combineprods.clear();

  delete[] combinematrix;

  long fsize= matrix_nx*matrix_ny;
  combinematrix= new int[fsize];
  for (int i=0; i<fsize; i++)
    combinematrix[i]= -1;


  // set correct time for labels
  for (vector<string>::iterator p=EdProd.labels.begin();p!=EdProd.labels.
  end();p++)
    *p=insertTime(*p,valid);
  //Merge labels from EdProd  with object label input strings
  plotm->updateEditLabels(EdProd.labels,EdProd.name,true);
  //save merged labels in editobjects
  vector <std::string> labels = plotm->writeAnnotations(EdProd.name);
  saveProductLabels(labels);
  objm->getEditObjects().labelsAreSaved();


  objm->setAllPassive();

  editCombine();

  return true;
}


bool EditManager::editCombine()
{
  METLIBS_LOG_SCOPE();

  int nfe = fedits.size();
  const Area& newarea = ( nfe > 0 ? fedits[0]->editfield->area : plotm->getMapArea() );

  fieldsCombined= false;

  int fsize= matrix_nx*matrix_ny;

  // delete all previous objects (not borders etc)
  objm->getEditObjects().clear();

  //check if recalcCombinematrix
  //(need to store the original one to avoid it!)
  if (!recalcCombineMatrix())
    return false;

  int cosize = objm->getCombiningObjects().objects.size();

  int nparts= objm->getCombiningObjects().objectCount(Border);
  //the following should be done whenever combinematrix is changed
  //or regions changed

  // first convert all remaining objects to editfield area
  objm->getEditObjects().changeProjection(newarea);
  objm->getCombiningObjects().changeProjection(newarea);

  // projection may have been changed when showing single region data
  for (int i=0; i<numregs; i++)
    combineobjects[i].changeProjection(newarea);

  int *regindex= new int[nparts];

  for (int i=0; i<nparts; i++)
    regindex[i]=-1;

  bool error= false;

  // check each region textstring's position
  // allow duplicates and avoid the impossible
  for (int j=0; j<cosize; j++){
    ObjectPlot * pobject = objm->getCombiningObjects().objects[j];
    if (!pobject->objectIs(RegionName)) continue;
    int idxold= pobject->combIndex(matrix_nx,matrix_ny,gridResolutionX,gridResolutionY,combinematrix);
    int idxnew= pobject->getType();
    if (idxold<0 || idxold>=nparts || idxnew<0 || idxnew>=nparts){
      error= true;
    }else if (regindex[idxold]==-1){
      regindex[idxold]= idxnew;
    }else if (regindex[idxold]!=idxnew){
      error= true;
    }
  }

  for (int i=0; i<nparts; i++)
    if(regindex[i]==-1) error= true;

  if (error) {
    delete[] regindex;
    // convert all objects to map area again
    objm->getCombiningObjects().changeProjection(plotm->getMapArea());
    objm->getEditObjects().changeProjection(plotm->getMapArea());

    return false;
  }

  //update combinematrix with correct region indices
  for (int i=0; i<fsize; i++)
    combinematrix[i]= regindex[combinematrix[i]];

  delete[] regindex;

  //loop over regions , VA ,VV, VNN
  for (int i=0; i<numregs; i++){

    //loop over objects
    int obsize = combineobjects[i].objects.size();
    for (int j = 0;j<obsize;j++){
      ObjectPlot * pobject = combineobjects[i].objects[j];
      if (pobject->isInRegion(i,matrix_nx, matrix_ny,gridResolutionX,gridResolutionY,combinematrix)){
        ObjectPlot * newobject;
        if (pobject->objectIs(wFront))
          newobject = new WeatherFront(*((WeatherFront*)(pobject)));
        else if (pobject->objectIs(wSymbol))
          newobject = new WeatherSymbol(*((WeatherSymbol*)(pobject)));
        else if (pobject->objectIs(wArea))
          newobject = new WeatherArea(*((WeatherArea*)(pobject)));
        objm->getEditObjects().objects.push_back(newobject);
      }
    }
  }

  // previous settings in editobjects lost above
  if (hiddenObjects)
    objm->editHideAll();
  else if (hiddenCombineObjects)
    objm->editHideCombineObjects(showRegion);

  // then convert all objects to map area again
  objm->getCombiningObjects().changeProjection(plotm->getMapArea());
  objm->getEditObjects().changeProjection(plotm->getMapArea());

  objm->getCombiningObjects().updateObjects();
  objm->getEditObjects().updateObjects();


  float zoneWidth= 8.;
  for (int i=0; i<cosize; i++){
    //check if objects are borders
    if (objm->getCombiningObjects().objects[i]->objectIs(Border)){
      zoneWidth= objm->getCombiningObjects().objects[i]->getTransitionWidth();
      break;
    }
  }

  fieldsCombined= combineFields(zoneWidth);

  objm->setDoCombine(false);
  // update combined field and objects
  return true;
}


void EditManager::stopCombine()
{
  METLIBS_LOG_SCOPE();

  objm->undofrontClear();

  objm->editNotMarked();

  objm->setDoCombine(false);
  objm->editNewObjectsAdded(0);

  vector <std::string> labels = plotm->writeAnnotations(EdProd.name);
  saveProductLabels(labels);
  objm->getEditObjects().labelsAreSaved();

  cleanCombineData(false);

  //delete combinematrix
  if (matrix_nx > 0 && matrix_ny > 0 && combinematrix!=0){
    delete[] combinematrix;
    matrix_nx= 0;
    matrix_ny= 0;
    combinematrix= 0;
  }

  if (!fieldsCombined) {
    for (unsigned int j=0; j<fedits.size(); j++)
      fedits[j]->setConstantValue(fieldUndef);
  }

  // always matching dialog ???????????
  if (fedits.size() > 0)
    fedits[0]->activate();
}


bool EditManager::combineFields(float zoneWidth) {

  // a very very simple approach...

  int nx= matrix_nx;
  int ny= matrix_ny;

  int i,n, size= nx*ny;

  int nf= fedits.size();
  //METLIBS_LOG_DEBUG("number of fedits:" << nf);
  //METLIBS_LOG_DEBUG("number of combinefields:" << mc);

  if ( nf < 1 ){
    METLIBS_LOG_ERROR("EditManager::combineFields, number of fields is zero");
    return false;
  }

  for (int j=0; j<nf; j++) {
    float *pdata= fedits[j]->editfield->data;
    for (i=0; i<size; i++)
      pdata[i]= combinefields[j][combinematrix[i]]->editfield->data[i];
  }

  int nsmooth= int(zoneWidth*0.5 + 0.5);
  if (nsmooth<1) return true;

  // find the rather small number of points to smooth
  // (much faster than calling standard smoothing functions
  //  that computes something at every gridpoint)

  bool *mark= new bool[size];

  int ij,io,jo,j,i1,i2,j1,j2;

  for (ij=0; ij<size; ij++) mark[ij]= false;

  for (jo=0; jo<ny-1; jo++) {
    for (io=0; io<nx-1; io++) {
      ij=jo*nx+io;
      if (combinematrix[ij]!=combinematrix[ij+1] ||
          combinematrix[ij]!=combinematrix[ij+nx]) {
        i1= io-nsmooth;
        if(i1<0) i1= 0;
        i2= io+nsmooth+1;
        if (combinematrix[ij]!=combinematrix[ij+1]) i2++;
        if(i2>nx) i2= nx;
        j1= jo-nsmooth;
        if(j1<0) j1= 0;
        j2= jo+nsmooth+1;
        if (combinematrix[ij]!=combinematrix[ij+nx]) j2++;
        if(j2>ny) j2= ny;
        for (j=j1; j<j2; j++)
          for (i=i1; i<i2; i++) mark[j*nx+i]= true;
      }
    }
  }

  n= 0;
  for (ij=0; ij<size; ij++) if (mark[ij]) n++;

  int *xdir= new int[n];
  int *ydir= new int[n];
  int *xdircp= new int[n];
  int *ydircp= new int[n];

  float *f2= new float[size];

  if (xdir==NULL || ydir==NULL || xdircp==NULL || ydircp==NULL || f2==NULL) {
    delete[] xdir;
    delete[] ydir;
    delete[] xdircp;
    delete[] ydircp;
    delete[] f2;
    METLIBS_LOG_ERROR("ERROR: OUT OF MEMORY in combineFields !!!!!!!!");
    METLIBS_LOG_ERROR("       NOT ABLE TO SMOOTH the result field !!!");
    return true;
  }

  int nxdir=0, nydir=0, nxdircp=0, nydircp=0;
  bool xok,yok;

  // using only first field ok ???????????????????????????????
  float *pdata= fedits[0]->editfield->data;

  for (ij=0; ij<size; ij++) {
    if (mark[ij]) {
      i= ij%nx;
      j= ij/nx;
      if (i>0 && i<nx-1)
        xok= (pdata[ij-1] !=fieldUndef &&
            pdata[ij]   !=fieldUndef &&
            pdata[ij+1] !=fieldUndef);
      else xok= false;
      if (j>0 && j<ny-1)
        yok= (pdata[ij-nx]!=fieldUndef &&
            pdata[ij]   !=fieldUndef &&
            pdata[ij+nx]!=fieldUndef);
      else yok= false;
      if (xok && yok) {
        xdir[nxdir++]= ij;
        ydir[nydir++]= ij;
      } else if (xok) {
        xdir[nxdir++]= ij;
        ydircp[nydircp++]= ij;
      } else if (yok) {
        ydir[nydir++]= ij;
        xdircp[nxdircp++]= ij;
      }
    }
  }

  // standard smoothing at selected gridpoints

  const float s= 0.25;

  for (int j=0; j<nf; j++) {

    float *f1= fedits[j]->editfield->data;

    for (ij=0; ij<size; ++ij) f2[ij]= f1[ij];

    for (n=0; n<nsmooth; n++) {
      for (i=0; i<nxdir; i++) {
        ij= xdir[i];
        f2[ij]= f1[ij] + s * (f1[ij-1] + f1[ij+1] - 2.*f1[ij]);
      }
      for (i=0; i<nxdircp; i++) {
        ij= xdircp[i];
        f2[ij]= f1[ij];
      }
      for (i=0; i<nydir; i++) {
        ij= ydir[i];
        f1[ij]= f2[ij] + s * (f2[ij-nx] + f2[ij+nx] - 2.*f2[ij]);
      }
      for (i=0; i<nydircp; i++) {
        ij= ydircp[i];
        f1[ij]= f2[ij];
      }
    }
  }

  delete[] mark;

  delete[] xdir;
  delete[] ydir;
  delete[] xdircp;
  delete[] ydircp;
  delete[] f2;

  return true;
}


struct polygon{
  vector<float> x;
  vector<float> y;
};


bool EditManager::recalcCombineMatrix(){

  //METLIBS_LOG_DEBUG("recalcCombineMatrix");
  int cosize= objm->getCombiningObjects().objects.size();
  int nf= fedits.size();

  if ( nf < 1 ){
    METLIBS_LOG_ERROR("EditManager::recalcCombineMatrix, number of fields is zero");
    return false;
  }

  int npos= 0;
  int n = 0;
  int *numv= new int[cosize];
  int *startv= new int[cosize];
  for (int i=0; i<cosize; i++){
    //check if objects are borders
    if (objm->getCombiningObjects().objects[i]->objectIs(Border)){
      numv[n]= objm->getCombiningObjects().objects[i]->getXYZsize();
      startv[n]= npos;
      npos+= numv[n];
      n++;
    }
  }

  float *xposis= new float[npos];
  float *yposis= new float[npos];

  int nborders = 0;
  int nposition=0;
  for (int i=0; i<cosize; i++){
    if (objm->getCombiningObjects().objects[i]->objectIs(Border)){
      vector <float> xborder=objm->getCombiningObjects().objects[i]->getX();
      vector <float> yborder=objm->getCombiningObjects().objects[i]->getY();
      for (int j=0; j<numv[nborders]; j++) {
        xposis[nposition]=xborder[j];
        yposis[nposition]=yborder[j];
        nposition++;
      }
      nborders++;
    }
  }
  //####################################################################
//  METLIBS_LOG_DEBUG("recalcCombineMatrix  nborders=" << nborders);
//  for (int nb=0; nb<nborders; nb++) {
//    METLIBS_LOG_DEBUG("PRE CONV border "<<nb);
//    for (int ip=startv[nb]; ip<startv[nb]+numv[nb]; ip++)
//      METLIBS_LOG_DEBUG("  x,y:  " << xposis[ip] << "  " << yposis[ip]);
//  }
  //####################################################################

  const Area& oldArea= plotm->getMapArea();
  const Area& newArea= fedits[0]->editfield->area;
  if (!gc.getPoints(oldArea.P(),newArea.P(),npos,xposis,yposis)) {
    METLIBS_LOG_ERROR("changeProjection: getPoints error");
    return false;
  }
  //####################################################################
//  for (int nb=0; nb<nborders; nb++) {
//    METLIBS_LOG_DEBUG("AFTER CONV border "<<nb);
//    for (int ip=startv[nb]; ip<startv[nb]+numv[nb]; ip++)
//      METLIBS_LOG_DEBUG("  x,y:  " << xposis[ip] << "  " << yposis[ip]);
//  }
  //####################################################################

  // Splines as shown (maybe approx due to map conversion...)
  int ndivs= 5;
  int m= npos*(ndivs+1) - nborders*ndivs;
  float *xpos= new float[m];
  float *ypos= new float[m];
  npos= 0;
  for (int i=0; i<nborders; i++){
    int is= startv[i];
    int nfirst= 0;
    int nlast=  numv[i] - 1;
    int ns= fedits[0]->smoothline(numv[i], &xposis[is], &yposis[is],
        nfirst, nlast, ndivs,
        &xpos[npos], &ypos[npos]);
    startv[i]= npos;
    numv[i]= ns;
    npos+=ns;
  }

  delete[] xposis;
  delete[] yposis;

  // find crossing with fieldarea
  vector<int> crossp, quadr; // 0=Down, 1=Left, 2=Up, 3=Right
  vector<float> crossx, crossy;
  Rectangle r= fedits[0]->editfield->area.R();

  r.setExtension(0.5*gridResolutionX);

  bool crossing;

  for (int i=0; i<nborders; i++){
    crossp.push_back(-1);
    quadr.push_back(-1);
    crossx.push_back(-1);
    crossy.push_back(-1);
    crossing = false;
    for (int j=startv[i]; j<numv[i]+startv[i]; j++){
      if (!r.isnear(xpos[j],ypos[j])){
        if (j==startv[i]) break;
        crossing= true;
        crossp[i]= j-1;
        break;
      }
    }

    // inter/extrapolation to border
    int p;
    if (crossing) p= crossp[i]+1;
    else          p= crossp[i]= startv[i]+numv[i]-1;

    float xc[2], yc[2];
    int cquadr[2];
    float dx= xpos[p] - xpos[p-1];
    float dy= ypos[p] - ypos[p-1];
    int nc= 0;
    if (dx<0.) {
      xc[nc]= -0.5*gridResolutionX;
      yc[nc]= ypos[p] + dy * (xc[nc]-xpos[p])/dx;
      cquadr[nc++]= 1;
    } else if (dx>0.) {
      xc[nc]= (matrix_nx-0.5)*gridResolutionX;
      yc[nc]= ypos[p] + dy * (xc[nc]-xpos[p])/dx;
      cquadr[nc++]= 3;
    }
    if (dy<0.) {
      yc[nc]= -0.5*gridResolutionY;
      xc[nc]= xpos[p] + dx * (yc[nc]-ypos[p])/dy;
      cquadr[nc++]= 0;
    } else if (dy>0.) {
      yc[nc]= (matrix_ny-0.5)*gridResolutionY;
      xc[nc]= xpos[p] + dx * (yc[nc]-ypos[p])/dy;
      cquadr[nc++]= 2;
    }
    if (nc==2) {
      float dx1= xc[0] - xpos[p];
      float dy1= yc[0] - ypos[p];
      float dx2= xc[1] - xpos[p];
      float dy2= yc[1] - ypos[p];
      if (dx1*dx1+dy1*dy1<dx2*dx2+dy2*dy2) nc= 1;
    }
    nc--;
    crossx[i]= xc[nc];
    crossy[i]= yc[nc];
    quadr[i] = cquadr[nc];
  }

  // sort legs counter-clockwise
  vector<int> legorder;
  for (int j=0; j<4; j++)
    for (int i=0; i<nborders; i++)
      if (quadr[i]==j)
        legorder.push_back(i);

  int numlegs= legorder.size();
  for (int j=1; j<numlegs; j++){
    int k= j;
    while ((k<numlegs) && (quadr[legorder[k]]==quadr[legorder[k-1]])){
      bool swap= false;
      if (quadr[legorder[k]]==0)
        swap= (crossx[legorder[k]]> crossx[legorder[k-1]]);
      else if (quadr[legorder[k]]==1)
        swap= (crossy[legorder[k]]< crossy[legorder[k-1]]);
      else if (quadr[legorder[k]]==2)
        swap= (crossx[legorder[k]]> crossx[legorder[k-1]]);
      else
        swap= (crossy[legorder[k]]> crossy[legorder[k-1]]);
      if (swap){
        int tmp= legorder[k];
        legorder[k]= legorder[k-1];
        legorder[k-1]= tmp;
      }
      k++;
    }
  }

  // make areas
  polygon P[10];
  int idx;
  int next;
  for (int i=0; i<numlegs; i++){
    idx= legorder[i];
    // next leg
    next= i+1;
    if (next==numlegs) next= 0;
    next= legorder[next];

    // down next leg
    for (int j=startv[next]; j<=crossp[next]; j++){
      P[i].x.push_back(xpos[j]);
      P[i].y.push_back(ypos[j]);
    }
    // add crossing point for next leg
    P[i].x.push_back(crossx[next]);
    P[i].y.push_back(crossy[next]);


    if ((quadr[next]!=quadr[idx]) || // check if jumping to new quadrant
        (i==numlegs-1)){ // or last polygon
      // add rectangle corners
      int k= quadr[next];
      bool first= true;
      while (k!=quadr[idx] || first){
        if (k==0){
          P[i].x.push_back(r.x2+1);
          P[i].y.push_back(r.y1-1);
        } else if (k==1){
          P[i].x.push_back(r.x1-1);
          P[i].y.push_back(r.y1-1);
        } else if (k==2){
          P[i].x.push_back(r.x1-1);
          P[i].y.push_back(r.y2+1);
        } else {
          P[i].x.push_back(r.x2+1);
          P[i].y.push_back(r.y2+1);
        }
        k--;
        if (k==-1) k=3;
        first= false;
      }
    }

    // first leg
    // add crossing point
    P[i].x.push_back(crossx[idx]);
    P[i].y.push_back(crossy[idx]);
    // up first leg
    for (int j=crossp[idx]; j>=startv[idx]; j--){
      P[i].x.push_back(xpos[j]);
      P[i].y.push_back(ypos[j]);
    }

  }

  int i,j,fsize= matrix_ny*matrix_nx;

  for (i=0; i<fsize; ++i)
    combinematrix[i]= -1;

  int *mark= new int[fsize];

  float x1,x2,y1,y2;
  int j1,j2;
  int nx= matrix_nx, ny= matrix_ny;

  for (int k=0; k<numlegs; k++){

    // mark line intersections to the left of (in front of) each gridpoint

    for (i=0; i<fsize; ++i) mark[i]= 0;

    int npol= P[k].x.size();
    for (int p=1; p<npol; p++){

      x1= P[k].x[p-1]/gridResolutionX;
      y1= P[k].y[p-1]/gridResolutionY;
      x2= P[k].x[p]/gridResolutionX;
      y2= P[k].y[p]/gridResolutionY;

      if (y1<y2) {
        j1= int(y1+1.);
        j2= int(y2+1.);
      } else {
        j1= int(y2+1.);
        j2= int(y1+1.);
      }
      if (j1<0)  j1= 0;
      if (j2>ny) j2= ny;
      for (j=j1; j<j2; j++) {
        i= int(x1 + (x2-x1) * (float(j)-y1)/(y2-y1) + 1.) - 1;
        if      (i<0)    mark[j*nx]++;
        else if (i<nx-1) mark[j*nx+i+1]++;
      }
    }

    // fill polygon interior with polygon index
    for (j=0; j<ny; j++) {
      int ncross= 0;
      for (i=0; i<nx; i++) {
        ncross= (ncross+mark[j*nx+i])%2;
        if (ncross==1) combinematrix[j*nx+i]= k;
      }
    }

  }

  delete[] mark;

  delete[] xpos;
  delete[] ypos;
  delete[] numv;
  delete[] startv;

  return true;
}



/*----------------------------------------------------------------------
---------------------------- end combine functions ---------------------
 -----------------------------------------------------------------------*/



/*----------------------------------------------------------------------
----------------   functions called from PlotModule   ------------------
 -----------------------------------------------------------------------*/


void EditManager::prepareEditFields(const std::string& plotName, const vector<std::string>& inp)
{
  METLIBS_LOG_SCOPE();

  // setting plot options

  if (fedits.size()==0) return;

  vector<std::string> vip= inp;
  unsigned int npi= vip.size();
  if (npi>fedits.size()) npi= fedits.size();

  for (unsigned int i=0; i<npi; i++) {
    fedits[i]->editfieldplot->prepare(plotName, vip[i]);
  }

  // for showing single region during and after combine
  npi= vip.size();
  if (npi>combinefields.size()) npi= combinefields.size();

  for (unsigned int i=0; i<npi; i++) {
    int nreg=combinefields[i].size();
    for (int r=0; r<nreg; r++) {
      combinefields[i][r]->editfieldplot->prepare(plotName, vip[i]);
    }
  }
}


bool EditManager::getFieldArea(Area& a)
{
  for (unsigned int i=0; i<fedits.size(); i++) {
    if (fedits[i]->editfield) {
      a= fedits[i]->editfield->area;
      return true;
    }
  }

  return false;
}

void EditManager::setEditMessage(const string& str)
{
  // set or remove (if empty string) an edit message

  if (apEditmessage) {
    delete apEditmessage;
    apEditmessage = 0;
  }

  if (not str.empty()) {
    string labelstr;
    labelstr = "LABEL text=\"" + str + "\"";
    labelstr += " tcolour=blue bcolour=red fcolour=red:128";
    labelstr += " polystyle=both halign=left valign=top";
    labelstr += " xoffset=0.01 yoffset=0.1 fontsize=30";
    apEditmessage = new AnnotationPlot();
    if (!apEditmessage->prepare(labelstr)) {
      delete apEditmessage;
      apEditmessage = 0;
    }
  }
}


void EditManager::plot(bool under, bool over)
{
//  METLIBS_LOG_DEBUG("EditManager::plot  under="<<under<<"  over="<<over
//  <<"  showRegion="<<showRegion);

  if (apEditmessage)
    apEditmessage->plot();

  bool plototherfield= false, plotactivefield= false, plotobjects= false;
  bool plotcombine= false, plotregion= false;

  int nf= fedits.size();

  if (showRegion<0) {

    if (under) {
      if (mapmode!=combine_mode)                        plototherfield= true;
      if (mapmode!=fedit_mode && mapmode!=combine_mode) plotactivefield= true;
      if (mapmode!=draw_mode  && mapmode!=combine_mode) plotobjects= true;
      if (mapmode!=combine_mode)                        plotcombine= true;
    }
    if (over) {
      if (mapmode==combine_mode && fieldsCombined)      plototherfield= true;
      if (mapmode==fedit_mode || mapmode==combine_mode) plotactivefield= true;
      if (mapmode==draw_mode  || mapmode==combine_mode) plotobjects= true;
      if (mapmode==combine_mode)                        plotcombine= true;
    }

  } else {

    if (under)                          plotregion= true;
    if (under && mapmode!=combine_mode) plotcombine= true;
    if (over  && mapmode==combine_mode) plotcombine= true;

  }

  bool plotinfluence= (mapmode==fedit_mode);

//  METLIBS_LOG_DEBUG(" plototherfield="<<plototherfield
//  <<" plotactivefield="<<plotactivefield
//  <<" plotobjects="<<plotobjects
//  <<" plotinfluence="<<plotinfluence
//  <<" plotregion="<<plotregion);

  if (plotcombine && under){
    int n= objm->getCombiningObjects().objects.size();
    for (int i=0; i<n; i++)
      objm->getCombiningObjects().objects[i]->plot();
  }

  if (plototherfield || plotactivefield) {
    for (int i=0; i<nf; i++) {
      if (fedits[i]->editfield && fedits[i]->editfieldplot) {
        bool act= fedits[i]->activated();
        if ((act && plotactivefield) || (!act && plototherfield)){
          //METLIBS_LOG_DEBUG("  Plotting field " << i);
          fedits[i]->plot(plotinfluence);
        }
      }
    }
  }

  if (plotobjects) {
    objm->getEditObjects().plot();
    if (over) {
      objm->getEditObjects().drawJoinPoints();
    }
  }

  if (plotregion) plotSingleRegion();

  if (plotcombine && over){
    int n= objm->getCombiningObjects().objects.size();
    float scale= gridResolutionX;
    if (nf > 0 && plotm->getMapArea().P() != fedits[0]->editfield->area.P() ) {
      int npos= 0;
      for (int i=0; i<n; i++)
        if (objm->getCombiningObjects().objects[i]->objectIs(Border))
          npos+=objm->getCombiningObjects().objects[i]->getXYZsize();
      float *x= new float[npos*2];
      float *y= new float[npos*2];
      npos= 0;
      for (int i=0; i<n; i++) {
        if (objm->getCombiningObjects().objects[i]->objectIs(Border)){
          vector <float> xborder=objm->getCombiningObjects().objects[i]->getX();
          vector <float> yborder=objm->getCombiningObjects().objects[i]->getY();
          int np= objm->getCombiningObjects().objects[i]->getXYZsize();
          for (int j=0; j<np; j++) {
            x[npos]  = xborder[j];
            y[npos++]= yborder[j];
            x[npos]  = xborder[j] + 1.0;
            y[npos++]= yborder[j] + 1.0;
          }
        }
      }
      if (gc.getPoints(plotm->getMapArea().P(),fedits[0]->editfield->area.P(),npos,x,y)) {
        float s= 0.;
        for (int j=0; j<npos; j+=2) {
          float dx= x[j] - x[j+1];
          float dy= y[j] - y[j+1];
          s+= sqrtf(dx*dx+dy*dy)/sqrtf(2.0);
        }
        scale= npos*0.5*gridResolutionX/s;
      } else {
        METLIBS_LOG_ERROR("EditManager::plot : getPoints error");
      }
      delete[] x;
      delete[] y;
    }

    objm->getCombiningObjects().setScaleToField(scale);

    objm->getCombiningObjects().plot();

    objm->getCombiningObjects().drawJoinPoints();
  }
}


void EditManager::plotSingleRegion()
{
  METLIBS_LOG_SCOPE();

  if (showRegion<0 || showRegion>=numregs)
    return;

  int nf= combinefields.size();

  for (int i=0; i<nf; i++) {
    if (showRegion<int(combinefields[i].size())) {
      if (combinefields[i][showRegion]->editfield &&
          combinefields[i][showRegion]->editfieldplot)
        combinefields[i][showRegion]->plot(false);
    }
  }

  if (showRegion<int(combineobjects.size())) {
    // projection may have been changed when showing single region data
    combineobjects[showRegion].changeProjection(plotm->getMapArea());

    combineobjects[showRegion].plot();
  }
}


bool EditManager::obs_mslp(ObsPositions& obsPositions) {

  if (fedits.size()==0) return false;

  if (!fedits[0]->editfield) return false;

  //change projection if needed
  if ( obsPositions.obsArea.P() != fedits[0]->editfield->area.P() ){
    gc.getPoints(obsPositions.obsArea.P(), fedits[0]->editfield->area.P(),
        obsPositions.numObs, obsPositions.xpos, obsPositions.ypos);
    obsPositions.obsArea= fedits[0]->editfield->area;
  }

  if ( obsPositions.convertToGrid ) {
    fedits[0]->editfield->convertToGrid(obsPositions.numObs,
        obsPositions.xpos, obsPositions.ypos);
    obsPositions.convertToGrid = false;
  }

  //get values
  int interpoltype=1;
  if (!fedits[0]->editfield->interpolate(obsPositions.numObs,
      obsPositions.xpos, obsPositions.ypos,
      obsPositions.values,
      interpoltype)) return false;

  return true;
}

/*----------------------------------------------------------------------
----------------   end functions called from PlotModule   --------------
 -----------------------------------------------------------------------*/

void EditManager::initEditTools(){
  /* called from EditManager constructor. Defines edit tools used
     for editing fields, and displaying and editing objects  */
  METLIBS_LOG_SCOPE();
  //defines edit and drawing tools

  eToolFieldStandard.push_back(newEditToolInfo("Change value",      edit_value));
  eToolFieldStandard.push_back(newEditToolInfo("Move",           edit_move));
  eToolFieldStandard.push_back(newEditToolInfo("Change gradient",   edit_gradient));
  eToolFieldStandard.push_back(newEditToolInfo("Line, without smooth",edit_line));
  eToolFieldStandard.push_back(newEditToolInfo("Line, with smooth",edit_line_smooth));
  eToolFieldStandard.push_back(newEditToolInfo("Line, limited, without smooth",
      edit_line_limited));
  eToolFieldStandard.push_back(newEditToolInfo("Line, limited, with smooth",
      edit_line_limited_smooth));
  eToolFieldStandard.push_back(newEditToolInfo("Smooth",           edit_smooth));
  eToolFieldStandard.push_back(newEditToolInfo("Replace undefined values",
      edit_replace_undef));

  eToolFieldClasses.push_back(newEditToolInfo("Line",             edit_class_line));
  eToolFieldClasses.push_back(newEditToolInfo("Copy value",      edit_class_copy));

  eToolFieldNumbers.push_back(newEditToolInfo("Copy value",      edit_class_copy));
  eToolFieldNumbers.push_back(newEditToolInfo("Change value",       edit_value));
  eToolFieldNumbers.push_back(newEditToolInfo("Move",            edit_move));
  eToolFieldNumbers.push_back(newEditToolInfo("Change gradient",    edit_gradient));
  eToolFieldNumbers.push_back(newEditToolInfo("Set undefined",    edit_set_undef));
  eToolFieldNumbers.push_back(newEditToolInfo("Smooth",            edit_smooth));
  eToolFieldNumbers.push_back(newEditToolInfo("Replace undefined values",
      edit_replace_undef));

  // draw_mode types
#ifdef SMHI
  fronts.push_back(newEditToolInfo("Kallfront",Cold,"blue"));
  fronts.push_back(newEditToolInfo("Varmfront",Warm,"red"));
  fronts.push_back(newEditToolInfo("Ocklusion",Occluded,"purple"));
  fronts.push_back(newEditToolInfo("Kall ocklusion",Occluded,"blue"));
  fronts.push_back(newEditToolInfo("Varm ocklusion",Occluded,"red"));
  fronts.push_back(newEditToolInfo("Stationär front",Stationary,"grey50"));
  fronts.push_back(newEditToolInfo("Tråg",TroughLine,"black"));
#else
  fronts.push_back(newEditToolInfo("Cold front",Cold,"blue"));
  fronts.push_back(newEditToolInfo("Warm front",Warm,"red"));
  fronts.push_back(newEditToolInfo("Occlusion",Occluded,"purple"));
  fronts.push_back(newEditToolInfo("Trough",Line,"blue"));
  fronts.push_back(newEditToolInfo("Squall line",SquallLine,"blue"));
#endif
#ifdef SMHI
  //fronts.push_back(newEditToolInfo("Signifikant v�der",SigweatherFront,"green"));
  fronts.push_back(newEditToolInfo("Molngräns",SigweatherFront,"green4"));
  fronts.push_back(newEditToolInfo("VMC-linje",Line,"black","black",-2,true,"dash2"));
  fronts.push_back(newEditToolInfo("CAT-linje",Line,"black","black",-2,true,"longlongdash"));
  fronts.push_back(newEditToolInfo("Jetström",ArrowLine,"grey50"));
  fronts.push_back(newEditToolInfo("Åsklinje röd",Line,"red","red",0,false));
  fronts.push_back(newEditToolInfo("Åsklinje grön",Line,"green","green",0,false,"dash3"));
  fronts.push_back(newEditToolInfo("Åsklinje blå",Line,"blue","blue",0,false,"dot"));
#else
  fronts.push_back(newEditToolInfo("Significant weather",SigweatherFront,"black"));
  fronts.push_back(newEditToolInfo("Significant weather TURB/VA/RC",SigweatherFront,"red"));
  fronts.push_back(newEditToolInfo("Significant weather ICE/TCU/CB",SigweatherFront,"blue"));
  fronts.push_back(newEditToolInfo("Jet stream",ArrowLine,"black","black",0,true));
  fronts.push_back(newEditToolInfo("Cold occlusion",Occluded,"blue"));
  fronts.push_back(newEditToolInfo("Warm occlusion",Occluded,"red"));
  fronts.push_back(newEditToolInfo("Stationary front",Stationary,"grey50"));
  fronts.push_back(newEditToolInfo("Black sharp line",Line,"black","black",0,false));
  fronts.push_back(newEditToolInfo("Black smooth line",Line,"black","black",0,true));
  fronts.push_back(newEditToolInfo("Red sharp line",Line,"red","red",0,false));
  fronts.push_back(newEditToolInfo("Red smooth line",Line,"red"));
  fronts.push_back(newEditToolInfo("Blue sharp line",Line,"blue","blue",0,false));
  fronts.push_back(newEditToolInfo("Blue smooth line",Line,"blue"));
  fronts.push_back(newEditToolInfo("Green sharp line",Line,"green","green",0,false));
  fronts.push_back(newEditToolInfo("Green smooth line",Line,"green"));
  fronts.push_back(newEditToolInfo("Black sharp line stipple",Line,"black","black",0,false,"dash2"));
  fronts.push_back(newEditToolInfo("Black smooth line stipple",Line,"black","black",0,true,"dash2"));
  fronts.push_back(newEditToolInfo("Red sharp line stipple",Line,"red","red",0,false,"dash2"));
  fronts.push_back(newEditToolInfo("Red smooth line stipple",Line,"red","red",0,true,"dash2"));
  fronts.push_back(newEditToolInfo("Blue sharp line stipple",Line,"blue","blue",0,false,"dash2"));
  fronts.push_back(newEditToolInfo("Blue smooth line stipple",Line,"blue","blue",0,true,"dash2"));
  fronts.push_back(newEditToolInfo("Green sharp line stipple",Line,"green","green",0,false,"dash2"));
  fronts.push_back(newEditToolInfo("Green smooth line stipple",Line,"green","green",0,true,"dash2"));
  fronts.push_back(newEditToolInfo("Black sharp arrow",ArrowLine,"black","black",0,false));
  fronts.push_back(newEditToolInfo("Black sharp thin arrow",ArrowLine,"black","black",-2,false));
  fronts.push_back(newEditToolInfo("Black smooth arrow",ArrowLine,"black"));
  fronts.push_back(newEditToolInfo("Red sharp arrow",ArrowLine,"red","red",0,false));
  fronts.push_back(newEditToolInfo("Red smooth arrow",ArrowLine,"red"));
  fronts.push_back(newEditToolInfo("Blue sharp arrow",ArrowLine,"blue","blue",0,false));
  fronts.push_back(newEditToolInfo("Blue smooth arrow",ArrowLine,"blue"));
#endif


#ifdef SMHI
  symbols.push_back(newEditToolInfo("Duggregn",180,"green4"));
  symbols.push_back(newEditToolInfo( "Snö",254,"green4"));
  //symbols.push_back(newEditToolInfo( "Regnskurar",1044,"green4","green4",4));
  symbols.push_back(newEditToolInfo( "Regnskurar",109,"green4"));
  symbols.push_back(newEditToolInfo( "Åska",119,"red"));
  symbols.push_back(newEditToolInfo( "Åska med hagel",122,"red"));
  //symbols.push_back(newEditToolInfo("Torrdis",88,"black"));
  //symbols.push_back(newEditToolInfo("R�k",42,"black"));
  //symbols.push_back(newEditToolInfo("Sn�blandad regn",78,"green4"));
  symbols.push_back(newEditToolInfo( "Snöblandad by",43,"green4"));
  symbols.push_back(newEditToolInfo( "Snöby",114,"green4","green4",4));
  //symbols.push_back(newEditToolInfo( "Sn�by",1043,"green4","green4",3));
  symbols.push_back(newEditToolInfo( "Underkylt duggregn",83,"red"));
  symbols.push_back(newEditToolInfo( "Underkylt regn",93,"red"));
  symbols.push_back(newEditToolInfo( "Hagelbyar",118,"green4"));
  symbols.push_back(newEditToolInfo( "Kornsnö",44,"red"));
  symbols.push_back(newEditToolInfo( "Snödrev",46,"black"));
  //symbols.push_back(newEditToolInfo("Mountain waves",130,"black"));
  //symbols.push_back(newEditToolInfo( "Dimma",1041,"gulbrun"));
  symbols.push_back(newEditToolInfo( "Dimma",62,"gulbrun"));
  //symbols.push_back(newEditToolInfo( "L�gtryck",1019,"black","black",1));
  symbols.push_back(newEditToolInfo( "Lågtryck",242,"black","black",1));
  //symbols.push_back(newEditToolInfo( "H�gtryck",1020,"black","black",1));
  symbols.push_back(newEditToolInfo( "Högtryck",243,"black","black",1));
  symbols.push_back(newEditToolInfo( "Trycktendens",900,"black"));
  symbols.push_back(newEditToolInfo( "Fall",900,"red"));
  symbols.push_back(newEditToolInfo( "Stig",900,"blue"));
#else
  symbols.push_back(newEditToolInfo("Low pressure",242,"red"));
  symbols.push_back(newEditToolInfo("High pressure",243,"blue"));
  symbols.push_back(newEditToolInfo("Fog",62,"darkYellow"));
  symbols.push_back(newEditToolInfo("Thunderstorm",119,"red"));
  symbols.push_back(newEditToolInfo("Freezing rain",93,"red"));
  symbols.push_back(newEditToolInfo("Freezing drizzle",83,"red"));
  symbols.push_back(newEditToolInfo( "Showers",109,"green"));
  symbols.push_back(newEditToolInfo( "Snow showers",114,"green"));
  symbols.push_back(newEditToolInfo( "Hail showers",117,"green"));
  symbols.push_back(newEditToolInfo( "Snow",254,"green"));
  symbols.push_back(newEditToolInfo( "Rain",89,"green"));
  symbols.push_back(newEditToolInfo("Drizzle",80,"green"));
  symbols.push_back(newEditToolInfo("Cold",244,"blue"));
  symbols.push_back(newEditToolInfo("Warm",245,"red"));
  symbols.push_back(newEditToolInfo( "Rain showers",110,"green"));
  symbols.push_back(newEditToolInfo( "Sleet showers",126,"green"));
  symbols.push_back(newEditToolInfo( "Thunderstorm with hail",122,"red"));
  symbols.push_back(newEditToolInfo( "Sleet",96,"green"));
  symbols.push_back(newEditToolInfo( "Hurricane",253,"black"));
  symbols.push_back(newEditToolInfo( "Disk",241,"red"));
  symbols.push_back(newEditToolInfo( "Circle",35,"blue"));
  symbols.push_back(newEditToolInfo( "Cross",255,"red"));
#endif
  symbols.push_back(newEditToolInfo("Text",0,"black"));

#ifdef SMHI
  areas.push_back(newEditToolInfo("Dis",Genericarea_constline,"red", "red", 0, true, "solid", "vdiagleft"));
  areas.push_back(newEditToolInfo("Dimma",Genericarea_constline,"red", "red", 0, true, "solid", "vldiagcross_little"));
  areas.push_back(newEditToolInfo("Regnområde",Genericarea_constline,"green4","blank", 0, true, "solid", "ldiagleft2"));
  sigsymbols.push_back(newEditToolInfo("Sig18",247,"black","black",-1));
#else
  areas.push_back(newEditToolInfo("Precipitation",Genericarea,"green4","green4"));
  areas.push_back(newEditToolInfo("Showers",Genericarea,"green3","green3",0,true,"dash2"));
  areas.push_back(newEditToolInfo("Fog",Genericarea,"darkGray","darkGrey",0,true,"empty","zigzag"));
  areas.push_back(newEditToolInfo("Significant weather",Sigweather,"black"));
  areas.push_back(newEditToolInfo("Significant weather  TURB/VA/RC",Sigweather,"red","red"));
  areas.push_back(newEditToolInfo("Significant weather  ICE/TCU/CB",Sigweather,"blue","blue"));
  areas.push_back(newEditToolInfo("Reduced visibility",Genericarea,"gulbrun","gulbrun",0,true,"dash2"));
  areas.push_back(newEditToolInfo("Clouds",Genericarea,"orange","orange:0",0,true,"solid","diagleft"));
  areas.push_back(newEditToolInfo("Ice",Genericarea,"darkYellow","darkYellow:255",0,true,"solid","paralyse"));
  areas.push_back(newEditToolInfo("Black sharp area",Genericarea,"black","black",0,false));
  areas.push_back(newEditToolInfo("Black smooth area",Genericarea,"black","black",0,true));
  areas.push_back(newEditToolInfo("Red sharp area",Genericarea,"red","red",0,false));
  areas.push_back(newEditToolInfo("Red smooth area",Genericarea,"red","red"));
  areas.push_back(newEditToolInfo("Blue sharp area",Genericarea,"blue","blue",0,false));
  areas.push_back(newEditToolInfo("Blue smooth area",Genericarea,"blue","blue"));
  areas.push_back(newEditToolInfo("Black sharp area stipple",Genericarea,"black","black",0,false,"dash2"));
  areas.push_back(newEditToolInfo("Black smooth area stipple",Genericarea,"black","black",0,true,"dash2"));
  areas.push_back(newEditToolInfo("Red sharp area stipple",Genericarea,"red","red",0,false,"dash2"));
  areas.push_back(newEditToolInfo("Red smooth area stipple",Genericarea,"red","red",0,true,"dash2"));
  areas.push_back(newEditToolInfo("Blue sharp area stipple",Genericarea,"blue","blue",0,false,"dash2"));
  areas.push_back(newEditToolInfo("Blue smooth area stipple",Genericarea,"blue","blue",0,true,"dash2"));
  areas.push_back(newEditToolInfo("Generic area",Genericarea,"red","red",0,false,"solid"));
  sigsymbols.push_back(newEditToolInfo("Sig18",1018,"black","black",-1));
#endif
  //arrow

  //sigsymbols.push_back(newEditToolInfo("Sig18",1018,"black","black",-1));
  sigsymbols.push_back(newEditToolInfo("Tekst_1",1000,"black"));
  //Low
  sigsymbols.push_back(newEditToolInfo("Sig19",1019,"black","black",1));
  sigsymbols.push_back(newEditToolInfo("Sig12",1012,"black","black"));
  sigsymbols.push_back(newEditToolInfo("Sig22",1022,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig11",1011,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig3",1003,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig6",1006,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig25",1025,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig14",1014,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig23",1023,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig8",1008,"black"));
  //High
  sigsymbols.push_back(newEditToolInfo("Sig20",1020,"black","black",1));
  sigsymbols.push_back(newEditToolInfo("Sig10",1010,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig16",1016,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig7",1007,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig4",1004,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig5",1005,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig26",1026,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig15",1015,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig24",1024,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig13",1013,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig21",1021,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig9",1009,"black"));
  //all texts have index ending in 0
  sigsymbols.push_back(newEditToolInfo("Tekst_2",2000,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig1",1001,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig2",1002,"black"));

  //new
  //Sea temp, blue circle
  sigsymbols.push_back(newEditToolInfo("Sig27",1027,"black", "blue"));
  //Mean SFC wind, red diamond
  sigsymbols.push_back(newEditToolInfo("Sig28",1028,"black", "red"));
  // Sea state, black flag
  sigsymbols.push_back(newEditToolInfo("Sig29",1029,"black", "black",2));
  // Freezing fog
  sigsymbols.push_back(newEditToolInfo("Sig_fzfg",1030,"gulbrun", "red"));
  //Nuclear
  sigsymbols.push_back(newEditToolInfo("Sig31",1031,"black"));
  //Visibility, black rectangular box
  //  sigsymbols.push_back(newEditToolInfo("Sig33",1033,"black"));
  //Vulcano box
  sigsymbols.push_back(newEditToolInfo("Sig34",1034,"black"));
  //New cross
  sigsymbols.push_back(newEditToolInfo("Sig35",1035,"black"));
  //Freezing level (new)
  sigsymbols.push_back(newEditToolInfo("Sig36",1036,"black", "blue"));
  //BR
  sigsymbols.push_back(newEditToolInfo("Sig_br",1037,"gulbrun"));
  sigsymbols.push_back(newEditToolInfo("Sig38",1038,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig39",1039,"red"));

  sigsymbols.push_back(newEditToolInfo("Clouds",1040,"black"));
  //Fog
  sigsymbols.push_back(newEditToolInfo("Sig_fg",1041,"gulbrun"));
#ifdef SMHI
  //High
  sigsymbols.push_back(newEditToolInfo("Sig20",243,"black","black",1));
  //Low
  sigsymbols.push_back(newEditToolInfo("Sig19",242,"black","black",1));
  //BR
  //sigsymbols.push_back(newEditToolInfo("Sig_br",1037,"gulbrun","gulbrun",2));
  sigsymbols.push_back(newEditToolInfo("Sig_br",39,"gulbrun","gulbrun",2));
  //sigsymbols.push_back(newEditToolInfo("Sig38",1038,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig38",106,"black"));
  sigsymbols.push_back(newEditToolInfo("Sig39",1039,"red","black",1));
  sigsymbols.push_back(newEditToolInfo("Clouds",1040,"black"));
  //Fog
  //sigsymbols.push_back(newEditToolInfo("Sig_fg",1041,"gulbrun","gulbrun",2));
  sigsymbols.push_back(newEditToolInfo("Sig_fg",62,"gulbrun","gulbrun",2));
  //precipitation, green lines
  sigsymbols.push_back(newEditToolInfo("Sig32",1032,"green4"));
  //snow
//ari  sigsymbols.push_back(newEditToolInfo( "Snow",1042,"green4"));
  sigsymbols.push_back(newEditToolInfo( "Snow",254,"green4"));
  //snow showers
  //sigsymbols.push_back(newEditToolInfo( "Snow_showers",1043,"green4","green4",4));
  sigsymbols.push_back(newEditToolInfo( "Snow_showers",114,"green4","green4",4));
  // Freezing fog
  sigsymbols.push_back(newEditToolInfo("Sig_fzfg",1030,"gulbrun", "red"));
  //showers
  //sigsymbols.push_back(newEditToolInfo( "Rainshower",1044,"green4","green4",4));
  sigsymbols.push_back(newEditToolInfo( "Rainshower",109,"green4","green4",4));
  sigsymbols.push_back(newEditToolInfo("Snow_rain",78,"green4","green4",2));
  sigsymbols.push_back(newEditToolInfo("Snow_rain_showers",43,"green4","green4",2));
  sigsymbols.push_back(newEditToolInfo("Drizzle",79,"green4","green4",2));
  sigsymbols.push_back(newEditToolInfo("Freezing_drizzle",83,"red","red",2));
  sigsymbols.push_back(newEditToolInfo("Corn_snow",44,"red","red",2));
  sigsymbols.push_back(newEditToolInfo("Hails",118,"red","red",2));
  sigsymbols.push_back(newEditToolInfo( "Thunderstorm",119,"red","red",2));
  sigsymbols.push_back(newEditToolInfo( "Thunderstorm_hail",122,"red","red",2));
  sigsymbols.push_back(newEditToolInfo("Drifting_snow",46,"black"));
  sigsymbols.push_back(newEditToolInfo("Haze",88,"black"));
  sigsymbols.push_back(newEditToolInfo("Smoke",42,"black"));
  sigsymbols.push_back(newEditToolInfo( "FZRA",93,"red"));
#else
  //precipitation, green lines
  sigsymbols.push_back(newEditToolInfo("Sig32",1032,"green"));
  //snow
  sigsymbols.push_back(newEditToolInfo( "Sig_snow",1042,"green","black",0));
  //snow showers
  sigsymbols.push_back(newEditToolInfo( "Sig_snow_showers",1043,"green","green",2));
  //showers
  sigsymbols.push_back(newEditToolInfo( "Sig_showers",1044,"green","green",2));
#endif
  //Freezing precip
  sigsymbols.push_back(newEditToolInfo( "FZRA",1045,"red"));



  WeatherFront::defineFronts(fronts);
  WeatherSymbol::defineSymbols(symbols);
  WeatherSymbol::defineSymbols(sigsymbols);
  WeatherSymbol::initComplexList();

  WeatherArea::defineAreas(areas);
}


void EditManager::setMapmodeinfo(){
  /* Called when a new edit sessions starts. Defines edit tools used in this
  edit session, which will appear in the edit dialog
  drawtools used for EditProducts are defined in setup-file */
  METLIBS_LOG_SCOPE();

  mapmodeinfo.clear();

  vector<editModeInfo> eMode;
  eMode.push_back(newEditModeInfo("Standard",eToolFieldStandard));
  eMode.push_back(newEditModeInfo("Klasser", eToolFieldClasses));
  eMode.push_back(newEditModeInfo("Tall",    eToolFieldNumbers));

  vector<editModeInfo> dMode;
  int emidx=0;
  map <int,object_modes> objectModes;
  int m=EdProd.drawtools.size();
  for (int i=0;i<m;i++){
    if (EdProd.drawtools[i]==OBJECTS_ANALYSIS){
      dMode.push_back(newEditModeInfo("Fronts",fronts));
      objectModes[emidx++]=front_drawing;
      dMode.push_back(newEditModeInfo("Symbols",symbols));
      objectModes[emidx++]=symbol_drawing;
      dMode.push_back(newEditModeInfo("Areas",areas));
      objectModes[emidx++]=area_drawing;
    }
    if (EdProd.drawtools[i]==OBJECTS_SIGMAPS){
      dMode.push_back(newEditModeInfo("Symbols(SIGWX)",sigsymbols));
      objectModes[emidx++]=symbol_drawing;
    }
  }


  // combine_mode types
  vector<editToolInfo> regionlines;
  regionlines.push_back(newEditToolInfo("regioner",0));
  vector<editToolInfo> regions;
  int n= regnames.size();
  for (int i=0; i<n; i++) {
    int j=i%3;
    std::string colour;
    if      (j==0) colour= "blue";
    else if (j==1) colour= "red";
    else           colour= "darkGreen";
    //all texts have index 0
    regions.push_back(newEditToolInfo( regnames[i],0,colour));
  }


  vector<editModeInfo> cMode;
  int cmidx=0;
  map <int,combine_modes> combineModes;

  cMode.push_back(newEditModeInfo("Regionlinjer",regionlines));
  combineModes[cmidx++]=set_borders;
  cMode.push_back(newEditModeInfo("Regioner",regions));
  combineModes[cmidx++]=set_region;

  mapmodeinfo.push_back(newMapModeInfo("fedit_mode",eMode));
  mapmodeinfo.push_back(newMapModeInfo("draw_mode",dMode));
  mapmodeinfo.push_back(newMapModeInfo("combine_mode",cMode));

  WeatherSymbol::defineRegions(regions);
  EditObjects::defineModes(objectModes,combineModes);
  WeatherSymbol::setStandardSize(EdProd.standardSymbolSize,
      EdProd.complexSymbolSize);
  WeatherFront::setDefaultLineWidth(EdProd.frontLineWidth);
  WeatherArea::setDefaultLineWidth(EdProd.areaLineWidth);
}

bool EditManager::getAnnotations(vector<string>& anno)
{
  for (size_t i=0; i<fedits.size(); i++)
    fedits[i]->getAnnotations(anno);
  return true;
}

const std::string EditManager::insertTime(const std::string& s, const miTime& time) {

  bool english  = false;
  bool norwegian= false;
  std::string es= s;
  if (miutil::contains(es, "$")) {
    if (miutil::contains(es, "$dayeng")) { miutil::replace(es, "$dayeng","%A"); english= true; }
    if (miutil::contains(es, "$daynor")) { miutil::replace(es, "$daynor","%A"); norwegian= true; }
    miutil::replace(es, "$day", "%A");
    miutil::replace(es, "$hour","%H");
    miutil::replace(es, "$min", "%M");
    miutil::replace(es, "$sec", "%S");
    miutil::replace(es, "$auto","$miniclock");
  }
  if (miutil::contains(es, "%")) {
    if (miutil::contains(es, "%anor")) { miutil::replace(es, "%anor","%a"); norwegian= true; }
    if (miutil::contains(es, "%Anor")) { miutil::replace(es, "%Anor","%A"); norwegian= true; }
    if (miutil::contains(es, "%bnor")) { miutil::replace(es, "%bnor","%b"); norwegian= true; }
    if (miutil::contains(es, "%Bnor")) { miutil::replace(es, "%Bnor","%B"); norwegian= true; }
    if (miutil::contains(es, "%aeng")) { miutil::replace(es, "%aeng","%a"); english= true; }
    if (miutil::contains(es, "%Aeng")) { miutil::replace(es, "%Aeng","%A"); english= true; }
    if (miutil::contains(es, "%beng")) { miutil::replace(es, "%beng","%b"); english= true; }
    if (miutil::contains(es, "%Beng")) { miutil::replace(es, "%Beng","%B"); english= true; }
  }

  if ((miutil::contains(es, "%") || miutil::contains(es, "$"))  && !time.undef()) {
    if (norwegian)
      es= time.format(es,"no");
    else if (english)
      es= time.format(es,"en");
  }

  return es;
}

//useful functions not belonging to EditManager

editToolInfo newEditToolInfo(const std::string & newName,
    const int newIndex,
    const std::string & newColour,
    const std::string & newBorderColour,
    const int & newsizeIncrement,
    const bool& newSpline,
    const std::string& newLinetype,
    const std::string& newFilltype){
  editToolInfo eToolInfo;
  eToolInfo.name=  newName;
  eToolInfo.index= newIndex;
  eToolInfo.colour= newColour;
  eToolInfo.borderColour= newBorderColour;
  eToolInfo.sizeIncrement= newsizeIncrement;
  eToolInfo.spline=newSpline;
  eToolInfo.linetype=newLinetype;
  eToolInfo.filltype=newFilltype;
  return eToolInfo;
}

editModeInfo newEditModeInfo(const std::string & newmode,
    const vector <editToolInfo> newtools){
  editModeInfo eModeInfo;
  eModeInfo.editmode=newmode;
  eModeInfo.edittools=newtools;
  return eModeInfo;
}


mapModeInfo newMapModeInfo(const std::string & newmode,
    const vector <editModeInfo> newmodeinfo){
  mapModeInfo mModeInfo;
  mModeInfo.mapmode= newmode;
  mModeInfo.editmodeinfo= newmodeinfo;
  return mModeInfo;
}
