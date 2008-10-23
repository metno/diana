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
#include "qtProfetDataModels.h"
#include "qtProfetEvents.h"
#include <QIcon>
#include <QPixmap>
#include <fet_object_normal.xpm>
#include <fet_object_tmp.xpm>
#include <fet_object_wind.xpm>
#include <fet_object_rain.xpm>
#include <fet_object_p.xpm>
#include <fet_object_sky.xpm>
#include <fet_object_wave.xpm>
#include <fet_object_fog.xpm>
#include <user.xpm>
#include <custom_user.xpm>
#include <user_admin.xpm>
#include <Robot.xpm>
#include "session_lock.xpm"
#include "session_open.xpm"
#include "session_deployed.xpm"
#include "session_operational.xpm"

namespace Profet {


QPixmap UserListModel::getUserIcon(int index) {
  QRgb rgbColor = getColorByIndex(index);
  QImage userImage(custom_user_xpm);
  for(int w=0; w<userImage.width(); w++){
    for(int h=0; h<userImage.height(); h++){
      int index = userImage.pixelIndex(w,h);
      if(userImage.color(index) == qRgb(0,0,0))
        userImage.setColor(index,rgbColor);
    }
  }
  return QPixmap::fromImage(userImage);
}

QRgb UserListModel::getColorByIndex(int index) {
  int nColors = 12;
  int i = (int) (index % nColors);
  if (i == 0) return qRgb(0,0,255);
  else if (i == 1) return qRgb(0,255,0);
  else if (i == 2) return qRgb(255,0,0);
  else if (i == 3) return qRgb(0,255,255);
  else if (i == 4) return qRgb(255,255,0);
  else if (i == 5) return qRgb(255,0,255);
  else if (i == 6) return qRgb(150,150,150);
  else if (i == 7) return qRgb(255,255,255);
  else if (i == 8) return qRgb(0,0,0);
  else if (i == 9) return qRgb(255,150,0);
  else if (i ==10) return qRgb(0,150,255);
  else if (i ==11) return qRgb(150,150,255);
  else return qRgb(0,0,0);
}

QVariant UserListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= users.size())
    return QVariant();
  if (role == Qt::DisplayRole){
    QString label(users[index.row()].name.cStr());
    label.append(" : ");
    label.append(users[index.row()].editingParameter.cStr());
    return label;
  }
  else if (role == Qt::DecorationRole) {
    if (users[index.row()].role == "forecast")
      return QVariant(QIcon(getUserIcon(index.row())));
    else if (users[index.row()].role == "admin")
      return QVariant(QIcon(QPixmap(user_admin_xpm)));
    else if (users[index.row()].role == "testrobot")
      return QVariant(QIcon(QPixmap(Robot_xpm)));
    else
      return QVariant(QIcon(getUserIcon(index.row())));
  }
  return QVariant();
}

PodsUser UserListModel::getUser(const QModelIndex &index) const throw(
    InvalidIndexException&) {
  if (index.row() >= users.size() || index.row() < 0)
    throw InvalidIndexException();
  return users[index.row()];
}

void UserListModel::setUser(const PodsUser & u) {
  vector<PodsUser>::iterator iter;
  for( iter = users.begin(); iter != users.end(); iter++ ){
    if(*iter == u) {
      *iter = u;
      reset();
      return;
    }
  }
  users.push_back(u); // Not found: Adding new user
  reset();
}

void UserListModel::setUsers(const vector<PodsUser> & u) {
  users = u;
  reset();
}

void UserListModel::removeUser(const PodsUser & u) {
  vector<PodsUser>::iterator iter;
  for( iter = users.begin(); iter != users.end(); iter++ ){
    if(*iter == u) {
      iter = users.erase(iter);
      reset();
      return;
    }
  }
}

void UserListModel::clearModel(){
  users.clear();
  reset();
}

//  *** FetSessionListModel ***


