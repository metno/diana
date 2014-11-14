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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtMailDialog.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QImage>
#include <QMessageBox>
#include <QTemporaryFile>

//******************************************************************************
//* MailDialog::MailDialog( QWidget* parent, Controller* llctrl )
//*  : QDialog(parent), m_ctrl(llctrl)
//*
//*   Purpose : MailDialog constructor
//*   To call :
//*   Returns :
//*      Note :
//******************************************************************************
MailDialog::MailDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent), m_ctrl(llctrl)
{

	//--- Create dialog elements ---
	createGridGroupBox();
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	//--- Connect buttons to actions ---
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	//--- Create dialog ---
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(gridGroupBox);
	mainLayout->addWidget(buttonBox);
	setLayout(mainLayout);
	//--- Set dialog title ---
	setWindowTitle(tr("E-Mail Picture"));
}


//******************************************************************************
//* Maildialog::createGridGroupBox()
//*
//*   Purpose : Creates dialog layout
//*   To call : Nothing
//*   Returns : Nothing
//*      Note :
//******************************************************************************
void MailDialog::createGridGroupBox()
{
	//--- Group box containg TO:, CC: & Subject elements ---
	gridGroupBox = new QGroupBox(tr("E-Mail Details"));
	QGridLayout *layout = new QGridLayout;
	//--- Create text & input box for TO: ---
	eToLabel = new QLabel("TO : ");
	eToEdit = new QLineEdit;
	layout->addWidget(eToLabel, 1, 0, Qt::AlignRight);
	layout->addWidget(eToEdit, 1, 1);
	//--- Create text & input box for CC: ---
	eCcLabel = new QLabel("CC : ");
	eCcEdit = new QLineEdit;
	layout->addWidget(eCcLabel, 2, 0, Qt::AlignRight);
	layout->addWidget(eCcEdit, 2, 1);
	//--- Create text & input box for Subject: ---
	eSubjectLabel = new QLabel("Subject : ");
	eSubjectEdit = new QLineEdit;
	layout->addWidget(eSubjectLabel, 3, 0, Qt::AlignRight);
	layout->addWidget(eSubjectEdit, 3, 1);
	//---
	eTextEdit = new QTextEdit;
	layout->addWidget(eTextEdit, 4, 0, 1, 2);
	//--- Set layout manager for widget ---
	gridGroupBox->setLayout(layout);
}



//******************************************************************************
//* Maildialog::accept()
//*
//*   Purpose : Sends mail
//*   To call : Nothing
//*   Returns : Nothing
//*      Note :
//******************************************************************************
void MailDialog::accept()
{
	//--- Create a temporary file for the image ---
	QTemporaryFile *mailtempfile = new QTemporaryFile(QDir::tempPath()+"/diana_XXXXXX.png");
	mailtempfile->open();
	QString filename = mailtempfile->fileName();

	//--- create the image ---
	emit saveImage(filename);

	//--- read the image and convert to Base64 byte array ---
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly)){
	  return;
	}
	QDataStream in(&file); // read the data serialized from the file
	QByteArray rawarray;   // the raw data
  QDataStream datastream(&rawarray, QIODevice::WriteOnly);
  char a;
  // copy between the two data streams
  while (in.readRawData(&a,1) != 0){
    datastream.writeRawData(&a,1);
  }
  QByteArray base64array = rawarray.toBase64();

	FILE* sendmail;

	//--- Send mail using code from qtUffdaDialog ---
  sendmail=popen("/usr/lib/sendmail -t","w");
	//--- Mail header ---
	fprintf(sendmail,"MIME-Version: 1.0\n");
	fprintf(sendmail,"From: diana_noreply@met.no\n");
	fprintf(sendmail,"To: %s\n", eToEdit->text().toStdString().c_str());
	fprintf(sendmail,"Cc: %s\n", eCcEdit->text().toStdString().c_str());
	fprintf(sendmail,"Subject: %s\n", eSubjectEdit->text().toStdString().c_str());
	fprintf(sendmail,"Content-Type: multipart/mixed; boundary=\"diana_auto_generated\"\n\n"),
	fprintf(sendmail,"This is a multi-part, MIME format, message.\n");
	//--- Mail body ---
	fprintf(sendmail,"--diana_auto_generated\n");
	fprintf(sendmail,"Content-type: text/plain\n\n");
	fprintf(sendmail,"%s\n\n", eTextEdit->toPlainText().toStdString().c_str());
	//--- Mail attachments ---
	fprintf(sendmail,"--diana_auto_generated\n");
	fprintf(sendmail,"Content-type: image/png; name=\"diana.png\"\n");
	fprintf(sendmail,"Content-Transfer-Encoding: base64\n");
	fprintf(sendmail,"Content-Disposition: inline; filename=\"diana.png\"\n\n");
	//--- the image data ---
	const char *data = base64array.constData();
	fprintf(sendmail,"%s",data);
  fprintf(sendmail,"\n\n");

	fprintf(sendmail,"--diana_auto_generated--\n\n");
	//--- Finished with sendmail ---
  pclose(sendmail);
	//--- Removing tmp file
	mailtempfile->remove();
	//--- We'll just hide ourselves, so we "remember" field contents from last time ---
	hide();
}
