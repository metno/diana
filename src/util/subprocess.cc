
#include "subprocess.h"

#define DO_NOT_USE_QPROCESS 1
#if !defined(DO_NOT_USE_QPROCESS)
#include <QProcess>
#endif
#if defined(DO_NOT_USE_QPROCESS) || defined(QT_NO_PROCESS)
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif // DO_NOT_USE_QPROCESS || QT_NO_PROCESS

namespace diutil {

#if defined(DO_NOT_USE_QPROCESS) || defined(QT_NO_PROCESS)
namespace {
char* qstrdup(const QString& qs)
{
  return strdup(qs.toStdString().c_str());
}
const int PIPE_READ = 0, PIPE_WRITE = 1;
} // namespace

int execute(const QString& command, const QStringList& args, QByteArray* childStdOut)
{
  const int MAX_ARGS = 250;
  if (args.size() >= MAX_ARGS)
    return -2;

  // see http://stackoverflow.com/a/12839498
  int pipeStdOut[2];
  if (childStdOut) {
    if (pipe(pipeStdOut) < 0)
      return -1;
  }

  const pid_t pid = fork();
  if (pid == 0) {
    // child process

    if (childStdOut) {
      close(STDIN_FILENO);
      if (dup2(pipeStdOut[PIPE_WRITE], STDOUT_FILENO) == -1)
        return -1;
      //if (dup2(pipeChildStdout[PIPE_WRITE], STDERR_FILENO) == -1)
      //    return -1;
      close(pipeStdOut[PIPE_READ]);
      close(pipeStdOut[PIPE_WRITE]);
    }

    char* cmd = qstrdup(command);
    char* argv[MAX_ARGS+2] = { 0 };
    argv[0] = qstrdup(command);
    for (int i=0; i<args.size(); ++i)
      argv[i+1] = qstrdup(args.at(i));

    execvp(cmd, argv);
    exit(1);
    // not reached
  } else if (pid > 0) {
    // parent process

    if (childStdOut) {
      close(pipeStdOut[PIPE_WRITE]);
      char buf;
      while (read(pipeStdOut[PIPE_READ], &buf, 1) == 1) {
        childStdOut->append(buf);
      }
      close(pipeStdOut[PIPE_READ]);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      return WEXITSTATUS(status);
    } else {
      return -1;
    }
  } else {
    // fork error
    if (childStdOut) {
      close(pipeStdOut[PIPE_READ]);
      close(pipeStdOut[PIPE_WRITE]);
    }
    return -1;
  }
}
#else // !(DO_NOT_USE_QPROCESS || QT_NO_PROCESS)
int execute(const QString& command, const QStringList& args, QByteArray* childStdOut)
{
  QProcess p;
  p.start(command, args);
  p.waitForFinished(-1);
  if (childStdOut) {
    *childStdOut = p.readAllStandardOutput();
  }
  return p.exitCode();
}
#endif // !(DO_NOT_USE_QPROCESS || QT_NO_PROCESS)

} // namespace diutil
