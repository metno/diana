
#include "diVcrossSelectionManager.h"

#include "diCommandParser.h"
#include "diUtilities.h"

#define MILOGGER_CATEGORY "diana.VcrossSelectionManager"
#include <miLogger/miLogging.h>

static const std::string EMPTY;

VcrossSelectionManager::VcrossSelectionManager(vcross::QtManager_p m)
  : vcrossm(m)
  , cp(new CommandParser())
  , setupFieldOptions(m->getAllFieldOptions())
{
  // add options to the cp's keyDataBase
  cp->addKey("model",      "",1,CommandParser::cmdString);
  cp->addKey("field",      "",1,CommandParser::cmdString);
  cp->addKey("hour.offset","",1,CommandParser::cmdInt);

  // add more plot options to the cp's keyDataBase
  cp->addKey("colour",         "",0,CommandParser::cmdString);
  cp->addKey("colours",        "",0,CommandParser::cmdString);
  cp->addKey("linewidth",      "",0,CommandParser::cmdInt);
  cp->addKey("linetype",       "",0,CommandParser::cmdString);
  cp->addKey("line.interval",  "",0,CommandParser::cmdFloat);
  cp->addKey("density",        "",0,CommandParser::cmdInt);
  cp->addKey("vector.unit",    "",0,CommandParser::cmdFloat);
  cp->addKey("extreme.type",   "",0,CommandParser::cmdString);
  cp->addKey("extreme.size",   "",0,CommandParser::cmdFloat);
  cp->addKey("extreme.limits", "",0,CommandParser::cmdString);
  cp->addKey("line.smooth",    "",0,CommandParser::cmdInt);
  cp->addKey("zero.line",      "",0,CommandParser::cmdInt);
  cp->addKey("value.label",    "",0,CommandParser::cmdInt);
  cp->addKey("label.size",     "",0,CommandParser::cmdFloat);
  cp->addKey("base",           "",0,CommandParser::cmdFloat);
  //cp->addKey("undef.masking",  "",0,CommandParser::cmdInt);
  //cp->addKey("undef.colour",   "",0,CommandParser::cmdString);
  //cp->addKey("undef.linewidth","",0,CommandParser::cmdInt);
  //cp->addKey("undef.linetype", "",0,CommandParser::cmdString);
  cp->addKey("palettecolours", "",0,CommandParser::cmdString);
  cp->addKey("minvalue",       "",0,CommandParser::cmdFloat);
  cp->addKey("maxvalue",       "",0,CommandParser::cmdFloat);
  cp->addKey("table",          "",0,CommandParser::cmdInt);
  cp->addKey("patterns",       "",0,CommandParser::cmdString);
  cp->addKey("patterncolour",  "",0,CommandParser::cmdString);
  cp->addKey("repeat",      "",0,CommandParser::cmdInt);
  cp->addKey("alpha",          "",0,CommandParser::cmdInt);
}

// ==================== interface towards GUI ====================

bool VcrossSelectionManager::addField(const std::string& model, const std::string& field, const std::string& fieldOpts, int position)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(field) << LOGVAL(fieldOpts));

  for (size_t j=0; j<selectedFields.size(); ++j) {
    SelectedField& sf = selectedFields[j];
    if (sf.model == model and sf.field == field)
      return false;
  }

  if (position < 0)
    position = 0;
  else if (position > int(selectedFields.size()))
    position = selectedFields.size();


  SelectedField sf(model, field, fieldOpts);
  fieldOptions[field] = fieldOpts;
  // sf.hourOffset = hourOffset;
  selectedFields.insert(selectedFields.begin() + position, sf);
  Q_EMIT fieldAdded(model, field, position);
  return true;
}

bool VcrossSelectionManager::updateField(const std::string& model, const std::string& field, const std::string& fieldOpts)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(field) << LOGVAL(fieldOpts));

  for (size_t j=0; j<selectedFields.size(); ++j) {
    SelectedField& sf = selectedFields[j];
    if (sf.model == model and sf.field == field) {
      if (sf.fieldOpts != fieldOpts) {
        sf.fieldOpts = fieldOpts;
        fieldOptions[field] = fieldOpts;
        Q_EMIT fieldUpdated(model, field, j);
      }
      return true;
    }
  }
  return false;
}

