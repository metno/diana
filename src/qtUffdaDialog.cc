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

#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox> 
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <fstream>
#include <qtUtility.h>
#include <qtUffdaDialog.h>
#include <diStationPlot.h>

/***************************************************************************/

UffdaDialog::UffdaDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent), m_ctrl(llctrl)
{
#ifdef dUffdaDlg 
  cout<<"UffdaDialog::UffdaDialog called"<<endl;
#endif

  //caption to appear on top of dialog
  setWindowTitle(tr("Uffda"));
    
  //********** the list of satellites to choose from **************
  satlist = new QListWidget( this );

  connect( satlist, SIGNAL( itemClicked(QListWidgetItem* ) ), 
  SLOT( satlistSlot(QListWidgetItem*) ) );

  //********** the list of classes to choose from **************
  classlist = new QListWidget( this );

  vector <miString> vUffdaClass;
  vector <miString> vUffdaClassTip;

  m_ctrl->getUffdaClasses(vUffdaClass,vUffdaClassTip);
  int n=vUffdaClass.size();
  int m=vUffdaClassTip.size();
  for (int i=0;i<n;i++){
    classlist->addItem(QString(vUffdaClass[i].c_str()));
  }

  connect( classlist, SIGNAL( itemClicked(QListWidgetItem* ) ), 
  SLOT( classlistSlot(QListWidgetItem*) ) );

  // qt4 fix: new routine to make ToolTip
  //t=new DynamicTip(classlist,vUffdaClassTip);


  //****  the box (with label) showing which positions have been choosen ****
      posLabel = TitleLabel( tr("Selected positions"), this);
  poslist = new QListWidget( this );
//   poslist->setMinimumHeight(HEIGHTLISTBOX);
//   poslist->setMinimumWidth(WIDTHLISTBOX);
  connect( poslist, SIGNAL( itemClicked(QListWidgetItem* ) ), 
  SLOT( poslistSlot(QListWidgetItem*) ) );

  satIndex=-1;
  classIndex=-1;

  //push buttons to delete or store  selections
  Deleteb = NormalPushButton(tr("Delete"), this );
  connect( Deleteb, SIGNAL(clicked()), SLOT(DeleteClicked()));
  DeleteAllb = NormalPushButton(tr("Delete All"), this );
  connect( DeleteAllb, SIGNAL( clicked() ), SLOT( DeleteAllClicked() )); 
  storeb = NormalPushButton(tr("Save"), this );
  connect( storeb, SIGNAL(clicked()), SLOT(storeClicked()));


  //push button to show help
  helpb = NormalPushButton( tr("Help"), this);  
  connect( helpb, SIGNAL(clicked()), SLOT( helpClicked()));    
  //push button to hide dialog
  hideb = NormalPushButton(tr("Hide"), this);
  connect( hideb, SIGNAL(clicked()), SIGNAL(uffdaHide()));
  //push button to send
  sendb = NormalPushButton(tr("Send"), this );
  connect( sendb, SIGNAL(clicked()), SLOT(sendClicked()));


// ********************* place all the widgets in layouts ****************

  QVBoxLayout* v3layout = new QVBoxLayout();
  v3layout->addWidget(satlist );
  v3layout->addWidget(classlist,3 );
  v3layout->addWidget(posLabel );
  v3layout->addWidget(poslist,2 );


  QHBoxLayout* h1layout = new QHBoxLayout();
  h1layout->addWidget( Deleteb );
  h1layout->addWidget( DeleteAllb );

  h1layout->addWidget( helpb );
  h1layout->addWidget( hideb );
  h1layout->addWidget( storeb );
  h1layout->addWidget( sendb );


  //now create a vertical layout to put all the other layouts in
  QVBoxLayout* vlayout = new QVBoxLayout( this);                            
  vlayout->addLayout( v3layout ); 
  vlayout->addLayout( h1layout );
  //stationplotpointer
  sp=0;


  mailto= m_ctrl->getUffdaMailAddress();
  if (mailto.empty()) mailto="uffda@met.no";


  //end of constructor
}

/***************************************************************************/
void UffdaDialog::classlistSlot(QListWidgetItem*){
  //called when an uffda class is selected
#ifdef dUffdaDlg 
    cerr<<"UffdaDialog::classlistSlot called"<<endl;
#endif 
    int posIndex = poslist->currentRow();
    if (posIndex < 0) return;
    QString satclass = classlist->currentItem()->text();
    v_uffda[posIndex].satclass=satclass;
    updatePoslist(v_uffda[posIndex],posIndex,false);
}

/***************************************************************************/
void UffdaDialog::satlistSlot(QListWidgetItem* item){
  //called when a satellite is selected
#ifdef dUffdaDlg 
    cerr<<"UffdaDialog::satlistSlot called"<<endl;
#endif 
    int posIndex = poslist->currentRow();
    if (posIndex < 0) return;
    QString sattime = satlist->currentItem()->text();
    v_uffda[posIndex].sattime=sattime;
    updatePoslist(v_uffda[posIndex],posIndex,false);
}



