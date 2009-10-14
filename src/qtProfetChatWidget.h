#ifndef QTPROFETCHATWIDGET_H_
#define QTPROFETCHATWIDGET_H_

#include <profet/ProfetCommon.h>
#include <QTextEdit>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>

class ProfetChatWidget : public QWidget {
  Q_OBJECT
private:
  QListView * userList;
  QTextEdit * textEdit;
  QLineEdit * lineEdit;
  QPushButton * sendButton;
  
public:
	ProfetChatWidget(QWidget * parent);
	virtual ~ProfetChatWidget();
	/**
	 * Model for user-list view
	 */
  void setUserModel(QAbstractItemModel * userModel);
	/**
   * Display a message
   * @param msg The message to display
   */
  void showMessage(const Profet::InstantMessage & msg);
  /**
   * Notify the user of an error
   * @param errorMsg The error information
   */
  void showErrorMessage(const QString & message);
  /**
   * Set GUI in connected/disconnected mode
   * @param connected mode
   */
  void setConnected(bool connected);

private slots:
  void sendMessagePerformed();
  
signals:
  void sendMessage(const QString & message);
  
};

#endif /*QTPROFETCHATWIDGET_H_*/
