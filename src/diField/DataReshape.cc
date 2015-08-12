
#include "DataReshape.h"

#include <puTools/miStringBuilder.h>

#define MILOGGER_CATEGORY "vcross.DataReshape"
#include "miLogger/miLogging.h"

#define THROW(ex, text)                                                 \
  do {                                                                  \
    std::string msg = (miutil::StringBuilder() << text).str();          \
    METLIBS_LOG_DEBUG(__LINE__ << " throwing: " << msg);                \
    throw ex(msg);                                                      \
  } while(false)

size_t calculate_volume(const std::vector<std::size_t>& lengths)
{
  return std::accumulate(lengths.begin(), lengths.end(), 1, std::multiplies<size_t>());
}

std::vector<std::size_t> calculate_dimension_volumes(const std::vector<std::size_t>& lengths)
{
  const size_t rank = lengths.size();
  std::vector<std::size_t> volumes;
  volumes.reserve(rank+1);
  volumes[0] = 1;
  for (size_t i=1; i<=rank; ++i)
    volumes[i] = volumes[i-1]*lengths[i-1];
  return volumes;
}

bool count_identical_dimensions(const std::vector<std::string>& shapeIn, const std::vector<std::size_t>& lengthsIn,
    const std::vector<std::string>& shapeOut, const std::vector<std::size_t>& lengthsOut,
    size_t& sameIn, size_t& sameOut, size_t& sameSize)
{
  const size_t sizeIn = shapeIn.size(), sizeOut = shapeOut.size();
  if (sizeIn != lengthsIn.size() or sizeOut != lengthsOut.size())
    THROW(std::runtime_error, "shape/length vector size mismatch");
  
  sameIn = sameOut = 0;
  sameSize=1;

  while (sameIn < sizeIn and sameOut < sizeOut) {
    if (lengthsIn[sameIn] == 1) {
      sameIn += 1;
    } else if (lengthsOut[sameOut] == 1) {
      sameOut += 1;
    } else if (lengthsIn[sameIn] == lengthsOut[sameOut] and shapeIn[sameIn] == shapeOut[sameOut]) {
      sameSize *= lengthsIn[sameIn]; // same size for out, as checked above
      sameIn += 1;
      sameOut += 1;
    } else {
      break;
    }
  }
  while (sameIn < sizeIn and lengthsIn[sameIn] == 1)
    sameIn += 1;
  while (sameOut < sizeOut and lengthsOut[sameOut] == 1)
    sameOut += 1;
  return (sameIn == sizeIn and sameOut == sizeOut);
}

std::vector<int> map_dimensions(const std::vector<std::string>& shapeIn, const std::vector<std::size_t>& lengthsIn,
    const std::vector<std::string>& shapeOut, const std::vector<std::size_t>& lengthsOut,
    size_t sameIn, size_t sameOut)
{
  typedef std::vector<std::string> string_v;

  const size_t sizeIn = shapeIn.size(), sizeOut = shapeOut.size();
  std::vector<int> positionOutIn(sizeOut, -1);
  for (size_t o=sameOut; o<sizeOut; ++o) {
    const string_v::const_iterator it = std::find(shapeIn.begin(), shapeIn.end(), shapeOut[o]);
    if (it != shapeIn.end()) {
      size_t i = it - shapeIn.begin();
      positionOutIn[o] = i;
      if (lengthsOut[o] != lengthsIn[i])
        THROW(std::runtime_error, "dim size mismatch for '" << shapeOut[o] << "': out=" << lengthsOut[o] << " in=" << lengthsIn[i]);
    }
  }
  for (size_t i=sameIn; i<sizeIn; ++i) {
    if (lengthsIn[i] != 1) {
      const string_v::const_iterator it = std::find(shapeOut.begin(), shapeOut.end(), shapeIn[i]);
      if (it == shapeOut.end())
        THROW(std::runtime_error, "dim '" << shapeIn[i] << "' with non-1 length " << lengthsIn[i] << " in output is not found in input");
    }
  }
  return positionOutIn;
}
