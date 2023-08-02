/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2022 met.no

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

#ifndef DATARESHAPE_HH
#define DATARESHAPE_HH 1

#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace diutil {
template <typename T>
using shared_array = std::shared_ptr<T[]>;

template <typename T>
shared_array<T> make_shared_array(size_t size)
{
#if __cplusplus >= 201703L
  return std::shared_ptr<T[]>(new T[size]);
#else
  return std::shared_ptr<T[]>(new T[size], std::default_delete<T[]>());
#endif
}
} // namespace diutil

size_t calculate_volume(const std::vector<std::size_t>& lengths);

std::vector<std::size_t> calculate_dimension_volumes(const std::vector<std::size_t>& lengths);

bool count_identical_dimensions(const std::vector<std::string>& shapeIn, const std::vector<std::size_t>& lengthsIn,
    const std::vector<std::string>& shapeOut, const std::vector<std::size_t>& lengthsOut,
    size_t& sameIn, size_t& sameOut, size_t& sameSize);

std::vector<int> map_dimensions(const std::vector<std::string>& shapeIn, const std::vector<std::size_t>& lengthsIn,
    const std::vector<std::string>& shapeOut, const std::vector<std::size_t>& lengthsOut,
    size_t sameIn, size_t sameOut);

template<typename T>
void copy_values(const std::vector<std::size_t>& lengthsIn, const std::vector<std::size_t>& lengthsOut,
    size_t sameOut, size_t sameSize, const std::vector<int> positionOutIn, const T* valuesIn, T* valuesOut)
{
  typedef std::vector<std::size_t> size_v;
  const size_v sliceIn = calculate_dimension_volumes(lengthsIn);
  const size_v sliceOut = calculate_dimension_volumes(lengthsOut);

  const size_t sizeIn = lengthsIn.size(), sizeOut = lengthsOut.size();
  size_t idxIn = 0, idxOut = 0, incIdxOut;
  size_v posIn(sizeIn, 0), posOut(sizeOut, 0);
  do {
    std::copy(valuesIn + idxIn, valuesIn + idxIn + sameSize, valuesOut + idxOut);

    for (incIdxOut = sameOut; incIdxOut < sizeOut; ++incIdxOut) {
      posOut[incIdxOut] += 1;
      idxOut += sliceOut[incIdxOut];
      const int incIdxIn = positionOutIn[incIdxOut];
      if (incIdxIn>=0) {
        posIn[incIdxIn] += 1;
        idxIn += sliceIn[incIdxIn];
      }
      if (posOut[incIdxOut] < lengthsOut[incIdxOut]) {
        break;
      } else {
        posOut[incIdxOut] = 0;
        idxOut -= sliceOut[incIdxOut+1];
        if (incIdxIn>=0) {
          posIn[incIdxIn] = 0;
          idxIn -= sliceIn[incIdxIn+1];
        }
      }
    }
  } while (incIdxOut < sizeOut);
}

template<typename T>
void reshape(const std::vector<std::string>& shapeIn, const std::vector<std::size_t>& lengthsIn,
    const std::vector<std::string>& shapeOut, const std::vector<std::size_t>& lengthsOut,
    const T* valuesIn, T* valuesOut)
{
  size_t sameIn=0, sameOut=0, sameSize=1; // TODO remove sameSize == sliceIn[sameIn] ?!
  count_identical_dimensions(shapeIn, lengthsIn, shapeOut, lengthsOut, sameIn, sameOut, sameSize);

  const std::vector<int> positionOutIn = map_dimensions(shapeIn, lengthsIn, shapeOut, lengthsOut, sameIn, sameOut);
  copy_values(lengthsIn, lengthsOut, sameOut, sameSize, positionOutIn, valuesIn, valuesOut);
}

template <typename T>
std::shared_ptr<T[]> reshape(const std::vector<std::string>& shapeIn, const std::vector<std::size_t>& lengthsIn, const std::vector<std::string>& shapeOut,
                             const std::vector<std::size_t>& lengthsOut, std::shared_ptr<T[]> dataIn)
{
  size_t sameIn=0, sameOut=0, sameSize=1; // TODO remove sameSize == sliceIn[sameIn] ?!
  if (count_identical_dimensions(shapeIn, lengthsIn, shapeOut, lengthsOut, sameIn, sameOut, sameSize)) {
    // all same, nothing to do, just return input array
    return dataIn;
  } else {
    auto dataOut = diutil::make_shared_array<T>(calculate_volume(lengthsOut));

    const std::vector<int> positionOutIn = map_dimensions(shapeIn, lengthsIn, shapeOut, lengthsOut, sameIn, sameOut);
    copy_values(lengthsIn, lengthsOut, sameOut, sameSize, positionOutIn, dataIn.get(), dataOut.get());
    
    return dataOut;
  }
}

#endif // DATARESHAPE_HH
