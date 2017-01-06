#ifndef QTTEMPDIR_H
#define QTTEMPDIR_H

#include <QDir>

class TempDir
{
public:
  TempDir();
  ~TempDir();

  bool create();
  void destroy();
  void nodestroy()
    { destroy_ = false; }

  const QDir& dir() const
    { return dir_; }

  bool exists() const
    { return created_ && dir_.exists(); }

  QString filePath(const QString& fileName) const
    { return dir_.filePath(fileName); }

private:
  QDir dir_;
  bool created_;
  bool destroy_;
};

#endif // QTTEMPDIR_H
