/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "diana_config.h"

#include "diEditManager.h"

#include "diAnnotationPlot.h"
#include "diEventResult.h"
#include "diField/diField.h"
#include "diFieldEdit.h"
#include "diFieldPlot.h"
#include "diFieldPlotCluster.h"
#include "diFieldPlotCommand.h"
#include "diFieldPlotManager.h"
#include "diLabelPlotCommand.h"
#include "diMapMode.h"
#include "diObjectManager.h"
#include "diObsPositions.h"
#include "diPlotCommandFactory.h"
#include "diPlotModule.h"
#include "diStaticPlot.h"
#include "diUndoFront.h"
#include "diUtilities.h"
#include "diWeatherArea.h"
#include "diWeatherFront.h"
#include "diWeatherSymbol.h"
#include "miSetupParser.h"
#include "util/charsets.h"
#include "util/misc_util.h"
#include "util/subprocess.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miDirtools.h>
#include <puTools/miStringFunctions.h>

#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <cmath>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QFile>
#include <QDir>

#define MILOGGER_CATEGORY "diana.EditManager"
#include <miLogger/miLogging.h>

//#define DEBUGPRINT
using namespace miutil;

EditManager::EditManager(PlotModule* pm, ObjectManager* om, FieldPlotManager* fm)
    : plotm(pm)
    , objm(om)
    , fieldPlotManager(fm)
    , mapmode(normal_mode)
    , editpause(false)
    , moved(false)
    , numregs(0)
    , hiddenObjects(false)
    , hiddenCombining(false)
    , hiddenCombineObjects(false)
    , showRegion(-1)
    , producttimedefined(false)
{
  if (plotm==0 || objm==0){
    METLIBS_LOG_WARN("Catastrophic error: plotm or objm == 0");
  }

  initEditTools();
  ObjectPlot::defineTranslations();
}

EditManager::~EditManager()
{
}

