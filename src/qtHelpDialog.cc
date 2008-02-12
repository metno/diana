/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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
#include <qlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <q3textbrowser.h>
#include <qmime.h> 
#include <qapplication.h> 
#include <qtooltip.h>

#include <qtHelpDialog.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QPixmap>
#include <Q3VBoxLayout>
#include <miString.h>
#include <iostream>

#include <qpainter.h>
#include <qprinter.h>
#include <q3simplerichtext.h>
#include <q3paintdevicemetrics.h>
#include <q3progressdialog.h>

#include <back.xpm>
#include <forward.xpm>
#include <tb_close.xpm>
#include <tb_print.xpm>

HelpDialog::HelpDialog( QWidget* parent, const miString p, const miString s )
  : QDialog(parent,"HelpDialog"), closebutton(0), path(p), firstdoc(true)
{
  ConstructorCernel( path, s );
}


void HelpDialog::ConstructorCernel( const miString& filepath, 
				    const miString& source)
{
  setCaption( tr("Diana Documentation") );

  m_font= qApp->font();

  backwardbutton=forwardbutton=0;

  tb = new Q3TextBrowser( this ); 
  
  Q3MimeSourceFactory::defaultFactory()->addFilePath(QString(filepath.c_str()) );
 
  if ( source.exists() ) tb->setSource( QString(source.c_str()) );  

  connect( tb, SIGNAL(backwardAvailable(bool)),this,SLOT(backwardAvailable(bool)));
  connect( tb, SIGNAL(forwardAvailable(bool)),this,SLOT(forwardAvailable(bool)));
  
  backwardbutton= new QPushButton( QPixmap(back_xpm), "", this );
  QToolTip::add(backwardbutton,tr("Go back one page"));
  backwardbutton->setEnabled(false);
  connect(backwardbutton, SIGNAL( clicked()), tb, SLOT( backward()));

  forwardbutton= new QPushButton( QPixmap(forward_xpm), "", this );
  QToolTip::add(forwardbutton,tr("Go forward one page"));
  forwardbutton->setEnabled(false);
  connect(forwardbutton, SIGNAL( clicked()), tb, SLOT( forward())); 

  closebutton= new QPushButton( QPixmap(tb_close_xpm),
				tr("Close"), this );
  QToolTip::add(closebutton,tr("Close window"));
  connect( closebutton, SIGNAL( clicked()), this, SLOT( hideHelp()) );

  printbutton= new QPushButton( QPixmap(tb_print_xpm),
				tr("Print.."), this );
  QToolTip::add(printbutton,tr("Print current document"));
  connect( printbutton, SIGNAL( clicked()), this, SLOT( printHelp()) );

  hlayout = new Q3HBoxLayout( 5 );    
  hlayout->addWidget( backwardbutton );
  hlayout->addWidget( forwardbutton );
  hlayout->addWidget( closebutton );
  hlayout->addWidget( printbutton );
  hlayout->addStretch();
  
  vlayout = new Q3VBoxLayout( this, 5, 5 );
  vlayout->addLayout( hlayout );
  vlayout->addWidget( tb );
  
  resize( 800, 600 );
}

void HelpDialog::backwardAvailable(bool b)
{
  if ( backwardbutton ) backwardbutton->setEnabled(b);
}

void HelpDialog::forwardAvailable(bool b)
{
  if ( forwardbutton ) forwardbutton->setEnabled(b);
}


void HelpDialog::hideHelp()
{
  hide();
}

void HelpDialog::printHelp()
{
  QPrinter printer;
#ifdef linux
  printer.setPrintProgram( QString("lpr") );
#else
  printer.setPrintProgram( QString("lp") );
#endif
  printer.setFullPage(TRUE);

  QString htmltext= tb->text();

  QFont font;

  // Split source for proper page-breaks
  QString sep("<!-- PAGEBREAK -->"); // the magic separator
  QStringList vs = QStringList::split ( sep, htmltext, false);

  bool usepagebreak = (vs.size() > 1);

  int frompage= 1;
  int topage = vs.size();

  if (usepagebreak){
    printer.setMinMax(frompage,topage);
    printer.setFromTo(frompage,topage);
  }

  if ( printer.setup( this ) ) {
    QPainter p( &printer );
    Q3PaintDeviceMetrics metrics(p.device());
    int dpix = metrics.logicalDpiX();
    int dpiy = metrics.logicalDpiY();
    const int margin = 40; // pt
    QRect body(margin*dpix/72, margin*dpiy/72,
	       metrics.width()-margin*dpix/72*2,
	       metrics.height()-margin*dpiy/72*2 );
    
    frompage= printer.fromPage();
    topage=   printer.toPage();

    bool firstpage= true;
    int page = (usepagebreak ? 0 : 1);

    Q3ProgressDialog* progress = 0;

    if (usepagebreak && vs.size() > 1){
      // make a progress-dialog if more than one pages
      progress= new Q3ProgressDialog( tr("Printing document..."),
				     tr("Cancel printing "), topage - frompage + 1,
				     this, "printing", TRUE );
    }

    for ( QStringList::Iterator it = vs.begin(); it != vs.end(); ++it ) {
      // print entire document or one page
      if (usepagebreak){
	page++;
	if (page < frompage)
	  continue;
	if (page > topage)
	  break;
	
	if (progress){ // show progress in dialog
	  progress->setProgress( page - frompage + 1);
	  qApp->processEvents();
	  if ( progress->wasCancelled() )
	    break;
	}
	if (!firstpage) printer.newPage();
      }

      firstpage= false;

      // reset all translations
      p.resetXForm();

      htmltext = *it;
      if (usepagebreak){
	// Add proper html-tags if necessary
	if (!htmltext.contains(QString("<html>")))
	  htmltext = QString("<html><head></head><body>") + htmltext;
	if (!htmltext.contains(QString("</html>")))
	  htmltext += QString("</body></html>");
      }

      Q3SimpleRichText richText( htmltext, font,
				tb->context(), tb->styleSheet(),
				tb->mimeSourceFactory(), body.height() );
      richText.setWidth( &p, body.width() );
      QRect view( body );
      do {
	richText.draw( &p, body.left(), body.top(), view, colorGroup() );
	view.moveBy( 0, body.height() );
	p.translate( 0 , -body.height() );
	p.drawText( view.right() -
		    p.fontMetrics().width( QString::number(page) ),
		    view.bottom() +
		    p.fontMetrics().ascent() + 5, QString::number(page) );
	if ( view.top()  >= richText.height() )
	  break;
	printer.newPage();
	page++;
      } while (TRUE);

    }
    if (usepagebreak && progress){
      progress->setProgress( progress->totalSteps());
    }
  }
}

void HelpDialog::addFilePath( const miString& filepath )
{
  Q3MimeSourceFactory::defaultFactory()->addFilePath( QString(filepath.c_str()) );
  return;
}


void HelpDialog::setSource( const miString& source )
{
  tb->setSource( QString(source.c_str()) );
  tb->update();

  if ( firstdoc ){
    tb->home();
    firstdoc= false;
  }
  return;
}


void HelpDialog::showdoc(const miString doc)
{
  setSource( doc );
  show();
}


void HelpDialog::jumpto( const miString tag )
{
  if ( tag.exists() )
    tb->scrollToAnchor( QString(tag.cStr()) );
  show();
}

