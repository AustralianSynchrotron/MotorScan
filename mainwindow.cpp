#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QPrinter>
#include <QDate>
#include <QPrintDialog>
#include<QMenu>
#include<QCursor>
#include <QAction>



QSettings * MainWindow::globalSettings = new QSettings("/etc/scanmx", QSettings::IniFormat);
const QString MainWindow::badStyle = "background-color: rgba(255, 0, 0, 64);";
const QString MainWindow::goodStyle = QString();

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  gotoTarget(0,0),
  nowLoading(true)
{

  ui->setupUi(this);
  connect(ui->addSignal, SIGNAL(clicked()), SLOT(addSignal()));
  connect(ui->startStop, SIGNAL(clicked()), SLOT(startStop()));
  connect(ui->browseSaveDir, SIGNAL(clicked()), SLOT(browseAutoSave()));
  connect(ui->printResult, SIGNAL(clicked()), SLOT(printResult()));
  connect(ui->saveResult, SIGNAL(clicked()), SLOT(saveResult()));
  connect(ui->qtiResults, SIGNAL(clicked()), SLOT(openQti()));
  connect(ui->scan2D, SIGNAL(toggled(bool)), SLOT(switchDimension(bool)));
  connect(ui->saveDir, SIGNAL(textChanged(QString)), SLOT(prepareAutoSave()));
  connect(ui->saveName, SIGNAL(textChanged(QString)), SLOT(prepareAutoSave()));
  connect(ui->autoName, SIGNAL(toggled(bool)), SLOT(prepareAutoSave()));
  connect(ui->addX, SIGNAL(clicked()), SLOT(addX()));
  connect(ui->delX, SIGNAL(clicked()), SLOT(delX()));
  connect(ui->addY, SIGNAL(clicked()), SLOT(addY()));
  connect(ui->delY, SIGNAL(clicked()), SLOT(delY()));



  connect(ui->xAxis, SIGNAL(statusChanged()), SLOT(checkReady()));
  connect(ui->yAxis, SIGNAL(statusChanged()), SLOT(checkReady()));
  connect(ui->xAxis, SIGNAL(limitReached()), SLOT(stopScan()));
  connect(ui->yAxis, SIGNAL(limitReached()), SLOT(stopScan()));

  xAxes << ui->xAxis;
  yAxes << ui->yAxis;

  // goto
  gotoMenu = new QMenu(this);
  QAction * action = new QAction(this);
  gotoMenu->addAction(action);
  connect( action, SIGNAL(triggered()), SLOT(catchGoTo()));

  // Check for qtiplot
  QProcess checkQti;
  checkQti.start("bash -c \"command -v qtiplot\"");
  checkQti.waitForFinished();
  qtiCommand = checkQti.readAll();
  qtiCommand.chop(1);
  ui->qtiResults->setVisible( ! qtiCommand.isEmpty() );

  // Restore global settings
  Signal::knownDetectors.clear();
  QFile detFile("/etc/listOfSignals.txt");
  if ( detFile.open(QIODevice::ReadOnly | QIODevice::Text) &&
       detFile.isReadable() )
    while ( !detFile.atEnd() ) {
      QString ln = detFile.readLine();
      if ( ln.at(ln.length()-1) == '\n')
        ln.chop(1);
      Signal::knownDetectors << ln;
    }


  // Restore local settings
  localSettings = new QSettings(QDir::homePath() + "/.scanmx",
                                QSettings::IniFormat);
  loadSettings();


  connect(ui->xAxis, SIGNAL(settingChanged()), SLOT(updatePlots()));
  connect(ui->xAxis, SIGNAL(settingChanged()), SLOT(storeSettings()));
  connect(ui->xAxis->motor->motor(), SIGNAL(changedPv(QString)), SLOT(storeSettings()));
  connect(ui->yAxis, SIGNAL(settingChanged()), SLOT(updatePlots()));
  connect(ui->yAxis, SIGNAL(settingChanged()), SLOT(storeSettings()));
  connect(ui->yAxis->motor->motor(), SIGNAL(changedPv(QString)), SLOT(storeSettings()));
  connect(ui->scan2D, SIGNAL(toggled(bool)), SLOT(updatePlots()));
  connect(ui->scan2D, SIGNAL(toggled(bool)), SLOT(storeSettings()));
  connect(ui->after, SIGNAL(activated(QString)), SLOT(storeSettings()));
  connect(ui->saveDir, SIGNAL(editingFinished()), SLOT(storeSettings()));
  connect(ui->saveName, SIGNAL(editingFinished()), SLOT(storeSettings()));
  connect(ui->autoName, SIGNAL(toggled(bool)), SLOT(storeSettings()));

  nowLoading = false;

}


