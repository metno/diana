/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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

#include "diColour.h"

#include <puTools/miStringFunctions.h>

#include <iomanip>
#include <ostream>

#define MILOGGER_CATEGORY "diana.Colour"
#include <miLogger/miLogging.h>

using namespace miutil;

std::map<std::string,Colour> Colour::cmap;
std::vector<Colour::ColourInfo> Colour::colours;

Colour Colour::BLACK(0, 0, 0, maxv);
Colour Colour::WHITE(maxv, maxv, maxv, maxv);
Colour Colour::RED(maxv, 0, 0, maxv);
Colour Colour::GREEN(0, maxv, 0, maxv);
Colour Colour::BLUE(0, 0, maxv, maxv);
Colour Colour::YELLOW(maxv, maxv, 0, maxv);
Colour Colour::CYAN(0, maxv, maxv, maxv);

Colour::Colour(const values& va) : v(va){
}

Colour::Colour()
{
  set(0, 0, 0);
  name = "black";
}

Colour::Colour(const std::string& name_)
{
  const std::string lname = miutil::to_lower(name_);
  const std::vector<std::string> vstr = miutil::split(lname, ":");
  const size_t n = vstr.size();
  if (n < 2) {
    std::map<std::string, Colour>::const_iterator it = cmap.find(lname);
    if (it != cmap.end())
      memberCopy(it->second);
    else {
      METLIBS_LOG_DEBUG("Colour:"<<lname<<" unknown");
      set(0,0,0);
      name = "black";
    }
  } else if (n < 3) {
    memberCopy(cmap[vstr[0]]);
    v.rgba[alpha] = miutil::to_int(vstr[1]);
  } else if (n < 4) {
    set(miutil::to_int(vstr[0]), miutil::to_int(vstr[1]), miutil::to_int(vstr[2]));
    name = lname;
  } else {
    set(miutil::to_int(vstr[0]), miutil::to_int(vstr[1]), miutil::to_int(vstr[2]), miutil::to_int(vstr[3]));
    name = lname;
  }
}

Colour::Colour(unsigned char r, unsigned char g, unsigned char b,  unsigned char a)
{
  set(r,g,b,a);
  generateName();
}

Colour Colour::fromF(float r, float g, float b,  float a)
{
  Colour c;
  c.setF(r,g,b,a);
  c.generateName();
  return c;
}

Colour::Colour(const Colour &rhs)
{
  memberCopy(rhs);
}

void Colour::generateName()
{
  name =miutil::from_number(R()) +":";
  name+=miutil::from_number(G()) +":";
  name+=miutil::from_number(B()) +":";
  name+=miutil::from_number(A());
}

Colour& Colour::operator=(const Colour &rhs)
{
  if (this != &rhs)
    memberCopy(rhs);
  return *this;
}

bool Colour::operator==(const Colour &rhs) const
{
  return v == rhs.v;
}

void Colour::memberCopy(const Colour& rhs)
{
  v = rhs.v;
  name = rhs.name;
}

void Colour::define(const std::string& name_, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  Colour c(r,g,b,a);
  const std::string lname = miutil::to_lower(name_);
  c.name = lname;
  cmap[lname] = c;
}

void Colour::define(const std::string name_, const values& va)
{
  Colour c(va);
  const std::string lname = miutil::to_lower(name_);
  c.name = lname;
  cmap[lname] = c;
}

void Colour::defineColourFromString(const std::string& rgba_string)
{
  unsigned char r,g,b,a;
  std::vector<std::string> stokens = miutil::split(rgba_string, ":");
  if (stokens.size()>2 ) {
    r = miutil::to_int(stokens[0]);
    g = miutil::to_int(stokens[1]);
    b = miutil::to_int(stokens[2]);
    if (stokens.size()>3) {
      a = miutil::to_int(stokens[3]);
    } else {
      a= 255;
    }
    define(rgba_string,r,g,b,a);
  }

}

void Colour::addColourInfo(const ColourInfo& ci)
{
  if (!ci.name.empty()) {
    for (ColourInfo& q : colours)
      if (q.name == ci.name) {
        q = ci;
        return;
      }
  }
  colours.push_back(ci);
}

Colour Colour::contrastColour() const
{
  const unsigned int sum = R() + G() + B();
  if (sum > 255 * 3 / 2)
    return Colour(0, 0, 0);
  else
    return Colour(255, 255, 255);
}

std::ostream& operator<<(std::ostream& out, const Colour& rhs)
{
  out << " name: " << rhs.name                                        // name
      << " red: " << std::setw(3) << std::setfill('0') << int(rhs.v.rgba[0])    // r
      << " green: " << std::setw(3) << std::setfill('0') << int(rhs.v.rgba[1])  // g
      << " blue: " << std::setw(3) << std::setfill('0') << int(rhs.v.rgba[2])   // b
      << " alpha: " << std::setw(3) << std::setfill('0') << int(rhs.v.rgba[3]); // alpha
  return out;
}
