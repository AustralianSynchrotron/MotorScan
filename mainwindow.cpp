#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QPrinter>
#include <QDate>
#include <QPrintDialog>


QSettings * MainWindow::globalSettings = new QSettings("/etc/scanmx", QSettings::IniFormat);
const QString MainWindow::badStyle = "background-color: rgba(255, 0, 0, 64);";
const QString MainWindow::goodStyle = QString();

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)  {

  ui->setupUi(this);
  ui->tabwin->hide();
  connect(ui->addSignal, SIGNAL(clicked()), SLOT(addSignal()));
  connect(ui->addTrigger, SIGNAL(clicked()), SLOT(addTrigger()));
  connect(ui->triggAll, SIGNAL(clicked()), SLOT(trigAll()));
  connect(ui->startStop, SIGNAL(clicked()), SLOT(startStop()));
  connect(ui->browseSaveDir, SIGNAL(clicked()), SLOT(browseAutoSave()));
  connect(ui->printResult, SIGNAL(clicked()), SLOT(printResult()));
  connect(ui->saveResult, SIGNAL(clicked()), SLOT(saveResult()));
  connect(ui->qtiResults, SIGNAL(clicked()), SLOT(openQti()));
  connect(ui->scan2D, SIGNAL(toggled(bool)), SLOT(switchDimension(bool)));
  connect(ui->saveDir, SIGNAL(textChanged(QString)), SLOT(prepareAutoSave()));
  connect(ui->saveName, SIGNAL(textChanged(QString)), SLOT(prepareAutoSave()));
  connect(ui->autoName, SIGNAL(toggled(bool)), SLOT(prepareAutoSave()));
  connect(ui->tabwin, SIGNAL(clicked()), SLOT(changeWinmode()));

  connect(ui->xAxis, SIGNAL(statusChanged()), SLOT(checkReady()));
  connect(ui->yAxis, SIGNAL(statusChanged()), SLOT(checkReady()));
  connect(ui->xAxis, SIGNAL(limitReached()), SLOT(stopScan()));
  connect(ui->yAxis, SIGNAL(limitReached()), SLOT(stopScan()));

  // Check for qtiplot
  QProcess checkQti;
  checkQti.start("bash -c \"command -v qtiplot\"");
  checkQti.waitForFinished();
  qtiCommand = checkQti.readAll();
  qtiCommand.chop(1);
  ui->qtiResults->setVisible( ! qtiCommand.isEmpty() );

  // Restore global settings

  int size = globalSettings->beginReadArray("detectors");
  for (int i = 0; i < size; ++i) {
    globalSettings->setArrayIndex(i);
    QString
        sigPV = globalSettings->value("detector").toString(),
        trigPV = globalSettings->value("trigger").toString(),
        trigval = globalSettings->value("triggerValue").toString();

    knownDeTrig.insert(sigPV, qMakePair(trigPV, trigval));
    Signal::knownDetectors.append(sigPV);
  }
  globalSettings->endArray();


  // Restore local settings

  localSettings = new QSettings(QDir::homePath() + "/.scanmx",
                                QSettings::IniFormat);

  if ( localSettings->contains("xMotor") )
    ui->xAxis->motor->setPv(localSettings->value("xMotor").toString());
  if ( localSettings->contains("xMotorStart") ) {
    bool ok;
    double val = localSettings->value("xMotorStart").toDouble(&ok);
    if (ok) {
      ui->xAxis->ui->start->setValue(val);
      ui->xAxis->startEndCh();
    }
  }
  if ( localSettings->contains("xMotorEnd") ) {
    bool ok;
    double val = localSettings->value("xMotorEnd").toDouble(&ok);
    if (ok) {
      ui->xAxis->ui->end->setValue(val);
      ui->xAxis->startEndCh();
    }
  }
  if ( localSettings->contains("xMotorPoints") ) {
    bool ok;
    int val = localSettings->value("xMotorPoints").toInt(&ok);
    if (ok) {
      ui->xAxis->ui->points->setValue(val);
      ui->xAxis->pointsCh(val);
    }
  }
  if ( localSettings->contains("xMotorMode") )
      ui->xAxis->ui->mode->setCurrentIndex(
          ui->xAxis->ui->mode->findText(
              localSettings->value("xMotorMode").toString() ) );

  bool secDim=false;
  if ( localSettings->contains("2D") )
    secDim = localSettings->value("2D").toBool();
  ui->scan2D->setChecked(secDim);
  ui->yAxis->setVisible(secDim);
  ui->yAxis->setEnabled(secDim);

  if ( localSettings->contains("yMotor") )
    ui->yAxis->motor->setPv(localSettings->value("yMotor").toString());
  if ( localSettings->contains("yMotorStart") ) {
    bool ok;
    double val = localSettings->value("yMotorStart").toDouble(&ok);
    if (ok) {
      ui->yAxis->ui->start->setValue(val);
      ui->yAxis->startEndCh();
    }
  }
  if ( localSettings->contains("yMotorEnd") ) {
    bool ok;
    double val = localSettings->value("yMotorEnd").toDouble(&ok);
    if (ok) {
      ui->yAxis->ui->end->setValue(val);
      ui->yAxis->startEndCh();
    }
  }
  if ( localSettings->contains("yMotorPoints") ) {
    bool ok;
    int val = localSettings->value("yMotorPoints").toInt(&ok);
    if (ok) {
      ui->yAxis->ui->points->setValue(val);
      ui->yAxis->pointsCh(val);
    }
  }
  if ( localSettings->contains("yMotorMode") )
      ui->yAxis->ui->mode->setCurrentIndex(
          ui->yAxis->ui->mode->findText(
              localSettings->value("yMotorMode").toString() ) );

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

  size = localSettings->beginReadArray("triggers");
  for (int i = 0; i < size; ++i) {
    localSettings->setArrayIndex(i);
    addTrigger(localSettings->value("trigger").toString(),
               localSettings->value("value").toString());
  }
  localSettings->endArray();

  size = localSettings->beginReadArray("detectors");
  for (int i = 0; i < size; ++i) {
    localSettings->setArrayIndex(i);
    addSignal(localSettings->value("detector").toString());
  }
  localSettings->endArray();

  connect(ui->xAxis, SIGNAL(settingChanged()), SLOT(storeSettings()));
  connect(ui->xAxis->motor, SIGNAL(changedPv(QString)), SLOT(storeSettings()));
  connect(ui->yAxis, SIGNAL(settingChanged()), SLOT(storeSettings()));
  connect(ui->yAxis->motor, SIGNAL(changedPv(QString)), SLOT(storeSettings()));
  connect(ui->scan2D, SIGNAL(toggled(bool)), SLOT(storeSettings()));
  connect(ui->after, SIGNAL(activated(QString)), SLOT(storeSettings()));
  connect(ui->saveDir, SIGNAL(editingFinished()), SLOT(storeSettings()));
  connect(ui->saveName, SIGNAL(editingFinished()), SLOT(storeSettings()));
  connect(ui->autoName, SIGNAL(toggled(bool)), SLOT(storeSettings()));

}



