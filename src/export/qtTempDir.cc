#include "qtTempDir.h"

#include <util/subprocess.h>

#define MILOGGER_CATEGORY "diana.TempDir"
#include <miLogger/miLogging.h>

TempDir::TempDir()
  : created_(false)
{
}

TempDir::~TempDir()
{
  destroy();
}

bool TempDir::create()
{
  if (!created_) {
    char tmpl[256];
    snprintf(tmpl, sizeof(tmpl), "%s/diana-XXXXXX", QDir::tempPath().toStdString().c_str());
    const char* outdir = mkdtemp(tmpl);
    if (outdir) {
      dir_ = QDir(outdir);
      created_ = true;
      destroy_ = true;
    } else {
      METLIBS_LOG_ERROR("could not create temp dir");
    }
  }
  return created_;
}

void TempDir::destroy()
{
  if (!created_)
    return;

  created_ = false;

  if (!destroy_)
    return;

  QString outdir = dir_.absolutePath();
  if (diutil::execute("rm", QStringList() << "-rf" << outdir) != 0)
    METLIBS_LOG_ERROR("could not remove temp dir '" << outdir.toStdString() << "'");
}