void MainWindow::updatePlots() {

  const int xPoints = ui->xAxis->points();
  xAxisData.resize(xPoints);

  double xStart = ui->xAxis->start();
  double xEnd = ui->xAxis->end();
  if (ui->xAxis->mode() == Axis::REL) {
    const double xInitPos = ui->xAxis->motor->motor()->getUserPosition();
    xStart += xInitPos;
    xEnd += xInitPos;
  }

  for( int xpoint = 0 ; xpoint < xPoints ; xpoint++ )
    xAxisData(xpoint) = xStart + ( xpoint * ( xEnd - xStart ) ) / (xPoints - 1);


  if ( ! ui->scan2D->isChecked() ) { // 2D
    yAxisData.resize(1);
    yAxisData=0;
    foreach (Signal * sig, signalsE)
      sig->setData(&xAxisData);
  } else { // 3D

    const int yPoints = ui->yAxis->points();
    yAxisData.resize(yPoints);

    double yStart = ui->yAxis->start();
    double yEnd = ui->yAxis->end();
    if (ui->yAxis->mode() == Axis::REL) {
      const double yInitPos = ui->yAxis->motor->motor()->getUserPosition();
      yStart += yInitPos;
      yEnd += yInitPos;
    }

    for( int ypoint = 0 ; ypoint < yPoints ; ypoint++ )
      yAxisData(ypoint) = yStart + ( ypoint * ( yEnd - yStart ) ) / (yPoints - 1);

    foreach (Signal * sig, signalsE)
      sig->setData(xPoints, yPoints, xStart, xEnd, yStart, yEnd);

  }


  columns.clear();
  ui->dataTable->setColumnCount(0);
  ui->dataTable->setRowCount(0);

  foreach (Axis * ax, xAxes + ( ui->scan2D->isChecked() ? yAxes : QList<Axis*>() ) ) {
    QTableWidgetItem * tableItem = new QTableWidgetItem(ax->motor->motor()->getPv());
    int tablePos = ui->dataTable->columnCount();
    columns[ax] = tablePos;
    ui->dataTable->insertColumn(tablePos);
    ui->dataTable->setHorizontalHeaderItem(tablePos, tableItem);
  }
  foreach (Signal * sg, signalsE) {
    QTableWidgetItem * tableItem = new QTableWidgetItem(sg->objectName());
    int tablePos = ui->dataTable->columnCount();
    columns[sg] = tablePos;
    ui->dataTable->insertColumn(tablePos);
    ui->dataTable->setHorizontalHeaderItem(tablePos, tableItem);
  }
  updateHeaders();

  updateGUI();

}


void MainWindow::updateHeaders() {
  QApplication::processEvents();
  foreach(QObject * obj, columns.keys())
    ui->dataTable->horizontalHeaderItem(columns[obj])->setText(obj->objectName());
}


void MainWindow::storeSettings() {

  if (nowLoading)
    return;

  localSettings->clear();

  localSettings->beginWriteArray("xmotors");
  for (int i=0; i< xAxes.size(); i++) {
    localSettings->setArrayIndex(i);
    Axis * ax = xAxes[i];
    localSettings->setValue("pv", ax->motor->motor()->getPv());
    localSettings->setValue("start", ax->start());
    localSettings->setValue("end", ax->end());
    if (!i)
      localSettings->setValue("points", ax->points());
    localSettings->setValue("mode", ax->modeString());
  }
  localSettings->endArray();

  localSettings->setValue("2D", ui->scan2D->isChecked());

  localSettings->beginWriteArray("ymotors");
  for (int i=0; i< yAxes.size(); i++) {
    localSettings->setArrayIndex(i);
    Axis * ax = yAxes[i];
    localSettings->setValue("pv", ax->motor->motor()->getPv());
    localSettings->setValue("start", ax->start());
    localSettings->setValue("end", ax->end());
    if (!i)
      localSettings->setValue("points", ax->points());
    localSettings->setValue("mode", ax->modeString());
  }
  localSettings->endArray();

  localSettings->setValue("afterScan", ui->after->currentText());
  localSettings->setValue("saveDir", ui->saveDir->text());
  localSettings->setValue("saveName", ui->saveName->text());
  localSettings->setValue("autoName", ui->autoName->isChecked());

  localSettings->beginWriteArray("detectors");
  for (int i = 0; i < signalsE.size(); ++i) {
    localSettings->setArrayIndex(i);
    localSettings->setValue("detector", signalsE[i]->sig->currentText());
  }
  localSettings->endArray();

}


