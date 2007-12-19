#include "qtProfetChatWidget.h"

#include <qtextedit.h> 
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qstylesheet.h>

ProfetChatWidget::ProfetChatWidget( QWidget* parent ) 
: QVBox(parent){
  
  
  textEdit = new QTextEdit(this);
  textEdit->setReadOnly(true);
  lineEdit = new QLineEdit(this);
  connect(lineEdit,SIGNAL(returnPressed()),this,SLOT(sendMessagePerformed()));
/*  
  sendButton = new QPushButton("Send",this);
  connect(sendButton,SIGNAL(clicked()),this,SLOT(sendMessagePerformed()));
*/  

  textEdit->setTextFormat( Qt::LogText );
  QStyleSheetItem * systemStyle = new QStyleSheetItem( textEdit->styleSheet(), "sys" );
  QStyleSheetItem * warnStyle = new QStyleSheetItem( textEdit->styleSheet(), "warn" );
  QStyleSheetItem * messageStyle = new QStyleSheetItem( textEdit->styleSheet(), "msg" );
  QStyleSheetItem * nameStyle = new QStyleSheetItem( textEdit->styleSheet(), "name" );
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