/***************************************************************************/
void UffdaDialog::poslistSlot(QListWidgetItem* item){
  //called when a satellite is selected
#ifdef dUffdaDlg 
    cerr<<"UffdaDialog::poslistSlot called"<<endl;
#endif 
    int posIndex=poslist->row(item);
    if (posIndex < 0) return;
    if (sp)
      sp->setSelectedStation(posIndex);
    emit stationPlotChanged();
}


/***************************************************************************/
void UffdaDialog::storeClicked(){
  //called when store button is pressed
#ifdef dUffdaDlg 
    cerr<<"UffdaDialog::storeClicked called"<<endl;
#endif 
    miString body=getUffdaString();  
    // write to local file (backup)
    ofstream file("uffda.dat");
    bool okfile;
    if (file){ 
      okfile=true;
      file << body;
      file.close();
    }
    QString messagestring;
    if (okfile)
      messagestring= " <nobr>" + tr("Data saved in") + " <i>uffda.dat</i></nobr>"; 
    else
      messagestring= "<nobr>" + tr("Data not saved") +"</nobr>"; 
    QMessageBox::information(this,tr("Uffda - info"),messagestring);
}


/***************************************************************************/
void UffdaDialog::sendClicked(){
  //called when help button is pressed
#ifdef dUffdaDlg 
    cerr<<"UffdaDialog::sendClicked called"<<endl;
#endif 
    bool okbody=false,okfile=false;
    miString subject ="REPLY";
    miString body=getUffdaString();  
    if (!body.empty()){
      // send mail to uffda user 
      okbody=true;
      send(mailto,subject,body); // this is the most important part
      // also write to local file (backup)
      ofstream file("uffda.dat");
      if (file){ 
	okfile=true;
	file << body;
	file.close();
      }
      //clear all postions
      DeleteAllClicked();
    }
    //the rest is just to show a messagebox to the user, not so important
    QString messagestring,tmp1; 
    QString tmp2= mailto.c_str();
    if (okbody){ 
      tmp1=" <nobr>" + tr("Data sent to") + "<i> "; 
      QString tmp3= "</i></nobr><br> <nobr> " + tr("Check incoming mail for receipt (up to 2 minutes)!") +
						   "</nobr> ";
      messagestring = tmp1+tmp2+tmp3;
      if (okfile){
	QString tmp4= "<br><nobr>" + tr("(Data also saved in local file") + " <i>uffda.dat</i></nobr>)"; 
	messagestring+=tmp4;
      }
    }
    else{
      tmp1=" <nobr>" + tr("Data not sent to") + " <i>";
      QString tmp3= "</i></nobr><br> <nobr> " + tr("Incomplete information") + "</nobr>";   
      messagestring = tmp1+tmp2 +tmp3;
    } 
      QMessageBox::information(this,tr("Uffda - info"),messagestring);
}


    
/***************************************************************************/
void UffdaDialog::DeleteClicked(){
  //called when delete/slett button is called
  //unselects and unhighlights everything
#ifdef dUffdaDlg 
    cerr<<"UffdaDialog::DeleteClicked called"<<endl;
#endif 
    //clear uffda deque
    int posIndex = poslist->currentRow();
    if (posIndex < 0) return;
    v_uffda.erase(v_uffda.begin()+posIndex);
    //clear listbox
    poslist->takeItem(posIndex);
    //new stationPlot
    updateStationPlot();
    emit stationPlotChanged();
    posIndex--;
    poslist->setCurrentRow(posIndex);
}



/***************************************************************************/
void UffdaDialog::DeleteAllClicked(){
  //called when delete/slett button is called
  //unselects and unhighlights everything
#ifdef dUffdaDlg 
    cerr<<"UffdaDialog::DeleteAllClicked called"<<endl;
#endif 
    vector <float> vlat_uffda;
    vector <float> vlon_uffda;
    vector <miString> vname_uffda;
    //clear uffda deque
    v_uffda.clear();
    //clear listbox
    poslist->clear();
    //new and empty stationPlot
    sp = new StationPlot(vname_uffda,vlon_uffda,vlat_uffda);
    sp->setName("uffda");
    m_ctrl->putStations(sp);
    emit stationPlotChanged();
}




/********************************************/
void UffdaDialog::helpClicked(){
  emit showsource("uffda.html");
}


/********************************************/
bool UffdaDialog::close(bool alsoDelete){
  emit uffdaHide();
  return true;
} 

/********************************************/

bool UffdaDialog::okToExit(){
  if (v_uffda.size()){
    raise(); //put dialog on top
    switch(QMessageBox::information(this,tr("Uffda - info"),
                                    tr("You have not sent Uffda data !\n Send before exiting?"),
				       tr("&Send"), tr("&Don't send"), tr("&Cancel"),
                                    0,2)){
    case 0: // send clicked
      sendClicked();
      break;
    case 1: // don't save, but exit
      break;
    case 2:
      return false; // cancel operation
      break;
    }
  }
  return true;
}
/********************************************/

