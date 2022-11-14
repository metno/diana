/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

#include "qtSatDialog.h"

#include "diSatDialogData.h"
#include "diSliderValues.h"
#include "qtSatDialogAdvanced.h"
#include "qtToggleButton.h"
#include "qtUtility.h"
#include "util/misc_util.h"
#include "util/string_util.h"
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
#include <QSlider>
#include <QVBoxLayout>

#include <iomanip>
#include <sstream>

#include "down12x12.xpm"
#include "sat.xpm"
#include "up12x12.xpm"

#define MILOGGER_CATEGORY "diana.SatDialog"
#include <miLogger/miLogging.h>


#define HEIGHTLISTBOX 45

namespace {

bool selectTextInList(QListWidget* lw, const std::string& text)
{
  const QList<QListWidgetItem*> channelitems = lw->findItems(QString::fromStdString(text), Qt::MatchExactly);
  if (channelitems.size() != 1)
    return false;
  lw->setCurrentItem(channelitems.front());
  return true;
}

const SliderValues sv_cut = {0, 5, 2, 0.01};      ///< rgb cutoff value for histogram stretching
const SliderValues sv_alphacut = {0, 10, 0, 0.1}; ///< rgb cutoff value for alpha blending
const SliderValues sv_alpha = {0, 10, 10, 0.1};   ///< alpha blending value
const SliderValues sv_timediff = {0, 96, 4, 15};  ///< max time difference

} // namespace

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

  availableImages = sdd->initSatDialog();

  //put satellite names (NOAA,Meteosat,radar) in namebox
  imageNameBox = new QComboBox(this);
  for (const auto& im : availableImages)
    imageNameBox->addItem(QString::fromStdString(im.image_name));
  imageNameBox->setCurrentIndex(0);
  connect(imageNameBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &SatDialog::imageNameBoxActivated);

  //fileListWidget contains filetypes for each satellite
  //insert filetypes for default sat - m_image[0]
  subtypeNameList = new QListWidget(this);

  updateSubTypeList(0);
  connect(subtypeNameList, &QListWidget::itemClicked, this, &SatDialog::subtypeNameListClicked);

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
  //timefileClicked is called when auto/time/file buttons are clicked
  connect(timefileBut, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SatDialog::timefileClicked);

  //list of times/files will be filled when time/file are clicked

  timefileList = new QListWidget( this);

  connect(timefileList, &QListWidget::itemClicked, this, &SatDialog::timefileListSlot);

  //channelbox filled with available channels
  QLabel *channellabel= TitleLabel( tr("Channels"), this);
  channelbox = new QListWidget( this);
  connect(channelbox, &QListWidget::itemClicked, this, &SatDialog::channelboxSlot);

  //pictures contains one or more selected pictures !
  QLabel* picturesLabel = TitleLabel(tr("Selected pictures"), this);
  pictures = new QListWidget( this );
  connect(pictures, &QListWidget::itemClicked, this, &SatDialog::picturesSlot);

  //**  all the other QT buttons

  upPictureButton = PixmapButton(QPixmap(up12x12_xpm), this, 14, 12);
  upPictureButton->setEnabled( false );
  connect(upPictureButton, &QPushButton::clicked, this, &SatDialog::upPicture);

  downPictureButton = PixmapButton(QPixmap(down12x12_xpm), this, 14, 12);
  downPictureButton->setEnabled( false );
  connect(downPictureButton, &QPushButton::clicked, this, &SatDialog::downPicture);

  Delete = new QPushButton(tr("Delete"), this);
  connect(Delete, &QPushButton::clicked, this, &SatDialog::DeleteClicked);

  DeleteAll = new QPushButton(tr("Delete All"), this);
  connect(DeleteAll, &QPushButton::clicked, this, &SatDialog::DeleteAllClicked);

  multiPicture = new ToggleButton(this, tr("Add picture"));
  multiPicture->setToolTip(tr("Add new picture if any of above settings change"));

  mosaic = new ToggleButton(this, tr("Mosaic"));
  connect(mosaic, &ToggleButton::toggled, this, &SatDialog::mosaicToggled);
  mosaic->setChecked(false);
  mosaic->setEnabled(false);

  //SLIDER FOR MAX TIME DIFFERENCE
  QLabel *diffLabel = new QLabel( tr("Time diff"), this);
  int difflength = sv_timediff.maxValue / 40 + 3;
  diffLcdnum= LCDNumber( difflength, this);
  diffSlider= new QSlider(Qt::Horizontal, this );
  diffSlider->setMinimum(sv_timediff.minValue);
  diffSlider->setMaximum(sv_timediff.maxValue);
  diffSlider->setValue(sv_timediff.value);
  QHBoxLayout* difflayout = new QHBoxLayout();
  difflayout->addWidget( diffLabel,0,0 );
  difflayout->addWidget( diffLcdnum, 0,0 );
  difflayout->addWidget( diffSlider,0,0  );
  connect(diffSlider, &QSlider::valueChanged, this, &SatDialog::doubleDisplayDiff);

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
  vlayout->addWidget(imageNameBox);
  vlayout->addWidget(subtypeNameList);
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

  // INNITIALISATION AND DEFAULT
  doubleDisplayDiff(sv_timediff.value);

  this->hide();
  setOrientation(Qt::Horizontal);
  sda = new SatDialogAdvanced(this, sv_cut, sv_alphacut, sv_alpha);
  setExtension(sda);
  showMore(false);
  connect(sda, &SatDialogAdvanced::getSatColours, this, &SatDialog::updateColours);
  connect(sda, &SatDialogAdvanced::SatChanged, this, &SatDialog::advancedChanged);
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
void SatDialog::imageNameBoxActivated(int in)
{
  METLIBS_LOG_SCOPE();

  updateSubTypeList(in);

  //clear timefileList and channelbox
  timefileList->clear();
  channelbox->clear();
}

