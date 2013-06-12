#include "script.h"
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QTimer>
#include <QList>
#include <QEventLoop>




Script::Script(QObject *parent) :
  QObject(parent),
  pth(),
  proc(this),
  fileExec(this)
{
  connect(&proc, SIGNAL(stateChanged(QProcess::ProcessState)),
          SLOT(onState(QProcess::ProcessState)));
  if ( ! fileExec.open() )
    qDebug() << "ERROR! Unable to open temporary file.";
}


int Script::setPath(const QString & _path) {
  pth = _path;
  return evaluate();
}

const QString Script::path() const {
  return pth;
}


int Script::evaluate() {

  if ( ! fileExec.isOpen() || isRunning() )
    return -1;

  fileExec.resize(0);
  fileExec.write( pth.toAscii() );
  fileExec.flush();

  QProcess tempproc;
  tempproc.start("/bin/sh -n " + fileExec.fileName());
  tempproc.waitForFinished();
  return tempproc.exitCode();

}


bool Script::start() {
  if ( ! fileExec.isOpen() || isRunning() || pth.isEmpty() )
    return false;
  proc.start("/bin/sh " + fileExec.fileName());
  return isRunning();
}

int Script::waitStop() {
  QEventLoop q;
  connect(&proc, SIGNAL(finished(int)), &q, SLOT(quit()));
  if (isRunning())
    q.exec();
  return proc.exitCode();
}

void Script::onState(QProcess::ProcessState state) {
  if (state==QProcess::NotRunning) {
    lastErr = proc.readAllStandardError();
    if( lastErr.size() && lastErr.at(lastErr.size()-1) == '\n' )
      lastErr.chop(1);
    lastOut = proc.readAllStandardOutput();
    if( lastOut.size() && lastOut.at(lastOut.size()-1) == '\n' )
      lastOut.chop(1);
    emit finished(proc.exitCode());
    emit outChanged( lastOut );

  } else if (state==QProcess::Running) {
    emit started();
  }
};

