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
#include <qtable.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <face_grin.xpm>

#include <qtEditTimeDialog.h>
#include <qtGridEditDialog.h>
#include <qlayout.h>


#include <iostream>
//#include <miString.h>

EditTimeDialog::EditTimeDialog( QWidget* parent, GridEditManager* gm )
    : QDialog(parent)
{

  gridm = gm;
  setCaption(tr("Profet object timemanager"));

  initialized = false;

  gridEditDialog = new GridEditDialog(this,gridm);
  connect( gridEditDialog, SIGNAL( updateGL() ), SIGNAL( updateGL() ) );
  connect( gridEditDialog, SIGNAL( markCell(bool) ), SLOT( markCell(bool) ) );
  connect( gridEditDialog, SIGNAL( parameterChanged() ), 
	   SLOT( parameterChanged() ) );
  connect( gridEditDialog, SIGNAL( paintMode(int) ), 
	   SIGNAL ( paintMode(int) ) );
  connect( gridEditDialog, SIGNAL(gridAreaChanged()), 
	   SIGNAL(gridAreaChanged()));

  sv = new QScrollView(this);
  sv->setResizePolicy(QScrollView::AutoOneFit);

  tableFrame           = new QFrame(sv);
  QVBoxLayout *flayout = new QVBoxLayout(tableFrame);
  table                = new QTable(tableFrame);
  dayTable             = new QHBoxLayout();
  header               = new QLabel(tableFrame);


  header->setFrameStyle( QFrame::Panel | QFrame::Raised );
  header->setBackgroundColor(QColor(88,150,237));



  connect( table, SIGNAL( clicked(int,int,int,const QPoint&) ),
	   this, SLOT( cellClicked(int,int) ) );
  
  QVBoxLayout* layout = new QVBoxLayout(this);
  flayout->addWidget(header);
  flayout->addLayout(dayTable);
  flayout->addWidget(table);
  

  sv->addChild(tableFrame);
  layout->addWidget(sv);

  QHBoxLayout* hlayout = new QHBoxLayout();
  
  hideButton = new QPushButton(tr("Hide"),this);
  connect( hideButton, SIGNAL( clicked() ), SLOT( hide() ));
  connect( hideButton, SIGNAL( clicked() ), SIGNAL( hideDialog() ));
  hlayout->addWidget( hideButton );

  refreshButton = new QPushButton(tr("Update"),this);
  connect( refreshButton, SIGNAL( clicked() ), SLOT( refreshClicked() ));
  hlayout->addWidget( refreshButton );               

  quitButton = new QPushButton(tr("Quit"),this);
  connect( quitButton, SIGNAL( clicked() ), SLOT( quitClicked() ));
  hlayout->addWidget( quitButton );

  layout->addLayout( hlayout );

}


void EditTimeDialog::showDialog()
{

  if(!initialized) {
    initialized = init();
  }

  if(initialized) show();

}

