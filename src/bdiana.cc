#include "bdiana_capi.h"

int main(int argc, char* argv[])
{
    if (diana_init(argc, argv) != DIANA_OK)
        return 1;
    if (diana_dealloc() != DIANA_OK)
        return 2;

    return 0;
}
