#ifndef QTPROFETEVENTS_H_
#define QTPROFETEVENTS_H_

#include <vector>
#include <QEvent>
#include <profet/ProfetCommon.h>
#include <profet/fetObject.h>

namespace Profet{

  static int MESSAGE_EVENT = QEvent::User + 1;
  static int USER_LIST_EVENT = QEvent::User + 2;
  static int UPDATE_MAP_EVENT = QEvent::User + 3;
  static int OBJECT_UPDATE_EVENT = QEvent::User + 4;
  static int SIGNATURE_UPDATE_EVENT = QEvent::User + 5;
  /**
   * Threadsafe event for incomming messages
   */  
  class MessageEvent : public QEvent {
  public:
    Profet::InstantMessage message;
    MessageEvent(Profet::InstantMessage m)
      :QEvent(QEvent::Type(MESSAGE_EVENT)),message(m){}
  };

  /**
   * Threadsafe event for incomming UserList
   */
  class UserListEvent : public QEvent {
  public:
    vector<Profet::PodsUser> users;
    UserListEvent():QEvent(QEvent::Type(USER_LIST_EVENT)){}
  };

  /**
   * Threadsafe event for incomming messages
   */  
  class ObjectUpdateEvent : public QEvent {
  public:
    vector<fetObject> objects;
    ObjectUpdateEvent(vector<fetObject> obj)
      :QEvent(QEvent::Type(OBJECT_UPDATE_EVENT)),objects(obj){}
  };
  
  /**
   * Threadsafe event for incomming messages
   */  
  class SignatureUpdateEvent : public QEvent {
  public:
    vector<fetObject::Signature> objects;
    SignatureUpdateEvent(vector<fetObject::Signature> s)
      :QEvent(QEvent::Type(SIGNATURE_UPDATE_EVENT)),objects(s){}
  };

  
} // end namespace Profet
#endif /*QTPROFETEVENTS_H_*/