QVariant SessionListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= sessions.size())
    return QVariant();
  if (role == Qt::DisplayRole){
    miTime rt = sessions[index.row()].referencetime();
    miString mn = sessions[index.row()].modelsource();
    miTime mt = sessions[index.row()].modeltime();

    QString str = QString("%1: %2, %3").arg(
        rt.isoTime().cStr()).arg(mn.cStr()).arg(mt.isoTime().cStr());
    return str;

  } else if (role == Qt::DecorationRole) {

    if ( sessions[index.row()].status() == fetSession::underconstruction ){
      if ( sessions[index.row()].editstatus() == fetSession::locked )
	return QVariant(QIcon(QPixmap(session_lock_xpm)));
      else
	return QVariant(QIcon(QPixmap(session_open_xpm)));
    } else if ( sessions[index.row()].status() == fetSession::deployed ){
      return QVariant(QIcon(QPixmap(session_deployed_xpm)));
    } else if ( sessions[index.row()].status() == fetSession::operational ){
      return QVariant(QIcon(QPixmap(session_operational_xpm)));
    }

  } else if (role == Qt::ForegroundRole){
    if ( sessions[index.row()].editstatus() == fetSession::editable ){
      return qVariantFromValue(QColor(Qt::darkGreen));
    } else {
      return qVariantFromValue(QColor(Qt::red));
    }
  }
  return QVariant();
}

fetSession SessionListModel::getSession(const QModelIndex &index) const throw(
    InvalidIndexException&) {
  if (index.row() >= sessions.size() || index.row() < 0)
    throw InvalidIndexException();
  return sessions[index.row()];
}

fetSession SessionListModel::getSession(int row_nr) const throw(InvalidIndexException&){
  try{
    QModelIndex i = index(row_nr,0);
    return getSession(i);
  }catch(...){ throw;}
  return fetSession();
}

void SessionListModel::setSession(const fetSession & s) {
  int irow = sessions.size();
  for( int i=0; i<sessions.size(); i++ ){
    if(sessions[i].referencetime() == s.referencetime()) {
      sessions[i] = s;
      emit dataChanged(index(i, 0),index(i, 0));
      return;
    } else if ( sessions[i].referencetime() < s.referencetime()) {
      irow = i;
      break;
    }
  }

  // Not found: Adding new session
  insertRows(irow,1);
  sessions[irow] = s;
}

void SessionListModel::setSessions(const vector<fetSession> & s) {
  sessions = s;
  std::sort(sessions.begin(),sessions.end());
  reset();
}

void SessionListModel::removeSession(const fetSession & s) {
  int irow = sessions.size();
  for( int i=0; i<sessions.size(); i++ ){
    if(sessions[i].referencetime() == s.referencetime()) {
      removeRows(i,1);
      return;
    }
  }
}

QModelIndex SessionListModel::getIndexByRefTime(const miTime & t){
  int n = sessions.size();
  for (int i=0; i<n; i++)
    if (sessions[i].referencetime() == t)
      return index(i, 0);
  return QModelIndex();
}

void SessionListModel::clearModel(){
  sessions.clear();
  reset();
}

void SessionListModel::customEvent(QEvent * e){
  if(e->type() == Profet::SESSION_LIST_EVENT){
    Profet::SessionListEvent * sle = (Profet::SessionListEvent*) e;
    if(sle->remove) removeSession(sle->session);
    else setSession(sle->session);
  }
}


bool SessionListModel::removeRows( int position, int rows, const QModelIndex & parent )
{
  beginRemoveRows(QModelIndex(), position, position+rows-1);

  for (int row = 0; row < rows; ++row) {
    sessions.erase(sessions.begin()+position);
  }

  endRemoveRows();
  return true;
}

bool SessionListModel::insertRows( int position, int rows, const QModelIndex & parent )
{
  beginInsertRows(QModelIndex(), position, position+rows-1);

  for (int row = 0; row < rows; ++row) {
    sessions.insert(sessions.begin()+position, fetSession());
  }

  endInsertRows();
  return true;
}

//  *** FetObjectListModel ***