bool VcrossSelectionManager::removeField(const std::string& model, const std::string& field)
{
  SelectedField_v::iterator it = selectedFields.begin();
  for (size_t j=0; j<selectedFields.size(); ++j, ++it) {
    SelectedField& sf = selectedFields[j];
    if (sf.model == model and sf.field == field) {
      selectedFields.erase(it);
      Q_EMIT fieldRemoved(model, field, j);
      return true;
    }
  }
  return false;
}

void VcrossSelectionManager::removeAllFields()
{
  if (not selectedFields.empty()) {
    selectedFields.clear();
    Q_EMIT fieldsRemoved();
  }
}

int VcrossSelectionManager::countFields() const
{
  return selectedFields.size();
}

const std::string& VcrossSelectionManager::getFieldAt(int position) const
{
  if (position < 0 || position >= int(selectedFields.size()))
    return EMPTY;
  return selectedFields.at(position).field;
}

const std::string& VcrossSelectionManager::getModelAt(int position) const
{
  if (position < 0 || position >= int(selectedFields.size()))
    return EMPTY;
  return selectedFields.at(position).model;
}

const std::string& VcrossSelectionManager::getOptionsAt(int position) const
{
  if (position < 0 || position >= int(selectedFields.size()))
    return EMPTY;
  return selectedFields.at(position).fieldOpts;
}

void VcrossSelectionManager::setVisibleAt(int position, bool visible)
{
  METLIBS_LOG_SCOPE(LOGVAL(position) << LOGVAL(visible));
  if (position < 0 || position >= int(selectedFields.size()))
    return;
  selectedFields.at(position).visible = visible;
}

bool VcrossSelectionManager::getVisibleAt(int position) const
{
  if (position < 0 || position >= int(selectedFields.size()))
    return false;
  return selectedFields.at(position).visible;
}

QStringList VcrossSelectionManager::allModels()
{
  const std::vector<std::string> models = vcrossm->getAllModels();
  QStringList msl;
  for (size_t i=0; i<models.size(); ++i)
    msl << QString::fromStdString(models[i]);
  return msl;
}

QStringList VcrossSelectionManager::availableFields(const QString& model)
{
  const std::string mdl = model.toStdString();
  const std::vector<std::string> fields = vcrossm->getFieldNames(mdl);
  QStringList fsl;
  for (size_t i=0; i<fields.size(); ++i) {
    const std::string& fld = fields[i];

    // do not list fields that are already selected
    bool alreadySelected = false;
    for (size_t j=0; (not alreadySelected) and (j<selectedFields.size()); ++j) {
      const SelectedField& sf = selectedFields[j];
      if (sf.model == mdl and sf.field == fld)
        alreadySelected = true;
    }

    if (not alreadySelected)
      fsl << QString::fromStdString(fld);
  }
  return fsl;
}

std::string VcrossSelectionManager::defaultOptions(const QString& model, const QString& field, bool setupOptions)
{
  return defaultOptions(model.toStdString(), field.toStdString(), setupOptions);
}

std::string VcrossSelectionManager::defaultOptions(const std::string& model, const std::string& field, bool setupOptions)
{
  if (!setupOptions) {
    const string_string_m::const_iterator itU = fieldOptions.find(field);
    if (itU != fieldOptions.end())
      return itU->second;
  }
  const string_string_m::const_iterator itS = setupFieldOptions.find(field);
  if (itS != setupFieldOptions.end())
    return itS->second;
  return "";
}

// ==================== interface towards quickmenu / .... ====================

