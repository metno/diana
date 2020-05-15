
#ifndef diana_diField_diFieldVerticalAxes_h
#define diana_diField_diFieldVerticalAxes_h

#include <map>
#include <vector>

class FieldVerticalAxes {
public:
  /// Vertical coordinate types
  enum VerticalType {
    vctype_none, ///< surface and other single level fields
    vctype_pressure, ///< pressure levels
    vctype_hybrid, ///< model levels, eta(Hirlam,ECMWF,...) and norlam_sigma
    vctype_atmospheric, ///< other model levels (needing pressure in each level)
    vctype_isentropic, ///< isentropic (constant pot.temp.) levels
    vctype_oceandepth, ///< ocean model depth levels
    vctype_other ///< other multilevel fields (without dedicated compute functions)
  };

  struct Zaxis_info {
    std::string name;
    VerticalType vctype;
    std::string levelprefix;
    std::string levelsuffix;
    bool index;

    Zaxis_info()
        : vctype(vctype_none), index(false) {}
  };

  static std::string FIELD_VERTICAL_COORDINATES_SECTION();

  static bool parseVerticalSetup(const std::vector<std::string>& lines, std::vector<std::string>& errors);

  static const Zaxis_info* findZaxisInfo(const std::string& name);

  static VerticalType getVerticalType(const std::string& vctype);

private:
  static std::map<std::string, Zaxis_info> Zaxis_info_map;
};

#endif // diana_diField_diFieldVerticalAxes_h
