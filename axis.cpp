#include "axis.h"
#include <QPushButton>

const QString Axis::badStyle = "background-color: rgba(255, 0, 0, 64);";
const QString Axis::goodStyle = QString();


Axis::Axis(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::axis),
    motor(new QCaMotorGUI)
{
  ui->setupUi(this);
  ui->setupPlace->insertWidget(0, motor->setupButton());

  connect(motor->motor(), SIGNAL(changedConnected(bool)), SLOT(setConnected(bool)));
  connect(motor->motor(), SIGNAL(changedMoving(bool)), SIGNAL(statusChanged()));
  connect(motor->motor(), SIGNAL(changedUserLoLimit(double)),  ui->start, SLOT(setMin(double)));
  connect(motor->motor(), SIGNAL(changedUserHiLimit(double)),  ui->start, SLOT(setMax(double)));
  connect(motor->motor(), SIGNAL(changedUserLoLimit(double)),  ui->end, SLOT(setMin(double)));
  connect(motor->motor(), SIGNAL(changedUserHiLimit(double)),  ui->end, SLOT(setMax(double)));
  connect(motor->motor(), SIGNAL(changedPrecision(int)),       ui->end, SLOT(setPrec(int)));
  connect(motor->motor(), SIGNAL(changedPrecision(int)),       ui->step, SLOT(setPrec(int)));
  connect(motor->motor(), SIGNAL(changedPrecision(int)),       ui->width, SLOT(setPrec(int)));
  connect(motor->motor(), SIGNAL(changedPrecision(int)),       ui->start, SLOT(setPrec(int)));
  connect(motor->motor(), SIGNAL(changedLoLimitStatus(bool)), SLOT(updateLimits()));
  connect(motor->motor(), SIGNAL(changedHiLimitStatus(bool)), SLOT(updateLimits()));
  connect(motor, SIGNAL(ioPositionChanged(QString)),
          ui->val, SLOT(setText(QString)));

  connect(ui->start, SIGNAL(valueEdited(double)), SLOT(startEndCh()));
  connect(ui->end, SIGNAL(valueEdited(double)), SLOT(startEndCh()));
  connect(ui->width, SIGNAL(valueEdited(double)), SLOT(widthCh(double)));
  connect(ui->step, SIGNAL(valueEdited(double)), SLOT(stepCh(double)));
  connect(ui->points, SIGNAL(valueEdited(int)), SLOT(pointsCh(int)));

  connect(ui->mode, SIGNAL(activated(QString)), this, SIGNAL(settingChanged()));

  setConnected(false);
  positionsAcceptable();

}


void Axis::setConnected(bool con) {
  motor->setupButton()->setStyleSheet( con ?  goodStyle : badStyle );
  if ( ! con )
    ui->val->setText("disconnected");
  positionsAcceptable();
  emit statusChanged();
}

void Axis::updateLimits(){
  if ( motor->motor()->getLoLimitStatus() || motor->motor()->getHiLimitStatus() ) {
    ui->val->setStyleSheet("background-color: rgb(128, 0, 0); color: rgb(255, 255, 255);");
    emit limitReached();
  } else {
    ui->val->setStyleSheet("");
  }
}

inline bool Axis::positionAcceptable(double pos) {
  return
      motor->motor()->isConnected() &&
      pos >= motor->motor()->getUserLoLimit() &&
      pos <= motor->motor()->getUserHiLimit();
}


bool Axis::positionsAcceptable() {
  bool
      stAcc = positionAcceptable(ui->start->value()),
      enAcc = positionAcceptable(ui->end->value()),
      wdAcc = ui->start->value() != ui->end->value();
  ui->start->setStyleSheet( (stAcc && wdAcc) ? goodStyle : badStyle ) ;
  ui->end->setStyleSheet( (enAcc && wdAcc) ? goodStyle : badStyle ) ;
  ui->width->setStyleSheet( wdAcc  ?  goodStyle : badStyle);
  return stAcc && enAcc && wdAcc;
}



void Axis::startEndCh(){
  double width = ui->end->value() - ui->start->value();
  ui->width->setValue( width );
  ui->step->setValue( width / (ui->points->value() - 1) );
  positionsAcceptable();
  emit statusChanged();
  emit settingChanged();
}


void Axis::widthCh(double val){
  ui->end->setValue( ui->start->value() + val );
  ui->step->setValue( val / (ui->points->value() - 1) );
  positionsAcceptable();
  if ( val < 0.0 )
    ui->step->setRange( val, 0 );
  else
    ui->step->setRange( 0, val );
  emit statusChanged();
  emit settingChanged();
}


void Axis::stepCh(double val){
  if (val != 0.0) {
    ui->points->setValue( 1 + ( ui->width->value() / val ) );
    ui->step->setStyleSheet(goodStyle);
  } else
    ui->step->setStyleSheet(badStyle);
  emit statusChanged();
  emit settingChanged();
}


void Axis::pointsCh(int val){
  ui->step->setValue( ( ui->end->value() - ui->start->value() ) / (val-1) );
  emit statusChanged();
  emit settingChanged();
}


bool Axis::isReady() {
  return motor->motor()->isConnected()
      && ! motor->motor()->isMoving()
      && positionsAcceptable();
}