/*********************************************/
void SatDialog::subtypeNameListClicked(QListWidgetItem* item)
{
  METLIBS_LOG_SCOPE();

  diutil::OverrideCursor waitCursor;

  //restore options if possible
  const std::string image_name = imageNameBox->currentText().toStdString();
  const std::string subtype_name = item->text().toStdString();
  const imageoptions_t::const_iterator itn = satoptions.find(image_name);
  if (itn != satoptions.end()) {
    const subtypeoptions_t::const_iterator ita = itn->second.find(subtype_name);
    if (ita != itn->second.end()) {
      putOptions(ita->second);
    }
  }

  updateTimefileList(true);
  updateChannelBox(true);
  emitSatTimes(false);
}

/*********************************************/
void SatDialog::timefileClicked(int tt)
{
  /* DESCRIPTION: This function is called when timefileBut (auto/time/file)is
   selected
   */
  METLIBS_LOG_SCOPE(LOGVAL(tt));

  if (subtypeNameList->currentRow() == -1)
    return;

  updateTimefileList(false);
  updateChannelBox(true);
  emitSatTimes(false);
}

/*********************************************/
void SatDialog::timefileListSlot(QListWidgetItem *)
{
  /* DESCRIPTION: This function is called when the signal highlighted() is
   sent from the list of time/file
   */
  METLIBS_LOG_SCOPE();

  int current_timefile = timefileList->currentRow();
  int current_picture = pictures->currentRow();
  if (current_picture > -1 && int(m_state.size()) > current_picture) {
    m_state[current_picture]->filename = files[current_timefile].name;
    m_state[current_picture]->filetime = files[current_timefile].time;
    m_time = files[current_timefile].time;
  }
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

  m_channelstr = item->text().toStdString();

  const int newIndex = addSelectedPicture();
  if (newIndex >= 0)
    updatePictures(newIndex, false);
}