void MainWindow::loadSettings() {

  int size;

  for (int i=0; i<xAxes.size()-1; i++)
    delX();
  size = localSettings->beginReadArray("xmotors");
  for (int i=0; i < size; i++) {

    localSettings->setArrayIndex(i);
    if (i) // first motor is already there
      addX();
    Axis * ax = xAxes.last();

    if ( localSettings->contains("pv") )
      ax->motor->motor()->setPv(localSettings->value("pv").toString());
    if ( localSettings->contains("start") ) {
      bool ok;
      double val = localSettings->value("start").toDouble(&ok);
      if (ok)
        ax->setStart(val);
    }
    if ( localSettings->contains("end") ) {
      bool ok;
      double val = localSettings->value("end").toDouble(&ok);
      if (ok)
        ax->setEnd(val);
    }
    if ( ! i && // this is the first motor
         localSettings->contains("points") ) {
      bool ok;
      int val = localSettings->value("points").toInt(&ok);
      if (ok)
        ax->setPoints(val);
    }
    if ( localSettings->contains("mode") )
      ax->setMode( localSettings->value("mode").toString() ) ;

  }
  localSettings->endArray();

  bool secDim=false;
  if ( localSettings->contains("2D") )
    secDim = localSettings->value("2D").toBool();
  ui->scan2D->setChecked(secDim);
  ui->ySet->setVisible(secDim);
  ui->ySet->setEnabled(secDim);

  for (int i=0; i<yAxes.size()-1; i++)
    delX();
  size = localSettings->beginReadArray("ymotors");
  for (int i=0; i < size; i++) {

    localSettings->setArrayIndex(i);
    if (i) // first motor is already there
      addY();
    Axis * ax = yAxes.last();

    if ( localSettings->contains("pv") )
      ax->motor->motor()->setPv(localSettings->value("pv").toString());
    if ( localSettings->contains("start") ) {
      bool ok;
      double val = localSettings->value("start").toDouble(&ok);
      if (ok)
        ax->setStart(val);
    }
    if ( localSettings->contains("end") ) {
      bool ok;
      double val = localSettings->value("end").toDouble(&ok);
      if (ok)
        ax->setEnd(val);
    }
    if ( ! i && // this is the first motor
         localSettings->contains("points") ) {
      bool ok;
      int val = localSettings->value("points").toInt(&ok);
      if (ok)
        ax->setPoints(val);
    }
    if ( localSettings->contains("mode") )
      ax->setMode( localSettings->value("mode").toString() ) ;

  }
  localSettings->endArray();

  if ( localSettings->contains("afterScan") )
      ui->after->setCurrentIndex(
          ui->after->findText(
              localSettings->value("afterScan").toString() ) );

  if ( localSettings->contains("saveDir") )
    ui->saveDir->setText(localSettings->value("saveDir").toString());
  else
    ui->saveDir->setText(QDir::homePath());
  if ( localSettings->contains("saveName") )
    ui->saveName->setText(localSettings->value("saveName").toString());
  if ( localSettings->contains("autoName") )
    ui->autoName->setChecked( localSettings->value("autoName").toBool() );

  updatePlots();

  size = localSettings->beginReadArray("detectors");
  for (int i = 0; i < size; ++i) {
    localSettings->setArrayIndex(i);
    addSignal(localSettings->value("detector").toString());
  }
  localSettings->endArray();

}


void MainWindow::browseAutoSave(){
  QString new_dir = QFileDialog::getExistingDirectory(this, "Open directory", ui->saveDir->text());
  if ( ! new_dir.isEmpty() )
    ui->saveDir->setText(new_dir);
}

