/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diana_config.h"

#include "qtSatDialog.h"

#include "diController.h"
#include "diFont.h"
#include "diPlotOptions.h"
#include "diSatDialogData.h"
#include "diUtilities.h"
#include "qtSatDialogAdvanced.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "util/misc_util.h"
#include "util/time_util.h"

#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPixmap>
#include <QRadioButton>
#include <QSlider>
#include <QToolTip>
#include <QVBoxLayout>
#include <qmessagebox.h>

#include <iomanip>
#include <sstream>

#include "down12x12.xpm"
#include "sat.xpm"
#include "up12x12.xpm"

#define MILOGGER_CATEGORY "diana.SatDialog"
#include <miLogger/miLogging.h>

using namespace std;

#define HEIGHTLISTBOX 45


/*********************************************/
SatDialog::SatDialog(SatDialogData* sdd, QWidget* parent)
    : DataDialog(parent, 0)
    , sdd_(sdd)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle(tr("Satellite and radar"));
  m_action = new QAction(QIcon(QPixmap(sat_xpm)), windowTitle(), this);
  m_action->setShortcut(Qt::ALT + Qt::Key_S);
  m_action->setCheckable(true);
  m_action->setIconVisibleInMenu(true);
  helpFileName = "ug_satdialogue.html";

  dialogInfo = sdd->initSatDialog();

  //put satellite names (NOAA,Meteosat,radar) in namebox
  vector<std::string> name;
  int nImage=dialogInfo.image.size();
  for( int i=0; i< nImage; i++ )
    name.push_back(dialogInfo.image[i].name);
  namebox = ComboBox( this, name, true, 0);
  connect( namebox, SIGNAL( activated(int) ),
	   SLOT( nameActivated(int) )  );


  //****

  //fileListWidget contains filetypes for each satellite
  //NOAA Europa N-Europa etc.
  //METEOSAT Visuell, IR
  //insert filetypes for default sat - m_image[0]-NOAA
  fileListWidget = new QListWidget( this );
  fileListWidget->setMinimumHeight(HEIGHTLISTBOX);

  updateFileListWidget(0);
  connect( fileListWidget, SIGNAL( itemClicked( QListWidgetItem * ) ),
	   SLOT( fileListWidgetClicked( QListWidgetItem * ) )  );

  autoButton = new ToggleButton(this, tr("Auto"));
  timeButton = new ToggleButton(this, tr("Time"));
  fileButton = new ToggleButton(this, tr("File"));
  timefileBut = new QButtonGroup( this );
  timefileBut->addButton(autoButton,0);
  timefileBut->addButton(timeButton,1);
  timefileBut->addButton(fileButton,2);
  QHBoxLayout* timefileLayout = new QHBoxLayout();
  timefileLayout->addWidget(autoButton);
  timefileLayout->addWidget(timeButton);
  timefileLayout->addWidget(fileButton);
  timefileBut->setExclusive( true );
  autoButton->setChecked(true);
  //timefileClicked is called when auto,tid,fil buttons clicked
  connect( timefileBut, SIGNAL( buttonClicked(int) ),
	   SLOT( timefileClicked(int) ) );

  //list of times or files will be filled when "tid","fil" clicked
  // (in timefileClicked)

  timefileList = new QListWidget( this);
  timefileList->setMinimumHeight(HEIGHTLISTBOX);

  //timefileListSlot called when an item (time of file) highlighted in
  // timefileList
  connect( timefileList, SIGNAL(itemClicked(QListWidgetItem *) ),
	   SLOT( timefileListSlot( QListWidgetItem *) ) );


  //channelbox filled with available channels
  QLabel *channellabel= TitleLabel( tr("Channels"), this);
  channelbox = new QListWidget( this);
  channelbox->setMinimumHeight(HEIGHTLISTBOX);

  //channelboxSlot called when an item highlighted in channelbox
  connect( channelbox, SIGNAL( itemClicked( QListWidgetItem * ) ),
	   SLOT( channelboxSlot( QListWidgetItem * ) ) );

  //****

  //pictures contains one or more selected pictures !
      QLabel *picturesLabel = TitleLabel(tr("Selected pictures"), this);
  pictures = new QListWidget( this );
  pictures->setMinimumHeight(HEIGHTLISTBOX);

  connect( pictures, SIGNAL( itemClicked( QListWidgetItem * ) ),
	   SLOT( picturesSlot( QListWidgetItem * ) ) );


  //**  all the other QT buttons


  // upPicture
  QPixmap upPicture = QPixmap(up12x12_xpm);
  upPictureButton = PixmapButton( upPicture, this, 14, 12 );
  upPictureButton->setEnabled( false );

  connect( upPictureButton, SIGNAL(clicked()), SLOT(upPicture()));

  // downPicture
  QPixmap downPicture = QPixmap(down12x12_xpm);
  downPictureButton = PixmapButton( downPicture, this, 14, 12 );
  downPictureButton->setEnabled( false );

  connect( downPictureButton, SIGNAL(clicked()), SLOT(downPicture()));

  Delete = NormalPushButton( tr("Delete"), this );
  connect( Delete, SIGNAL(clicked()),
	   SLOT(DeleteClicked()));

  DeleteAll = NormalPushButton( tr("Delete All"), this );
  connect( DeleteAll, SIGNAL(clicked()),
	   SLOT(DeleteAllClicked()));

  multiPicture = new ToggleButton(this, tr("Add picture"));
  multiPicture->setToolTip(tr("Add new picture if any of above settings change"));

  mosaic = new ToggleButton(this, tr("Mosaic"));
  connect( mosaic, SIGNAL( toggled(bool)), SLOT( mosaicToggled( bool) ));
  mosaic->setChecked(false);
  mosaic->setEnabled(false);

  //SLIDER FOR MAX TIME DIFFERENCE
  m_scalediff= dialogInfo.timediff.scale;
  QLabel *diffLabel = new QLabel( tr("Time diff"), this);
  int difflength=dialogInfo.timediff.maxValue/40+3;
  diffLcdnum= LCDNumber( difflength, this);
  diffSlider= new QSlider(Qt::Horizontal, this );
  diffSlider->setMinimum(dialogInfo.timediff.minValue);
  diffSlider->setMaximum(dialogInfo.timediff.maxValue);
  diffSlider->setValue(dialogInfo.timediff.value);
  QHBoxLayout* difflayout = new QHBoxLayout();
  difflayout->addWidget( diffLabel,0,0 );
  difflayout->addWidget( diffLcdnum, 0,0 );
  difflayout->addWidget( diffSlider,0,0  );
  connect( diffSlider, SIGNAL( valueChanged(int)), SLOT( doubleDisplayDiff(int) ));

  advanced = new ToggleButton(this, tr("<<Less"), tr("More>>"));
  advanced->setChecked(false);
  connect(advanced, &QAbstractButton::toggled, this, &DataDialog::showMore);

  QHBoxLayout* hlayout2 = new QHBoxLayout();
  hlayout2->addWidget(upPictureButton);
  hlayout2->addWidget( Delete );
  hlayout2->addWidget( DeleteAll );

  QHBoxLayout* hlayout1 = new QHBoxLayout();
  hlayout1->addWidget(downPictureButton);
  hlayout1->addWidget( multiPicture );
  hlayout1->addWidget( mosaic );

  QHBoxLayout* hlayout4 = new QHBoxLayout();
  hlayout4->addWidget( advanced );

  QLayout* hlayout3 = createStandardButtons(true);

  QVBoxLayout* vlayout = new QVBoxLayout( this);
  vlayout->setMargin(10);
  vlayout->addWidget( namebox );
  vlayout->addWidget( fileListWidget );
  vlayout->addLayout( timefileLayout );
  vlayout->addWidget( timefileList );
  vlayout->addWidget( channellabel );
  vlayout->addWidget( channelbox );
  vlayout->addWidget( picturesLabel );
  vlayout->addWidget( pictures );
  vlayout->addLayout( hlayout2 );
  vlayout->addLayout( hlayout1 );
  vlayout->addLayout( difflayout );
  vlayout->addLayout( hlayout4 );
  vlayout->addLayout( hlayout3 );
  vlayout->activate();
  //       vlayout->freeze();


  // INNITIALISATION AND DEFAULT
  doubleDisplayDiff(dialogInfo.timediff.value);

  this->hide();
  setOrientation(Qt::Horizontal);
  sda = new SatDialogAdvanced( this,  dialogInfo);
  setExtension(sda);
  showMore(false);
  connect(sda,SIGNAL(getSatColours()),SLOT(updateColours()));
  connect(sda,SIGNAL(SatChanged()),SLOT(advancedChanged()));

  times.clear();
}

