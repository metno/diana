#ifndef QTPROFETEVENTS_H_
#define QTPROFETEVENTS_H_

#include <vector>
#include <qevent.h>
//Added by qt3to4:
#include <QCustomEvent>
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
  class MessageEvent : public QCustomEvent {
  public:
    Profet::InstantMessage message;
    MessageEvent(Profet::InstantMessage m)
      :QCustomEvent(MESSAGE_EVENT),message(m){}
  };

  /**
   * Threadsafe event for incomming UserList
   */
  class UserListEvent : public QCustomEvent {
  public:
    vector<Profet::PodsUser> users;
    UserListEvent():QCustomEvent(USER_LIST_EVENT){}
  };

  /**
   * Threadsafe event for incomming messages
   */  
  class ObjectUpdateEvent : public QCustomEvent {
  public:
    vector<fetObject> objects;
    ObjectUpdateEvent(vector<fetObject> obj)
      :QCustomEvent(OBJECT_UPDATE_EVENT),objects(obj){}
  };
  
  /**
   * Threadsafe event for incomming messages
   */  
  class SignatureUpdateEvent : public QCustomEvent {
  public:
    vector<fetObject::Signature> objects;
    SignatureUpdateEvent(vector<fetObject::Signature> s)
      :QCustomEvent(SIGNATURE_UPDATE_EVENT),objects(s){}
  };

  
} // end namespace Profet
#endif /*QTPROFETEVENTS_H_*/
