#include "diObsPositions.h"

ObsPositions::ObsPositions()
  : numObs(0)
  , xpos(0)
  , ypos(0)
  , interpolatedEditField(0)
  , convertToGrid(true)
{
}

ObsPositions::~ObsPositions()
{
  dealloc();
}

void ObsPositions::dealloc()
{
  delete [] xpos;
  delete [] ypos;
  delete [] interpolatedEditField;
  xpos = ypos = interpolatedEditField = 0;
  numObs = 0;
}

void ObsPositions::resize(int num)
{
  if (num == numObs && xpos != 0)
    return;

  dealloc();

  numObs = num;
  xpos = new float[numObs];
  ypos = new float[numObs];
  interpolatedEditField = new float[numObs];
}