QVariant FetObjectListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= objects.size())
    return QVariant();
  if (role == Qt::DisplayRole){
    miString user    = objects[index.row()].user();
    miString objname = objects[index.row()].name();
    miTime edittime  = objects[index.row()].editTime();
    QString str = QString("%1: %2 - %3").arg(user.cStr()).arg(objname.cStr()).arg(edittime.isoTime().cStr());
    return str; //QString(objects[index.row()].id().cStr());
  } else if (role == Qt::DecorationRole) {
    miString param = objects[index.row()].parameter();
    if (param == "MSLP")
      return QVariant(QIcon(QPixmap(fet_object_p_xpm)));
    else if (param == "T.2M")
      return QVariant(QIcon(QPixmap(fet_object_tmp_xpm)));
    else if (param.contains("VIND"))
      return QVariant(QIcon(QPixmap(fet_object_wind_xpm)));
    else if (param.contains("SKYDEKKE"))
      return QVariant(QIcon(QPixmap(fet_object_sky_xpm)));
    else if (param.contains("NEDBØR"))
      return QVariant(QIcon(QPixmap(fet_object_rain_xpm)));
    else if (param.contains("FOG"))
      return QVariant(QIcon(QPixmap(fet_object_fog_xpm)));
    else if (param.contains("Wave"))
      return QVariant(QIcon(QPixmap(fet_object_wave_xpm)));
    return QVariant(QIcon(QPixmap(fet_object_normal_xpm)));
  } else if (role == Qt::ToolTipRole) {
    QString str = objects[index.row()].reason().c_str();
    return str;
  } else
    return QVariant();
}

QModelIndex FetObjectListModel::getIndexById(const miString & id) const {
  for (int i=0; i<objects.size(); i++)
    if (objects[i].id() == id)
      return index(i, 0);
  return QModelIndex();
}

fetObject FetObjectListModel::getObject(const QModelIndex &index) const throw(
    InvalidIndexException&) {
  if (index.row() >= objects.size() || index.row() < 0)
    throw InvalidIndexException();
  return objects[index.row()];
}

void FetObjectListModel::setObjects(const vector<fetObject> & obj) {
  objects = obj;
  reset();
}

void FetObjectListModel::setObject(const fetObject & obj) {
  QModelIndex objIndex = getIndexById(obj.id());
  if(objIndex.isValid()){ // object exist
    objects[objIndex.row()] = obj;
  }
  else { // object does not exist
    objects.push_back(obj);
    objIndex = getIndexById(obj.id());
  }
  if(objIndex.isValid())
    reset();
//    dataChanged(objIndex,objIndex);
}

bool FetObjectListModel::removeObject(const miString & id){
  vector<fetObject>::iterator iter = objects.begin();
  for( ; iter != objects.end(); iter++ ){
    if((*iter).id() == id) {
      iter = objects.erase(iter);
      reset();
      return true;
    }
  }
  return false;
}

void FetObjectListModel::clearModel(){
  objects.clear();
  reset();
}

//  *** FetObjectTableModel ***


QVariant FetObjectTableModel::headerData(int section,
    Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Vertical && parameters.size() <= section)
    return QVariant();
  else if (orientation == Qt::Horizontal && times.size() <= section)
    return QVariant();

  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Vertical)
      return QString(parameters[section].cStr());
    else
      return QString(times[section].format("%a %k").cStr());
  } else if (role == Qt::DecorationRole && orientation == Qt::Vertical) {
    miString param = parameters[section];
    if (param == "MSLP")
      return QVariant(QIcon(QPixmap(fet_object_p_xpm)));
    else if (param == "T.2M")
      return QVariant(QIcon(QPixmap(fet_object_tmp_xpm)));
    else if (param.contains("VIND"))
      return QVariant(QIcon(QPixmap(fet_object_wind_xpm)));
    else if (param.contains("SKYDEKKE"))
      return QVariant(QIcon(QPixmap(fet_object_sky_xpm)));
    else if (param.contains("NEDBØR"))
      return QVariant(QIcon(QPixmap(fet_object_rain_xpm)));
    else if (param.contains("FOG"))
      return QVariant(QIcon(QPixmap(fet_object_fog_xpm)));
    else if (param.contains("Wave"))
      return QVariant(QIcon(QPixmap(fet_object_wave_xpm)));
    return QVariant(QIcon(QPixmap(fet_object_normal_xpm)));
  }
  return QVariant();
}