QString MainWindow::prepareAutoSave() {

  QString dn = ui->saveDir->text();
  if ( ! dn.endsWith('/') )
    dn += "/";
  QFileInfo dirInfo(dn);
  bool dirOK = dirInfo.exists() && dirInfo.isWritable();
  ui->saveDir->setStyleSheet( dirOK  ?  goodStyle  :  badStyle );

  QString tryName;

  ui->saveName->setEnabled( ! ui->autoName->isChecked() );
  if ( ui->autoName->isChecked() ) {

    const QString fn = "scan_" +
                       QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    const QString ext = ".dat";

    int count = 1;
    tryName = fn + ext;
    while ( QFile::exists( dn + tryName ) )
      tryName = fn + "_(" + QString::number(count++) + ")" + ext;
    ui->saveName->setText(tryName);

  }


  tryName = ui->saveName->text();
  QFileInfo fileInfo( dn + ui->saveName->text() );
  bool fileOK = ! fileInfo.exists() ||
                ( ! fileInfo.isDir() && fileInfo.isWritable() );
  ui->saveName->setStyleSheet( fileOK  ?  goodStyle  :  badStyle );

  checkReady();

  return  ( dirOK && fileOK )  ?  dn + tryName  :  "" ;

}

void MainWindow::printResult(){
  QPrinter printer;
  QPrintDialog dialog(&printer);
  QMdiSubWindow  * active =ui->plots->subWindowList(QMdiArea::ActivationHistoryOrder).last();
  if ( dialog.exec() )
    foreach(Signal* sig, signalsE)
      if (active == sig->plotWin)
        sig->print(printer);

}


void MainWindow::saveResult(){
  if (tableWasSavedTo.isEmpty())
    return;
  QString dataName =
      QFileDialog::getSaveFileName(this, "Save data", ui->saveDir->text() ) ;
  if (QFile::exists(dataName))
    QFile::remove(dataName);
  QFile::copy(tableWasSavedTo, dataName);
}


void MainWindow::checkReady(){
  bool iAmReady = true;
  foreach(Axis * ax, xAxes)
    iAmReady &= ax->isReady() ;
  if ( ui->scan2D->isChecked() )
    foreach(Axis * ax, yAxes)
      iAmReady &= ax->isReady() ;
  iAmReady &= (bool) signalsE.size();
  iAmReady &=
      ( ui->saveDir->styleSheet() == goodStyle ) &&
      ( ui->saveName->styleSheet() == goodStyle );
  ui->startStop->setEnabled(iAmReady || nowScanning() );
}

void MainWindow::constructSignalsLayout(){
  foreach(Signal * sg, signalsE){
    int position=signalsE.indexOf(sg) + 1;
    ui->signalsL->addWidget(sg->rem,   position, 0);
    ui->signalsL->addWidget(sg->sig,    position, 1);
    ui->signalsL->addWidget(sg->val,   position, 2);
  }
  ui->addSignal->setStyleSheet( signalsE.size() ? goodStyle : badStyle );
  storeSettings();
}


void MainWindow::addSignal(const QString & pvName){

  Signal * sg = new Signal(this);
  sg->sig->addItem(pvName);
  sg->sig->setCurrentIndex( sg->sig->findText(pvName) );
  signalsE.append(sg);

  connect(sg->rem, SIGNAL(clicked()), this, SLOT(removeSignal()));
  connect(sg->sig, SIGNAL(editTextChanged(QString)), SLOT(storeSettings()));
  connect(sg, SIGNAL(nameChanged(QString)), SLOT(updateHeaders()));
  connect(sg, SIGNAL(rightClicked(QPointF)), SLOT(reactSignalRightClick(QPointF)));

  if ( ! ui->scan2D->isChecked() ) { // 2D
    sg->setData(&xAxisData);
  } else { // 3D

    double xStart = ui->xAxis->start();
    double xEnd = ui->xAxis->end();
    if (ui->xAxis->mode() == Axis::REL) {
      const double xInitPos = ui->xAxis->motor->motor()->getUserPosition();
      xStart += xInitPos;
      xEnd += xInitPos;
    }

    double yStart = ui->yAxis->start();
    double yEnd = ui->yAxis->end();
    if (ui->yAxis->mode() == Axis::REL) {
      const double yInitPos = ui->yAxis->motor->motor()->getUserPosition();
      yStart += yInitPos;
      yEnd += yInitPos;
    }

    sg->setData(xAxisData.size(), yAxisData.size(), xStart, xEnd, yStart, yEnd);

  }

  ui->plots->addSubWindow(sg->plotWin)->showMaximized();

  constructSignalsLayout();
  updatePlots();

}



