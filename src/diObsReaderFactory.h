#ifndef DIOBSREADERFACTORY_H
#define DIOBSREADERFACTORY_H

#include "diObsReader.h"
#include <string>

ObsReader_p makeObsReader(const std::string& format);

#endif // DIOBSREADERFACTORY_H
