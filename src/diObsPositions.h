#ifndef OBSPOSITIONS_H
#define OBSPOSITIONS_H

#include "diField/diArea.h"

struct ObsPositions {
  Area obsArea;
  int numObs;
  float* xpos;
  float* ypos;
  float* interpolatedEditField;
  bool convertToGrid;

  ObsPositions();
  ~ObsPositions();
  void dealloc();
  void resize(int num);
};

#endif // OBSPOSITIONS_H