SatDialog::~SatDialog()
{
}

std::string SatDialog::name() const
{
  static const std::string SAT_DATATYPE = "sat";
  return SAT_DATATYPE;
}

void SatDialog::doShowMore(bool show)
{
  showExtension(show);
}

void SatDialog::updateDialog()
{
}

/*********************************************/
void SatDialog::nameActivated(int in)
{
  METLIBS_LOG_SCOPE();

  //insert in fileListWidget the list of files for selected satellite
  //but no file is selected, takes to much time
  updateFileListWidget(in);

  //clear timefileList and channelbox
  timefileList->clear();
  channelbox->clear();
}

/*********************************************/
void SatDialog::fileListWidgetClicked(QListWidgetItem * item)
{
  METLIBS_LOG_SCOPE();

  diutil::OverrideCursor waitCursor;

  int index = timefileBut->checkedId();

  //restore options if possible
  std::string name = namebox->currentText().toStdString();
  std::string area = item->text().toStdString();
  if (not satoptions[name][area].empty()) {
    putOptions(decodeString(satoptions[name][area]));

    bool restore = multiPicture->isChecked();
    multiPicture->setChecked(false);
    timefileBut->button(index)->setChecked(true);
    timefileClicked(index);
    multiPicture->setChecked(restore);
  } else { //no options saved
    timefileBut->button(index)->setChecked(true);
    timefileClicked(index);
  }
}

