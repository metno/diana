/*
 * GridInventoryTypes.cc
 *
 *  Created on: Mar 11, 2010
 *      Author: audunc
 */

#include "GridInventoryTypes.h"
#include "diFieldFunctions.h"

#include "util/misc_util.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <climits>

#define MILOGGER_CATEGORY "diField.GridInventoryTypes"
#include "miLogger/miLogging.h"

using namespace gridinventory;

//#define DEBUGPRINT


std::vector<std::string> InventoryBase::getStringValues() const
{

  return stringvalues;
}

bool InventoryBase::valueExists(const std::string& value) const
{
  return (std::find(stringvalues.begin(), stringvalues.end(), value) != stringvalues.end());
}

void Zaxis::setStringValues()
{
  stringvalues.clear();

  //use zaxis-info from setup
  vc_type = FieldVerticalAxes::vctype_other;
  std::string levelprefix;
  std::string levelsuffix;
  std::string name = verticalType;
  bool index = false;
  if (const FieldVerticalAxes::Zaxis_info* zi = FieldVerticalAxes::findZaxisInfo(name)) {
    vc_type = zi->vctype;
    levelprefix = zi->levelprefix;
    levelsuffix = zi->levelsuffix;
    index = zi->index;
  }

  if (positive) {
    for (int i = values.size() - 1 ; i>-1; --i) {
      std::ostringstream ost;
      if ( index )
        ost << levelprefix<<i+1<<levelsuffix;
      else
        ost << levelprefix<<values[i]<<levelsuffix;
      stringvalues.push_back(ost.str());
    }
  } else {
    for (size_t i = 0; i < values.size(); ++i) {
      std::ostringstream ost;
      if ( index )
        ost << levelprefix<<i+1<<levelsuffix;
      else
        ost << levelprefix<<values[i]<<levelsuffix;
      stringvalues.push_back(ost.str());
    }
  }
}

void ExtraAxis::setStringValues()
{

  stringvalues.clear();
  for (size_t i = 0; i < values.size(); ++i) {
    std::ostringstream ost;
    ost <<values[i];
    stringvalues.push_back(ost.str());
  }
}

static Taxis emptytaxis;

const Taxis& ReftimeInventory::getTaxis(const std::string& name) const
{
  std::set<Taxis>::const_iterator itr = taxes.begin();
  for (; itr != taxes.end(); ++itr) {
    if (itr->name == name ) {
      return *itr;
    }
  }

  // TODO: do we need to return reference to time axis?
  return emptytaxis;
}

static Zaxis emptyzaxis;

const Zaxis& ReftimeInventory::getZaxis(const std::string& name) const
{
  std::set<Zaxis>::const_iterator itr = zaxes.begin();
  for (; itr != zaxes.end(); ++itr) {
    if (itr->name == name ) {
      return *itr;
    }
  }

  return emptyzaxis;
}

static ExtraAxis emptyextraaxis;

const ExtraAxis& ReftimeInventory::getExtraAxis(const std::string& name) const
{
  std::set<ExtraAxis>::const_iterator itr = extraaxes.begin();
  for (; itr != extraaxes.end(); ++itr) {
    if (itr->name == name ) {
      return *itr;
    }
  }

  return emptyextraaxis;
}