bool EditTimeDialog::init()
{
  vector<miString>       parameter;
  vector<miTime>         time;
  vector< vector<bool> > mark;
  miString               headString;

  if( !gridm->initSession(time,parameter,mark,headString)){
    return false;
  }

  int nhLabel = time.size();
  int nvLabel = parameter.size();

  table->setNumRows(nvLabel);
  table->setNumCols(nhLabel);

  header->setText(headString.cStr());
  header->setAlignment(Qt::AlignHCenter);
  QFont Fo=header->font();
  Fo.setPointSize(Fo.pointSize()+2);
  header->setFont(Fo);
  Fo.setPointSize(Fo.pointSize()-1);

  int tday   = time[0].day();
  int twidth = 0;

  table->setHScrollBarMode(QScrollView::AlwaysOff); 
  table->setVScrollBarMode(QScrollView::AlwaysOff); 

  for(int i=0; i<nvLabel;i++){
    table->verticalHeader()->setLabel(i,parameter[i].cStr());
    parameterMap[parameter[i]] = i;
  }

  vector<int>    dayWidth;
  vector<miDate> days; 
  dayWidth.push_back( table->verticalHeader()->width());
  miDate         day= time[0].date();

  int wid=0;

  for(int i=0; i<nhLabel;i++){
    timeMap[i]   = time[i];
    miString  hour(time[i].hour());
    table->horizontalHeader()->setLabel(i,hour.cStr());
    table->setColumnWidth(i,50);

    if(day !=time[i].date() ) {
      dayWidth.push_back(wid);
      days.push_back(day);
      wid=0;
      day=time[i].date();
    }
    wid+=table->columnWidth(i);
  }
  dayWidth.push_back(wid);
  days.push_back(time[nhLabel-1].date());


  table->horizontalHeader()->setStretchEnabled(false );
  table->horizontalHeader()->setClickEnabled(  false );
  table->horizontalHeader()->setResizeEnabled( false );

  int labh = 0;
  int w    = 0;
  for(int i=0;i<dayWidth.size();i++) {
    miString l;
  
    QLabel * lab = new QLabel(tableFrame);

    lab->setFixedWidth(dayWidth[i]);
    lab->setAlignment(Qt::AlignHCenter);
    lab->setFont(Fo);
    if(i) { 
      l=days[i-1].format("<b>%A %e.%b</b>"); 
      lab->setText(l.cStr());
      if(lab->height() > labh)
	labh=lab->height();
    }      
    w+=dayWidth[i];
    
    dayTable->addWidget(lab);
  }

  dayTable->addStretch(1);
  

  
  //mark cells
  QRect rect = table->cellRect( 0, 1 );
  gpixmap    = QPixmap( rect.width(), rect.height() );
  rpixmap    = QPixmap( rect.width(), rect.height() );
  ypixmap    = QPixmap( rect.width(), rect.height() );
    


  rpixmap.fill(QColor("red"));
  gpixmap.fill(QColor("lightGray"));
  ypixmap.fill(QColor("yellow"));
    

  tday   = time[0].day();
  
  whitecell.clear();
  for(int i=0; i<nhLabel;i++) {
    int dd=time[i].day(); 
    whitecell.push_back((dd-tday)%2);
    for(int j=0; j<nvLabel;j++) {
      table->clearCell(j,i);
      if(mark[j][i]){
	table->setPixmap(j,i,rpixmap);
      } else if((dd-tday)%2){  
	table->setPixmap(j,i,gpixmap);
      }
    }
  }
  
  int h=table->height()+header->height()+labh;
  w+=30;
  resize(w,h/2);

  return true;

}

void EditTimeDialog::cellClicked(int row, int col)
{
  //  cerr <<"EditTimeDialog::cellClicked:"<<row<<"   "<<col<<endl;

  //Prev cell
  if(table){
    if(crow >= 0 && crow >= 0   &&
       crow <  table->numRows() &&
       ccol <  table->numCols())
      if( !gridm->currentObjectOK() ){
	if(whitecell[ccol]){
	  table->setPixmap(crow,ccol,gpixmap);
	} else {
	  table->clearCell(crow,ccol);
	}
      }
  }

  miTime time = timeMap[col];
  gridm->setCurrent(table->verticalHeader()->label(row).latin1(),time);

  //remember cell 
  crow=row;
  ccol=col;

  //set colour
  if( gridm->currentObjectOK() ){
    table->setPixmap(crow,ccol,rpixmap);
  } else {
    table->setPixmap(crow,ccol,ypixmap);
  }

  //GridEditDialog
  gridEditDialog->changeDialog();
  gridEditDialog->show();
  
  //Plot string
  miString plotString;
  plotString =  "FIELD ";
  plotString += "Profet "; //model
  plotString += table->verticalHeader()->label(row).latin1();

  emit showField(plotString);
  emit setTime(time);
  emit apply();

}

void EditTimeDialog::parameterChanged()
{
  int col = table->currentColumn();
  int row = row=parameterMap[gridm->getCurrentParameter()];

  if(col > -1 && row > -1 ) {
    table->setCurrentCell(row,col);
    cellClicked(row,col);
  }

}

void EditTimeDialog::refreshClicked()
{
  gridm->refresh();
  gridEditDialog->changeDialog();
  
}

void EditTimeDialog::quitClicked()
{

  initialized=false;
  gridEditDialog->hide();
  gridm->quit();
  emit hideDialog();
  emit showField("REMOVE FIELD Profet");
  emit apply();

}

void EditTimeDialog::markCell(bool mark)
{

  if(mark){
    table->setPixmap(table->currentRow(),table->currentColumn(),rpixmap);
  } else {
    table->setPixmap(table->currentRow(),table->currentColumn(),ypixmap);
  }
}


void EditTimeDialog::catchGridAreaChanged()
{
  //  gridm->gridAreaChanged();
  gridEditDialog->catchGridAreaChanged();

}