SatImageAndSubType SatDialog::currentSatImageAndSubType() const
{
  return SatImageAndSubType{imageNameBox->currentText().toStdString(), subtypeNameList->currentItem()->text().toStdString()};
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

  SatPlotCommand_p cmd = std::make_shared<SatPlotCommand>();
  cmd->sist = currentSatImageAndSubType();
  cmd->plotChannels = channelbox->currentItem()->text().toStdString();
  if (timeButton->isChecked() || fileButton->isChecked()) {
    //"time"/"file" clicked, find filename
    const int current = timefileList->currentRow();
    if (current > -1 && current < int(files.size())) {
      cmd->filename = files[current].name;
      cmd->filetime = files[current].time;
    }
  }
  cmd->mosaic = false;
  cmd->timediff = 60;

  int newIndex = -1;
  if (!multiPicture->isChecked()) {
    const int i = pictures->currentRow();
    if (i > -1 && i < int(m_state.size())) {
      //replace existing picture(same sat). advanced options saved
      // get info about picture we are replacing
      SatPlotCommand_cp msi = m_state[i];
      cmd->mosaic = msi->mosaic;
      cmd->timediff = msi->timediff;
      const auto f = sdd_->getSatFiles(msi->sist, false);
      if (!f.empty() && (f[0].palette != files[0].palette)) {
        //special case changing from palette to rgb or vice versa
      } else {
        // copy values set in SatDialogAdvanced
        cmd->cut = msi->cut;
        cmd->alphacut = msi->alphacut;
        cmd->alpha = msi->alpha;
        cmd->classtable = msi->classtable;
        cmd->coloursToHideInLegend = msi->coloursToHideInLegend;
        if (files[0].palette) {
          std::vector<Colour> c;
          sda->setColours(c);
        }
      }
      m_state[i] = cmd;
      newIndex = i;
    }
  }
  if (newIndex == -1) {
    newIndex = m_state.size();
    m_state.push_back(cmd);
  }

  sda->setFromCommand(cmd);
  return newIndex;
}

std::string SatDialog::pictureString(SatPlotCommand_cp cmd, bool timefile)
{
  std::string str = cmd->image_name();
  if (cmd->mosaic)
    str += " MOSAIC ";
  str += " " + cmd->subtype_name() + " " + cmd->plotChannels + " ";
  if (timefile && cmd->hasFileTime())
    str += cmd->filetime.isoTime();
  return str;
}

void SatDialog::picturesSlot(QListWidgetItem*)
{
  METLIBS_LOG_SCOPE(LOGVAL(m_state.size()));

  const int index = pictures->currentRow();
  if (index > -1) {
    SatPlotCommand_p cmd = m_state[index];
    imageNameBox->setCurrentText(QString::fromStdString(cmd->image_name()));
    imageNameBoxActivated(imageNameBox->currentIndex()); // this updates fileListWidget

    selectTextInList(subtypeNameList, cmd->subtype_name());

    if (cmd->isAuto()) {
      autoButton->setChecked(true);
      updateTimefileList(false);
    } else {
      if (cmd->hasFileTime())
        timeButton->setChecked(true);
      else // if (cmd->hasFileName())
        fileButton->setChecked(true);
      updateTimefileList(false);
      if (cmd->hasFileTime())
        selectTextInList(timefileList, cmd->filetime.isoTime());
      else // if (cmd->hasFileName())
        selectTextInList(timefileList, cmd->filename);
    }

    updateChannelBox(false);
    selectTextInList(channelbox, cmd->plotChannels);
    m_channelstr = cmd->plotChannels;

    updateColours();

    sda->setPictures(pictureString(cmd, false));
    sda->setFromCommand(cmd);
    sda->greyOptions();
    int number = int(cmd->timediff / sv_timediff.scale);
    diffSlider->setValue(number);
    mosaic->setChecked(cmd->mosaic);
    mosaic->setEnabled(true);
  } else {
    sda->setPictures("");
    sda->setColours(std::vector<Colour>());
  }
  enableUpDownButtons();
}

