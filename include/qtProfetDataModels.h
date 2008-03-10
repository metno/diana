#ifndef QTPROFETDATAMODELS_H_
#define QTPROFETDATAMODELS_H_

#include <QAbstractListModel>
#include <profet/ProfetCommon.h>
#include <vector>

namespace Profet{

class UserListModel : public QAbstractListModel {
  Q_OBJECT

private:
  vector<PodsUser> users;

public:
  UserListModel(QObject * parent);

  int rowCount(const QModelIndex &parent = QModelIndex()) const{
    return users.size();
  }
  
  QVariant headerData(int section, Qt::Orientation orientation,
      int role = Qt::DisplayRole) const{
    return QVariant();
  }
  
  QVariant data(const QModelIndex &index, int role) const;

  PodsUser getUser(const QModelIndex &index) const;
  void setUsers(const vector<PodsUser> & u);
  
/*
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole); 
*/
};

}

#endif /*QTPROFETDATAMODELS_H_*/
