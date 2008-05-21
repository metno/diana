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
#include <QIcon>
#include <QPixmap>
#include <fet_object_normal.xpm> 
#include <fet_object_tmp.xpm>
#include <fet_object_wind.xpm> 
#include <fet_object_rain.xpm>
#include <fet_object_p.xpm>
#include <fet_object_sky.xpm>
#include <user.xpm>
namespace Profet {

QVariant UserListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= users.size())
    return QVariant();
  if (role == Qt::DisplayRole)
    return QString(users[index.row()].name.cStr());
  else if (role == Qt::DecorationRole) {
    return QVariant(QIcon(QPixmap(user_xpm)));
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

//  *** FetSessionListModel ***


QVariant SessionListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= sessions.size())
    return QVariant();
  if (role == Qt::DisplayRole){
    miTime rt = sessions[index.row()].referencetime();
    miString mn = sessions[index.row()].modelname();
    miTime mt = sessions[index.row()].modeltime();
    
    QString str = QString("%1: %2, %3").arg(
        rt.isoTime().cStr()).arg(mn.cStr()).arg(mt.isoTime().cStr());
    return str;
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
  vector<fetSession>::iterator iter;
  for( iter = sessions.begin(); iter != sessions.end(); iter++ ){
    if((*iter).referencetime() == s.referencetime()) {
      *iter = s;
      reset();
      return;
    }
  }
  sessions.push_back(s); // Not found: Adding new user
  reset();
}

void SessionListModel::setSessions(const vector<fetSession> & s) {
  sessions = s;
  reset();
}

void SessionListModel::removeSession(const fetSession & s) {
  vector<fetSession>::iterator iter;
  for( iter = sessions.begin(); iter != sessions.end(); iter++ ){
    if((*iter).referencetime() == s.referencetime()) {
      iter = sessions.erase(iter);
      reset();
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

//  *** FetObjectListModel *** 

QVariant FetObjectListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= objects.size())
    return QVariant();
  if (role == Qt::DisplayRole)
    return QString(objects[index.row()].id().cStr());
  else if (role == Qt::DecorationRole) {
    miString param = objects[index.row()].parameter();
    if (param == "MSLP")
      return QVariant(QIcon(QPixmap(fet_object_p_xpm)));
    else if (param == "T.2M")
      return QVariant(QIcon(QPixmap(fet_object_tmp_xpm)));
    else if (param == "VIND.10M")
      return QVariant(QIcon(QPixmap(fet_object_wind_xpm)));
    else if (param == "TOTALT.SKYDEKKE")
      return QVariant(QIcon(QPixmap(fet_object_sky_xpm)));
    else if (param == "NEDBØR.3T")
      return QVariant(QIcon(QPixmap(fet_object_rain_xpm)));
    return QVariant(QIcon(QPixmap(fet_object_normal_xpm)));
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
    else if (param == "VIND.10M")
      return QVariant(QIcon(QPixmap(fet_object_wind_xpm)));
    else if (param == "TOTALT.SKYDEKKE")
      return QVariant(QIcon(QPixmap(fet_object_sky_xpm)));
    else if (param == "NEDBØR.3T")
      return QVariant(QIcon(QPixmap(fet_object_rain_xpm)));
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
      reset();
//      dataChanged(objIndex,objIndex);
      return;
    }
  }
  // Object is new
  objects.push_back(obj);
  signatureIndexMap[tIndex][pIndex].push_back((objects.size() - 1));
//  dataChanged(objIndex,objIndex);
  reset();
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
//          dataChanged(objIndex,objIndex);
          reset();
          return true;
        }
    }
  }
  return false;
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
