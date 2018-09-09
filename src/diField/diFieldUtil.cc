
#include "diFieldUtil.h"

namespace diutil {

GridArea makeGridArea(const gridinventory::Grid& grid)
{
  const float x0 = grid.x_0, y0 = grid.y_0;
  const Rectangle rect(x0, y0, x0 + (grid.nx - 1) * grid.x_resolution, y0 + (grid.ny - 1) * grid.y_resolution);
  const Projection proj(grid.projection);
  return GridArea(Area(proj, rect), grid.nx, grid.ny, grid.x_resolution, grid.y_resolution);
}

} // namespace diutil
