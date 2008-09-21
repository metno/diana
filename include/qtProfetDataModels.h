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
#include <QColor>
#include <QEvent>
#include <profet/ProfetCommon.h>
#include <profet/fetObject.h>
#include <profet/fetSession.h>
#include <puTools/miTime.h>
#include <vector>
#include <map>

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
  void setUser(const PodsUser & u);
  void removeUser(const PodsUser & u);
  void clearModel();
  void customEvent(QEvent * e);
};


/**
 * Container for fetSession objects working as a model
 * for list-views. 
 */
class SessionListModel : public QAbstractListModel {
  Q_OBJECT
private:
  vector<fetSession> sessions;
public:
  SessionListModel(QObject * parent): QAbstractListModel(parent){}
  int rowCount(const QModelIndex &parent = QModelIndex()) const{
    return sessions.size();
  }
  QVariant headerData(int section, Qt::Orientation orientation,
      int role = Qt::DisplayRole) const{ return QVariant(); }
  QVariant data(const QModelIndex &index, int role) const;
  /**
   * Gets fetSession with specified index
   * @throw InvalidIndexException
   */
  fetSession getSession(const QModelIndex &index) const
      throw(InvalidIndexException&);
  /**
   * Gets fetSession in specified row
   * @throw InvalidIndexException
   */
  fetSession getSession(int row) const
        throw(InvalidIndexException&);
  /**
   * Setting all sessions in model
   * Connected sessions are updated
   */
  void setSessions(const vector<fetSession> & s);
  void setSession(const fetSession & s);
  void removeSession(const fetSession & s);
  QModelIndex getIndexByRefTime(const miTime & t);
  void clearModel();
  void customEvent(QEvent * e);
  bool insertRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
  bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
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
  /**
   * Insert or replace object in model
   * Connected views are updated
   */
  void setObject(const fetObject & obj);
  /**
   * Remove object from model (by fetObject::id())
   * Connected views are updated
   */
  bool removeObject(const miString & id);
  void clearModel();
};

class FetObjectTableModel : public QAbstractTableModel {
  Q_OBJECT
public:
  enum CellType {CURRENT_CELL,EMPTY_CELL,WITH_DATA_CELL,UNREAD_DATA_CELL};
private:
  vector<fetObject::Signature> objects;
  vector<miString> parameters;
  vector<miTime> times;
  QModelIndex lastSelected;
  map<miString,int> paramIndexMap;
  map<miTime,int> timeIndexMap;
  // map< timeIndex, map<paramIndex, signatureIndex> >
  map< int, map< int, vector< int > > > signatureIndexMap;
  // const access to signatureIndexMap
  vector<int> getObjectIndexList(int timeIndex, int paramIndex) const;
  QColor getCellBackgroundColor(CellType type, bool odd) const;
  
public:
  FetObjectTableModel(QObject * parent): QAbstractTableModel(parent){
    lastSelected = index(0,0);
  }
  int rowCount(const QModelIndex &parent = QModelIndex()) const{
    return parameters.size();
  }
  int columnCount(const QModelIndex &parent = QModelIndex()) const{
    return times.size();
  }
  QVariant headerData(int section, Qt::Orientation orientation,
      int role = Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index, int role) const;
  void initTable(const vector<miTime> & t, const vector<miString> & param);
  bool inited(){ return (parameters.size() && times.size()); }
  void setLastSelectedIndex(const QModelIndex & lsi){ lastSelected = lsi; }
  void setObjectSignatures(const vector<fetObject::Signature> & objects);
  void setObjectSignature(const fetObject::Signature & obj);
  bool removeObjectSignature(const miString & id);
  miTime getTime(const QModelIndex &index) const
    throw(InvalidIndexException&);
  miString getParameter(const QModelIndex &index) const
    throw(InvalidIndexException&);
  miTime getCurrentTime() const throw(InvalidIndexException&);
  miString getCurrentParameter() const throw(InvalidIndexException&);
  void clearModel();
  void customEvent(QEvent * e);
};

}

#endif /*QTPROFETDATAMODELS_H_*/