void MainWindow::reactSignalRightClick(const QPointF &point) {

  if (nowScanning())
    return;

  gotoTarget = point;

  QString actionText = "Move motor here: ";
  if (xAxes.isEmpty())
    actionText += "none";
  else if (xAxes[0]->motor->motor()->isConnected() )
    actionText += QString::number(gotoTarget.x());
  else
    actionText += "no link";
  if ( ui->scan2D->isChecked() ) {
    actionText += ", ";
    if (yAxes.isEmpty())
      actionText += "none";
    else if (yAxes[0]->motor->motor()->isConnected() )
      actionText += QString::number(gotoTarget.y());
    else
      actionText += "no link";

  }
  gotoMenu->actions().at(0)->setText(actionText);
  gotoMenu->exec( QCursor::pos() );
}

void MainWindow::catchGoTo() {
  if ( ! xAxes.isEmpty() && xAxes[0]->motor->motor()->isConnected() )
    xAxes[0]->motor->motor()->goUserPosition(gotoTarget.x());
  if (  ui->scan2D->isChecked() &&
        ! yAxes.isEmpty() && yAxes[0]->motor->motor()->isConnected() )
    yAxes[0]->motor->motor()->goUserPosition(gotoTarget.y());
}





void MainWindow::removeSignal(){

  Signal * sg=0;
  foreach(Signal * sig, signalsE)
    if (sig->rem == sender())
      sg = sig;
  if (!sg)
    return;

  signalsE.removeOne(sg);
  storeSettings();
  delete sg;
  constructSignalsLayout();
  updatePlots();

}



void MainWindow::addX() {

  Axis * xax = new Axis(this);
  xAxes << xax;
  ui->setupLayout->insertWidget(ui->setupLayout->indexOf(ui->xButtons), xax);
  ui->delX->setEnabled(xAxes.size() > 1);
  xax->setPoints(ui->xAxis->points());
  xax->setPointsEnabled(false);

  connect(xax, SIGNAL(statusChanged()), SLOT(checkReady()));
  connect(xax, SIGNAL(limitReached()), SLOT(stopScan()));
  connect(xax, SIGNAL(settingChanged()), SLOT(storeSettings()));
  connect(xax->motor->motor(), SIGNAL(changedPv(QString)), SLOT(storeSettings()));
  connect(xax->motor->motor(), SIGNAL(changedPv(QString)), SLOT(updateHeaders()));
  connect(ui->xAxis, SIGNAL(pointsChanged(int)), xax, SLOT(setPoints(int)));

  updatePlots();

}

void MainWindow::delX() {
  if (xAxes.size() < 2)
    return;
  delete xAxes.takeLast();
  ui->delX->setEnabled(xAxes.size() > 1);
  updatePlots();
}

void MainWindow::addY() {

  Axis * yax = new Axis(this);
  yAxes << yax;
  ui->yLay->insertWidget(ui->yLay->indexOf(ui->yButtons), yax);
  ui->delY->setEnabled(yAxes.size() > 1);
  yax->setPoints(ui->yAxis->points());
  yax->setPointsEnabled(false);

  connect(yax, SIGNAL(statusChanged()), SLOT(checkReady()));
  connect(yax, SIGNAL(limitReached()), SLOT(stopScan()));
  connect(yax, SIGNAL(settingChanged()), SLOT(storeSettings()));
  connect(yax->motor->motor(), SIGNAL(changedPv(QString)), SLOT(storeSettings()));
  connect(yax->motor->motor(), SIGNAL(changedPv(QString)), SLOT(updateHeaders()));
  connect(ui->yAxis, SIGNAL(pointsChanged(int)), yax, SLOT(setPoints(int)));

  updatePlots();

}

void MainWindow::delY() {
  if (yAxes.size() < 2)
    return;
  delete yAxes.takeLast();
  ui->delY->setEnabled(yAxes.size() > 1);
  updatePlots();
}








void MainWindow::switchDimension(bool secondDim){

  if (secondDim) {
    ui->dataTable->insertColumn(1);
    ui->dataTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Y Axis"));
  } else {
    ui->dataTable->removeColumn(1);
  }

  checkReady();

}


void MainWindow::startStop(){
  if (nowScanning())
    stopScan();
  else
    startScan();
}


