#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "bdiana_capi.h"


MODULE = Metno::Bdiana		PACKAGE = Metno::Bdiana		

void
init()
  PREINIT:
    char* argv[] =  {"-v", "-use_qimage", "-use_singlebuffer"};
  CODE:
    mi_di_init(3 , argv);

void
readSetupFile(const char* setupFilename, OUTLIST int ret)
  CODE:
    ret = mi_di_readSetupFile(setupFilename);

void
parseAndProcessString(const char* str, OUTLIST int ret)
  CODE:
    ret = mi_di_parseAndProcessString(str);