/*********************************************/
void SatDialog::timefileClicked(int tt)
{
  /* DESCRIPTION: This function is called when timefileBut (auto/tid/fil)is
   selected, and is returned without doing anything if the new value
   selected is equal to the old one
   */
  METLIBS_LOG_SCOPE(LOGVAL(tt));

  if (fileListWidget->currentRow() == -1)
    return;

  //update list of files
  updateTimefileList();

  if (tt == 0) {
    // AUTO clicked

    sdd_->setSatAuto(true, namebox->currentText().toStdString(), fileListWidget->currentItem()->text().toStdString());

    //    timefileList->setEnabled( false );
    updateChannelBox(true);

  } else {
    // "time"/"file" clicked

    sdd_->setSatAuto(false, namebox->currentText().toStdString(), fileListWidget->currentItem()->text().toStdString());

    //    timefileList->setEnabled ( true );

    timefileListSlot(timefileList->currentItem());
  }
}

/*********************************************/
void SatDialog::timefileListSlot(QListWidgetItem *)
{
  /* DESCRIPTION: This function is called when the signal highlighted() is
   sent from the list of time/file and a new list item is highlighted
   */
  METLIBS_LOG_SCOPE();

  int index = timefileList->currentRow();

  if (index < 0 && timefileList->count() > 0)
    index = 0;

  if (index < 0 || int(files.size()) <= index)
    return;

  m_time = files[index].time;
  plottimes_t tt;
  tt.insert(m_time);
  emitTimes(tt, false);

  updateChannelBox(true);
}

/*********************************************/
void SatDialog::channelboxSlot(QListWidgetItem * item)
{
  /* DESCRIPTION: This function is called when a channel is selected
   -add new state to m_state (if picture already exists with this)
   satellite, just replace, else add.
   update pictures box accordingly
   */
  METLIBS_LOG_SCOPE();

  //currently selected channel
  //  if(channelbox->currentRow() == -1) return;
  m_channelstr = item->text().toStdString();

  int newIndex = addSelectedPicture();

  // AF: avoid coredump, and delete a previous selection in same "group"
  if (newIndex < 0) {
    METLIBS_LOG_DEBUG("   newIndex " << newIndex);
    if (newIndex == -1)
      DeleteClicked();
    return;
  }

  updatePictures(newIndex, false);
}


