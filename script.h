#ifndef SCRIPT_H
#define SCRIPT_H

#include <QProcess>
#include <QTemporaryFile>


class Script : public QObject {
  Q_OBJECT;

private:
  QString pth;
  QString lastOut;
  QString lastErr;
  QProcess proc;
  QTemporaryFile fileExec;

public:
  explicit Script(QObject *parent = 0);

  int setPath(const QString & _path);
  const QString out() {return lastOut;}
  const QString err() {return lastErr;}
  int waitStop();
  bool isRunning() const { return proc.pid(); };
  const QString path() const;

public slots:
  bool start();
  int execute() { return start() ? waitStop() : -1 ; };
  void stop() {if (isRunning()) proc.kill();};

private slots:
  int evaluate();
  void onState(QProcess::ProcessState state);
  void onStartStop() { if (isRunning()) stop(); else start(); };

signals:

  void executed();
  void finished(int status);
  void started();
  void outChanged(const QString & out);

};


#endif // SCRIPT_H