void MainWindow::storeSettings() {

  localSettings->setValue("xMotor", ui->xAxis->motor->getPv());
  localSettings->setValue("xMotorStart", ui->xAxis->ui->start->value());
  localSettings->setValue("xMotorEnd", ui->xAxis->ui->end->value());
  localSettings->setValue("xMotorPoints", ui->xAxis->ui->points->value());
  localSettings->setValue("xMotorMode", ui->xAxis->ui->mode->currentText());
  localSettings->setValue("2D", ui->scan2D->isChecked());
  localSettings->setValue("yMotor", ui->yAxis->motor->getPv());
  localSettings->setValue("yMotorStart", ui->yAxis->ui->start->value());
  localSettings->setValue("yMotorEnd", ui->yAxis->ui->end->value());
  localSettings->setValue("yMotorPoints", ui->yAxis->ui->points->value());
  localSettings->setValue("yMotorMode", ui->yAxis->ui->mode->currentText());
  localSettings->setValue("afterScan", ui->after->currentText());
  localSettings->setValue("saveDir", ui->saveDir->text());
  localSettings->setValue("saveName", ui->saveName->text());
  localSettings->setValue("autoName", ui->autoName->isChecked());

  localSettings->beginWriteArray("triggers");
  for (int i = 0; i < triggersE.size(); ++i) {
    localSettings->setArrayIndex(i);
    localSettings->setValue("trigger", triggersE[i]->trig->text());
    localSettings->setValue("value", triggersE[i]->val->text());
  }
  localSettings->endArray();

  localSettings->beginWriteArray("detectors");
  for (int i = 0; i < signalsE.size(); ++i) {
    localSettings->setArrayIndex(i);
    localSettings->setValue("detector", signalsE[i]->sig->currentText());
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
  QMdiSubWindow  * active = ui->plots->activeSubWindow();
  if ( dialog.exec() )
    foreach(Signal* sig, signalsE)
      if (active == sig->plotWin)
        sig->graph->plot->print(printer);

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
  bool iAmReady = ui->xAxis->isReady() ;
  iAmReady = iAmReady && ( ! ui->scan2D->isChecked() || ui->yAxis->isReady() );
  foreach(Trigger* trig, triggersE)
    iAmReady = iAmReady && trig->pv->isConnected();
  foreach(Signal* sig, signalsE)
    iAmReady = iAmReady && sig->isConnected();
  iAmReady = iAmReady && signalsE.size();
  iAmReady = iAmReady
             && ( ui->saveDir->styleSheet() == goodStyle )
             && ( ui->saveName->styleSheet() == goodStyle );

  ui->startStop->setEnabled(iAmReady || nowScanning() );
}

void MainWindow::constructTriggersLayout(){
  ui->triggersList->setVisible(triggersE.size());
  foreach(Trigger * tr, triggersE){
    int position = triggersE.indexOf(tr) + 1;
    ui->triggersL->addWidget(tr->rem,   position, 0);
    ui->triggersL->addWidget(tr->trig,    position, 1);
    ui->triggersL->addWidget(tr->val,   position, 2);
  }
  storeSettings();
}

void MainWindow::addTrigger(const QString & pvName, const QString & value){
  Trigger * tr = new Trigger(this);
  tr->trig->setText(pvName);
  tr->val->setText(value);
  connect(tr, SIGNAL(remove()), this, SLOT(removeTrigger()));
  connect(tr->pv, SIGNAL(connectionChanged(bool)), SLOT(checkReady()));
  triggersE.append(tr);
  constructTriggersLayout();
}

void MainWindow::removeTrigger(){
  Trigger * tr = (Trigger *) sender();
  triggersE.removeOne(tr);
  delete tr;
  constructTriggersLayout();
}


bool MainWindow::triggersContains(const QString & trigPV, const QString & trigval){
  bool found = false;
  foreach (Trigger * trig, triggersE)
    found = found ||
            ( ( trig->trig->text() == trigPV ) &&
              ( trigval.isEmpty() || ( trig->val->text() == trigval ) ) );
  return found;
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

int MainWindow::column(Signal * sig) const {
  for (int icur = 0 ; icur < ui->dataTable->columnCount() ; icur++ )
    if ( ui->dataTable->horizontalHeaderItem(icur)  == sig->tableItem )
      return icur;
  return -1;
}


void MainWindow::addSignal(const QString & pvName){

  Signal * sg = new Signal(this);
  sg->sig->addItem(pvName);
  sg->sig->setCurrentIndex( sg->sig->findText(pvName) );

  connect(sg, SIGNAL(remove()), this, SLOT(removeSignal()));
  connect(sg, SIGNAL(connectionChanged(bool)), SLOT(checkReady()));
  connect(sg->sig, SIGNAL(currentIndexChanged(QString)), SLOT(updateSignal(QString)));

  ui->plots->addSubWindow(sg->plotWin) -> show();


  sg->tableItem = new QTableWidgetItem(pvName);
  signalsE.append(sg);
  int tablePos = ui->dataTable->columnCount();
  ui->dataTable->insertColumn(tablePos);
  ui->dataTable->setHorizontalHeaderItem(tablePos, sg->tableItem);

  constructSignalsLayout();

}

void MainWindow::updateSignal(QString pvName){
  if ( knownDeTrig.contains(pvName) &&
       ! knownDeTrig[pvName].first.isEmpty() &&
       ! triggersContains(knownDeTrig[pvName].first, knownDeTrig[pvName].second)
       )
    addTrigger(knownDeTrig[pvName].first, knownDeTrig[pvName].second);
}


void MainWindow::removeSignal(){
  Signal * sg = (Signal *) sender();
  ui->dataTable->removeColumn(column(sg));
  ui->plots->removeSubWindow(sg->plotWin);
  signalsE.removeOne(sg);
  delete sg;
  constructSignalsLayout();

}



void MainWindow::switchDimension(bool secondDim){

  if (secondDim) {

    // data table
    ui->dataTable->insertColumn(1);
    ui->dataTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Y Axis"));

  } else {

    // data table
    ui->dataTable->removeColumn(1);

    //plot

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
      ui->xAxis->motor->stop();
      ui->yAxis->motor->stop();
    }
}


void MainWindow::updateGUI() {
  QCoreApplication::processEvents();
  QCoreApplication::flush();
}

bool MainWindow::nowScanning() {
  return ! ui->setup->isEnabled();
}

void MainWindow::trigAll(){
  foreach(Trigger * trig, triggersE)
    trig->trigMe();
}

void MainWindow::openQti() {
  if (qtiCommand.isEmpty())
    return;

  QString dataN = tableWasSavedTo + "_qti.dat";
  QFile dataFile(dataN);
  dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly);
  QTextStream dataStr(&dataFile);

  dataStr
      << "X "
      << ( ui->scan2D->isChecked() ? "Y " : "" );
  foreach (Signal * sig, signalsE)
    dataStr
        << sig->pv->pv() << " ";
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



void MainWindow::changeWinmode(){
  if ( ui->plots->viewMode() == QMdiArea::SubWindowView ) {
    ui->plots->setViewMode(QMdiArea::TabbedView);
    ui->tabwin->setText("Window view");
  } else {
    ui->plots->setViewMode(QMdiArea::SubWindowView);
    ui->tabwin->setText("Tab view");
  }
}
















void MainWindow::startScan(){

  if (nowScanning())
    return;

  stopNow = false;

  ui->setup->setEnabled(false);
  ui->startStop->setText("Stop");

  // Data file
  tableWasSavedTo = prepareAutoSave();
  QFile dataFile(tableWasSavedTo);
  dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly);
  QTextStream dataStr(&dataFile);

  // buttons
  ui->saveResult->setEnabled(true);
  ui->printResult->setEnabled(true);
  ui->qtiResults->setEnabled(true);

  dataStr
      << "# ScanMX\n"
      << "#\n"
      << "# Date: " << QDate::currentDate().toString() << "\n"
      << "# Time: " << QTime::currentTime().toString() << "\n"
      << "#\n";


  //sizes
  const int xPoints = ui->xAxis->points();
  const int yPoints = ui->scan2D->isChecked() ?
                      ui->yAxis->points()  :  1 ;
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

  xAxisData.resize(xPoints);
  yAxisData.resize(yPoints);

  dataStr
      << "# X axis PV: \"" << ui->xAxis->motor->getPv() << "\"\n"
      << "# X axis Description: \"" << ui->xAxis->motor->getDescription() << "\"\n"
      << "#\n";
  if ( ui->scan2D->isChecked() )
    dataStr
        << "# Y axis PV: \"" << ui->yAxis->motor->getPv() << "\"\n"
        << "# Y axis Description: \"" << ui->yAxis->motor->getDescription() << "\"\n"
        << "#\n";


  // get initial positions
  const double xInitPos = ui->xAxis->motor->getUserPosition();
  const double yInitPos = ui->scan2D->isChecked() ?
                          ui->yAxis->motor->getUserPosition()  :  0 ;


  dataStr << "# Initial position of X axis: " << xInitPos << "\n";
  if ( ui->scan2D->isChecked() )
    dataStr << "# Initial position of Y axis: " << yInitPos << "\n";
  dataStr << "#\n";


  // actual starting positions
  const double xStart = ui->xAxis->start() +
                        ( ui->xAxis->mode() == Axis::REL  ?
                        xInitPos  :  0 ) ;
  const double yStart = ui->yAxis->start() +
                        ( ui->yAxis->mode() == Axis::REL  ?
                        yInitPos  :  0 ) ;
  const double xEnd = ui->xAxis->end() +
                        ( ui->xAxis->mode() == Axis::REL ?
                        xInitPos  :  0 ) ;
  const double yEnd = ui->yAxis->end() +
                        ( ui->yAxis->mode() == Axis::REL ?
                        yInitPos  :  0 ) ;

  dataStr << "# X axis scan range: " << xStart << " ... " << xEnd << "\n";
  if ( ui->scan2D->isChecked() )
    dataStr << "# Y axis scan range: " << yStart << " ... " << yEnd << "\n";
  dataStr << "#\n";

  // resize data storage(s) and set initial values
  foreach (Signal * sig, signalsE) {
    if ( ui->scan2D->isChecked() )
      sig->graph->changePlot( xPoints, xStart, xEnd,
                       yPoints, yStart, yEnd );
    else
      sig->graph->changePlot( xPoints, xStart, xEnd );
  }
  updateGUI();

  dataStr
      << "# Signals:\n"
      << "#\n";
  foreach (Signal * sig, signalsE)
    dataStr
        << "# PV: \"" << sig->pv->pv() << "\"\n";
  dataStr << "#\n";

  if ( triggersE.size() ) {
    dataStr
        << "# Triggers:\n"
        << "#\n";
    foreach (Trigger * trig, triggersE)
      dataStr
          << "# PV: \"" << trig->pv->pv() << "\"\n";
    dataStr << "#\n";
  }


  // set axis positions
  for( int xpoint = 0 ; xpoint < xPoints ; xpoint++ )
    xAxisData[xpoint] = xStart + ( xpoint * ( xEnd - xStart ) ) / (xPoints - 1);
  for( int ypoint = 0 ; ypoint < yPoints ; ypoint++ )
    yAxisData[ypoint] = ui->scan2D->isChecked() ?
                        yStart + ( ypoint * ( yEnd - yStart ) ) / (yPoints - 1) :
                        0;

  // reset progress
  ui->dataTable->setRowCount(0);
  ui->progressBar->setMaximum(totalPoints);

  dataStr
      << "# Data columns:\n"
      << "# "
      << "%Point "
      << "%X "
      << ( ui->scan2D->isChecked() ? "%Y " : "" );
  foreach (Signal * sig, signalsE)
    dataStr
        << "%" << sig->pv->pv() << " ";
  dataStr << "\n";



  ////////////////
  // doing scan //
  ////////////////
  int curpoint = 0;
  for( int ypoint = 0 ; ypoint < yPoints ; ypoint++ ) {

    if ( ui->scan2D->isChecked() )
      ui->yAxis->motor->goUserPosition( yAxisData[ypoint] , true );
    if ( ui->yAxis->motor->getLoLimitStatus() ||
         ui->yAxis->motor->getHiLimitStatus() )
      dataStr <<  "# Y Axis: limit hit.\n";
    double yPos = ui->yAxis->motor->getUserPosition();

    updateGUI();
    if ( stopNow )
      break;

    for( int xpoint = 0 ; xpoint < xPoints ; xpoint++ ) {

      ui->dataTable->insertRow(curpoint);
      ui->dataTable->setVerticalHeaderItem(curpoint,
                                           new QTableWidgetItem(QString::number(curpoint+1)));
      ui->dataTable->scrollToBottom();

      ui->xAxis->motor->goUserPosition( xAxisData[xpoint] , true );
      double xPos = ui->xAxis->motor->getUserPosition();
      if ( ui->xAxis->motor->getLoLimitStatus() ||
           ui->xAxis->motor->getHiLimitStatus() )
        dataStr <<  "# X Axis: limit hit.\n";

      updateGUI();
      if ( stopNow )
        break;

      ui->dataTable->setItem(curpoint, 0,
                             new QTableWidgetItem(QString::number(xPos)));
      if ( ui->scan2D->isChecked() )
        ui->dataTable->setItem(curpoint, 1,
                               new QTableWidgetItem(QString::number(yPos)));

      dataStr
          << curpoint+1 << " "
          << QString::number(xPos, 'e') << " "
          << ( ui->scan2D->isChecked() ? QString::number(yPos, 'e') + " " : "" );

      foreach(Signal * sig, signalsE)
        sig->needUpdated();
      trigAll();
      updateGUI();
      foreach(Signal * sig, signalsE) {
        double value = sig->getUpdated(curpoint);
        ui->dataTable->setItem(curpoint, column(sig) ,
                               new QTableWidgetItem(QString::number(value)));
        dataStr << QString::number(value, 'e') << " ";
        sig->graph->plot->replot();
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
  if ( ui->after->currentText() == "Start position" ) {
    ui->xAxis->motor->goUserPosition(xStart, true);
    if ( ui->scan2D->isChecked() )
      ui->yAxis->motor->goUserPosition(yStart);
  } else if ( ui->after->currentText() == "Prior position" ) {
    ui->xAxis->motor->goUserPosition(xInitPos, true);
    if ( ui->scan2D->isChecked() )
      ui->yAxis->motor->goUserPosition(yInitPos);
  }


  // finishing
  ui->startStop->setText("Start");
  ui->setup->setEnabled(true);
}

















CloseFilter * MainWindow::Signal::closeFilt = new CloseFilter;
QStringList MainWindow::Signal::knownDetectors = QStringList();

MainWindow::Signal::Signal(QWidget* parent) :

    rem(new QPushButton("-", parent)),
    sig(new QComboBox(parent)),
    val(new QLabel(parent)),
    tableItem(new QTableWidgetItem()),
    plotWin(new QMdiSubWindow(parent)),

    pv(new QEpicsPv(this)),

    graph(new Graph)

{

  setConnected(false);

  sig->setEditable(true);
  sig->setDuplicatesEnabled(false);
  sig->addItems(knownDetectors);
  sig->clearEditText();
  sig->setToolTip("PV of the signal.");

  rem->setToolTip("Remove the signal.");
  val->setToolTip("Current value.");

  connect(sig, SIGNAL(editTextChanged(QString)), pv, SLOT(setPV(QString)));
  connect(sig, SIGNAL(editTextChanged(QString)), SLOT(setHeader(QString)));
  connect(pv, SIGNAL(connectionChanged(bool)), SLOT(setConnected(bool)));
  connect(pv, SIGNAL(valueUpdated(QVariant)), SLOT(updateValue(QVariant)));
  connect(rem, SIGNAL(clicked()), SIGNAL(remove()));

  plotWin->installEventFilter(closeFilt);
  plotWin->setWidget(graph);
  plotWin->showMaximized();
  QIcon icon;
  icon.addFile(":/new/prefix1/Graph1-small.png", QSize(), QIcon::Normal, QIcon::Off);
  plotWin->setWindowIcon(icon);

}


MainWindow::Signal::~Signal() {
  delete pv;
  delete rem;
  delete sig;
  delete val;
  delete plotWin;
};


double MainWindow::Signal::getUpdated(int point) {
  return graph->newData(point, pv->getUpdated().toDouble());
}

double MainWindow::Signal::get(int point) {
  return graph->newData(point, pv->get().toDouble());
}

void MainWindow::Signal::setConnected(bool con) {
  rem->setStyleSheet( con ? "" :
                      "background-color: rgba(255, 0, 0,64);");
  val->setText( con ? "" : "disconnected");
  emit connectionChanged(con);
}

void MainWindow::Signal::setHeader(const QString & text) {
  tableItem->setText(text);
  plotWin->setWindowTitle(text);
  graph->plot->setTitle(text);
}