/*********************************************/
int SatDialog::addSelectedPicture()
{
  /* DESCRIPTION: this function adds a new picture to
   the list of pictures to plot, in the vector m_state,
   with information from the dialogbox...
   If a picture already exists for this satellite, replace,
   else add a new one.
   return index for this picture...
   */
  METLIBS_LOG_SCOPE();

  if (!files.size())
    return -2;

  std::string fstring;
  miutil::miTime ltime;
  if (timeButton->isChecked() || fileButton->isChecked()) {
    //"time"/"file" clicked, find filename
    int current = timefileList->currentRow();
    if (current > -1 && current < int(files.size())) {
      fstring = files[current].name;
      ltime = files[current].time;
    }
  }

  //define a new state !
  state lstate;
  lstate.iname = namebox->currentIndex();
  lstate.iarea = fileListWidget->currentRow();
  lstate.iautotimefile = timefileBut->checkedId();
  lstate.ifiletime = timefileList->currentRow();
  lstate.ichannel = channelbox->currentRow();
  lstate.name = namebox->currentText().toStdString();
  lstate.area = fileListWidget->currentItem()->text().toStdString();
  lstate.filetime = ltime;
  lstate.channel = channelbox->currentItem()->text().toStdString();
  lstate.filename = fstring;
  lstate.mosaic = false;
  lstate.totalminutes = 60;

  int newIndex = -1;

  //check if we tried to select an existing picture
  //   for( int i=0; i<m_state.size(); i++ ){
  //      if(pictureString(lstate,true)== pictureString(m_state[i],true)){
  //        //set already existing picture as selected
  //        newIndex=i;
  //        break;
  //      }
  //   }

  if (!multiPicture->isChecked()) {
    int i = pictures->currentRow();
    if (i > -1 && i < int(m_state.size())) {
      //replace existing picture(same sat). advanced options saved
      // get info about picture we are replacing
      vector<SatFileInfo> f = sdd_->getSatFiles(m_state[i].name, m_state[i].area, false);
      lstate.mosaic = m_state[i].mosaic;
      lstate.totalminutes = m_state[i].totalminutes;
      if ((f.size() && f[0].palette && !files[0].palette) || (f.size()
          && !f[0].palette && files[0].palette)) {
        //special case changing from palette to rgb or vice versa
        sda->setStandard();
        lstate.advanced = sda->getOKString();
      } else if (files[0].palette && lstate.area != m_state[i].area) {
        //clear colour selection in advanced
        lstate.advanced = m_state[i].advanced;
        sda->putOKString(lstate.advanced);
        vector<Colour> c;
        sda->setColours(c);
        lstate.advanced = sda->getOKString();
      } else {
        lstate.advanced = m_state[i].advanced;
      }
      m_state[i] = lstate;
      newIndex = i;
    }
  }

  if (newIndex == -1) {
    //new picture (new sat)
    sda->setStandard();
    lstate.advanced = sda->getOKString();
    m_state.push_back(lstate);
    newIndex = m_state.size() - 1;
  }

  return newIndex;
}

/*********************************************/
std::string SatDialog::pictureString(const state& i_state, bool timefile)
{
  /* make a string of the picture in m_state[i]*/
  METLIBS_LOG_SCOPE();

  std::string str = i_state.name;
  if (i_state.mosaic)
    str += " MOSAIKK ";
  str += " " + i_state.area + " " + i_state.channel + " ";
  if (timefile && !i_state.filename.empty())
    str += i_state.filetime.isoTime();
  return str;
}

/*********************************************/
void SatDialog::picturesSlot(QListWidgetItem*)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG("m_state.size:"<<m_state.size());

  const int index = pictures->currentRow();
  if (index > -1) {
    state& s = m_state[index];
    namebox->setCurrentIndex(s.iname);
    nameActivated(s.iname); // update fileListWidget
    fileListWidget->setCurrentItem(fileListWidget->item(s.iarea));
    if (s.iautotimefile == 0) {
      autoButton->setChecked(true);
      sdd_->setSatAuto(true, namebox->currentText().toStdString(), fileListWidget->currentItem()->text().toStdString());
    } else {
      if (s.iautotimefile == 1)
        timeButton->setChecked(true);
      else if (s.iautotimefile == 2)
        fileButton->setChecked(true);
      sdd_->setSatAuto(false, namebox->currentText().toStdString(), fileListWidget->currentItem()->text().toStdString());
      updateTimefileList();
      timefileList->setCurrentRow(s.ifiletime);
      plottimes_t tt;
      tt.insert(files[m_state[index].ifiletime].time);
      emitTimes(tt, false);
    }
    updateChannelBox(false);
    channelbox->setCurrentRow(s.ichannel);
    m_channelstr = s.channel;
    updateColours();
    sda->setPictures(pictureString(m_state[index], false));
    sda->putOKString(s.advanced);
    sda->greyOptions();
    int number = int(s.totalminutes / m_scalediff);
    diffSlider->setValue(number);
    mosaic->setChecked(s.mosaic);
    mosaic->setEnabled(true);
  } else {
    sda->setPictures("");
    sda->setColours(vector<Colour>());
  }

  //check if up and down buttons should be enabled
  upPictureButton->setEnabled(index > 1);
  downPictureButton->setEnabled(index >= 0 && index < (int)m_state.size() - 1);
}