Inventory Inventory::merge(const Inventory& i) const
{
  METLIBS_LOG_SCOPE(LOGVAL(reftimes.size()) << LOGVAL(i.reftimes.size()));
  Inventory result;

    // grid and axes: if grid or axis with same id but different values exist -> skip all parameters from this refTimeInventory

    // Exception
    // zaxis, taxis, extraaxis: if axis with same id but different values exist AND the new axis has just one value -> merge new value into the existing axis


  for (Inventory::reftimes_t::const_iterator it_r1 = reftimes.begin(); it_r1 != reftimes.end(); ++it_r1) {
    const ReftimeInventory& r1 = it_r1->second;
    METLIBS_LOG_DEBUG(" checking reftime:" << r1.referencetime);

    ReftimeInventory reftime = r1; // first make a copy of this reference time
    std::map<std::string, ReftimeInventory>::const_iterator r2itr = i.reftimes.find(r1.referencetime);
    if (r2itr != i.reftimes.end()) { // this reference time also exist in i for this model
      METLIBS_LOG_DEBUG("    this reftime is also in 2");
      const ReftimeInventory& r2 = r2itr->second;

      // Check attributes

      bool axisOk = true; //false if grid or axes not ok

      // Do not merge grids, and do not use grid as part of the parameter id
      // Currently the changes are only commented out.  Corresponding changes have been made in FieldManager


      // - zaxis:
      for (const Zaxis& zax2 : r2.zaxes) {
        std::set<Zaxis>::const_iterator zaxis1itr = reftime.zaxes.find(zax2);
        if (zaxis1itr == reftime.zaxes.end()) {
          // add new zaxis
          reftime.zaxes.insert(zax2);
        } else {
          // identical zaxis name, check the other attributes
          if (zaxis1itr->values != zax2.values) {
            // same id, different values
            if(zax2.values.size() == 1){
              //one value, assume merge values
              std::vector<double> newvalues = zaxis1itr->values;
              diutil::insert_all(newvalues, zax2.values);
              Zaxis newzaxis(zaxis1itr->id, zaxis1itr->name, zaxis1itr->positive, newvalues);
              METLIBS_LOG_DEBUG("     must change zaxis:");
              reftime.zaxes.erase(zaxis1itr); // remove zaxis
              reftime.zaxes.insert(newzaxis); // add zaxis
            } else {
              METLIBS_LOG_WARN("same zaxis:"<<zaxis1itr->id<<" already exist with different values: old size :"
                  <<zaxis1itr->values.size()<<" new size:"<<zax2.values.size());
              axisOk = false;
            }
          }
        }
      }

      // - Taxis
      for (const Taxis& tax2 : r2.taxes) {
        std::set<Taxis>::iterator taxis1itr = reftime.taxes.find(tax2);
        if (taxis1itr == reftime.taxes.end()) {
          METLIBS_LOG_DEBUG(" add new taxis");
          reftime.taxes.insert(tax2);
        } else {
          // identical taxis name, check the other attributes");
          if (taxis1itr->values != tax2.values) {
            METLIBS_LOG_DEBUG(" same id, different values, merge values");
              std::vector<double> newvalues = taxis1itr->values;
              diutil::insert_all(newvalues, tax2.values);
              Taxis newtaxis(taxis1itr->name, newvalues);
              METLIBS_LOG_DEBUG("     must change taxis: old size:" <<taxis1itr->values.size()
                  << " new size: " << newvalues.size());
              reftime.taxes.erase(*taxis1itr); // remove taxis
              reftime.taxes.insert(newtaxis); // add taxis
         }
        }
      } // Taxis end


      // - ExtraAxis
      for (const ExtraAxis& eax2 : r2.extraaxes) {
        std::set<ExtraAxis>::iterator eax1it = reftime.extraaxes.find(eax2);
        if (eax1it == reftime.extraaxes.end()) {
          // add new extraaxis
          reftime.extraaxes.insert(eax2);
        } else {
          // identical extraaxis name, check the other attributes
          if (eax1it->values != eax2.values) {
            // same id, different values
            if ( eax2.values.size() == 1 ) {
              //one value, assume merge values
              std::vector<double> newvalues = eax1it->values;
              diutil::insert_all(newvalues, eax2.values);
              ExtraAxis newextraaxis(eax1it->id, eax1it->name, newvalues);
              METLIBS_LOG_DEBUG("     must change extraaxis:");
              reftime.extraaxes.erase(*eax1it); // remove old extraaxis
              reftime.extraaxes.insert(newextraaxis); // add extraaxis
            } else {
              METLIBS_LOG_ERROR("same extraxis:" << eax1it->id <<" already exist with different values: old size :"
                  << eax1it->values.size() << " new size:" << eax2.values.size());
              axisOk = false;
            }
          }
        }
      }

      // add new parameters
      if ( axisOk ) {
        for (const GridParameter& p : r2.parameters) {
          if (reftime.parameters.find(p) == reftime.parameters.end()) {
            reftime.parameters.insert(p);
          }
        }
      }
    }

    result.reftimes[reftime.referencetime] = reftime; // add
  } // reference time loop

  // add new models from i
  for(Inventory::reftimes_t::const_iterator it_rf = i.reftimes.begin(); it_rf != i.reftimes.end(); ++it_rf) {
    const ReftimeInventory& rf = it_rf->second;
    if (result.reftimes.find(rf.referencetime) == result.reftimes.end()) {
      METLIBS_LOG_DEBUG(" Adding new reftime:" << rf.referencetime);
      result.reftimes[rf.referencetime] = rf;
    }
  }

  return result;
}