void MainWindow::stopScan(){
    stopNow=true;
    if (nowScanning()) {
      foreach (Axis * ax, xAxes)
        ax->motor->motor()->stop();
      foreach (Axis * ax, yAxes)
        ax->motor->motor()->stop();
    }
}


void MainWindow::updateGUI() {
  QCoreApplication::processEvents();
  QCoreApplication::flush();
}

bool MainWindow::nowScanning() {
  return ! ui->setup->isEnabled();
}


void MainWindow::openQti() {
  if (qtiCommand.isEmpty())
    return;

  QString dataN = tableWasSavedTo + "_qti.dat";
  QFile dataFile(dataN);
  dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly);
  QTextStream dataStr(&dataFile);

  for (int i=0; i< xAxes.size(); i++)
    dataStr << "X" << i << " ";
  for (int i=0; i< yAxes.size(); i++)
    dataStr << "Y" << i << " ";
  foreach (Signal * sig, signalsE)
    dataStr
        << sig->objectName() << " ";
  dataStr << "\n";

  for ( int y = 0 ; y < ui->dataTable->rowCount(); y++ ) {
    for ( int x = 0 ; x < ui->dataTable->columnCount() ; x++ )
      dataStr << ( ui->dataTable->item(y,x) ?
                   ui->dataTable->item(y,x)->text() : "NaN") << " ";
    dataStr << "\n";
  }
  dataFile.close();

  QProcess::startDetached("qtiplot " + dataN);
}




void describeAndPrepareAxis(Axis * ax, QTextStream & dataStr,
                            QHash<Axis*, double> & initPos,
                            QHash<Axis*, QPair<double,double> > & range) {

  dataStr << "# PV: \"" << ax->motor->motor()->getPv() << "\"\n"
          << "# Description: \"" << ax->motor->motor()->getDescription() << "\"\n";

  const double init = ax->motor->motor()->getUserPosition();
  initPos.insert(ax,init);
  dataStr << "# Initial position: " << init << "\n";

  double start = ax->start();
  double end = ax->end();
  if (ax->mode() == Axis::REL) {
    start += init;
    end += init;
  }
  range.insert(ax, qMakePair(start, end));
  dataStr << "# Scan range: " << start << " ... " << end << "\n"
          << "#\n";

}










