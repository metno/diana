
#include "diFieldVerticalAxes.h"

#include <puTools/miStringFunctions.h>

std::map< std::string, FieldVerticalAxes::Zaxis_info > FieldVerticalAxes::Zaxis_info_map;

// static
std::string FieldVerticalAxes::FIELD_VERTICAL_COORDINATES_SECTION()
{
  return "FIELD_VERTICAL_COORDINATES";
}

// static
const FieldVerticalAxes::Zaxis_info* FieldVerticalAxes::findZaxisInfo(const std::string& name)
{
  const std::map< std::string, Zaxis_info>::const_iterator it = Zaxis_info_map.find(name);
  if (it != Zaxis_info_map.end())
    return &(it->second);
  else
    return 0;
}

// static
FieldVerticalAxes::VerticalType FieldVerticalAxes::getVerticalType(const std::string& vctype)
{
  if ( vctype == "none" )
    return FieldVerticalAxes::vctype_none;
  if ( vctype == "pressure")
    return FieldVerticalAxes::vctype_pressure;
  if ( vctype == "hybrid")
    return FieldVerticalAxes::vctype_hybrid;
  if ( vctype == "atmospheric")
    return FieldVerticalAxes::vctype_atmospheric;
  if ( vctype == "isentropic")
    return FieldVerticalAxes::vctype_isentropic;
  if ( vctype == "oceandepth")
    return FieldVerticalAxes::vctype_oceandepth;
  if ( vctype == "other")
    return FieldVerticalAxes::vctype_other;

  return FieldVerticalAxes::vctype_none;
}

bool FieldVerticalAxes::parseVerticalSetup(const std::vector<std::string>& lines,
                                        std::vector<std::string>& errors)
{
  const std::string key_name = "name";
  const std::string key_vc_type = "vc_type";
  const std::string key_levelprefix = "levelprefix";
  const std::string key_levelsuffix = "levelsuffix";
  const std::string key_index = "index";

  int nlines = lines.size();

  for (int l = 0; l < nlines; l++) {
    Zaxis_info zaxis_info;
    std::vector<std::string> tokens= miutil::split_protected(lines[l], '"','"');
    for (size_t  i = 0; i < tokens.size(); i++) {
      std::vector<std::string> stokens= miutil::split_protected(tokens[i], '"','"',"=",true);
      if (stokens.size() == 2 )  {
        if( stokens[0] == key_name ) {
          zaxis_info.name = stokens[1];
        } else  if( stokens[0] == key_vc_type ) {
          zaxis_info.vctype= getVerticalType(stokens[1]);
        } else  if( stokens[0] == key_levelprefix ) {
          zaxis_info.levelprefix = stokens[1];
        } else  if( stokens[0] == key_levelsuffix ) {
          zaxis_info.levelsuffix = stokens[1];
        } else  if( stokens[0] == key_index ) {
          zaxis_info.index = (stokens[1]=="true");
        }
      }
    }
    Zaxis_info_map[zaxis_info.name] = zaxis_info;
  }

  return true;
}