QVariant FetObjectTableModel::data(const QModelIndex &index, int role) const {
  if(role == Qt::DisplayRole) {
    vector<int> oil = getObjectIndexList(index.column(),index.row());
    if(oil.size()) return oil.size();
  }
  if(role == Qt::TextAlignmentRole) return Qt::AlignCenter;
  if(role == Qt::BackgroundRole) {
    bool oddDay = (times[index.column()].date().julianDay()%2) != 0;
    if(index == lastSelected)
      return getCellBackgroundColor(CURRENT_CELL, oddDay);
    vector<int> oil = getObjectIndexList(index.column(),index.row());
    if(oil.size())
      return getCellBackgroundColor(WITH_DATA_CELL, oddDay);
    else
      return getCellBackgroundColor(EMPTY_CELL, oddDay);
  }
  if(role == Qt::DecorationRole) {
    map<QModelIndex, vector<PodsUser> >::const_iterator i = userLocationMap.begin();
    for(; i != userLocationMap.end(); i++){
      if((*i).first == index && (*i).second.size()){
        QPixmap customIcon = UserListModel::getUserIcon(index.column());
        return QVariant(QIcon(customIcon));
      }
    }
  }
  return QVariant();
}

vector<int> FetObjectTableModel::getObjectIndexList(
    int timeIndex, int paramIndex) const{
  vector<int> indexList;
  map< int, map< int, vector< int > > >::const_iterator i;
  for(i=signatureIndexMap.begin();i!=signatureIndexMap.end();i++){
    if((*i).first == timeIndex){
      map< int, vector< int > >::const_iterator j;
      for(j=(*i).second.begin(); j!=(*i).second.end(); j++){
        if((*j).first == paramIndex){
          return (*j).second;
        }
      }
    }
  }
  vector<int> emptyIndex;
  return emptyIndex;
}

void FetObjectTableModel::initTable(const vector<miTime> & t,
    const vector<miString> & param) {
  times = t;
  parameters = param;
  int nParam = parameters.size();
  for(int i=0;i<nParam; i++){
    paramIndexMap[parameters[i]] = i;
  }
  int nTimes = times.size();
  for(int i=0;i<nTimes;i++){
    timeIndexMap[times[i]] = i;
  }
  signatureIndexMap[nTimes][nParam];
  objects.clear();
  reset();
}

void FetObjectTableModel::setObjectSignatures(
    const vector<fetObject::Signature> & obj) {
  objects = obj;
  // build signatureIndexMap
  signatureIndexMap.clear();
  int nObj = objects.size();
  for(int i=0;i<nObj; i++){
    fetObject::Signature s = objects[i];
    int tIndex = timeIndexMap[s.validTime];
    int pIndex = paramIndexMap[s.parameter];
    signatureIndexMap[tIndex][pIndex].push_back(i);
  }
  reset();
}

void FetObjectTableModel::setObjectSignature(
    const fetObject::Signature & obj) {
  int tIndex = timeIndexMap[obj.validTime];
  int pIndex = paramIndexMap[obj.parameter];
  QModelIndex objIndex = index(tIndex,pIndex);
  int nObj = objects.size();
  for(int i=0;i<nObj; i++){
    if(objects[i].id == obj.id){
      objects[i] = obj;
      //reset();
      emit dataChanged(objIndex,objIndex);
      return;
    }
  }
  // Object is new
  objects.push_back(obj);
  signatureIndexMap[tIndex][pIndex].push_back((objects.size() - 1));
  emit dataChanged(objIndex,objIndex);
  //reset();
}