void MainWindow::startScan(){

  if (nowScanning())
    return;

  stopNow = false;

  updatePlots();

  ui->setup->setEnabled(false);
  ui->startStop->setText("Stop");

  // Data file
  tableWasSavedTo = prepareAutoSave();
  QFile dataFile(tableWasSavedTo);
  dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly);
  QTextStream dataStr(&dataFile);

  // buttons
  ui->saveResult->setEnabled(true);
  ui->qtiResults->setEnabled(true);

  dataStr
      << "# ScanMX\n"
      << "#\n"
      << "# Date: " << QDate::currentDate().toString() << "\n"
      << "# Time: " << QTime::currentTime().toString() << "\n"
      << "#\n";

  //sizes
  const int xPoints = xAxisData.size();
  const int yPoints = yAxisData.size();
  const int totalPoints = xPoints * yPoints;

  dataStr
      << "# " << (ui->scan2D->isChecked() ? "2" : "1") << "D scan\n"
      << "# Number of data points: " << totalPoints << "\n";
  if ( ui->scan2D->isChecked() )
    dataStr
        << "# Number of X axis points: " << xPoints << "\n"
        << "# Number of Y axis points: " << yPoints << "\n"
        << "#\n";
  dataStr << "#\n";

  QHash<Axis*,double> initPos;
  QHash<Axis*, QPair<double,double> > range;


  dataStr << "# Number of X motors:" << xAxes.size() << "\n";
  foreach (Axis * ax, xAxes) {
    if (xAxes.size() > 1)
      dataStr << "# X axis, motor " << xAxes.indexOf(ax) << "\n";
    else
      dataStr << "# X axis\n";
    describeAndPrepareAxis(ax, dataStr, initPos, range);
  }

  if ( ui->scan2D->isChecked() ) {
    foreach (Axis * ax, yAxes) {
      if (yAxes.size() > 1)
        dataStr << "# Y axis, motor " << yAxes.indexOf(ax) << "\n";
      else
        dataStr << "# Y axis\n";
      describeAndPrepareAxis(ax, dataStr, initPos, range);
    }
  }

  dataStr << "#\n"
          << "#\n"
          << "# Signals:\n"
          << "#\n";
  foreach (Signal * sig, signalsE)
    dataStr
        << "# PV / script: \"" << sig->objectName() << "\"\n";
  dataStr << "#\n";

  // reset progress
  ui->progressBar->setMaximum(totalPoints);

  dataStr
      << "# Data columns:\n"
      << "# "
      << "%Point "
      << "%X "
      << ( ui->scan2D->isChecked() ? "%Y " : "" );
  foreach (Signal * sig, signalsE)
    dataStr
        << "%" << sig->objectName() << " ";
  dataStr << "\n";



  ////////////////
  // doing scan //
  ////////////////
  int curpoint = 0;
  for( int ypoint = 0 ; ypoint < yPoints ; ypoint++ ) {

    QHash<Axis*,double> yPos;
    if ( ui->scan2D->isChecked() ) {

      yPos.clear();
      foreach(Axis * ax, yAxes)
        ax->motor->motor()->goUserPosition
            ( range[ax].first +
              ( ypoint * ( range[ax].second - range[ax].first ) ) / (yPoints - 1),
              QCaMotor::STARTED );
      foreach(Axis * ax, yAxes) {
        ax->motor->motor()->wait_stop();
        if ( ax->motor->motor()->getLoLimitStatus() ||
             ax->motor->motor()->getHiLimitStatus() )
          dataStr <<  "# Y Axis: " + ax->motor->motor()->getPv() + " limit hit.\n";
        yPos[ax] = ax->motor->motor()->getUserPosition();
      }

      if (ui->relaxY->value() != 0.0 ) {
        qtWait(ui->startStop, SIGNAL(clicked()), ui->relaxY->value() * 1000 ); // relaxation must finish on start/stop clicked
        QApplication::processEvents(); // if start/stop was clicked the stopNow must be updated in here
      }

      updateGUI();
      if ( stopNow )
        break;

    }

    QHash<Axis*,double> xPos;
    for( int xpoint = 0 ; xpoint < xPoints ; xpoint++ ) {

      ui->dataTable->insertRow(curpoint);
      ui->dataTable->setVerticalHeaderItem(curpoint,
                                           new QTableWidgetItem(QString::number(curpoint+1)));
      ui->dataTable->scrollToBottom();

      xPos.clear();
      foreach(Axis * ax, xAxes)
        ax->motor->motor()->goUserPosition
            ( range[ax].first +
              ( xpoint * ( range[ax].second - range[ax].first ) ) / (xPoints - 1),
              QCaMotor::STARTED );
      foreach(Axis * ax, xAxes) {
        ax->motor->motor()->wait_stop();
        if ( ax->motor->motor()->getLoLimitStatus() ||
             ax->motor->motor()->getHiLimitStatus() )
          dataStr <<  "# X Axis: " + ax->motor->motor()->getPv() + " limit hit.\n";
        xPos[ax] = ax->motor->motor()->getUserPosition();
      }

      /*
      if (ui->relax->value() != 0.0 ) {
        qtWait(ui->startStop, SIGNAL(clicked()), ui->relax->value() * 1000 ); // relaxation must finish on start/stop clicked
        QApplication::processEvents(); // if start/stop was clicked the stopNow must be updated in here
      }
      */

      if ( ! ui->scan2D->isChecked() )
        xAxisData(xpoint) = xPos[ui->xAxis];

      updateGUI();
      if ( stopNow )
        break;

      dataStr << curpoint+1 << " ";
      foreach(Axis * ax, xAxes) {
        ui->dataTable->setItem(curpoint, columns[ax],
                               new QTableWidgetItem(QString::number(xPos[ax])));
        dataStr << QString::number(xPos[ax], 'e') << " ";
      }
      if ( ui->scan2D->isChecked() )
        foreach(Axis * ax, yAxes) {
          ui->dataTable->setItem(curpoint, columns[ax],
                                 new QTableWidgetItem(QString::number(yPos[ax])));
          dataStr << QString::number(yPos[ax], 'e') << " ";
        }

      updateGUI();
      foreach(Signal * sig, signalsE)
        sig->beforeGet();
      foreach(Signal * sig, signalsE) {
        QString strval = sig->get(curpoint).toString();
        ui->dataTable->setItem(curpoint, columns[sig],
                               new QTableWidgetItem(strval));
        dataStr << strval << " ";
      }
      dataStr <<  "\n";

      ui->progressBar->setValue(++curpoint);
      updateGUI();

      if ( stopNow )
        break;

    }

    if ( stopNow )
      break;

  }


  dataStr << (stopNow ? "# Stopped unfinished" : "# All done") << ".\n";
  dataFile.close();

  // after scan positioning
  foreach (Axis * ax, xAxes + ( ui->scan2D->isChecked() ? yAxes : QList<Axis*>() ) )
    if ( ui->after->currentText() == "Start position" )
      ax->motor->motor()->goUserPosition(range[ax].first, QCaMotor::STARTED);
    else if ( ui->after->currentText() == "Prior position" )
      ax->motor->motor()->goUserPosition(initPos[ax], QCaMotor::STARTED);

  foreach (Axis * ax, xAxes + ( ui->scan2D->isChecked() ? yAxes : QList<Axis*>() ) )
    ax->motor->motor()->wait_stop();

  // finishing
  ui->startStop->setText("Start");
  ui->setup->setEnabled(true);
}

















