#ifndef QTPROFETEVENTS_H_
#define QTPROFETEVENTS_H_

#include <vector>
#include <qevent.h>
#include <profet/ProfetCommon.h>

namespace Profet{

  static int MESSAGE_EVENT =   QEvent::User + 1;
  static int USER_LIST_EVENT = QEvent::User + 2;
  
  class MessageEvent : public QCustomEvent {
  public:
    Profet::InstantMessage message;
    MessageEvent(Profet::InstantMessage m)
      :QCustomEvent(MESSAGE_EVENT),message(m){}
  };
  
  class UserListEvent : public QCustomEvent {
  public:
    vector<Profet::PodsUser> users;
    UserListEvent():QCustomEvent(USER_LIST_EVENT){}
  };
} // end namespace Profet
#endif /*QTPROFETEVENTS_H_*/