/*********************************************/

void SatDialog::updateTimes()
{
  METLIBS_LOG_SCOPE();
  diutil::OverrideCursor waitCursor;

  // update times (files) of all selected pictures
  for (unsigned int i = 0; i < m_state.size(); i++) {
    sdd_->getSatFiles(m_state[i].name, m_state[i].area, true);
  }

  // update the timefilelist if "time" og "file" is selected
  if (!autoButton->isChecked())
    updateTimefileList(false);

  emitSatTimes();
}

/*********************************************/

void SatDialog::mosaicToggled(bool on)
{
  int index = pictures->currentRow();
  if (index > -1 && int(m_state.size()) > index)
    m_state[index].mosaic = on;
  updatePictures(index, false);
}

/*********************************************/

void SatDialog::advancedChanged()
{
  int index = pictures->currentRow();
  if (index > -1)
    m_state[index].advanced = sda->getOKString();
}

/**********************************************/

void SatDialog::upPicture()
{
  int index = pictures->currentRow();
  state lstate = m_state[index];
  m_state[index] = m_state[index - 1];
  m_state[index - 1] = lstate;
  updatePictures(index - 1, true);
}

/**********************************************/

void SatDialog::downPicture()
{
  int index = pictures->currentRow();
  state lstate = m_state[index];
  m_state[index] = m_state[index + 1];
  m_state[index + 1] = lstate;
  updatePictures(index + 1, true);
}

/*********************************************/
void SatDialog::doubleDisplayDiff(int number)
{
  /* This function is called when diffSlider sends a signald valueChanged(int)
   and changes the numerical value in the lcd display diffLcdnum */
  int totalminutes = int(number * m_scalediff);
  int index = pictures->currentRow();
  if (index > -1 && int(m_state.size()) > index)
    m_state[index].totalminutes = totalminutes;
  int hours = totalminutes / 60;
  int minutes = totalminutes - hours * 60;
  ostringstream ostr;
  ostr << hours << ":" << setw(2) << setfill('0') << minutes;
  diffLcdnum->display(QString::fromStdString(ostr.str()));
}

/*********************************************/
void SatDialog::DeleteAllClicked()
{
  METLIBS_LOG_SCOPE();

  fileListWidget->clearSelection();
  timefileList->clear();
  channelbox->clear();

  m_state.clear();
  //  m_picturesIndex = -1;
  pictures->clear();
  downPictureButton->setEnabled(false);
  upPictureButton->setEnabled(false);
  mosaic->setChecked(false);
  mosaic->setEnabled(false);
  sda->setPictures("");
  sda->setOff();
  sda->greyOptions();

  downPictureButton->setEnabled(false);
  upPictureButton->setEnabled(false);

  //Emit empty time list
  times.clear();
  emitTimes(times, false);
}

/*********************************************/
void SatDialog::DeleteClicked()
{
  METLIBS_LOG_SCOPE();

  if (m_state.size() == 1) {
    // check needed when empty list was found
    DeleteAllClicked();
  } else {
    if (pictures->currentRow() > -1) {
      //remove from m_state and picture listbox
      vector<state>::iterator p;
      p = m_state.begin() + pictures->currentRow();
      m_state.erase(p);
      pictures->takeItem(pictures->currentRow());
      //      if (m_picturesIndex > m_state.size()-1) m_picturesIndex--;
      //      updatePictures(pictures->currentRow());
      picturesSlot(pictures->currentItem());
      emitSatTimes();
    }
  }
}

/*********************************************/
PlotCommand_cpv SatDialog::getOKString()
{
  METLIBS_LOG_SCOPE();

  PlotCommand_cpv vstr;
  if (pictures->count()) {
    for (unsigned int i = 0; i < m_state.size(); i++) {
      SatPlotCommand_p cmd = makeOKString(m_state[i]);
      satoptions[m_state[i].name][m_state[i].area] = cmd->all();
      vstr.push_back(cmd);
    }
  }
  return vstr;
}

