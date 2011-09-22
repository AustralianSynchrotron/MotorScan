#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QList>
#include <QHash>
#include <QTableWidget>
#include <QCheckBox>
#include <QDebug>
#include <QSettings>
#include <QMdiSubWindow>
#include <QMenu>
#include <QProcess>
#include <QComboBox>

#include <qcamotorgui.h>

#include "graph.h"


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT;

public:

    MainWindow(QWidget *parent = 0);

private:

    Ui::MainWindow *ui;

    QString qtiCommand;
    QSettings * localSettings;
    static QSettings * globalSettings;
    bool nowLoading;
    static const QString badStyle;
    static const QString goodStyle;


    QString tableWasSavedTo;

    bool stopNow;


    class Signal;
    QList<Signal*> signalsE;
    void constructSignalsLayout();

    bool nowScanning();

    static void updateGUI();

    int column(Signal* sig) const;

    Line xAxisData;
    Line yAxisData;

private slots:

    void browseAutoSave();
    QString prepareAutoSave();
    void printResult();
    void saveResult();
    void startStop();
    void startScan();
    void stopScan();
    void addSignal(const QString & pvName="");
    void removeSignal();
    void switchDimension(bool secondDim);
    void checkReady();
    void storeSettings();
    void openQti();
    void updatePlots();

};



class CloseFilter : public QObject {
    Q_OBJECT;
protected:
    bool eventFilter(QObject *obj, QEvent *event) {
      if (event->type() == QEvent::Close) {
        ( (QMdiSubWindow *) obj ) -> showMinimized();
        event->ignore();
        return true;
      } else {
        return QObject::eventFilter(obj, event);
      }
    }
};






class MainWindow::Signal : public QObject {

  Q_OBJECT;

public:

  static QStringList knownDetectors;
  QPushButton * rem;
  QComboBox * sig;
  QLabel * val;
  QTableWidgetItem * tableItem;
  QMdiSubWindow * plotWin;
  QEpicsPv * pv;

private:

  Line data;
  Graph * graph;
  static CloseFilter * closeFilt;
  int point;

public:

  Signal(QWidget* parent=0);
  ~Signal();

  void setData(Line * xData);
  void setData(int width, int height,
               double xStart, double xEnd,
               double yStart, double yEnd);

  inline void print(QPrinter & printer) {graph->print(printer);}

  double get(int pos=-1);

private slots:

  void setHeader(const QString & text);
  inline void updateValue(const QVariant & data) { val->setText(data.toString()); }

};

#endif // MAINWINDOW_H