CloseFilter * MainWindow::Signal::closeFilt = new CloseFilter;
QStringList MainWindow::Signal::knownDetectors = QStringList();

MainWindow::Signal::Signal(QWidget* parent) :
  rem(new QPushButton("-", parent)),
  sig(new QComboBox(parent)),
  val(new QPushButton(parent)),
  plotWin(new QMdiSubWindow(parent)),
  scr(new Script(this)),
  pv(new QEpicsPv(this)),
  graph(new Graph)
{

  sig->setEditable(true);
  sig->setDuplicatesEnabled(false);
  sig->addItems(knownDetectors);
  sig->clearEditText();
  sig->setToolTip("PV / script");

  rem->setToolTip("Remove the signal.");
  val->setToolTip("Current value.");
  val->setFlat(true);

  connect(sig, SIGNAL(editTextChanged(QString)), SLOT(setText(QString)));
  connect(scr, SIGNAL(outChanged(QString)), SLOT(updateValue()));
  connect(val, SIGNAL(clicked()), scr, SLOT(execute()));
  connect(pv, SIGNAL(valueUpdated(QVariant)), SLOT(updateValue()));
  connect(graph, SIGNAL(rightClicked(QPointF)), SIGNAL(rightClicked(QPointF)));

  plotWin->installEventFilter(closeFilt);
  plotWin->setWidget(graph);
  QIcon icon;
  icon.addFile(":/new/prefix1/Graph1-small.png", QSize(), QIcon::Normal, QIcon::Off);
  plotWin->setWindowIcon(icon);

}


MainWindow::Signal::~Signal() {
  delete pv;
  delete scr;
  delete rem;
  delete sig;
  delete val;
  delete graph;
  delete plotWin;
};

void MainWindow::Signal::beforeGet() {
  if (pv->isConnected())
    pv->needUpdated();
}


QVariant MainWindow::Signal::get(int pos) {

  QVariant val=QVariant();

  if (pv->isConnected()) {

    if ( ! pv->isConnected() )
      return val;

    val = pv->getUpdated();
    if ( ! val.isValid() )
      val = pv->get();

  } else {

    if ( ! scr->start() )
      return val;
    scr->waitStop();

    val = scr->out();

  }

  if ( pos >= 0 && pos < data.size() ) {
    double rval = val.toDouble();
    data(pos) = rval;
    graph->updateData(rval);
  }

  return val;

}

void MainWindow::Signal::setData(Line * xData) {
  point = 0;
  data.resize(xData->size());
  data=NAN;
  graph->changePlot(xData->data(), data.data(), xData->size());
}

void MainWindow::Signal::setData(int width, int height,
                    double xStart, double xEnd,
                    double yStart, double yEnd) {
  point = 0;
  data.resize(width*height);
  data=NAN;
  graph->changePlot(data.data(), width, height, xStart, xEnd, yStart, yEnd);
}


void MainWindow::Signal::setText(const QString & text) {

  setObjectName(text);
  emit nameChanged(text);

  pv->setPV(text);
  scr->setPath(text);

  plotWin->setWindowTitle(text);
  graph->setTitle(text);

}

void MainWindow::Signal::updateValue() {
  if (pv->isConnected())
    val->setText(pv->get().toString());
  else
    val->setText(scr->out());

}