void UffdaDialog::addPosition(float lat, float lon){
#ifdef dUffdaDlg 
  cerr<<"UffdaDialog::addPosition called"<<endl;
#endif 
  int currIndex=-1;
  QString sattime="dummy";
  uffdaElement ue;
  ue.lat=lat;
  ue.lon=lon;

  //index=satlist->currentItem();
  if (satIndex > -1) sattime = satlist->item(satIndex)->text();
  //now get list of satellites available
  satlist->clear();
  vector <miString> satnames = m_ctrl->getSatnames();
  for (int i=0; i<satnames.size(); i++){
    QString satname = QString(satnames[i].c_str());
    satname=satname.simplifyWhiteSpace();
    satlist->addItem(satname);
    if (satnames[i]==sattime.toStdString()) currIndex=i;
  }
  if (currIndex>-1){
    satlist->setCurrentRow(currIndex);
    ue.sattime=sattime;
  }

  //index=classlist->currentItem();
  if (classIndex > -1){
    classlist->setCurrentRow(classIndex);
    ue.satclass = classlist->item(classIndex)->text();
  }
  updatePoslist(ue,0,true);
  v_uffda.push_front(ue);
  updateStationPlot();
}
 

/********************************************/

void UffdaDialog::updatePoslist(uffdaElement &ue,int nr, bool newItem) {
#ifdef dUffdaDlg
  cout<<"uffdaDialog::update_posList"<<endl;
#endif
  //Make string and insert in posList
  ue.ok=true;
 QString uffStr,latStr,lonStr;
  latStr.setNum(fabsf(ue.lat),'f',2);
  if(ue.lat<0)
    latStr+= "S";
  else
    latStr+= "N";
  lonStr.setNum(fabsf(ue.lon),'f',2);
  if(ue.lon<0)
    lonStr += "W";
  else
    lonStr += "E";
  ue.posstring=latStr + "  " + lonStr;
  if (!ue.sattime.isEmpty())
    uffStr=ue.sattime+" ";
  else
    ue.ok=false;
  uffStr+=ue.posstring;
  if (!ue.satclass.isEmpty())
    uffStr+=" " + ue.satclass;
  else
    ue.ok=false;
  if (newItem)
    poslist->insertItem(0,uffStr);
  else
    poslist->item(nr)->setText(uffStr);
  poslist->setCurrentRow(nr);
}

/********************************************/

void UffdaDialog::updateStationPlot(){
  vector <float> vlat_uffda;
  vector <float> vlon_uffda;
  vector <miString> vname_uffda;
  int n= v_uffda.size();
  for (int i=0;i<n;i++){  
    vlat_uffda.push_back(v_uffda[i].lat);
    vlon_uffda.push_back(v_uffda[i].lon);
    vname_uffda.push_back(v_uffda[i].posstring.toStdString());
  }
  sp = new StationPlot(vname_uffda,vlon_uffda,vlat_uffda);
  sp->setName("uffda");
  m_ctrl->putStations(sp);
  sp->setSelectedStation(poslist->currentRow());
}


/********************************************/

void UffdaDialog::clearSelection(){
  classlist->clearSelection();
  classIndex=-1;
  satlist->clearSelection();
  satIndex=-1;
}

/********************************************/

void UffdaDialog::pointClicked(miString uffstation){
   int n= v_uffda.size();
   for (int i=0;i<n;i++){  
     if (v_uffda[i].posstring.toStdString()== uffstation){
       poslist->setCurrentRow(i);
      return;
    }
  }  
}

/********************************************/

miString UffdaDialog::getUffdaString(){
  miString uffstr;
  int n= poslist->count();
  for (int i=0;i<n;i++){  
    if (i<v_uffda.size() && v_uffda[i].ok)
      uffstr+= poslist->item(i)->text().toStdString() + miString("\n");
  }  
  return uffstr;
}

/********************************************/

// fra Juergen - sendmail rutine
//
void UffdaDialog::send(const miString& to, 
	  const miString& subject,const miString& body)
{
   FILE* sendmail;
   sendmail=popen("/usr/lib/sendmail -t","w");
   fprintf(sendmail,"To: %s\nSubject: %s\n\n",to.cStr(),subject.cStr());
   fprintf(sendmail,"<DATA>\n");   
   fprintf(sendmail,body.cStr());
   fprintf(sendmail,"\n</DATA>\n");
   pclose(sendmail);
} 


/********************************************/

/*DynamicTip::DynamicTip( QWidget * parent,vector <miString> tips )
    : QToolTip( parent )
{
  int i, n=tips.size();
  for (i=0;i<n;i++){
  vClasstips.push_back(tips[i].c_str());
  }
}*/

// qt4 fix: ported to new QToolTip API
// use each widgets setToolTip(const QString &) instead
/*void DynamicTip::maybeTip( const QPoint &pos )
{
  Q3ListBox * list ((Q3ListBox*)parentWidget()) ;
  Q3ListBoxItem * it = list->itemAt(pos);
  if (it){
    int i= list->index(it);
    if (i>-1 && i<vClasstips.size()){
      QRect tr=list->itemRect(it);
      tip(tr,vClasstips[i]);
    }
  }
}*/
































