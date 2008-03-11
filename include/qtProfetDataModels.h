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
#ifndef QTPROFETDATAMODELS_H_
#define QTPROFETDATAMODELS_H_

#include <QAbstractListModel>
#include <profet/ProfetCommon.h>
#include <profet/fetObject.h>
#include <vector>

namespace Profet{

/**
 * An exception thrown when requesting an object with invalid index
 */
class InvalidIndexException: public std::exception{
private:
  const char* message;
public:
  InvalidIndexException() : message("Invalid index requested"){}
  const char* what() const throw()  {return message;}
};

/**
 * Container for PodsUser objects working as a model
 * for list-views. 
 */
class UserListModel : public QAbstractListModel {
  Q_OBJECT
private:
  vector<PodsUser> users;
public:
  UserListModel(QObject * parent): QAbstractListModel(parent){}
  int rowCount(const QModelIndex &parent = QModelIndex()) const{
    return users.size();
  }
  QVariant headerData(int section, Qt::Orientation orientation,
      int role = Qt::DisplayRole) const{ return QVariant(); }
  QVariant data(const QModelIndex &index, int role) const;
  /**
   * Gets PodsUser with specified index
   * @throw InvalidIndexException
   */
  PodsUser getUser(const QModelIndex &index) const
      throw(InvalidIndexException&);
  /**
   * Setting all users in model
   * Connected views are updated
   */
  void setUsers(const vector<PodsUser> & u);
};

/**
 * Container for fet-objects working as a model
 * for list-views. 
 */
class FetObjectListModel : public QAbstractListModel {
  Q_OBJECT
private:
  vector<fetObject> objects;
public:
  FetObjectListModel(QObject * parent): QAbstractListModel(parent){}
  int rowCount(const QModelIndex &parent = QModelIndex()) const{
    return objects.size();
  }
  QVariant headerData(int section, Qt::Orientation orientation,
      int role = Qt::DisplayRole) const{ return QVariant(); }
  QVariant data(const QModelIndex &index, int role) const;
  /**
   * Gets fetObject with specified index
   * @throw InvalidIndexException
   */
  fetObject getObject(const QModelIndex &index) const
      throw(InvalidIndexException&);
  /**
   * Get index of object with specified id
   */
  QModelIndex getIndexById(const miString & id) const;
  /**
   * Setting all objects in model
   * Connected views are updated
   */
  void setObjects(const vector<fetObject> & obj);
};

}

#endif /*QTPROFETDATAMODELS_H_*/
