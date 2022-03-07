/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2022 met.no

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

#ifndef diObsDataContainer_h
#define diObsDataContainer_h

#include "diObsData.h"

#include <vector>

class ObsDataRef;

/*! Abstract container for observation data.
 *
 * Observations are accessed by index and are read-only in this interface.
 */
struct ObsDataContainer
{
  virtual ~ObsDataContainer();

  virtual bool empty() const { return size() == 0; }
  virtual size_t size() const = 0;
  virtual ObsDataRef at(size_t idx) const = 0;
  ObsDataRef operator[](size_t idx) const;

  virtual const ObsDataBasic& basic(size_t i) const = 0;
  virtual const ObsDataMetar& metar(size_t i) const = 0;

  virtual std::vector<std::string> get_keys() const = 0;
  virtual const std::string* get_string(size_t i, const std::string& key) const = 0;
  virtual const float* get_float(size_t i, const std::string& key) const = 0;
};

typedef std::shared_ptr<ObsDataContainer> ObsDataContainer_p;
typedef std::shared_ptr<const ObsDataContainer> ObsDataContainer_cp;

/*! Helper class for accessing a specific observation in an ObsDataContainer.
 *
 * Consists of a pointer to the container and the index in that container.
 * Contains some shortcut functions to access observation values.
 */
class ObsDataRef
{
public:
  //! construct accessor for observation with index `ì` in ObsDataContainer `c`
  ObsDataRef(const ObsDataContainer& c, size_t i) : c_(c), i_(i) { }

  const std::string& dataType() const { return c_.basic(i_).dataType; }
  const std::string& id() const { return c_.basic(i_).id; }
  float xpos() const { return c_.basic(i_).xpos; }
  float ypos() const { return c_.basic(i_).ypos; }
  const miutil::miTime& obsTime() const { return c_.basic(i_).obsTime; };

  // metar
  bool ship_buoy() const { return c_.metar(i_).ship_buoy; };
  std::string metarId() const { return c_.metar(i_).metarId; };
  bool CAVOK() const { return c_.metar(i_).CAVOK; };
  const std::vector<std::string>& REww() const { return c_.metar(i_).REww; };
  const std::vector<std::string>& ww() const { return c_.metar(i_).ww; }
  const std::vector<std::string>& cloud() const { return c_.metar(i_).cloud; }

  const std::string* get_string(const std::string& key) const { return c_.get_string(i_, key); }
  const float* get_float(const std::string& key) const { return c_.get_float(i_, key); }

private:
  const ObsDataContainer& c_;
  const size_t i_;
};

inline ObsDataRef ObsDataContainer::at(size_t idx) const
{
  return ObsDataRef(*this, idx);
}

inline ObsDataRef ObsDataContainer::operator[](size_t idx) const
{
  return at(idx);
}

#endif // diObsDataContainer_h
