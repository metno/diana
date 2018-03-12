#include "bdiana_capi.h"

int main(int argc, char* argv[])
{
  int status = 0;
  try {
    if (diana_init(argc, argv) != DIANA_OK)
      status = 1;
  } catch (...) {
    status = 1;
  }
  diana_dealloc();

  return status;
}
