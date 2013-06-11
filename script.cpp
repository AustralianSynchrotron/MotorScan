#include "script.h"
#include "ui_script.h"
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include<QTimer>
#include <QList>



typedef QPair<QGridLayout*, int> GridColumnInfo;

class ColumnResizerPrivate {
public:
  ColumnResizerPrivate(ColumnResizer* q_ptr)
    : q(q_ptr)
    , m_updateTimer(new QTimer(q))
  {
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(0);
    QObject::connect(m_updateTimer, SIGNAL(timeout()), q, SLOT(updateWidth()));
  }

  void scheduleWidthUpdate()
  {
    m_updateTimer->start();
  }

  ColumnResizer* q;
  QTimer* m_updateTimer;
  QList<QWidget*> m_widgets;
  QList<GridColumnInfo> m_gridColumnInfoList;
};

ColumnResizer::ColumnResizer(QObject* parent)
  : QObject(parent)
  , d(new ColumnResizerPrivate(this))
{}

ColumnResizer::~ColumnResizer()
{
  delete d;
}

void ColumnResizer::addWidget(QWidget* widget) {
  d->m_widgets.append(widget);
  widget->installEventFilter(this);
  d->scheduleWidthUpdate();
}

void ColumnResizer::updateWidth() {
  int width = 0;
  foreach (QWidget* widget, d->m_widgets)
    width = qMax(widget->sizeHint().width(), width);
  foreach (GridColumnInfo info, d->m_gridColumnInfoList)
    info.first->setColumnMinimumWidth(info.second, width);
}

bool ColumnResizer::eventFilter(QObject*, QEvent* event) {
  if (event->type() == QEvent::Resize) {
    d->scheduleWidthUpdate();
  }
  return false;
}


void ColumnResizer::addWidgetsFromGridLayout(QGridLayout* layout, int column) {
  for (int row = 0; row < layout->rowCount(); ++row) {
    QLayoutItem* item = layout->itemAtPosition(row, column);
    if (!item) {
      continue;
    }
    QWidget* widget = item->widget();
    if (!widget) {
      continue;
    }
    addWidget(widget);
  }
  d->m_gridColumnInfoList << GridColumnInfo(layout, column);
}






Script::Script(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Script),
  fileExec(this)
{
  ui->setupUi(this);
  connect(ui->path, SIGNAL(textChanged(QString)), SLOT(evaluate()));
  connect(ui->browse, SIGNAL(clicked()), SLOT(browse()));
  connect(ui->execute, SIGNAL(clicked()), SLOT(onStartStop()));
  connect(ui->path, SIGNAL(editingFinished()), SIGNAL(editingFinished()));
  connect(&proc, SIGNAL(stateChanged(QProcess::ProcessState)),
          SLOT(onState(QProcess::ProcessState)));
  if ( ! fileExec.open() )
    qDebug() << "ERROR! Unable to open temporary file.";
}


Script::~Script() {
  delete ui;
}

void Script::setPath(const QString & _path) {
  ui->path->setText(_path);
}

const QString Script::path() const {
  return ui->path->text();
}

void Script::browse() {
  ui->path->setText(
        QFileDialog::getOpenFileName(0, "Command", QDir::currentPath()) );
}


void Script::evaluate() {

  if ( ! fileExec.isOpen() || isRunning() )
    return;
  ui->execute->setStyleSheet("");

  fileExec.resize(0);
  fileExec.write( ui->path->text().toAscii() );
  fileExec.flush();

  QProcess tempproc;
  tempproc.start("/bin/sh -n " + fileExec.fileName());
  tempproc.waitForFinished();
  ui->path->setStyleSheet( tempproc.exitCode() ? "color: rgb(255, 0, 0);" : "");

}


bool Script::start() {
  if ( ! fileExec.isOpen() || isRunning() )
    return false;
  if (ui->path->text().isEmpty())
    return true;
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

  ui->browse->setEnabled( state==QProcess::NotRunning );
  ui->path->setEnabled( state==QProcess::NotRunning );
  ui->execute->setText( state==QProcess::NotRunning ?
                         "Execute" : "Stop" );

  if (state==QProcess::NotRunning) {
    ui->execute->setStyleSheet( proc.exitCode() ? "color: rgb(255, 0, 0);" : "");
    emit finished(proc.exitCode());
  } else if (state==QProcess::Running) {
    emit started();
  }

};


void Script::addToColumnResizer(ColumnResizer * columnizer) {
  if ( ! columnizer )
    return;
  columnizer->addWidgetsFromGridLayout(ui->gridLayout, 1);
  columnizer->addWidgetsFromGridLayout(ui->gridLayout, 2);
}