void SatDialog::enableUpDownButtons()
{
  const int index = pictures->currentRow();
  //check if up and down buttons should be enabled
  upPictureButton->setEnabled(index > 0);
  downPictureButton->setEnabled(index >= 0 && index < (int)m_state.size() - 1);
}

/*********************************************/

void SatDialog::updateTimes()
{
  METLIBS_LOG_SCOPE();
  diutil::OverrideCursor waitCursor;

  // update times (files) of all selected pictures
  for (SatPlotCommand_cp cmd : m_state)
    sdd_->getSatFiles(cmd->sist, true);

  // update the timefilelist if "time" og "file" is selected
  if (!autoButton->isChecked())
    updateTimefileList(false);

  emitSatTimes(false);
}

/*********************************************/

void SatDialog::mosaicToggled(bool on)
{
  int index = pictures->currentRow();
  if (index > -1 && int(m_state.size()) > index)
    m_state[index]->mosaic = on;
  updatePictures(index, false);
}

/*********************************************/

void SatDialog::advancedChanged()
{
  int index = pictures->currentRow();
  if (index > -1)
    sda->applyToCommand(m_state[index]);
}

/**********************************************/

void SatDialog::upPicture()
{
  const int index = pictures->currentRow();
  if (index > 0 && index < (int)m_state.size()) {
    std::swap(m_state[index], m_state[index - 1]);
    updatePictures(index - 1, true);
  }
}

void SatDialog::downPicture()
{
  const int index = pictures->currentRow();
  if (index >= 0 && index < (int)m_state.size() - 1) {
    std::swap(m_state[index], m_state[index + 1]);
    updatePictures(index + 1, true);
  }
}

void SatDialog::doubleDisplayDiff(int number)
{
  /* This function is called when diffSlider sends a signald valueChanged(int)
   and changes the numerical value in the lcd display diffLcdnum */
  int totalminutes = int(number * sv_timediff.scale);

  int index = pictures->currentRow();
  if (index > -1 && int(m_state.size()) > index)
    m_state[index]->timediff = totalminutes;

  int hours = totalminutes / 60;
  int minutes = totalminutes - hours * 60;
  std::ostringstream ostr;
  ostr << hours << ":" << std::setw(2) << std::setfill('0') << minutes;
  diffLcdnum->display(QString::fromStdString(ostr.str()));
}

void SatDialog::DeleteAllClicked()
{
  METLIBS_LOG_SCOPE();

  subtypeNameList->clearSelection();
  timefileList->clear();
  channelbox->clear();

  m_state.clear();
  pictures->clear();
  downPictureButton->setEnabled(false);
  upPictureButton->setEnabled(false);
  mosaic->setChecked(false);
  mosaic->setEnabled(false);
  sda->setPictures("");
  sda->setOff();
  sda->greyOptions();

  //Emit empty time list
  emitTimes(plottimes_t(), false);
}

/*********************************************/
void SatDialog::DeleteClicked()
{
  METLIBS_LOG_SCOPE();

  if (m_state.size() == 1) {
    // check needed when empty list was found
    DeleteAllClicked();
  } else {
    const int row = pictures->currentRow();
    if (row > -1 && row < (int)m_state.size()) {
      //remove from m_state and picture listbox
      m_state.erase(m_state.begin() + row);
      delete pictures->takeItem(row);
      picturesSlot(pictures->currentItem());
      emitSatTimes(false);
    }
  }
}

PlotCommand_cpv SatDialog::getOKString()
{
  METLIBS_LOG_SCOPE();

  PlotCommand_cpv cmds;
  cmds.reserve(m_state.size());
  for (SatPlotCommand_cp cmd : m_state) {
    satoptions[cmd->image_name()][cmd->subtype_name()] = cmd;
#if 0
    // needs a copy of cmd
    cmd->add(miutil::KeyValue(PlotOptions::key_fontname, diutil::BITMAPFONT));
    cmd->add(miutil::KeyValue(PlotOptions::key_fontface, "normal"));
#endif
    cmds.push_back(cmd);
  }
  return cmds;
}

