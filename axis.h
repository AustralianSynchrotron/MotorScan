#ifndef AXIS_H
#define AXIS_H

#include <QWidget>
#include <ui_axis.h>
#include <qcamotorgui.h>

class Axis : public QWidget {
  Q_OBJECT;

private:

  static const QString badStyle;
  static const QString goodStyle;

  bool positionAcceptable(double pos);
  bool positionsAcceptable();

public:
  explicit Axis(QWidget *parent = 0);

  Ui::axis * ui;
  QCaMotorGUI * motor;

  enum Mode { ABS=1, REL=0 };

  inline int points() { return ui->points->value(); }
  inline double start() { return ui->start->value(); }
  inline double end() { return ui->end->value(); }
  inline Mode mode() { return (Mode) ui->mode->currentIndex(); }

  bool isReady();

signals:

  void settingChanged();
  void statusChanged();
  void limitReached();

public slots:
  void startEndCh();
  void pointsCh(int val);

private slots:

  void setConnected(bool con);
  void widthCh(double val);
  void stepCh(double val);
  void updateLimits();

};

#endif // AXIS_H
