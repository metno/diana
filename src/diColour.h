/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#ifndef diColour_h
#define diColour_h

#include <map>
#include <string>
#include <vector>

/**
  \brief Colour type

  Colour definition, with R, G, B and alpha (translucency) component.
  - static list of defined colours, reachable by name
*/

class Colour {
public:
  enum cIndex{
    red   =0,
    green =1,
    blue  =2,
    alpha =3
  };
  enum {maxv= 255};
  /// 4 component colour data
  struct values{
    unsigned char rgba[4];
    inline values();
    inline values& operator=(const values &rhs);
    inline bool operator==(const values &rhs) const ;
  };
  /// 3 component colour data and name
  struct ColourInfo {
    int rgb[3];
    std::string name;
  };

private:
  std::string name;
  values v;
  unsigned char colourindex;
  static std::map<std::string,Colour> cmap;
  static std::vector<ColourInfo> colours;

  void memberCopy(const Colour& rhs);
  void generateName();

public:
  explicit Colour(const std::string&);
  explicit Colour(const values&);
  explicit Colour(unsigned long int =0);
  explicit Colour(unsigned char, unsigned char, unsigned char, unsigned char = maxv);
  static Colour fromF(float, float, float, float = 1.0f);
  Colour(const Colour &rhs);

  ~Colour() { }

  Colour& operator=(const Colour &rhs);
  bool operator==(const Colour &rhs) const;

  // static functions for static colour-map
  static void define(const std::string&, const unsigned char, const unsigned char,
      const unsigned char, const unsigned char =maxv);
  static void define(const std::string, const values&);
  static void defineColourFromString(const std::string& rgba_string);
  static void setindex(const std::string&, const unsigned char);

  // static functions for static vector <ColourInfo> colours
  static void addColourInfo(const ColourInfo& ci);
  static std::vector<ColourInfo> getColourInfo(){return colours;}

  void set(unsigned char r, unsigned char g, unsigned char b, unsigned char a =maxv)
    { v.rgba[red]=r; v.rgba[green]=g; v.rgba[blue]=b; v.rgba[alpha]=a;}

  void setF(float r, float g, float b, float a = 1.0)
    { v.rgba[red] = r*maxv; v.rgba[green] = g*maxv; v.rgba[blue] = b*maxv; v.rgba[alpha] = a*maxv;}

  void set(const values& va)
    {v= va;}

  void set(cIndex i, unsigned char b)
    { v.rgba[i]=b; }

  void setF(cIndex i, float b)
    { v.rgba[i]=b*maxv; }

  unsigned char R() const {return v.rgba[red];   }
  unsigned char G() const {return v.rgba[green]; }
  unsigned char B() const {return v.rgba[blue];  }
  unsigned char A() const {return v.rgba[alpha]; }

  float fR() const {return 1.0*v.rgba[red]/maxv;  }
  float fG() const {return 1.0*v.rgba[green]/maxv;}
  float fB() const {return 1.0*v.rgba[blue]/maxv; }
  float fA() const {return 1.0*v.rgba[alpha]/maxv;}

  const unsigned char* RGBA() const {return v.rgba; }
  const unsigned char* RGB()  const {return v.rgba; }
  unsigned char Index() const {return colourindex; }

  const std::string& Name() const {return name;}

  Colour contrastColour() const;

  friend std::ostream& operator<<(std::ostream& out, const Colour& rhs);
};


// inline Colour::values member functions

inline Colour::values::values(){
  rgba[0]=0; rgba[1]=0;
  rgba[2]=0; rgba[3]=0;
}


inline Colour::values& Colour::values::operator=(const Colour::values &rhs){
  if (this != &rhs){
    rgba[0]= rhs.rgba[0];
    rgba[1]= rhs.rgba[1];
    rgba[2]= rhs.rgba[2];
    rgba[3]= rhs.rgba[3];
  }
  return *this;
}

inline bool Colour::values::operator==(const Colour::values &rhs) const {
  return (rgba[0]==rhs.rgba[0] && rgba[1]==rhs.rgba[1] &&
      rgba[2]==rhs.rgba[2] && rgba[3]==rhs.rgba[3]);
}

#endif