void SatDialog::putOKString(const PlotCommand_cpv& vstr)
{
  METLIBS_LOG_SCOPE();

  DeleteAllClicked();

  for (PlotCommand_cp pc : vstr) {
    SatPlotCommand_cp cmd = std::dynamic_pointer_cast<const SatPlotCommand>(pc);
    if (!cmd || cmd->image_name().empty() || cmd->subtype_name().empty() || cmd->plotChannels.empty())
      continue;

    const int name_index = imageNameBox->findText(QString::fromStdString(cmd->image_name()), Qt::MatchExactly);
    if (name_index != -1) {
      imageNameBox->setCurrentIndex(name_index);
      imageNameBoxActivated(name_index);
    } else {
      continue;
    }

    const QList<QListWidgetItem*> subtype_match = subtypeNameList->findItems(QString::fromStdString(cmd->subtype_name()), Qt::MatchExactly);
    if (subtype_match.size() == 1) {
      subtypeNameList->setCurrentItem(subtype_match.front());
    } else {
      continue;
    }

    SatPlotCommand_p ccmd = std::make_shared<SatPlotCommand>(*cmd); // make an editable copy
    if (ccmd->hasFileName() && !ccmd->hasFileTime()) {
      // update list of files/times
      const auto cmdfiles = sdd_->getSatFiles(ccmd->sist, true);
      // find time for filename
      for (const auto& sfi : cmdfiles) {
        if (sfi.name == ccmd->filename) {
          ccmd->filetime = sfi.time;
          break;
        }
      }
    }
    m_state.push_back(ccmd); // add modified copy
  }

  emitSatTimes(true);

  updatePictures(0, true);
}

void SatDialog::putOptions(SatPlotCommand_cp cmd)
{
  METLIBS_LOG_SCOPE();

  if (!selectTextInList(channelbox, cmd->plotChannels))
    return;

  m_channelstr = cmd->plotChannels;
  channelboxSlot(channelbox->currentItem());

  if (cmd->timediff >= 0) {
    int number = int(cmd->timediff / sv_timediff.scale);
    diffSlider->setValue(number);
  }
  mosaic->setChecked(cmd->mosaic);

  sda->setFromCommand(cmd);
  advancedChanged();
}


std::string SatDialog::getShortname()
{
  std::string name;
  for (SatPlotCommand_cp cmd : m_state)
    name += pictureString(cmd, false);
  if (!name.empty())
    name = "<font color=\"#990000\">" + name + "</font>";
  return name;
}

void SatDialog::archiveMode()
{
  emitSatTimes(true);
  updateTimefileList(true);
}

void SatDialog::updateSubTypeList(int in)
{
  if (in >= 0 && in < int(availableImages.size())) {
    subtypeNameList->clear();
    for (const auto& im_subtype : availableImages[in].subtype_names)
      subtypeNameList->addItem(QString::fromStdString(im_subtype));
  }
}

