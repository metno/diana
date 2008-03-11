#include "qtProfetChatWidget.h"


#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

ProfetChatWidget::ProfetChatWidget( QWidget* parent) 
: QWidget(parent){
  QHBoxLayout * mainLayout = new QHBoxLayout(this);
  mainLayout->setMargin(0);
  QVBoxLayout * leftLayout = new QVBoxLayout();
  leftLayout->addWidget(new QLabel(tr("Messages")));
  textEdit = new QTextEdit();
  textEdit->setReadOnly(true);
  leftLayout->addWidget(textEdit,1);
  QHBoxLayout * sendLayout = new QHBoxLayout();
  lineEdit = new QLineEdit();
  sendLayout->addWidget(lineEdit,1);
  sendButton = new QPushButton(tr("&Send"));
  sendLayout->addWidget(sendButton,0);
  leftLayout->addLayout(sendLayout,0);
  mainLayout->addLayout(leftLayout,1);
  userList = new QListView();
  QVBoxLayout * rightLayout = new QVBoxLayout();
  rightLayout->addWidget(new QLabel(tr("Users")),0);
  rightLayout->addWidget(userList,1);
  mainLayout->addLayout(rightLayout,0);
//  sendbutton emited on enter...  
//  connect(lineEdit,SIGNAL(returnPressed()),this,SLOT(sendMessagePerformed()));
  connect(sendButton,SIGNAL(clicked()),this,SLOT(sendMessagePerformed()));
}

ProfetChatWidget::~ProfetChatWidget(){
}


void ProfetChatWidget::setUserModel(QAbstractItemModel * userModel){
  userList->setModel(userModel);
}

void ProfetChatWidget::setConnected(bool connected){
  lineEdit->setEnabled(connected);
}

void ProfetChatWidget::showMessage(const Profet::InstantMessage & msg){
  QString m = QString( "<b>%1</b>: %2" )
                .arg(msg.sender.cStr(),msg.message.cStr());
   textEdit->append(m);
}

void ProfetChatWidget::showErrorMessage(const QString & message){
  textEdit->setText(   QString("<i>") + 
      message + QString("</i>"));
}

void ProfetChatWidget::sendMessagePerformed(){
  emit sendMessage(lineEdit->text());
  lineEdit->clear();
}