bool  FetObjectTableModel::removeObjectSignature(const miString & id) {
  int nObj = objects.size();
  for(int i=0;i<nObj; i++){
    if(objects[i].id == id){
      fetObject::Signature s = objects[i];
      int tIndex = timeIndexMap[s.validTime];
      int pIndex = paramIndexMap[s.parameter];
      vector<int> v = signatureIndexMap[tIndex][pIndex];
      vector< int >::iterator iter = v.begin();
      for(;iter!=v.end();iter++)
        if((*iter) == i) {
          iter = v.erase(iter);
          signatureIndexMap[tIndex][pIndex] = v;
          QModelIndex objIndex = index(tIndex,pIndex);
          emit dataChanged(objIndex,objIndex);
//          reset();
          return true;
        }
    }
  }
  return false;
}


void FetObjectTableModel::setUserLocation(
    const PodsUser & user)
{
  removeUserLocation(user);
  QModelIndex newIndex = index(
      paramIndexMap[user.editingParameter],
      timeIndexMap[user.editingTime]);
  // add to new location
  userLocationMap[newIndex].push_back(user);
  emit dataChanged(newIndex,newIndex);
}

void FetObjectTableModel::removeUserLocation(
    const PodsUser & user)
{
  QModelIndex currentIndex;
  // remove from previous location
  map<QModelIndex, vector<PodsUser> >::iterator i;
  for(i=userLocationMap.begin();i!=userLocationMap.end();i++){
    vector<PodsUser>::iterator userIter = (*i).second.begin();
    //TODO : Why coredumb without copy iterator ??
    vector<PodsUser>::iterator userIterEnd = (*i).second.end();
    for(; userIter != userIterEnd; userIter ++){
      if( (*userIter) == user ) {
        currentIndex = (*i).first;
        userIter = (*i).second.erase(userIter);
      }
    }
  }
  if(currentIndex.isValid())
    emit dataChanged(currentIndex,currentIndex);
}

void FetObjectTableModel::clearModel(){
  objects.clear();
  parameters.clear();
  times.clear();
  paramIndexMap.clear();
  timeIndexMap.clear();
  signatureIndexMap.clear();
  reset();
}

void FetObjectTableModel::customEvent(QEvent * e){
  if (e->type() == Profet::SIGNATURE_UPDATE_EVENT) { //SignatureListUpdateEvent
    Profet::SignatureUpdateEvent * sue = (Profet::SignatureUpdateEvent*) e;
    if (sue->remove) removeObjectSignature(sue->object.id);
    else setObjectSignature(sue->object);
  } else if (e->type() == Profet::SIGNATURE_LIST_UPDATE_EVENT) {
    Profet::SignatureListUpdateEvent * sue = (Profet::SignatureListUpdateEvent*) e;
    setObjectSignatures(sue->objects);
  }
}

miTime FetObjectTableModel::getTime(const QModelIndex &index) const throw(
    InvalidIndexException&) {
  int col = index.column();
  if(col < 0)
    throw InvalidIndexException();
  if (col >= times.size())
    throw InvalidIndexException();
  return times[col];
}

miString FetObjectTableModel::getParameter(const QModelIndex &index) const throw(
    InvalidIndexException&) {
  int row = index.row();
  if(row < 0)
    throw InvalidIndexException();
  if (row >= parameters.size())
    throw InvalidIndexException();
  return parameters[row];
}


miTime FetObjectTableModel::getCurrentTime() const throw(InvalidIndexException&){
  try{ return getTime(lastSelected);}
  catch(InvalidIndexException & iie){ throw iie; }
}

miString FetObjectTableModel::getCurrentParameter() const throw(InvalidIndexException&){
  try{ return getParameter(lastSelected); }
  catch(InvalidIndexException & iie){ throw iie; }
}

QColor FetObjectTableModel::getCellBackgroundColor(CellType type, bool odd) const {
  switch (type) {
  case CURRENT_CELL:
    if (odd)
      return QColor(122, 122, 255);
    else
      return QColor(175, 175, 242);
  case EMPTY_CELL:
    if (odd)
      return QColor(255, 255, 255);
    else
      return QColor(242, 242, 242);
  case WITH_DATA_CELL:
    if (odd)
      return QColor(180, 255, 180);
    else
      return QColor(204, 242, 204);
  default:
    return QColor(255, 255, 255);
  }
}

}