std::string VcrossSelectionManager::getShortname()
{
  std::ostringstream shortname;
  std::string previousModel;

  for (size_t i = 0; i < selectedFields.size(); i++) {
    const SelectedField& sf = selectedFields[i];
    if (!sf.visible)
      continue;
    const std::string& mdl = sf.model;
    if (mdl != previousModel) {
      if (i > 0)
        shortname << ' ';
      shortname << mdl;
      previousModel = mdl;
    }

    shortname << ' ' << selectedFields[i].field;
  }

  return shortname.str();
}

std::vector<std::string> VcrossSelectionManager::getOKString()
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> vstr;
  if (selectedFields.size()==0)
    return vstr;
  
  std::vector<std::string> hstr;

  for (size_t i=0; i<selectedFields.size(); i++) {
    const SelectedField& sf = selectedFields.at(i);
    METLIBS_LOG_DEBUG(LOGVAL(sf.model) << LOGVAL(sf.field) << LOGVAL(sf.visible));
    if (not sf.visible)
      continue;

    std::ostringstream ostr;
    ostr << "VCROSS "
         << "model=" << sf.model
         << " field=" <<  sf.field;
    if (sf.hourOffset != 0)
      ostr << " hour.offset=" << sf.hourOffset;
    ostr << " " << sf.fieldOpts;

    vstr.push_back(ostr.str());
  }
  
  return vstr;
}

void VcrossSelectionManager::putOKString(const std::vector<std::string>& vstr,
    bool vcrossPrefix, bool checkOptions)
{
  METLIBS_LOG_SCOPE();

  removeAllFields();

  if (vstr.empty())
    return;

  std::string cached_model;
  std::vector<std::string> cached_fields;

  for (size_t i=0; i<vstr.size(); ++i) {
    const std::string& okstring = vstr[i];

    std::vector<ParsedCommand> vpc;
    if (checkOptions) {
      std::string str = checkFieldOptions(okstring, vcrossPrefix);
      if (str.empty())
        continue;
      vpc = cp->parse(str);
    } else {
      vpc = cp->parse(okstring);
    }

    std::string model, field, fOpts;
    int hourOffset = 0;
    for (size_t j=0; j<vpc.size(); j++) {
      const ParsedCommand& pc = vpc[j];

      if (pc.key=="model")
        model = pc.strValue[0];
      else if (pc.key=="field")
        field = pc.strValue[0];
      else if (pc.key=="hour.offset")
        hourOffset = pc.intValue[0];
      else if (pc.key!="unknown") {
        if (fOpts.length()>0)
          fOpts+=" ";
        fOpts += (pc.key + "=" + pc.allValue);
      }
    }
    if (model.empty() or field.empty())
      continue;

    if (model != cached_model) {
      cached_fields = vcrossm->getFieldNames(model);
      cached_model = model;
    }

    const std::vector<std::string>::const_iterator it = std::find(cached_fields.begin(), cached_fields.end(), field);
    if (it != cached_fields.end()) {
      SelectedField sf(model, field, fOpts);
      sf.hourOffset = hourOffset;
      selectedFields.push_back(sf);
      Q_EMIT fieldAdded(model, field, selectedFields.size()-1);
    }
  }
}

std::vector<std::string> VcrossSelectionManager::writeLog()
{
  string_v loglines;

  // this line is for the history -- we have no history, but we have to keep this line
  loglines.push_back("================");

  // write used field options

  for (string_string_m::const_iterator itO = fieldOptions.begin(); itO != fieldOptions.end(); ++itO)
    loglines.push_back(itO->first + " " + itO->second);
  loglines.push_back("================");

  return loglines;
}