/********************************************/

SatPlotCommand_p SatDialog::makeOKString(state& okVar)
{
  /* This function is called by getOKString,
   makes the part of OK string corresponding to state okVar  */
  METLIBS_LOG_SCOPE();

  SatPlotCommand_p cmd = std::make_shared<SatPlotCommand>();

  cmd->satellite = okVar.name;
  cmd->filetype = okVar.area;
  cmd->filename = okVar.filename;
  cmd->plotChannels = okVar.channel;

  cmd->add("timediff", miutil::from_number(okVar.totalminutes));

  cmd->add("mosaic", okVar.mosaic ? "1" : "0");

  cmd->add(okVar.advanced);

  cmd->add(miutil::KeyValue(PlotOptions::key_fontname, diutil::BITMAPFONT));
  cmd->add(miutil::KeyValue(PlotOptions::key_fontface, "normal"));

  cmd->add(okVar.external);

  //should only be cleared if something has changed at this picture
  okVar.external.clear();

  return cmd;
}

/*********************************************
 ***********quickMenu functions***************
 **********************************************/

void SatDialog::putOKString(const PlotCommand_cpv& vstr)
{
  METLIBS_LOG_SCOPE();

  //update dialog
  DeleteAllClicked();

  if (vstr.empty())
    return;

  bool restore = multiPicture->isChecked();
  multiPicture->setChecked(true);

  m_state.clear();
  for (PlotCommand_cp pc : vstr) {
    SatPlotCommand_cp cmd = std::dynamic_pointer_cast<const SatPlotCommand>(pc);
    if (!cmd)
      continue;

    state okVar = decodeString(cmd->all());
    okVar.name = cmd->satellite;
    okVar.area = cmd->filetype;
    okVar.filename = cmd->filename;
    okVar.channel = cmd->plotChannels;
    if (okVar.name.empty() || okVar.area.empty() || okVar.channel.empty())
      break;

    bool found = false;
    int ns = namebox->count();
    for (int j = 0; j < ns; j++) {
      QString qstr = namebox->itemText(j);
      if (qstr.isNull())
        continue;
      std::string listname = qstr.toStdString();
      if (okVar.name == listname) {
        namebox->setCurrentIndex(j);
        nameActivated(j);
        found = true;
      }
    }
    if (!found)
      continue;

    found = false;
    int ng = fileListWidget->count();
    for (int j = 0; j < ng; j++) {
      QString qstr = fileListWidget->item(j)->text();
      if (qstr.isNull())
        continue;
      std::string listname = qstr.toStdString();
      if (okVar.area == listname) {
        fileListWidget->setCurrentRow(j);
        found = true;
      }
    }
    if (!found)
      continue;

    putOptions(okVar);
  }

  multiPicture->setChecked(restore);
}

void SatDialog::putOptions(const state& okVar)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(okVar.filetime) << LOGVAL(okVar.filename));
  if (!okVar.filetime.undef()) {
    timefileBut->button(1)->setChecked(true);
    updateTimefileList();
    int nt = files.size();
    for (int j = 0; j < nt; j++) {
      if (okVar.filetime == files[j].time) {
        timefileList->setCurrentRow(j);
      }
    }
  } else if (!okVar.filename.empty()) {
    timefileBut->button(2)->setChecked(true);;
    updateTimefileList();
    int nf = files.size();
    for (int j = 0; j < nf; j++) {
      if (okVar.filename == files[j].name) {
        timefileList->setCurrentRow(j);
      }
    }
  } else {
    timefileBut->button(0)->setChecked(true); //auto
    updateTimefileList();
  }

  updateChannelBox(false);
  bool found = false;
  int nc = channelbox->count();
  for (int j = 0; j < nc; j++) {
    QString qstr = channelbox->item(j)->text();
    if (qstr.isNull())
      continue;
    std::string listchannel = qstr.toStdString();
    if (okVar.channel == listchannel) {
      unsigned int np = m_state.size();
      channelbox->setCurrentRow(j);
      channelboxSlot(channelbox->currentItem());
      // check if new picture added..
      found = (m_state.size() > np);
      found = true;
      break;
    }
  }

  if (!found)
    return;

  if (okVar.totalminutes >= 0) {
    int number = int(okVar.totalminutes / m_scalediff);
    diffSlider->setValue(number);
  }
  mosaic->setChecked(okVar.mosaic);

  //advanced dialog
  int index = pictures->currentRow();
  if (index < 0 || index >= int(m_state.size()))
    return;
  m_state[index].external = sda->putOKString(okVar.advanced);
  advancedChanged();
  m_state[index].advanced = sda->getOKString();
}