bool EditManager::parseSetup()
{
  METLIBS_LOG_SCOPE();

  std::string section="EDIT";
  std::vector<std::string> vstr;

  if (!SetupParser::getSection(section,vstr)){
    METLIBS_LOG_WARN("No " << section << " section in setupfile, ok.");
    return true;
  }

  size_t nv=0;
  const size_t nvstr=vstr.size();
  std::string key,error;
  std::vector<std::string> values, vsub;
  bool ok= true;

  while (ok && nv<nvstr) {

    SetupParser::splitKeyValue(vstr[nv],key,values);

    // yet only products in this setup section...
    EditProduct ep;
    ep.standardSymbolSize=60;
    ep.complexSymbolSize=6;
    ep.frontLineWidth=8;
    ep.areaLineWidth=4;
    ep.startEarly= false;
    ep.startLate=  false;
    ep.minutesStartEarly= 0;
    ep.minutesStartLate=  0;
    ep.combineBorders="./ANAborders.";  // default as old version
    ep.autoremove= -1;

    if (key=="product" && values.size()==1) {
      ep.name= values[0];
      nv++;
    } else {
      ok= false;
    }

    while (ok && nv<nvstr) {

      SetupParser::splitKeyValue(vstr[nv],key,values);
      const size_t nval= values.size();
      if (key=="end.product") {
        nv++;
        break;
      } else if (nval==0) {
        // keywords without any values
        if (key != "grid.minimize")
          ok= false;

      } else if (key=="local_save_dir") {
        diutil::insert_all(ep.local_savedirs, values);

      } else if (key=="prod_save_dir") {
        diutil::insert_all(ep.prod_savedirs, values);

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
        for (size_t j=0; j<nval; j++) {
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
        for (size_t i=0; i<nval; i++) {
          EditProductId pid;
          pid.name= values[i];
          pid.sendable= false;
          pid.combinable= false;
          ep.pids.push_back(pid);
        }

      } else if (key=="prod_idents") {
        for (size_t i=0; i<nval; i++) {
          EditProductId pid;
          pid.name= values[i];
          pid.sendable= true;
          pid.combinable= false;
          ep.pids.push_back(pid);
        }

      } else if (key=="combine_ident" && nval>=3) {
        const size_t n= ep.pids.size();
        size_t i= 0;
        while (i<n && ep.pids[i].name!=values[0]) i++;
        if (i<n) {
          ep.pids[i].combinable= true;
          ep.pids[i].combineids.clear();
          for (size_t j=1; j<nval; j++)
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
      } else if (key=="time_start_early" && nval==1){
        std::vector<std::string> vs = miutil::split(values[0], 0, ":", true);
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
        std::vector<std::string> vs = miutil::split(values[0], 0, ":", true);
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
      if (!ep.local_savedirs.size())
        ep.local_savedirs.push_back(".");
      // insert savedir as the last inputdir and the last combinedir,
      // this sequence is also kept when timesorting
      ep.inputdirs.push_back(ep.local_savedirs[0]);
      ep.combinedirs.push_back(ep.local_savedirs[0]);
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
  METLIBS_LOG_SCOPE();
  // the commands OKstrings to be exectuted when we start an
  // edit session, for the time being called from parseSeup
  // and the OKstrings stored for each product
  std::string s;
  bool merge= false, newmerge;
  int linenum=0;
  std::vector<std::string> tmplines;

  // open filestream
  std::ifstream file(ep.commandFilename.c_str());
  if (!file){
    METLIBS_LOG_ERROR("ERROR OPEN (READ) " << ep.commandFilename);
    return;
  }

  diutil::GetLineConverter convertline("#");
  while (convertline(file,s)){
    linenum++;
    const size_t n = s.length();
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
  ep.labels.clear();
  ep.OKstrings.clear();
  for (std::string s : tmplines) {
    miutil::trim(s);
    if (s.empty())
      continue;
    PlotCommand_cp cmd = makeCommand(s);
    if (cmd->commandKey()=="LABEL")
      ep.labels.push_back(cmd);
    else
      ep.OKstrings.push_back(cmd);
  }
  METLIBS_LOG_DEBUG("++ EditManager::readCommandFile start reading --------");
  for (PlotCommand_cp cmd : ep.labels)
    METLIBS_LOG_DEBUG("   " << cmd->toString());
  METLIBS_LOG_DEBUG("++ EditManager::readCommandFile finish reading ------------");
}

/*----------------------------------------------------------------------
----------------------------  edit Dialog methods ----------------------
 -----------------------------------------------------------------------*/


EditDialogInfo EditManager::getEditDialogInfo()
{
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
    editmode = 0;
    return;
  }

  int n= mapmodeinfo.size();
  int mmidx=0;
  while (mmidx<n && mmode!=mapmodeinfo[mmidx].mapmode) mmidx++;
  if (mmidx==n){
    METLIBS_LOG_ERROR("diEditManager::setEditMode  no info for mapmode:"
        << mmode);
    editmode = 0;
    return;
  }

  n= mapmodeinfo[mmidx].editmodeinfo.size();
  if (n==0){ // no defined modes or tools for this mapmode
    editmode = 0;
    return;
  }
  int emidx=0;
  while (emidx<n &&
      emode!=mapmodeinfo[mmidx].editmodeinfo[emidx].editmode) emidx++;
  if (emidx==n){
    METLIBS_LOG_ERROR("diEditManager::setEditMode  unknown editmode:" << emode);
    editmode = 0;
    return;
  }
  editmode= emidx;

  if (modeChange) showAllObjects();

  objm->setEditMode(mapmode, editmode, etool);
}

/*----------------------------------------------------------------------
----------------------------  end of edit Dialog methods ----------------
 -----------------------------------------------------------------------*/


/*----------------------------------------------------------------------
----------------------------  keyboard/mouse event ----------------------
 -----------------------------------------------------------------------*/

namespace {
inline bool matchButton(const QMouseEvent* me, Qt::MouseButton button)
{
  if (me->type() == QEvent::MouseMove)
    return (me->buttons() & button);
  else if (me->type() == QEvent::MouseButtonPress)
    return (me->button() == button);
  else
    return false;
}
} // namespace

bool EditManager::sendMouseEvent(QMouseEvent* me, EventResult& res)
{
  //  METLIBS_LOG_SCOPE();
  res.enable_background_buffer = true;
  res.update_background_buffer = false;
  res.repaint= false;
  res.newcursor= edit_cursor;

  if (showRegion>=0 && mapmode!=combine_mode)
    return false;

  plotm->PhysToMap(me->x(),me->y(),newx,newy);
  objm->getEditObjects().setMouseCoordinates(newx,newy);

  if (mapmode == fedit_mode) { // field editing
    const bool m_move = (me->type() == QEvent::MouseMove), m_press = (me->type() == QEvent::MouseButtonPress);
    if (m_move && me->buttons() == Qt::NoButton) {
      res.newcursor = edit_value_cursor;
      res.action = browsing;
      return true;
    }
    EditEvent ee(newx, newy);
    if (m_move || m_press) {
      if (m_press) {
        ee.order = start_event;
      } else {
        res.action = quick_browsing;
        ee.order = normal_event;
      }
      if (matchButton(me, Qt::LeftButton)) {
        ee.type = edit_pos;
      } else if (matchButton(me, Qt::MidButton)) {
        ee.type = edit_inspection;
      } else if (matchButton(me, Qt::RightButton)) {
        ee.type = edit_size;
      } else {
        return false;
      }
      res.repaint = notifyEditEvent(ee);
    } else if (me->type() == QEvent::MouseButtonRelease) {
      if (me->button() == Qt::LeftButton) {
        ee.type = edit_pos;
        ee.order = stop_event;
        res.repaint = notifyEditEvent(ee);
        if (res.repaint)
          res.action = fields_changed;
      } else {
        return false;
      }
    } else if (me->type() == QEvent::MouseButtonDblClick) {
      if (me->button() == Qt::LeftButton) {
        ee.type = edit_pos; // ..type edit_pos
        ee.order = start_event;
        res.repaint = notifyEditEvent(ee);
      } else {
        return false;
      }
    }
  } else { // draw_mode or combine mode
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
      } else {
        return false;
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
      } else {
        return false;
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
    } else {
      return false;
    }
  }

  return true;
}

bool EditManager::sendKeyboardEvent(QKeyEvent* ke, EventResult& res)
{
  //  METLIBS_LOG_SCOPE();
  res.enable_background_buffer = true;
  res.update_background_buffer = false;
  res.repaint= false;

  // numregs==0 if not combine (and then edit)
  const int keyRegMin = Qt::Key_1;
  const int keyRegMax = std::min((Qt::Key)(Qt::Key_0 + numregs), Qt::Key_9);

  const bool isKeyReg = (ke->key() >= keyRegMin && ke->key() <= keyRegMax);
  if (showRegion>=0 && mapmode!=combine_mode) {
    if (ke->type() != QEvent::KeyPress || !isKeyReg)
      return false;
  }

  if (ke->type() == QEvent::KeyPress){
    if (ke->key() == Qt::Key_Shift) {
      res.newcursor= normal_cursor; // show user possible mode-change
      return true;
    }
    bool handled = true;
    if (ke->key() == Qt::Key_G) {
      //hide/show all objects.
      if (hiddenObjects || hiddenCombineObjects) {
        objm->editUnHideAll();
        hiddenObjects= false;
      } else {
        objm->editHideAll();
        hiddenObjects= true;
      }
      hiddenCombineObjects= false;
      res.update_background_buffer = (mapmode==fedit_mode);
      setEditMessage("");
    }
    else if (ke->key() == Qt::Key_L) {
      //hide/show all combining objects.
      if (hiddenCombining)
        objm->editUnHideCombining();
      else
        objm->editHideCombining();
      hiddenCombining= !hiddenCombining;
      res.update_background_buffer = true;
    } else if (isKeyReg) {
      // show objects and fields from one region (during and after combine)
      int reg= ke->key() - keyRegMin;
      if (reg!=showRegion) {
        showRegion= reg;
        setEditMessage(regnames[showRegion]);
      } else {
        showRegion= -1;
        setEditMessage(std::string());
      }
      res.update_background_buffer = true;
    } else {
      handled = false;
    }
    if (handled) {
      res.repaint = true;
      return true;
    }
  }

  if (mapmode!= fedit_mode){
    // first keypress events
    bool handled = false;
    if (ke->type() == QEvent::KeyPress){
      handled = true;
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
      else
        handled = false;
    }
    if (handled) {
      if (mapmode == combine_mode) {
        objm->setAllPassive();
        if (objm->toDoCombine())
          editCombine();
      } else { // OK?
        if (objm->haveObjectsChanged())
          res.action = objects_changed;
      }
      res.repaint = true;
      return true;
    }
  }

  // then key release events
  if (ke->type() == QEvent::KeyRelease) {
    if (ke->key() == Qt::Key_Shift) { // reset cursor
      if (mapmode != fedit_mode) {
        res.newcursor = (objm->inDrawing() ? draw_cursor : edit_cursor);
      } else {                            // field-editing
        res.newcursor = edit_value_cursor;
      }
      return true;
    }
  }

  return false;
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
  std::unique_ptr<FieldEdit> fed(new FieldEdit(fieldPlotManager));
  return fed->notifyEditEvent(ee);
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


bool EditManager::getProductTime(miTime& t) const
{
  //METLIBS_LOG_SCOPE();
  if (producttimedefined) {
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


void EditManager::saveProductLabels(const PlotCommand_cpv& labels)
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

  std::ostringstream ostr;
  ostr << std::setw(4) << std::setfill('0') << yyyy << std::setw(2) << std::setfill('0') << mm << std::setw(2) << std::setfill('0') << dd << std::setw(2)
       << std::setfill('0') << hh << std::setw(2) << std::setfill('0') << min;

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
      time_string = "_" + time.format("%Y%m%dt%H%M%S", "", true);
    }

    if (ci.sendable ) {
      for (size_t i=0; i<ep.prod_savedirs.size();++i) {
        outputFilename = ep.prod_savedirs[i] + "/work/";
        std::string filename = ci.name + "_" + ep.fields[j].filenamePart + time_string + ".nc";
        outputFilename += filename;
        QString qs(outputFilename.c_str());
	if ( QFile::exists(qs) ) {
          message = qs + " already exists, do you want to continue?";
          return false;
        }
      }
    }

    for (size_t i=0; i<ep.local_savedirs.size();++i) {
      outputFilename = ep.local_savedirs[i] + "/";
      std::string filename = ci.name + "_" + ep.fields[j].filenamePart + time_string + ".nc";
      outputFilename += filename;
      QString qs(outputFilename.c_str());
      if ( QFile::exists(qs) ) {
        message = qs + " already exists, do you want to continue?";
        return false;
      }
    }

  }


  if (ci.sendable ) {
    for (size_t i=0; i<ep.prod_savedirs.size();++i) {
      std::string outputFilename = ep.prod_savedirs[i] + "/work/";
      std::string objectsFilename= editFileName(outputFilename,ci.name,
          ep.objectsFilenamePart,time);
      QString qstr(objectsFilename.c_str());
      if ( QFile::exists(qstr) ) {
        message = qstr + " already exists, do you want to continue?";
        return false;
      }
    }
  }
  for (size_t i=0; i<ep.local_savedirs.size();++i) {
    std::string objectsFilename= editFileName(ep.local_savedirs[i],ci.name,
        ep.objectsFilenamePart,time);
    QString qstr(objectsFilename.c_str());
    if ( QFile::exists(qstr) ) {
      message = qstr + " already exists, do you want to continue?";
      return false;
    }
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

  std::vector<std::string> outputFilenames;

  std::string time_string;
  if ( producttimedefined )
    time_string= "_" + producttime.format("%Y%m%dt%H%M%S", "", true);
  EdProd.fields[fnum].filename = EdProdId.name + "_" + EdProd.fields[fnum].filenamePart + time_string + ".nc";

  if ( local ) {
    for (size_t i=0; i<EdProd.local_savedirs.size();++i) {
      outputFilenames.push_back(EdProd.local_savedirs[i] + "/" + EdProd.fields[fnum].filename);
    }
  } else {
    for (size_t i=0; i<EdProd.prod_savedirs.size();++i) {
      METLIBS_LOG_INFO(LOGVAL(EdProd.prod_savedirs[i]));
      QDir qdir(EdProd.prod_savedirs[i].c_str());
      if ( !qdir.exists() ) {
        if ( !qdir.mkpath(EdProd.prod_savedirs[i].c_str())) {
          METLIBS_LOG_WARN("could not make:" <<EdProd.prod_savedirs[i]);
        }
      }
      if ( !qdir.mkpath("work")) {
        METLIBS_LOG_WARN("could not make dir:" <<EdProd.prod_savedirs[i]<<"work");
      }
      if ( !qdir.mkpath("products")) {
        METLIBS_LOG_WARN("could not make dir:" <<EdProd.prod_savedirs[i]<<"products");
      }

      outputFilenames.push_back(EdProd.prod_savedirs[i] + "/work/" + EdProd.fields[fnum].filename);
    }
  }

  if ( local ) {
    EdProd.fields[fnum].localFilename = outputFilenames;
  } else {
    EdProd.fields[fnum].prodFilename = outputFilenames;
  }

  //  outputFilename += EdProd.fields[fnum].filename;
  for (const std::string& ofi : outputFilenames) {
    const QString qofi = QString::fromStdString(ofi);
    const QString qtfi = QString::fromStdString(EdProd.templateFilename);
    if (QFile::exists(qofi) && !QFile::remove(qofi)) {
      METLIBS_LOG_WARN("Copy from " << qtfi.toStdString() << " to " << qofi.toStdString() << "  failed. (File exsists, but can't be overwritten.)");
    }
    if (!qfile.copy(qofi)) {
      METLIBS_LOG_WARN("Copy from " << qtfi.toStdString() << " to " << qofi.toStdString() << "  failed");
    }

    const std::string modelName = ofi;
    fieldPlotManager->addGridCollection(modelName, ofi, true);

    FieldPlotGroupInfo_v fgi;
    std::string reftime = fieldPlotManager->getBestFieldReferenceTime(modelName, 0, -1);
    METLIBS_LOG_INFO(LOGVAL(modelName) << LOGVAL(reftime));
    fieldPlotManager->getFieldPlotGroups(modelName, reftime, true, fgi);
  }
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

  removeEditFiles(ep);

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
    std::unique_ptr<FieldEdit> fed(new FieldEdit(fieldPlotManager));
    fed->setSpec(EdProd, j);

    const std::string& fieldname= EdProd.fields[j].name;

    if (EdProd.fields[j].fromfield) {

      // edit field from existing field, find correct fieldplot

      const std::vector<FieldPlot*> vfp = plotm->fieldplots()->getFieldPlots();
      Field_pv vf;
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

    } else {

      // get field from a saved product
      std::string filename = EdProd.fields[j].fromprod.filename;
      METLIBS_LOG_DEBUG("filename for saved field file to open:" << filename);
      if(!fed->readEditFieldFile(filename, fieldname, producttime))
        return false;

    }
    fedits.push_back(fed.release());

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
  for (PlotCommand_cp& lc : EdProd.labels)
    lc = insertTime(lc, valid);
  //Merge labels from EdProd  with object label input strings
  plotm->updateEditLabels(EdProd.labels,EdProd.name,newProduct);
  //save merged labels in editobjects
  PlotCommand_cpv labels = plotm->writeAnnotations(EdProd.name);
  saveProductLabels(labels);
  objm->getEditObjects().labelsAreSaved();

  if (!fedits.empty())
    fedits[0]->activate();

  return true;
}

void EditManager::removeEditFiles(const EditProduct& ep)
{
  METLIBS_LOG_SCOPE(LOGVAL(ep.name));

  if (ep.autoremove <= 0)
    return;

  std::string filenamePart;
  int n = ep.fields.size();
  for (int i = -1; i < n; i++) {
    if (i == -1)
      filenamePart = "/*_" + ep.objectsFilenamePart + "*";
    else
      filenamePart = "/*_" + ep.fields[i].filenamePart + "*";

    // remove old files from the production directories (work and products)
    for (const std::string& dir : ep.prod_savedirs) {
      std::string fileString = dir + "/work" + filenamePart;
      removeFiles(fileString, ep.autoremove);
      fileString = dir + "/products" + filenamePart;
      removeFiles(fileString, ep.autoremove);
    }

    // remove old files from the local directory
    for (const std::string& dir : ep.local_savedirs) {
      std::string fileString = dir + filenamePart;
      removeFiles(fileString, ep.autoremove);
    }
  }
}

void EditManager::removeFiles(const std::string& fileString, int autoremove)
{
  miTime now = miTime::nowTime();

  for (const std::string& name : diutil::glob(fileString)) {
    miTime ptime = objm->timeFileName(name);
    if (autoremove > 0 && !ptime.undef() && miTime::hourDiff(now, ptime) > autoremove) {
      METLIBS_LOG_DEBUG("Removing file: " << name);
      QFile::remove(name.c_str());
    }
  }
}

void EditManager::writeEditProduct(QString& message, const bool wfield, const bool wobjects, const bool send, const bool isapproved)
{
  METLIBS_LOG_SCOPE();

  message.clear();

  for (size_t i=0; i<EdProd.local_savedirs.size();++i) {
  QDir qdir(EdProd.local_savedirs[i].c_str());
    if ( !qdir.exists() ) {
      if ( !qdir.mkpath(EdProd.local_savedirs[i].c_str())) {
        METLIBS_LOG_WARN("could not make:" <<EdProd.local_savedirs[i]);
      }
    }
  }
  if( EdProdId.sendable && send ) {
    for (size_t i=0; i<EdProd.prod_savedirs.size();++i) {
      QDir qdir(EdProd.prod_savedirs[i].c_str());
      if ( !qdir.exists() ) {
        if ( !qdir.mkpath(EdProd.prod_savedirs[i].c_str())) {
          METLIBS_LOG_WARN("could not make:" <<EdProd.prod_savedirs[i]);
        }
      }
      if ( !qdir.mkpath("work")) {
        METLIBS_LOG_WARN("could not make dir:" <<EdProd.prod_savedirs[i]<<"/work");
      }
      if ( !qdir.mkpath("products")) {
        METLIBS_LOG_WARN("could not make dir:" <<EdProd.prod_savedirs[i]<<"/products");
      }
    }
  }

  miTime t = producttime;

  if (wfield) {
    for (unsigned int i=0; i<fedits.size(); i++) {
      for (size_t j=0; j<EdProd.fields[i].localFilename.size();++j) {
        if(fedits[i]->writeEditFieldFile(EdProd.fields[i].localFilename[j]) ) {
          METLIBS_LOG_INFO("Writing field:" << EdProd.fields[i].localFilename[j]);
        } else {
          message += QString("Could not write field to file:") + QString(EdProd.fields[i].localFilename[j].c_str()) + "\n\n";
        }
      }
      if(EdProdId.sendable ) {
        for (size_t j=0; j<EdProd.fields[i].prodFilename.size();++j) {
          if ( send ) {
            if(fedits[i]->writeEditFieldFile(EdProd.fields[i].prodFilename[j]) ) {
              METLIBS_LOG_INFO("Writing field:" << EdProd.fields[i].prodFilename[j]);
            } else {
              message += "Could not write field to file:" + QString(EdProd.fields[i].prodFilename[j].c_str()) + "\n\n";
            }
          }
          if( isapproved ) {
            QString workFile = EdProd.fields[i].prodFilename[j].c_str();
            QFile qfile(workFile);
            QString prodFile = workFile.replace("work","products");
            METLIBS_LOG_INFO("Writing field:" << prodFile.toStdString());
            if ( QFile::exists(prodFile) && !QFile::remove(prodFile) ) {
              METLIBS_LOG_WARN("Could not save file: "<<prodFile.toStdString()<<"(File already exists and could not be removed)");
              message += "Could not write field to file:" + prodFile + "\n\n";
            } else if (!qfile.copy(prodFile) ) {
              METLIBS_LOG_WARN("Could not copy file: "<<prodFile.toStdString());
              makeNewFile(i,false,message);
              message += "Could not write field to file:" + prodFile + "\n\n";
            }
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
      for (size_t i=0; i<EdProd.local_savedirs.size();++i) {
        objectsFilename= editFileName(EdProd.local_savedirs[i],EdProdId.name,
            objectsFilenamePart,t);
 	if( QFile::exists(QString(objectsFilename.c_str())) && !QFile::remove(QString(objectsFilename.c_str())))
	      METLIBS_LOG_WARN("Could not save file: "<<objectsFilename<<" (File already exists and could not be removed)");
        if (!objm->writeEditDrawFile(objectsFilename,editObjectsString)){
          saveok= false;
          message += "Could not write objects to file:" + QString(objectsFilename.c_str()) + "\n\n";
        }
      }

      if (EdProdId.sendable ) {
        for (size_t i=0; i<EdProd.prod_savedirs.size();++i) {
          if ( send ) {
            objectsFilename = editFileName(EdProd.prod_savedirs[i] + "/work",EdProdId.name,
                objectsFilenamePart,t);
	    if( QFile::exists(QString(objectsFilename.c_str())) && !QFile::remove(QString(objectsFilename.c_str())))
	      METLIBS_LOG_WARN("Could not save file: "<<objectsFilename<<" (File already exists and could not be removed)");
	    if (!objm->writeEditDrawFile(objectsFilename,editObjectsString)){
              saveok= false;
              message += "Could not write objects to file:" + QString(objectsFilename.c_str()) + "\n\n";
            }
          }


          if( isapproved ) {
            QString workFile = objectsFilename.c_str();
            QFile qfile(workFile);
            QString prodFile = workFile.replace("work","products");
	    if ( QFile::exists(prodFile) && !QFile::remove(prodFile) ) {
              METLIBS_LOG_WARN("Could not save file: "<<prodFile.toStdString()<<"(File already exists and could not be removed)");
              message += "Could not write objects to file:" + prodFile + "\n";
            } else if (!qfile.copy(prodFile) ) {
              METLIBS_LOG_WARN("Could not copy file: "<<prodFile.toStdString());
              message += "Could not write objects to file:" +  prodFile+ "\n";
            }
          }
        }

      }


    }
    objm->setObjectsSaved(saveok);
    if (saveok) objm->getEditObjects().labelsAreSaved();

    if ( objm->getEditObjects().hasComments() ) {
      bool saveok= true;
      //get comment string from objm to put in database and local files
      //only do this if comments have changed !

      std::string commentFilename, commentFilenamePart,editCommentString;
      editCommentString = objm->getComments();

      if (not editCommentString.empty()) {
        //first save to local file
        commentFilenamePart= EdProd.commentFilenamePart;
        for (size_t i=0; i<EdProd.local_savedirs.size();++i) {
          commentFilename= editFileName(EdProd.local_savedirs[i],EdProdId.name,
              commentFilenamePart,t);

          if (!objm->writeEditDrawFile(commentFilename,editCommentString)){
            saveok= false;
            message += "Could not write comments to file:" + QString(commentFilename.c_str()) + "\n";
          }
        }
        if (EdProdId.sendable ) {
          if ( send ) {
            for (size_t i=0; i<EdProd.prod_savedirs.size();++i) {

              commentFilename= editFileName(EdProd.prod_savedirs[i] + "/work",EdProdId.name,
                  commentFilenamePart,t);

              if (!objm->writeEditDrawFile(commentFilename,editCommentString)){
                saveok= false;
                message += "Could not write comments to file:" + QString(commentFilename.c_str()) + "\n";
              }
            }
          }
          if( isapproved ) {
            QString workFile = commentFilename.c_str();
            QFile qfile(workFile);
            QString prodFile = workFile.replace("work","products");
	    if ( QFile::exists(prodFile) && !QFile::remove(prodFile) ) {
              METLIBS_LOG_WARN("Could not save file: "<<prodFile.toStdString()<<"(File already exists and could not be removed)");
              message += "Could not write field to file:" + prodFile + "\n";
            }

            if (!qfile.copy(prodFile) ) {
              METLIBS_LOG_WARN("Could not copy file: "<<prodFile.toStdString());
              message += "Could not write field to file:" +  prodFile+ "\n";
            }
          }
        }
      }
      if (saveok) objm->getEditObjects().commentsAreSaved();
    }


    if ( EdProdId.sendable ) {
      std::string text =t.isoTime("t") + "\n" +miutil::miTime::nowTime().isoTime("t");
      if( send ) {
        for (size_t i=0; i<EdProd.prod_savedirs.size();++i) {
          std::string filename = EdProd.prod_savedirs[i] + "/lastsaved." + EdProdId.name;
          QFile lastsaved(filename.c_str());
          lastsaved.open(QIODevice::WriteOnly);
          lastsaved.write(text.c_str());
          lastsaved.close();
        }
      }
      if ( isapproved ) {
        for (size_t i=0; i<EdProd.prod_savedirs.size();++i) {
          std::string filename = EdProd.prod_savedirs[i] + "/lastfinished." + EdProdId.name;
          QFile lastfinnished(filename.c_str());
          lastfinnished.open(QIODevice::WriteOnly);
          lastfinnished.write(text.c_str());
          lastfinnished.close();
        }
      }
    }
  }
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

std::vector<savedProduct> EditManager::getSavedProducts(const EditProduct& ep, std::string fieldname)
{
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

std::vector<savedProduct> EditManager::getSavedProducts(const EditProduct& ep, int element)
{
  std::vector<savedProduct> prods;

  std::string fileString,filenamePart, dir,pid;

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
    findSavedProducts(prods, fileString, (i == n - 1), element);
  }

  //give correct product name
  int m = prods.size();
  for (int i = 0;i<m;i++)
    prods[i].productName=ep.name;

  return prods;
}

std::vector<miTime> EditManager::getCombineProducts(const EditProduct& ep, const EditProductId& ei)
{
  METLIBS_LOG_DEBUG("getCombineProducts");

  std::vector<miTime> ctime;

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
    METLIBS_LOG_DEBUG("Looking in directory " << dir);
    for (int j=-1; j<numfields; j++) {
      if (j == -1) filenamePart= ep.objectsFilenamePart;
      else         filenamePart= ep.fields[j].filenamePart;
      fileString = dir + "/" + pid + "_" + filenamePart+ "*";
      METLIBS_LOG_DEBUG("    find " << fileString);
      findSavedProducts(combineprods, fileString, false, j);
    }
  }
  dir = ep.local_savedirs[0];
  METLIBS_LOG_DEBUG("Looking in directory " << dir);
  for (int j=-1; j<numfields; j++) {
    if (j == -1) filenamePart= ep.objectsFilenamePart;
    else         filenamePart= ep.fields[j].filenamePart;
    fileString = dir + "/" + pid + "_" + filenamePart+ "*";
    METLIBS_LOG_DEBUG("    find " << fileString);
    findSavedProducts(combineprods, fileString, true, j);
  }
  // combineprods are timesorted, newest first
  int ncp= combineprods.size();

  if (ncp==0) return ctime;

  // some test on combinations (pids and objects/fields)
  // before adding legal time

  std::vector<std::string> okpids;

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

std::vector<std::string> EditManager::findAcceptedCombine(int ibegin, int iend, const EditProduct& ep, const EditProductId& ei)
{

  METLIBS_LOG_SCOPE();

  std::vector<std::string> okpids;

  // a "table" of found pids and object/fields
  int nf= ep.fields.size();
  int mf= 1 + nf;
  int np= ei.combineids.size();
  int j,ip;

  std::unique_ptr<bool[]> table(new bool[mf * np]);
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

  // and finally: one pid is nothing to combine!
  if (okpids.size()==1) okpids.clear();

  return okpids;
}

void EditManager::findSavedProducts(std::vector<savedProduct>& prods, const std::string fileString, bool localSource, int element)
{

  diutil::string_v matches = diutil::glob(fileString);
  for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
    const std::string& name = *it;
    METLIBS_LOG_DEBUG("Found a file " << name);
    savedProduct savedprod;
    savedprod.ptime= objm->timeFileName(name);
    savedprod.pid= objm->prefixFileName(name);
    savedprod.filename = name;
    savedprod.localSource = localSource;
    savedprod.element= element;
    // sort files with the newest files first !
    if (prods.empty()) {
      prods.push_back(savedprod);
    } else {
      std::vector<savedProduct>::iterator p = prods.begin();
      // test >= to keep directory sequence for each time too
      while (p!=prods.end() && p->ptime>=savedprod.ptime) p++;
      prods.insert(p,savedprod);
    }
  }

}

std::vector<std::string> EditManager::getValidEditFields(const EditProduct& ep, const int element)
{
  METLIBS_LOG_SCOPE();

  // return names of existing fields valid for editing
  std::vector<std::string> vstr;
  std::string fname= miutil::to_lower(ep.fields[element].name);
  const std::vector<FieldPlot*> vfp = plotm->fieldplots()->getFieldPlots();
  for (size_t i=0; i<vfp.size(); i++){
    Field_pv vf = vfp[i]->getFields();
    // only accept scalar fields
    if (vf.size() == 1) {
      std::string s= miutil::to_lower(vf[0]->name);
      if (s == fname) {
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

  if (!isInEdit())
    return;

  cleanCombineData(true);

  diutil::delete_all_and_clear(fedits);
  plotm->deleteAllEditAnnotations();
  objm->getEditObjects().clear();
  objm->getCombiningObjects().clear();

  objm->setObjectsSaved(true);
  objm->undofrontClear();
}

std::vector<std::string> EditManager::getEditProductNames()
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> names;
  for ( size_t i = 0; i<editproducts.size(); ++i ) {
    names.push_back(editproducts[i].name);
  }
  return names;
}

std::vector<EditProduct> EditManager::getEditProducts()
{
  return editproducts;
}

std::string EditManager::savedProductString(const savedProduct& sp)
{
  std::string str = sp.pid
      + " " + sp.productName;
  if (!sp.ptime.undef())
    str += " " + sp.ptime.isoTime();
  str += " types=" + sp.selectObjectTypes
        + " " + sp.filename;
  return str;
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
  std::vector<ObjectPlot*>::iterator p = objm->getCombiningObjects().objects.begin();
  while (p!=objm->getCombiningObjects().objects.end()){
    ObjectPlot * pobject = *p;
    if (!pobject->objectIs(Border)){
      p = objm->getCombiningObjects().objects.erase(p);
      delete pobject;
    }
    else p++;
  }

}

std::vector<std::string> EditManager::getCombineIds(const miTime& valid, const EditProduct& ep, const EditProductId& ei)
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> pids;
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

bool EditManager::startCombineEdit(const EditProduct& ep, const EditProductId& ei, const miTime& valid, std::vector<std::string>& pids, QString& message)
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
  if (!objm->getCombiningObjects().readAreaBorders(filename)) {
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
        std::unique_ptr<FieldEdit> fed(new FieldEdit(fieldPlotManager));
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
            gridResolutionX = fed->areaspec.resolutionX;
            gridResolutionY = fed->areaspec.resolutionY;
          } else if (nx!=matrix_nx || ny!=matrix_ny
              || gridResolutionX != fed->areaspec.resolutionX
              || gridResolutionY != fed->areaspec.resolutionY) {
            ok= false;
          }
        } else {
          ok= false;
        }
        if (ok)
          combinefields[j].push_back(fed.release());
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
    std::unique_ptr<FieldEdit> fed(new FieldEdit(fieldPlotManager));
    *(fed)= *(combinefields[j][0]);
    fed->setConstantValue(fieldUndef);
    fedits.push_back(fed.release());
    if (EdProdId.sendable) {
      //make new prd file from template
      if (!makeNewFile(j, false, message)){
        return false;
      }
    }

  }

  objm->getCombiningObjects().switchProjection(newarea);

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
      objm->readEditDrawFile(filename, wo);
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

  long fsize= matrix_nx*matrix_ny;
  combinematrix.reset(new int[fsize]);
  for (int i=0; i<fsize; i++)
    combinematrix[i]= -1;


  // set correct time for labels
  for (PlotCommand_cp& lc : EdProd.labels)
    lc = insertTime(lc, valid);
  //Merge labels from EdProd  with object label input strings
  plotm->updateEditLabels(EdProd.labels,EdProd.name,true);
  //save merged labels in editobjects
  PlotCommand_cpv labels = plotm->writeAnnotations(EdProd.name);
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
  objm->getEditObjects().switchProjection(newarea);
  objm->getCombiningObjects().switchProjection(newarea);

  // projection may have been changed when showing single region data
  for (int i=0; i<numregs; i++)
    combineobjects[i].switchProjection(newarea);

  std::unique_ptr<int[]> regindex(new int[nparts]);

  for (int i=0; i<nparts; i++)
    regindex[i]=-1;

  bool error= false;

  // check each region textstring's position
  // allow duplicates and avoid the impossible
  for (int j=0; j<cosize; j++){
    ObjectPlot * pobject = objm->getCombiningObjects().objects[j];
    if (!pobject->objectIs(RegionName)) continue;
    int idxold = pobject->combIndex(matrix_nx, matrix_ny, gridResolutionX, gridResolutionY, combinematrix.get());
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
    // convert all objects to map area again
    objm->getCombiningObjects().switchProjection(plotm->getMapArea());
    objm->getEditObjects().switchProjection(plotm->getMapArea());

    return false;
  }

  //update combinematrix with correct region indices
  for (int i=0; i<fsize; i++)
    combinematrix[i]= regindex[combinematrix[i]];

  //loop over regions , VA ,VV, VNN
  for (int i=0; i<numregs; i++){

    //loop over objects
    int obsize = combineobjects[i].objects.size();
    for (int j = 0;j<obsize;j++){
      ObjectPlot * pobject = combineobjects[i].objects[j];
      if (pobject->isInRegion(i, matrix_nx, matrix_ny, gridResolutionX, gridResolutionY, combinematrix.get())) {
        std::unique_ptr<ObjectPlot> newobject;
        if (pobject->objectIs(wFront))
          newobject.reset(new WeatherFront(*((WeatherFront*)(pobject))));
        else if (pobject->objectIs(wSymbol))
          newobject.reset(new WeatherSymbol(*((WeatherSymbol*)(pobject))));
        else if (pobject->objectIs(wArea))
          newobject.reset(new WeatherArea(*((WeatherArea*)(pobject))));
        if (newobject)
          objm->getEditObjects().objects.push_back(newobject.release());
      }
    }
  }

  // previous settings in editobjects lost above
  if (hiddenObjects)
    objm->editHideAll();
  else if (hiddenCombineObjects)
    objm->editHideCombineObjects(showRegion);

  // then convert all objects to map area again
  objm->getCombiningObjects().switchProjection(plotm->getMapArea());
  objm->getEditObjects().switchProjection(plotm->getMapArea());

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

  PlotCommand_cpv labels = plotm->writeAnnotations(EdProd.name);
  saveProductLabels(labels);
  objm->getEditObjects().labelsAreSaved();

  cleanCombineData(false);

  //delete combinematrix
  if (matrix_nx > 0 && matrix_ny > 0 && combinematrix) {
    matrix_nx= 0;
    matrix_ny= 0;
    combinematrix.reset(0);
  }

  if (!fieldsCombined) {
    for (unsigned int j=0; j<fedits.size(); j++)
      fedits[j]->setConstantValue(fieldUndef);
  }

  // always matching dialog ???????????
  if (!fedits.empty())
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

  std::unique_ptr<bool[]> mark(new bool[size]);

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

  std::unique_ptr<int[]> xdir(new int[n]);
  std::unique_ptr<int[]> ydir(new int[n]);
  std::unique_ptr<int[]> xdircp(new int[n]);
  std::unique_ptr<int[]> ydircp(new int[n]);

  std::unique_ptr<float[]> f2(new float[size]);

  if (!xdir || !ydir || !xdircp || !ydircp || !f2) {
    METLIBS_LOG_ERROR("ERROR: OUT OF MEMORY in combineFields !!!!!!!! NOT ABLE TO SMOOTH the result field !!!");
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

  return true;
}


struct polygon{
  std::vector<float> x;
  std::vector<float> y;
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
  std::unique_ptr<int[]> numv(new int[cosize]);
  std::unique_ptr<int[]> startv(new int[cosize]);
  for (int i=0; i<cosize; i++){
    //check if objects are borders
    if (objm->getCombiningObjects().objects[i]->objectIs(Border)){
      numv[n]= objm->getCombiningObjects().objects[i]->getXYZsize();
      startv[n]= npos;
      npos+= numv[n];
      n++;
    }
  }

  std::unique_ptr<float[]> xposis(new float[npos]);
  std::unique_ptr<float[]> yposis(new float[npos]);

  int nborders = 0;
  int nposition=0;
  for (int i=0; i<cosize; i++){
    if (objm->getCombiningObjects().objects[i]->objectIs(Border)){
      for (int j=0; j<numv[nborders]; j++) {
        const XY& xyborder = objm->getCombiningObjects().objects[i]->getXY(j);
        xposis[nposition]=xyborder.x();
        yposis[nposition]=xyborder.y();
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
  if (!newArea.P().convertPoints(oldArea.P(), npos, xposis.get(), yposis.get())) {
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
  std::unique_ptr<float[]> xpos(new float[m]);
  std::unique_ptr<float[]> ypos(new float[m]);
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

  // find crossing with fieldarea
  std::vector<int> crossp, quadr; // 0=Down, 1=Left, 2=Up, 3=Right
  std::vector<float> crossx, crossy;
  const Rectangle& r = fedits[0]->editfield->area.R();
  const Rectangle rex = diutil::extendedRectangle(r, 0.5 * gridResolutionX);

  bool crossing;

  for (int i=0; i<nborders; i++){
    crossp.push_back(-1);
    quadr.push_back(-1);
    crossx.push_back(-1);
    crossy.push_back(-1);
    crossing = false;
    for (int j=startv[i]; j<numv[i]+startv[i]; j++){
      if (!rex.isinside(xpos[j], ypos[j])) {
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
      if (miutil::absval2(dx1, dy1) < miutil::absval2(dx2, dy2))
        nc = 1;
    }
    nc--;
    crossx[i]= xc[nc];
    crossy[i]= yc[nc];
    quadr[i] = cquadr[nc];
  }

  // sort legs counter-clockwise
  std::vector<int> legorder;
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

  std::unique_ptr<int[]> mark(new int[fsize]);

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

  return true;
}



/*----------------------------------------------------------------------
---------------------------- end combine functions ---------------------
 -----------------------------------------------------------------------*/



/*----------------------------------------------------------------------
----------------   functions called from PlotModule   ------------------
 -----------------------------------------------------------------------*/


void EditManager::prepareEditFields(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();

  if (!isInEdit() || inp.empty())
    return;

  FieldPlotCommand_cp cmd = std::dynamic_pointer_cast<const FieldPlotCommand>(inp[0]);
  if (!cmd)
    return;

  const std::string& plotName = cmd->field.plot;

  // setting plot options

  if (fedits.empty())
    return;

  const size_t npif = std::min(inp.size(), fedits.size());
  for (size_t i=0; i<npif; i++) {
    if (FieldPlotCommand_cp cmdi = std::dynamic_pointer_cast<const FieldPlotCommand>(inp[i])) {
      fieldPlotManager->applySetupOptionsToCommand(plotName, cmdi);
      fedits[i]->editfieldplot->prepare(cmdi);
    }
  }

  // for showing single region during and after combine
  const size_t npic = std::min(inp.size(), combinefields.size());
  for (size_t i=0; i<npic; i++) {
    size_t nreg = combinefields[i].size();
    for (size_t r=0; r<nreg; r++) {
      if (FieldPlotCommand_cp cmdi = std::dynamic_pointer_cast<const FieldPlotCommand>(inp[i])) {
        fieldPlotManager->applySetupOptionsToCommand(plotName, cmdi);
        combinefields[i][r]->editfieldplot->prepare(cmdi);
      }
    }
  }
}

bool EditManager::getFieldArea(Area& a)
{
  if (isInEdit()) {
    for (FieldEdit* fe : fedits) {
      if (fe->editfield) {
        a = fe->editfield->area;
        return true;
      }
    }
  }
  return false;
}

void EditManager::setEditMessage(const std::string& str)
{
  // set or remove (if empty string) an edit message

  apEditmessage.reset(0);

  if (!str.empty()) {
    LabelPlotCommand_p cmd = std::make_shared<LabelPlotCommand>();
    cmd->add(miutil::KeyValue("text", "\"" + str + "\"", true));
    cmd->add("tcolour", "blue");
    cmd->add("bcolour", "red");
    cmd->add("fcolour", "red:128");
    cmd->add("polystyle", "both");
    cmd->add("halign", "left");
    cmd->add("valign", "top");
    cmd->add("xoffset", "0.01");
    cmd->add("yoffset", "0.1");
    cmd->add("fontsize", "30");
    apEditmessage.reset(new AnnotationPlot());
    if (!apEditmessage->prepare(cmd))
      apEditmessage.reset(0);
  }
}

void EditManager::plot(DiGLPainter* gl, PlotOrder zorder)
{
  const bool under = (zorder == PO_LINES);
  const bool over = (zorder == PO_OVERLAY);

  if (!isInEdit() || !(under || over))
    return;

  if (apEditmessage)
    apEditmessage->plot(gl, zorder);

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
      objm->getCombiningObjects().objects[i]->plot(gl, zorder);
  }

  if (plototherfield || plotactivefield) {
    for (int i=0; i<nf; i++) {
      if (fedits[i]->editfield && fedits[i]->editfieldplot) {
        bool act= fedits[i]->activated();
        if ((act && plotactivefield) || (!act && plototherfield))
          fedits[i]->plot(gl, zorder, plotinfluence);
      }
    }
  }

  if (plotobjects) {
    objm->getEditObjects().plot(gl, zorder);
    if (over) {
      objm->getEditObjects().drawJoinPoints(gl);
    }
  }

  if (plotregion)
    plotSingleRegion(gl, zorder);

  if (plotcombine && over){
    int n= objm->getCombiningObjects().objects.size();
    float scale= gridResolutionX;
    if (nf > 0 && plotm->getMapArea().P() != fedits[0]->editfield->area.P() ) {
      int npos= 0;
      for (int i=0; i<n; i++)
        if (objm->getCombiningObjects().objects[i]->objectIs(Border))
          npos+=objm->getCombiningObjects().objects[i]->getXYZsize();
      std::unique_ptr<float[]> x(new float[npos * 2]);
      std::unique_ptr<float[]> y(new float[npos * 2]);
      npos= 0;
      for (int i=0; i<n; i++) {
        if (objm->getCombiningObjects().objects[i]->objectIs(Border)){
          const int np = objm->getCombiningObjects().objects[i]->getXYZsize();
          for (int j=0; j<np; j++) {
            const XY& xyborder = objm->getCombiningObjects().objects[i]->getXY(j);
            x[npos]  = xyborder.x();
            y[npos++]= xyborder.y();
            x[npos]  = xyborder.x() + 1.0;
            y[npos++]= xyborder.y() + 1.0;
          }
        }
      }
      const auto tf = fedits[0]->editfield->area.P().transformationFrom(plotm->getStaticPlot()->getMapProjection());
      if (tf->forward(npos, x.get(), y.get())) {
        float s= 0.;
        for (int j=0; j<npos; j+=2) {
          float dx= x[j] - x[j+1];
          float dy= y[j] - y[j+1];
          s += miutil::absval(dx, dy) / sqrtf(2.0);
        }
        scale= npos*0.5*gridResolutionX/s;
      } else {
        METLIBS_LOG_ERROR("EditManager::plot : getPoints error");
      }
    }

    objm->getCombiningObjects().setScaleToField(scale);

    objm->getCombiningObjects().plot(gl, zorder);

    objm->getCombiningObjects().drawJoinPoints(gl);
  }
}

void EditManager::plotSingleRegion(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();

  if (showRegion<0 || showRegion>=numregs)
    return;

  int nf= combinefields.size();

  for (int i=0; i<nf; i++) {
    if (showRegion<int(combinefields[i].size())) {
      if (combinefields[i][showRegion]->editfield &&
          combinefields[i][showRegion]->editfieldplot)
        combinefields[i][showRegion]->plot(gl, zorder, false);
    }
  }

  if (showRegion<int(combineobjects.size())) {
    // projection may have been changed when showing single region data
    combineobjects[showRegion].switchProjection(plotm->getMapArea());

    combineobjects[showRegion].plot(gl, zorder);
  }
}


bool EditManager::interpolateEditField(ObsPositions* obsPositions)
{
  // TODO this is one step in changeProjection of all ObsPlot's with mslp() == true
  // TODO does something when staticplot area changes or fedits[0]->editfield->area changes

  if (fedits.empty() || !obsPositions)
    return false;

  Field_cp ef = fedits[0]->editfield;
  if (!ef)
    return false;

  // TODO this does not properly detect if only ef->area is changed

  // change projection if needed
  if (obsPositions->obsArea.P() != ef->area.P()) {
    ef->area.P().convertPoints(obsPositions->obsArea.P(), obsPositions->numObs, obsPositions->xpos, obsPositions->ypos);
    obsPositions->obsArea= ef->area;
  }

  if (obsPositions->convertToGrid) {
    ef->convertToGrid(obsPositions->numObs, obsPositions->xpos, obsPositions->ypos);
    obsPositions->convertToGrid = false;
  }

  // interpolate values
  return ef->interpolate(obsPositions->numObs, obsPositions->xpos, obsPositions->ypos,
      obsPositions->interpolatedEditField, Field::I_BESSEL);
}

/*----------------------------------------------------------------------
----------------   end functions called from PlotModule   --------------
 -----------------------------------------------------------------------*/

void EditManager::initEditTools()
{
  METLIBS_LOG_SCOPE();
  eToolFieldStandard = std::vector<editToolInfo>{{"Change value", edit_value},
                                                 {"Move", edit_move},
                                                 {"Change gradient", edit_gradient},
                                                 {"Line, without smooth", edit_line},
                                                 {"Line, with smooth", edit_line_smooth},
                                                 {"Line, limited, without smooth", edit_line_limited},
                                                 {"Line, limited, with smooth", edit_line_limited_smooth},
                                                 {"Smooth", edit_smooth},
                                                 {"Replace undefined values", edit_replace_undef}};

  eToolFieldClasses = std::vector<editToolInfo>{{"Line", edit_class_line}, {"Copy value", edit_class_copy}};

  eToolFieldNumbers = std::vector<editToolInfo>{
      {"Copy value", edit_class_copy},
      {"Change value", edit_value},
      {"Move", edit_move},
      {"Change gradient", edit_gradient},
      {"Set undefined", edit_set_undef},
      {"Smooth", edit_smooth},
      {"Replace undefined values", edit_replace_undef},
  };

  // draw_mode types
  fronts = std::vector<editToolInfo>{
      {"Cold front", Cold, "blue"},
      {"Warm front", Warm, "red"},
      {"Occlusion", Occluded, "purple"},
      {"Trough", Line, "blue"},
      {"Squall line", SquallLine, "blue"},
      {"Significant weather", SigweatherFront, "black"},
      {"Significant weather TURB/VA/RC", SigweatherFront, "red"},
      {"Significant weather ICE/TCU/CB", SigweatherFront, "blue"},
      {"Jet stream", ArrowLine, "black", "black", 0, true},
      {"Cold occlusion", Occluded, "blue"},
      {"Warm occlusion", Occluded, "red"},
      {"Stationary front", Stationary, "grey50"},
      {"Black sharp line", Line, "black", "black", 0, false},
      {"Black smooth line", Line, "black", "black", 0, true},
      {"Red sharp line", Line, "red", "red", 0, false},
      {"Red smooth line", Line, "red"},
      {"Blue sharp line", Line, "blue", "blue", 0, false},
      {"Blue smooth line", Line, "blue"},
      {"Green sharp line", Line, "green", "green", 0, false},
      {"Green smooth line", Line, "green"},
      {"Black sharp line stipple", Line, "black", "black", 0, false, "dash2"},
      {"Black smooth line stipple", Line, "black", "black", 0, true, "dash2"},
      {"Red sharp line stipple", Line, "red", "red", 0, false, "dash2"},
      {"Red smooth line stipple", Line, "red", "red", 0, true, "dash2"},
      {"Blue sharp line stipple", Line, "blue", "blue", 0, false, "dash2"},
      {"Blue smooth line stipple", Line, "blue", "blue", 0, true, "dash2"},
      {"Green sharp line stipple", Line, "green", "green", 0, false, "dash2"},
      {"Green smooth line stipple", Line, "green", "green", 0, true, "dash2"},
      {"Black sharp arrow", ArrowLine, "black", "black", 0, false},
      {"Black sharp thin arrow", ArrowLine, "black", "black", -2, false},
      {"Black smooth arrow", ArrowLine, "black"},
      {"Red sharp arrow", ArrowLine, "red", "red", 0, false},
      {"Red smooth arrow", ArrowLine, "red"},
      {"Blue sharp arrow", ArrowLine, "blue", "blue", 0, false},
      {"Blue smooth arrow", ArrowLine, "blue"},
  };

  symbols = std::vector<editToolInfo>{
      {"Low pressure", 242, "red"},
      {"High pressure", 243, "blue"},
      {"Fog", 62, "darkYellow"},
      {"Thunderstorm", 119, "red"},
      {"Freezing rain", 93, "red"},
      {"Freezing drizzle", 83, "red"},
      {"Showers", 109, "green"},
      {"Snow showers", 114, "green"},
      {"Hail showers", 117, "green"},
      {"Snow", 254, "green"},
      {"Rain", 89, "green"},
      {"Drizzle", 80, "green"},
      {"Cold", 244, "blue"},
      {"Warm", 245, "red"},
      {"Rain showers", 110, "green"},
      {"Sleet showers", 126, "green"},
      {"Thunderstorm with hail", 122, "red"},
      {"Sleet", 96, "green"},
      {"Hurricane", 253, "black"},
      {"Disk", 241, "red"},
      {"Circle", 35, "blue"},
      {"Cross", 255, "red"},
      {"Text", 0, "black"},
  };

  areas = std::vector<editToolInfo>{
      {"Precipitation", Genericarea, "green4", "green4"},
      {"Showers", Genericarea, "green3", "green3", 0, true, "dash2"},
      {"Fog", Genericarea, "darkGray", "darkGrey", 0, true, "empty", "zigzag"},
      {"Significant weather", Sigweather, "black"},
      {"Significant weather  TURB/VA/RC", Sigweather, "red", "red"},
      {"Significant weather  ICE/TCU/CB", Sigweather, "blue", "blue"},
      {"Reduced visibility", Genericarea, "gulbrun", "gulbrun", 0, true, "dash2"},
      {"Clouds", Genericarea, "orange", "orange:0", 0, true, "solid", "diagleft"},
      {"Ice", Genericarea, "darkYellow", "darkYellow:255", 0, true, "solid", "paralyse"},
      {"Black sharp area", Genericarea, "black", "black", 0, false},
      {"Black smooth area", Genericarea, "black", "black", 0, true},
      {"Red sharp area", Genericarea, "red", "red", 0, false},
      {"Red smooth area", Genericarea, "red", "red"},
      {"Blue sharp area", Genericarea, "blue", "blue", 0, false},
      {"Blue smooth area", Genericarea, "blue", "blue"},
      {"Black sharp area stipple", Genericarea, "black", "black", 0, false, "dash2"},
      {"Black smooth area stipple", Genericarea, "black", "black", 0, true, "dash2"},
      {"Red sharp area stipple", Genericarea, "red", "red", 0, false, "dash2"},
      {"Red smooth area stipple", Genericarea, "red", "red", 0, true, "dash2"},
      {"Blue sharp area stipple", Genericarea, "blue", "blue", 0, false, "dash2"},
      {"Blue smooth area stipple", Genericarea, "blue", "blue", 0, true, "dash2"},
      {"Generic area", Genericarea, "red", "red", 0, false, "solid"},
  };

  sigsymbols = std::vector<editToolInfo>{
      {"Sig18", 1018, "black", "black", -1}, // arrow
      // {"Sig18",1018,"black","black",-1},
      {"Tekst_1", 1000, "black"},
      // Low
      {"Sig19", 1019, "black", "black", 1},
      {"Sig12", 1012, "black", "black"},
      {"Sig22", 1022, "black"},
      {"Sig11", 1011, "black"},
      {"Sig3", 1003, "black"},
      {"Sig6", 1006, "black"},
      {"Sig25", 1025, "black"},
      {"Sig14", 1014, "black"},
      {"Sig23", 1023, "black"},
      {"Sig8", 1008, "black"},
      // High
      {"Sig20", 1020, "black", "black", 1},
      {"Sig10", 1010, "black"},
      {"Sig16", 1016, "black"},
      {"Sig7", 1007, "black"},
      {"Sig4", 1004, "black"},
      {"Sig5", 1005, "black"},
      {"Sig26", 1026, "black"},
      {"Sig15", 1015, "black"},
      {"Sig24", 1024, "black"},
      {"Sig13", 1013, "black"},
      {"Sig21", 1021, "red"},
      {"Sig9", 1009, "black"},
      // all texts have index ending in 0
      {"Tekst_2", 2000, "black"},
      {"Sig1", 1001, "black"},
      {"Sig2", 1002, "black"},

      // new
      // Sea temp, blue circle
      {"Sig27", 1027, "black", "blue"},
      // Mean SFC wind, red diamond
      {"Sig28", 1028, "black", "red"},
      // Sea state, black flag
      {"Sig29", 1029, "black", "black", 2},
      // Freezing fog
      {"Sig_fzfg", 1030, "gulbrun", "red"},
      // Nuclear
      {"Sig31", 1031, "black"},
      // Visibility, black rectangular box
      //  {"Sig33",1033,"black"},
      // Vulcano box
      {"Sig34", 1034, "black"},
      // New cross
      {"Sig35", 1035, "black"},
      // Freezing level (new)
      {"Sig36", 1036, "black", "blue"},
      // BR
      {"Sig_br", 1037, "gulbrun"},
      {"Sig38", 1038, "black"},
      {"Sig39", 1039, "red"},

      {"Clouds", 1040, "black"},
      // Fog
      {"Sig_fg", 1041, "gulbrun"},
      // precipitation, green lines
      {"Sig32", 1032, "green"},
      // snow
      {"Sig_snow", 1042, "green", "black", 0},
      // snow showers
      {"Sig_snow_showers", 1043, "green", "green", 2},
      // showers
      {"Sig_showers", 1044, "green", "green", 2},
      // Freezing precip
      {"FZRA", 1045, "red"},
  };

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

  std::vector<editModeInfo> eMode = {{"Standard", eToolFieldStandard}, {"Klasser", eToolFieldClasses}, {"Tall", eToolFieldNumbers}};

  std::vector<editModeInfo> dMode;
  int emidx=0;
  std::map<int, object_modes> objectModes;
  dMode.push_back(editModeInfo("Fronts", fronts));
  objectModes[emidx++] = front_drawing;
  dMode.push_back(editModeInfo("Symbols", symbols));
  objectModes[emidx++] = symbol_drawing;
  dMode.push_back(editModeInfo("Areas", areas));
  objectModes[emidx++] = area_drawing;
  dMode.push_back(editModeInfo("Symbols(SIGWX)", sigsymbols));
  objectModes[emidx++] = symbol_drawing;

  // combine_mode types
  std::vector<editToolInfo> regionlines = {{"regioner", 0}};

  std::vector<editToolInfo> regions;
  const char* colours[3] = { "blue", "red", "darkGreen" };
  for (size_t i=0; i<regnames.size(); i++)
    regions.push_back(editToolInfo(regnames[i], 0, colours[i % 3]));

  std::vector<editModeInfo> cMode;
  int cmidx=0;
  std::map<int, combine_modes> combineModes;

  cMode.push_back(editModeInfo("Regionlinjer", regionlines));
  combineModes[cmidx++]=set_borders;
  cMode.push_back(editModeInfo("Regioner", regions));
  combineModes[cmidx++]=set_region;

  mapmodeinfo.push_back(mapModeInfo("fedit_mode", eMode));
  mapmodeinfo.push_back(mapModeInfo("draw_mode", dMode));
  mapmodeinfo.push_back(mapModeInfo("combine_mode", cMode));

  WeatherSymbol::defineRegions(regions);
  EditObjects::defineModes(objectModes,combineModes);
  WeatherSymbol::setStandardSize(EdProd.standardSymbolSize,
      EdProd.complexSymbolSize);
  WeatherFront::setDefaultLineWidth(EdProd.frontLineWidth);
  WeatherArea::setDefaultLineWidth(EdProd.areaLineWidth);
}

void EditManager::getDataAnnotations(std::vector<std::string>& anno) const
{
  for (FieldEdit* fe : fedits)
    fe->getAnnotations(anno);
}

PlotCommand_cp EditManager::insertTime(PlotCommand_cp lc, const miTime& time)
{
  LabelPlotCommand_cp pc = std::dynamic_pointer_cast<const LabelPlotCommand>(lc);
  if (!pc)
    return lc;

  std::string lang;
  LabelPlotCommand_p tpc = std::make_shared<LabelPlotCommand>();
  for (const KeyValue& kv : pc->all())
    tpc->add(kv.key(), AnnotationPlot::insertTime(kv.value(), time, lang));

  return tpc;
}

void EditManager::getFieldPlotOptions(const std::string& name, PlotOptions& po)
{
  miutil::KeyValue_v fdo;
  return fieldPlotManager->getFieldPlotOptions(name, po, fdo);
}
