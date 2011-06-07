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

#include <qcamotorgui.h>

#include <graph.h>


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT;

public:

    MainWindow(QWidget *parent = 0);

private:

    QString qtiCommand;
    QSettings * localSettings;
    static QSettings * globalSettings;
    static const QString badStyle;
    static const QString goodStyle;

    QHash < QString, QPair<QString,QString> > knownDeTrig;

    QString tableWasSavedTo;

    Ui::MainWindow *ui;
    bool stopNow;

    class Trigger;
    QList<Trigger*> triggersE;
    void constructTriggersLayout();
    bool triggersContains(const QString & trigPV, const QString & trigval = QString());

    class Signal;
    QList<Signal*> signalsE;
    void constructSignalsLayout();

    QwtArray<double> xAxisData;
    QwtArray<double> yAxisData;

    bool nowScanning();

    static void updateGUI();

    int column(Signal* sig) const;

private slots:

    void browseAutoSave();
    QString prepareAutoSave();
    void printResult();
    void saveResult();
    void startStop();
    void startScan();
    void stopScan();
    void addTrigger(const QString & pvName="", const QString & value="");
    void removeTrigger();
    void addSignal(const QString & pvName="");
    void updateSignal(QString pvName);
    void removeSignal();
    void switchDimension(bool secondDim);
    void checkReady();
    void trigAll();
    void storeSettings();
    void openQti();
    void changeWinmode();

};

class MainWindow::Trigger : public QObject {
  Q_OBJECT;
public:
  QEpicsPv * pv;
  QPushButton * rem;
  QLineEdit * trig;
  QLineEdit * val;
  inline Trigger(QWidget* parent=0) :
      pv(new QEpicsPv(this)),
      rem(new QPushButton("-", parent)),
      trig(new QLineEdit(parent)),
      val(new QLineEdit(parent)) {
    setConnected(false);
    connect(trig, SIGNAL(textChanged(QString)), pv, SLOT(setPV(QString)));
    connect(pv, SIGNAL(connectionChanged(bool)), SLOT(setConnected(bool)));
    connect(rem, SIGNAL(clicked()), SIGNAL(remove()));
    trig->setToolTip("PV of the trigger.");
    rem->setToolTip("Remove the trigger.");
    val->setToolTip("Value to be written to the trigger's PV.");
  }
  inline ~Trigger() {
    delete pv;
    delete rem;
    delete trig;
    delete val;
  }
private slots:
  inline void setConnected(bool con) {
    rem->setStyleSheet( con ? "" :
                        "background-color: rgba(255, 0, 0,64);");
  }
public slots:
  inline void trigMe() {
    QEventLoop q;
    connect(pv, SIGNAL(connectionChanged(bool)), &q, SLOT(quit()));
    connect(pv, SIGNAL(destroyed()), &q, SLOT(quit()));
    connect(pv, SIGNAL(valueUpdated(QVariant)), &q, SLOT(quit()));
    pv->set(val->text());
    q.exec();
  }
signals:
  void remove();
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
  Graph * graph;

private:

  static CloseFilter * closeFilt;


public:

  Signal(QWidget* parent=0);
  ~Signal();

  inline void needUpdated() {pv->needUpdated();}
  double getUpdated(int point=-1);
  double get(int point=-1);

  inline bool isConnected() {return pv->isConnected();}

private slots:

  void setConnected(bool con);
  void setHeader(const QString & text);
  inline void updateValue(const QVariant & data) { val->setText(data.toString()); }

signals:

  void connectionChanged(bool);
  void remove();

};

#endif // MAINWINDOW_H