/*********************************************/
SatDialog::state SatDialog::decodeString(const miutil::KeyValue_v& tokens)
{
  /* This function is called by putOKstring.
   It decodes tokens, and puts plot variables into struct state */
  METLIBS_LOG_SCOPE();

  state okVar;
  okVar.totalminutes = -1;
  okVar.mosaic = false;

  for (const miutil::KeyValue& kv : tokens) {
    const std::string& key = kv.key();
    const std::string& value = kv.value();
    if (key == "time") {
      okVar.filetime = miutil::timeFromString(value);
    } else if (key == "timediff") {
      okVar.totalminutes = kv.toInt();
    } else if (key == "mosaic") {
      okVar.mosaic = kv.toBool();
      //ignore keys "font" and "face"
    } else if (key != "font" && key != "face") {
      //add to advanced string
      okVar.advanced.push_back(kv);
    }
  }

  return okVar;
}

/*********************************************/

std::string SatDialog::getShortname()
{
  std::string name;

  if (pictures->count()) {
    for (unsigned int i = 0; i < m_state.size(); i++)
      name += pictureString(m_state[i], false);
  }
  if (not name.empty())
    name = "<font color=\"#990000\">" + name + "</font>";
  return name;
}

/**********************************************/

void SatDialog::updateFileListWidget(int in)
{
  // METLIBS_LOG_DEBUG("updateFileListWidget:"<<in);
  if ( in < int(dialogInfo.image.size())) {

    fileListWidget->clear();
    //insert in fileListWidget the list of files.. Europa,N-Europa etc...
    int nfile = dialogInfo.image[in].file.size();
    for (int k = 0; k < nfile; k++) {
      fileListWidget->addItem(QString::fromStdString(dialogInfo.image[in].file[k].name));
    }
  }
}

/**********************************************/

void SatDialog::updateTimefileList(bool update)
{
  METLIBS_LOG_SCOPE();

  //if no file group selected, nothing to do
  if (fileListWidget->currentRow() == -1)
    return;

  //clear box with list of files
  timefileList->clear();

  //get new list of sat files
  { diutil::OverrideCursor waitCursor;
    files = sdd_->getSatFiles(namebox->currentText().toStdString(), fileListWidget->currentItem()->text().toStdString(), update);
  }

  if (autoButton->isChecked())
    return;

  channelbox->clear();

  int nr_file = files.size();
  if (nr_file == 0)
    return;

  //insert times into timefileList
  if (timeButton->isChecked()) {

    for (int i = 0; i < nr_file; i++) {
      timefileList->addItem(QString::fromStdString(files[i].time.isoTime()));
    }

  } else if (fileButton->isChecked()) {

    for (int i = 0; i < nr_file; i++) {
      timefileList->addItem(QString::fromStdString(files[i].name));
    }

  }

  if (timeButton->isChecked() || fileButton->isChecked()) {
    //set current item in timefileList to same as before, or first item
    int timeDefIndex = -1;
    for (int i = 0; i < nr_file; i++) {
      if (m_time == files[i].time) {
        timeDefIndex = i;
      }
    }

    //    m_timefileListIndex=timeDefIndex;
    if (timeDefIndex > -1)
      timefileList->setCurrentRow(timeDefIndex);
    else {
      m_time = miutil::miTime();
      timefileList->setCurrentRow(0);
    }
  }
}

