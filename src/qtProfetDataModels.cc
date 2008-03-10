#include "qtProfetDataModels.h"

namespace Profet{

UserListModel::UserListModel(QObject * parent): QAbstractListModel(parent){
}

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

PodsUser UserListModel::getUser(const QModelIndex &index) const{
  if (index.row() >= users.size() || index.row() < 0)
    return PodsUser();
  return users[index.row()];
}

void UserListModel::setUsers(const vector<PodsUser> & u){
  users = u;
  reset();
}

/*
Qt::ItemFlags UserListModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;
  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool UserListModel::setData(const QModelIndex &index, const QVariant &value,
    int role) {
  if (index.isValid() && role == Qt::EditRole) {
    stringList.replace(index.row(), value.toString());
    emit dataChanged(index, index);
    return true;
  }
  return false;
}
*/

}