void SatDialog::updateTimefileList(bool update)
{
  METLIBS_LOG_SCOPE();

  //if no file group selected, nothing to do
  if (subtypeNameList->currentRow() == -1)
    return;

  //clear box with list of files
  timefileList->clear();

  //get new list of sat files
  { diutil::OverrideCursor waitCursor;
    files = sdd_->getSatFiles(currentSatImageAndSubType(), update);
  }

  if (autoButton->isChecked())
    return;

  if (files.empty())
    return;

  if (timeButton->isChecked()) {
    // insert times into timefileList
    for (const auto& f : files)
      timefileList->addItem(QString::fromStdString(f.time.isoTime()));

  } else if (fileButton->isChecked()) {
    for (const auto& f : files)
      timefileList->addItem(QString::fromStdString(f.name));
  }

  if (timeButton->isChecked() || fileButton->isChecked()) {
    //set current item in timefileList to same as before, or first item
    for (size_t i = 0; i < files.size(); i++) {
      if (m_time == files[i].time) {
        timefileList->setCurrentRow(i);
        return;
      }
    }

    timefileList->setCurrentRow(0);
    m_time = files[0].time;
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

  const std::vector<std::string> channels = sdd_->getSatChannels(currentSatImageAndSubType(), index);
  if (channels.empty())
    return;

  int row_m_channelstr = -1;
  for (auto& ch : channels) {
    channelbox->addItem(QString::fromStdString(ch));
    if (select && m_channelstr == ch)
      row_m_channelstr = channelbox->count() - 1;
  }

  channelbox->setEnabled(true);

  if (select) {
    if (row_m_channelstr < 0)
      row_m_channelstr = 0; // ok, we checked that channels is not empty
    channelbox->setCurrentRow(row_m_channelstr);
    channelboxSlot(channelbox->currentItem());
  }
}

void SatDialog::updatePictures(int index, bool updateAbove)
{
  METLIBS_LOG_SCOPE(LOGVAL(index));
  /* DESCRIPTION: This function updates the list of selected pictures
   send new list of times to timeslider
   */
  pictures->clear();
  for (SatPlotCommand_cp cmd : m_state)
    pictures->addItem(QString::fromStdString(pictureString(cmd, true)));

  if (index > -1 && index < int(m_state.size())) {
    SatPlotCommand_cp cmd = m_state[index];
    pictures->setCurrentRow(index);
    if (updateAbove)
      picturesSlot(pictures->currentItem());
    updateColours();
    sda->setPictures(pictureString(cmd, false));
    sda->setFromCommand(cmd);
    sda->greyOptions();
    diffSlider->setValue(int(cmd->timediff / sv_timediff.scale));
    mosaic->setChecked(cmd->mosaic);
    mosaic->setEnabled(true);
  }

  enableUpDownButtons();
}

void SatDialog::updateColours()
{
  METLIBS_LOG_SCOPE();

  int index = pictures->currentRow();
  if (index > -1) {
    SatPlotCommand_cp cmd = m_state[index];
    const std::vector<Colour>& colours = sdd_->getSatColours(cmd->sist);
    sda->setColours(colours);
  }
}

/*********************************************/
void SatDialog::emitSatTimes(bool update)
{
  METLIBS_LOG_SCOPE();
  diutil::OverrideCursor waitCursor;
  plottimes_t times;

  bool useTimes = false;
  for (SatPlotCommand_cp cmd : m_state) {
    useTimes |= cmd->isAuto();
    if (!cmd->hasFileName() || update) {
      //get times to send to timeslider
      for (const auto& f : sdd_->getSatFiles(cmd->sist, update))
        times.insert(f.time);
    }
  }

  emitTimes(times, useTimes);
}

std::vector<std::string> SatDialog::writeLog()
{
  METLIBS_LOG_SCOPE();
  std::vector<std::string> vstr;
  for (const imageoptions_t::value_type& so : satoptions) {
    for (const subtypeoptions_t::value_type& ao : so.second) {
      const std::string s = ao.second->toString().substr(4); // FIXME skips "SAT ", better not include it
      vstr.push_back(s);
    }
  }
  return vstr;
}

void SatDialog::readLog(const std::vector<std::string>& vstr, const std::string& /*thisVersion*/, const std::string& /*logVersion*/)
{
  readSatOptionsLog(vstr, satoptions);
}

// static
void SatDialog::readSatOptionsLog(const std::vector<std::string>& vstr, imageoptions_t& satoptions)
{
  METLIBS_LOG_SCOPE();
  for (std::string s : vstr) { // make a copy
    if (diutil::startswith(s, "SAT "))
      s = s.substr(4);
    SatPlotCommand_cp cmd = SatPlotCommand::fromString(s);
    if (cmd)
      satoptions[cmd->image_name()][cmd->subtype_name()] = cmd;
  }
}
