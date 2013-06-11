#ifndef SCRIPT_H
#define SCRIPT_H

#include <QWidget>
#include <QProcess>
#include <QTemporaryFile>
#include <QString>
#include <QGridLayout>


/*
#include <QPlainTextEdit>
#include <QLabel>

#include <QLineEdit>
#include <QToolButton>
#include <QStyle>

#include <QTableWidget>
#include <QWidgetList>
#include <QProgressBar>
*/


struct ColumnResizerPrivate;
class ColumnResizer : public QObject {
  Q_OBJECT;
public:
  ColumnResizer(QObject* parent = 0);
  ~ColumnResizer();

  void addWidget(QWidget* widget);
  void addWidgetsFromGridLayout(QGridLayout*, int column);

private Q_SLOTS:
  void updateWidth();

protected:
  bool eventFilter(QObject*, QEvent* event);

private:
  ColumnResizerPrivate* const d;
};



namespace Ui {
class Script;
}

class Script : public QWidget {
  Q_OBJECT;

private:
  Ui::Script *ui;
  QProcess proc;
  QTemporaryFile fileExec;

public:
  explicit Script(QWidget *parent = 0);
  ~Script();

  void setPath(const QString & _path);
  const QString out() {return proc.readAllStandardOutput();}
  const QString err() {return proc.readAllStandardError();}
  int waitStop();
  bool isRunning() const { return proc.pid(); };
  const QString path() const;

  void addToColumnResizer(ColumnResizer * columnizer);

public slots:
  bool start();
  int execute() { return start() ? waitStop() : -1 ; };
  void stop() {if (isRunning()) proc.kill();};

private slots:
  void browse();
  void evaluate();
  void onState(QProcess::ProcessState state);
  void onStartStop() { if (isRunning()) stop(); else start(); };

signals:
  void editingFinished();
  void executed();
  void finished(int status);
  void started();

};


#endif // SCRIPT_H
