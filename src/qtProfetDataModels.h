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
#include <QRgb>
#include <QEvent>
#include <QPixmap>
#include <profet/ProfetCommon.h>
#include <profet/fetObject.h>
#include <profet/fetSession.h>
#include <profet/fetParameter.h>
#include <puTools/miTime.h>
#include <diColour.h>
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
public:
  static QPixmap getUserIcon(int index);
  static QRgb getColorByIndex(int index);
  static QIcon getUserIcon(const PodsUser& user);

private:
  std::vector<PodsUser> users;
  int getNextIconIndex();

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
  void setUsers(const std::vector<PodsUser> & u);
  PodsUser setUser(const PodsUser & u);
  void removeUser(const PodsUser & u);
  void clearModel();
};


/**
 * Container for fetSession objects working as a model
 * for list-views.
 */
class SessionListModel : public QAbstractListModel {
  Q_OBJECT
private:
  std::vector<fetSession> sessions;
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
  void setSessions(const std::vector<fetSession> & s);
  void setSession(const fetSession & s);
  void removeSession(const fetSession & s);
  QModelIndex getIndexByRefTime(const miutil::miTime & t);
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
  std::vector<fetObject> objects;
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
  QModelIndex getIndexById(const miutil::miString & id) const;
  /**
   * Setting all objects in model
   * Connected views are updated
   */
  void setObjects(const std::vector<fetObject> & obj);
  /**
   * Insert or replace object in model
   * Connected views are updated
   */
  void setObject(const fetObject & obj);
  /**
   * Remove object from model (by fetObject::id())
   * Connected views are updated
   */
  bool removeObject(const miutil::miString & id);
  void clearModel();
};

class FetObjectTableModel : public QAbstractTableModel {
  Q_OBJECT
public:
  enum CellType {
    CURRENT_CELL, EMPTY_CELL, WITH_DATA_CELL, UNREAD_DATA_CELL
  };
  enum HeaderDisplayType {
    PARAM_ICON = 1,
    PARAM_COLOUR_GRADIENT = 2,
    PARAM_COLOUR_RECT = 4
  };
private:
  std::vector<fetObject::Signature> objects;
  std::map<miutil::miString,fetParameter> name2par;
  std::vector<miutil::miString> parameters;
  std::vector<miutil::miTime> times;
  QModelIndex lastSelected;
  miutil::miTime currentSessionRefTime;
  std::map<QModelIndex, std::vector<PodsUser> > userLocationMap;
  std::map<miutil::miString,int> paramIndexMap;
  std::map<miutil::miTime,int> timeIndexMap;
  // map< timeIndex, map<paramIndex, signatureIndex> >
  std::map< int, std::map< int, std::vector< int > > > signatureIndexMap;
  std::map<miutil::miString,Colour> parameterColours;
  int headerDisplayMask;

  // const access to signatureIndexMap
  std::vector<int> getObjectIndexList(int timeIndex, int paramIndex) const;
  QColor getCellBackgroundColor(CellType type, bool odd) const;
  std::vector<PodsUser> getUsers( const QModelIndex & index,
      const miutil::miTime & sessionRefTime) const;

public:
  FetObjectTableModel(QObject * parent): QAbstractTableModel(parent), headerDisplayMask(PARAM_ICON){
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
  void initTable(const std::vector<miutil::miTime> & t, const std::vector<miutil::miString> & param);
  bool inited(){ return (parameters.size() && times.size()); }
  void setLastSelectedIndex(const QModelIndex & lsi){ lastSelected = lsi; }
  void setCurrentSessionRefTime(const miutil::miTime & t) { currentSessionRefTime = t; }
  void setObjectSignatures(const std::vector<fetObject::Signature> & objects);
  void setObjectSignature(const fetObject::Signature & obj);
  bool removeObjectSignature(const miutil::miString & id);
  miutil::miTime getTime(const QModelIndex &index) const
    throw(InvalidIndexException&);
  miutil::miString getParameter(const QModelIndex &index) const
    throw(InvalidIndexException&);
  miutil::miTime getCurrentTime() const throw(InvalidIndexException&);
  miutil::miString getCurrentParameter() const throw(InvalidIndexException&);
  void setUserLocation(const PodsUser &);
  void removeUserLocation(const PodsUser &);
  void clearModel();
  void customEvent(QEvent * e);
  QModelIndex getModelIndex(miutil::miTime time, miutil::miString param);
  void setParamColours(std::map<miutil::miString,Colour>& paramCol);
  void setHeaderDisplayMask(int mask);
  void setParameters(const std::vector<fetParameter>& vp);
};

}

#endif /*QTPROFETDATAMODELS_H_*/