/**********************************************/
void SatDialog::updateChannelBox(bool select)
{
  /* DESCRIPTION: This function updates the list of channels in
   ChannelBox.
   If auto is pressed the list is taken from SatDialogInfo (setupfile)
   If "time" or "file" is pressed the list is taken from file header
   Selected channel is set to the same as before, if possible
   */
  METLIBS_LOG_SCOPE();

  channelbox->clear();

  int index;
  if (autoButton->isChecked())
    index = -1;
  else
    index = timefileList->currentRow();

  std::vector<std::string> vstr = sdd_->getSatChannels(namebox->currentText().toStdString(), fileListWidget->currentItem()->text().toStdString(), index);
  const int nr_channel = vstr.size();
  if (nr_channel <= 0)
    return;

  for (auto& ch : vstr) {
    miutil::trim(ch);
    channelbox->addItem(QString::fromStdString(ch));
  }

  //insert string list in channelbox
  channelbox->setEnabled(true);

  if (!select)
    return;
  //HK ??? comment out this part which remembers selected channels

  miutil::trim(m_channelstr);

  // select same channel as last time, if possible ...
  for (int i = 0; i < nr_channel; i++) {
    if (m_channelstr == vstr[i]) {
      channelbox->setCurrentRow(i);
      channelboxSlot(channelbox->currentItem());
      return;
    }
  }

  // ... or first channel
  m_channelstr = vstr[0];
  channelbox->setCurrentRow(0);
  channelboxSlot(channelbox->currentItem());
}

/**********************************************/
void SatDialog::updatePictures(int index, bool updateAbove)
{
  METLIBS_LOG_SCOPE(LOGVAL(index));
  /* DESCRIPTION: This function updates the list of selected pictures
   send new list of times to timeslider
   */
  pictures->clear();

  for (unsigned int i = 0; i < m_state.size(); i++) {
    pictures->addItem(QString::fromStdString(pictureString(m_state[i], true)));
  }

  if (index > -1 && index < int(m_state.size())) {
    pictures->setCurrentRow(index);
    if (updateAbove)
      picturesSlot(pictures->currentItem());
    updateColours();
    std::string str = pictureString(m_state[index], false);
    sda->setPictures(str);
    sda->putOKString(m_state[index].advanced);
    sda->greyOptions();
    int number = int(m_state[index].totalminutes / m_scalediff);
    diffSlider->setValue(number);
    bool mon = m_state[index].mosaic;
    mosaic->setChecked(mon);
    mosaic->setEnabled(true);
  }

  //check if up and down buttons should be enabled
  if (m_state.size() > 1) {
    if (index == 0) {
      //downbutton
      downPictureButton->setEnabled(true);
      upPictureButton->setEnabled(false);
    } else if (index == int(m_state.size()) - 1) {
      //upbutton
      upPictureButton->setEnabled(true);
      downPictureButton->setEnabled(false);
    } else {
      downPictureButton->setEnabled(true);
      upPictureButton->setEnabled(true);
    }
  } else {
    downPictureButton->setEnabled(false);
    upPictureButton->setEnabled(false);
  }

  emitSatTimes();
}

/**********************************************/
void SatDialog::updateColours()
{
  METLIBS_LOG_SCOPE();

  int index = pictures->currentRow();
  if (index > -1) {
    state lstate = m_state[index];
    const std::vector<Colour>& colours = sdd_->getSatColours(lstate.name, lstate.area);
    sda->setColours(colours);
  }
}

/*********************************************/
void SatDialog::emitSatTimes(bool update)
{
  METLIBS_LOG_SCOPE();
  diutil::OverrideCursor waitCursor;
  times.clear();

  for (unsigned int i = 0; i < m_state.size(); i++) {
    if (m_state[i].filename.empty() || update) {
      //auto option for this state
      //get times to send to timeslider
      vector<SatFileInfo> f = sdd_->getSatFiles(m_state[i].name, m_state[i].area, update);
      for (unsigned int i = 0; i < f.size(); i++)
        times.insert(f[i].time);
    } else
      times.insert(m_state[i].filetime);
  }

  bool useTimes = (times.size() > m_state.size());
  emitTimes(times, useTimes);
}

vector<string> SatDialog::writeLog()
{
  vector<string> vstr;
  for (const satoptions_t::value_type& so : satoptions) {
    for (const areaoptions_t::value_type& ao : so.second)
      vstr.push_back(miutil::mergeKeyValue(ao.second));
  }
  return vstr;
}

void SatDialog::readLog(const vector<string>& vstr,
    const string& thisVersion, const string& logVersion)
{
  for (const std::string& s : vstr) {
    const miutil::KeyValue_v kvs = miutil::splitKeyValue(s);
    if (kvs.size() >= 4)
      satoptions[kvs[1].key()][kvs[2].key()] = kvs;
  }
}
