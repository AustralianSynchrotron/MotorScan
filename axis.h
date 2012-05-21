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

  Ui::axis * ui;

public:
  explicit Axis(QWidget *parent = 0);
  ~Axis();

  QCaMotorGUI * motor;

  enum Mode { ABS=1, REL=0 };

  inline int points() { return ui->points->value(); }
  inline double start() { return ui->start->value(); }
  inline double end() { return ui->end->value(); }
  inline Mode mode() { return (Mode) ui->mode->currentIndex(); }
  inline QString modeString() { return ui->mode->currentText(); }

  bool isReady();

signals:

  void settingChanged();
  void statusChanged();
  void limitReached();
  void pointsChanged(int);

public slots:
  void setPoints(int val);
  void setStart(double val);
  void setEnd(double val);
  void setMode(const QString & mod);
  void setPointsEnabled(bool enab);


private slots:

  void startEndCh();
  void pointsCh(int val);
  void setConnected(bool con);
  void widthCh(double val);
  void stepCh(double val);
  void updateLimits();
  inline void setName() {setObjectName(motor->motor()->getPv());}

};

#endif // AXIS_H
