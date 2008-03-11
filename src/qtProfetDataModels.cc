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

namespace Profet{

QVariant UserListModel::data(const QModelIndex &index, int role) const{
  if (!index.isValid())
    return QVariant();
  if (index.row() >= users.size())
    return QVariant();
  if (role == Qt::DisplayRole)
      return QString(users[index.row()].name.cStr());
  else
      return QVariant();
}

PodsUser UserListModel::getUser(const QModelIndex &index) const
throw(InvalidIndexException&){
  if (index.row() >= users.size() || index.row() < 0)
    throw new InvalidIndexException();
  return users[index.row()];
}

void UserListModel::setUsers(const vector<PodsUser> & u){
  users = u;
  reset();
}

//  *** FetObjectListModel *** 

QVariant FetObjectListModel::data(const QModelIndex &index, int role) const{
  if (!index.isValid())
    return QVariant();
  if (index.row() >= objects.size())
    return QVariant();
  if (role == Qt::DisplayRole)
      return QString(objects[index.row()].id().cStr());
  else
      return QVariant();
}

QModelIndex FetObjectListModel::getIndexById(const miString & id) const{
  for(int i=0;i<objects.size();i++)
    if(objects[i].id() == id)
      return index(i,0);
  //TODO is this index valid (without parent)?
  return QModelIndex();
}

fetObject FetObjectListModel::getObject(const QModelIndex &index) const
throw(InvalidIndexException&){
  if (index.row() >= objects.size() || index.row() < 0)
    throw new InvalidIndexException();
  return objects[index.row()];
}

void FetObjectListModel::setObjects(const vector<fetObject> & obj){
  objects = obj;
  reset();
}

}
