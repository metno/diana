#ifndef DIANA_UTIL_SUBPROCESS_H
#define DIANA_UTIL_SUBPROCESS_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace diutil {

int execute(const QString& command, const QStringList& args, QByteArray* childStdOut=0);

} // namespace diutil

#endif // DIANA_UTIL_SUBPROCESS_H