void VcrossSelectionManager::readLog(const string_v& loglines,
    const std::string& thisVersion, const std::string& logVersion)
{
  fieldOptions.clear();
  if (not selectedFields.empty()) {
    selectedFields.clear();
    Q_EMIT fieldsRemoved();
  }

  string_v::const_iterator itL = loglines.begin();
  while (itL != loglines.end() and not diutil::startswith(*itL, "===="))
    ++itL;

  if (itL != loglines.end() and diutil::startswith(*itL, "===="))
    ++itL;

  // field options
  // (do not destroy any new options in the program,
  //  and get rid of old unused options)
  for (; itL != loglines.end(); ++itL) {
    if (diutil::startswith(*itL, "===="))
      break;
    const int firstspace = itL->find_first_of(' ');
    if (firstspace <= 0 or firstspace >= int(itL->size())-1)
      continue;

    const std::string fieldname = itL->substr(0, firstspace);
    const std::string options = itL->substr(firstspace+1);

    string_string_m::iterator itO = fieldOptions.find(fieldname);
    if (itO != fieldOptions.end()) {
      std::vector<ParsedCommand> vpopt = cp->parse(itO->second);
      std::vector<ParsedCommand> vplog = cp->parse(options);
      const int nopt= vpopt.size(), nlog = vplog.size();
      bool changed= false;
      for (int i=0; i<nopt; i++) {
        int j=0;
        while (j<nlog && vplog[j].key!=vpopt[i].key)
          j++;
        if (j<nlog) {
          // there is no option with variable no. of values, YET...
          if (vplog[j].allValue!=vpopt[i].allValue &&
              vplog[j].strValue.size()==vpopt[i].strValue.size())
          {
            cp->replaceValue(vpopt[i],vplog[j].allValue,-1);
            changed= true;
          }
        }
      }
      for (int i=0; i<nlog; i++) {
        int j=0;
        while (j<nopt && vpopt[j].key!=vplog[i].key)
          j++;
        if (j==nopt) {
          cp->replaceValue(vpopt,vplog[i].key,vplog[i].allValue);
        }
      }
      if (changed)
        itO->second = cp->unParse(vpopt);
    } else {
      fieldOptions[fieldname] = options;
    }
  }
}

std::string VcrossSelectionManager::checkFieldOptions(const std::string& str, bool vcrossPrefix)
{
  METLIBS_LOG_SCOPE();
  std::string newstr;

  const std::vector<ParsedCommand> vplog = cp->parse(str);
  const int nlog = vplog.size();
  const int first = (vcrossPrefix) ? 1 : 0;

  if (nlog>=2+first && vplog[first].key=="model" && vplog[first+1].key=="field")
  {
    std::string fieldname = vplog[first+1].allValue;
    string_string_m::const_iterator pfopt = setupFieldOptions.find(fieldname);
    if (pfopt != setupFieldOptions.end()) {
      std::vector<ParsedCommand> vpopt = cp->parse(pfopt->second);
      const int nopt = vpopt.size();
      if (vcrossPrefix)
        newstr = "VCROSS ";
      for (int i=first; i<nlog; i++) {
        if (vplog[i].idNumber==1)
          newstr += (" " + vplog[i].key + "=" + vplog[i].allValue);
      }

      // loop through current options, replace the value if the new
      // string has same option with different value
      for (int j=0; j<nopt; j++) {
        int i=0;
        while (i<nlog && vplog[i].key!=vpopt[j].key)
          i++;
        if (i<nlog) {
          // there is no option with variable no. of values, YET...
          if (vplog[i].allValue != vpopt[j].allValue
              && vplog[i].strValue.size() == vpopt[j].strValue.size())
            cp->replaceValue(vpopt[j], vplog[i].allValue, -1);
        }
      }

      //loop through new options, add new option if it is not a part of current options
      for (int i = 2; i < nlog; i++) {
        int j = 0;
        while (j < nopt && vpopt[j].key != vplog[i].key)
          j++;
        if (j == nopt) {
          cp->replaceValue(vpopt, vplog[i].key, vplog[i].allValue);
        }
      }

      newstr += " ";
      newstr += cp->unParse(vpopt);
      if (vcrossPrefix) {
        // from quickmenu, keep "forecast.hour=..." and "forecast.hour.loop=..."
        for (int i=2+first; i<nlog; i++) {
          if (vplog[i].idNumber==4 || vplog[i].idNumber==-1)
            newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
        }
      }
      for (int i=2+first; i<nlog; i++) {
        if (vplog[i].idNumber==3)
          newstr+= (" " + vplog[i].key + "=" + vplog[i].allValue);
      }
    }
  }

  return newstr;
}
