#include "qtProfetChatWidget.h"

#include <q3textedit.h> 
#include <qlineedit.h>
#include <qpushbutton.h>
#include <q3stylesheet.h>

ProfetChatWidget::ProfetChatWidget( QWidget* parent ) 
: Q3VBox(parent){
  
  
  textEdit = new Q3TextEdit(this);
  textEdit->setReadOnly(true);
  lineEdit = new QLineEdit(this);
  connect(lineEdit,SIGNAL(returnPressed()),this,SLOT(sendMessagePerformed()));
/*  
  sendButton = new QPushButton("Send",this);
  connect(sendButton,SIGNAL(clicked()),this,SLOT(sendMessagePerformed()));
*/  

  textEdit->setTextFormat( Qt::LogText );
  Q3StyleSheetItem * systemStyle = new Q3StyleSheetItem( textEdit->styleSheet(), "sys" );
  Q3StyleSheetItem * warnStyle = new Q3StyleSheetItem( textEdit->styleSheet(), "warn" );
  Q3StyleSheetItem * messageStyle = new Q3StyleSheetItem( textEdit->styleSheet(), "msg" );
  Q3StyleSheetItem * nameStyle = new Q3StyleSheetItem( textEdit->styleSheet(), "name" );
  systemStyle->setColor("gray");
  warnStyle->setColor("red");
  messageStyle->setColor("black");
  nameStyle->setColor("blue");
  
}

ProfetChatWidget::~ProfetChatWidget(){
}

void ProfetChatWidget::setConnected(bool connected){
//  if(connected) textEdit->append("<sys>Connected to server</sys>");
//  else textEdit->append("<sys>Disconnected from server</sys>");
  lineEdit->setEnabled(connected);
}

void ProfetChatWidget::showMessage(const Profet::InstantMessage & msg){
  QString m = QString( "<name>%1</name><msg>: %2</msg>" )
                .arg(msg.sender.cStr(),msg.message.cStr());
   textEdit->append(m);
}

void ProfetChatWidget::showErrorMessage(const QString & message){
  textEdit->setText(   QString("<warn>") + 
      message + QString("</warn>"));
}

void ProfetChatWidget::sendMessagePerformed(){
  emit sendMessage(lineEdit->text());
  lineEdit->clear();
}
