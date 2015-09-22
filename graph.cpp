
#include <QPrinter>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_raster_data.h>
#include <qwt_symbol.h>
#include <qwt_interval.h>
#include <qwt_plot_renderer.h>
#include <qwt_color_map.h>
#include <qwt_picker_machine.h>

#include <qwt_matrix_raster_data.h>

#include "graph.h"
#include "ui_graph.h"
#include "error.h"


QwtText MyPicker::trackerTextF(const QPointF &pos) const {
  latestPos = pos;
  emit gimmeValue(pos);
  QColor bg(Qt::white);
  bg.setAlpha(200);
  QString label = QString("<html><head/><body><p>%1, %2").arg(pos.x()).arg(pos.y());
  if (!isnan(val))
    label += QString(" : <span style=\" font-weight:600;\">%3</span></p></body></html>").arg(val);
  QwtText text(label);
  text.setBackgroundBrush( QBrush( bg ));
  return text;

}

MyPicker::MyPicker(QwtPlotCanvas *canvas):
  QwtPlotPicker(canvas),
  val(0),
  latestPos(0,0)
{
  setTrackerMode(AlwaysOn);
  //installEventFilter(canvas);
}


bool MyPicker::eventFilter(QObject * object, QEvent *event) {
  QMouseEvent * mevent = dynamic_cast<QMouseEvent *>(event);
  if ( object == canvas() &&
       mevent &&
       mevent->type() == QEvent::MouseButtonPress &&
       mevent->button() == Qt::RightButton ) {
    emit rightClicked(latestPos, val);
  }
  return QwtPlotPicker::eventFilter(object, event);
}



class PlotData {
protected:

  double _min;
  double _max;
  double * _data;
  double _size;

  PlotData() : _min(NAN), _max(NAN), _data(0), _size(0) {}

public:

  double min() const {return _min;}
  double max() const {return _max;}
  double * data() const {return _data;}
  size_t size() const {return _size;}

  QwtInterval interval() const {
    QwtInterval nint(_min,_max);
    if ( isnan(_min) )
      nint.setMinValue(0);
    if ( isnan(_max) )
      nint.setMaxValue(0);
    if ( nint.width() == 0.0 ) {
      if (nint.minValue()==0.0)
        nint.setInterval(-1,1);
      else
        nint.setInterval(nint.minValue()*0.9, nint.minValue()*1.9);
    }
    return nint;
  }


  virtual void updateData() {
    _min = NAN;
    _max = NAN;
    for (int idx=0 ; idx<_size ; idx++) {
      double point = *(_data+idx);
      if ( ! isnan(point) ) {
        if ( isnan(_min) || point < _min) _min = point;
        if ( isnan(_max) || point > _max) _max = point;
      }
    }
  }


  virtual bool updateData(double point) {
    if ( isnan(point) )
      return false;
    bool newRange = false;
    if ( isnan(_min) || point < _min) {
      newRange = true;
      _min = point;
    }
    if ( isnan(_max) || point > _max) {
      newRange = true;
      _max = point;
    }
    return newRange;
  }

};






class PlotLine : public QwtPlotCurve, public PlotData {
private:
  QwtPointArrayData * arrayData;

public :

  QwtPlotGrid * grid;

  PlotLine(const QVector<double> & _yData, double xStart, double xEnd) :
    QwtPlotCurve(),
    PlotData(),
    grid(new QwtPlotGrid)
  {

    setPen(QPen(QColor(255,0,0)));
    setStyle(QwtPlotCurve::Lines);
    QwtSymbol * symbol = new QwtSymbol(QwtSymbol::Ellipse);
    symbol->setSize(8);
    setSymbol(symbol);
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setPaintAttribute(QwtPlotCurve::CacheSymbols);

    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMajPen(Qt::DashLine);
    grid->setMinPen(Qt::DotLine);

    _size = _yData.size();
    QVector<double> xData(_size);
    for (int icur=0 ; icur < _size ; icur++)
      xData[icur] = xStart + icur*(xEnd-xStart)/(_size-1);
    arrayData = new QwtPointArrayData( xData, _yData);
    setData(arrayData);
    _data = const_cast<double *>( arrayData->yData().data() );
    updateData();

  }


  ~PlotLine() {
    grid->detach();
    delete grid;
  //  delete arrayData;
  }

  void updateData() {
    PlotData::updateData();
    QRectF bnd = arrayData->boundingRect();
    bnd.setBottom(min());
    bnd.setTop(max());
    arrayData->setRectOfInterest(bnd);
  }

  bool updateData(double point) {
    bool ret = PlotData::updateData(point);
    int idx=0;
    while ( idx < _size && ! isnan(*( _data+idx)) )
      idx++;
    ret |= ( (int) dataSize() != idx );
    if (ret) {
      QRectF bnd = arrayData->boundingRect();
      bnd.setBottom(min());
      bnd.setTop(max());
      arrayData->setRectOfInterest(bnd);
    }
    return ret;
  }

  double value(double pos) {
    if (_size<1)
      return NAN;
    const double xStart = arrayData->xData().front();
    const double xEnd   = arrayData->xData().back();
    if ( _size < 1  ||  xStart == xEnd  ||  pos < qMin(xStart, xEnd)  ||  pos > qMax(xStart, xEnd) )
      return NAN;
    int idx = (_size-1) * ( pos - xStart ) / (xEnd - xStart);
    return arrayData->yData().at(idx);
  }

};







class MapRasterDataOrig : public QwtRasterData {

public:

  int width;
  int height;
  const double * zData;

  MapRasterDataOrig(const double * _zData, int _width, int _height,
                double xStart, double xEnd,
                double yStart, double yEnd) :
    width(_width),
    height(_height),
    zData(_zData)

  {

    if ( ! zData || ! width*height )
      throw_error("No or zero-sized data", "PlotMap");

    if (xStart == xEnd) {
      xStart = 1;
      xEnd = width;
    }
    setInterval(Qt::XAxis, QwtInterval(xStart,xEnd));

    if (yStart == yEnd) {
      yStart = 1;
      yEnd = height;
    }
    setInterval(Qt::YAxis, QwtInterval(yStart,yEnd));

  }


  double value (double x, double y) const {

    const QwtInterval & xInt = interval(Qt::XAxis);
    const QwtInterval & yInt = interval(Qt::YAxis);

    double val;
    if ( ! xInt.normalized().contains(x) || ! yInt.normalized().contains(y) )
      val = NAN;
    else {
      double xwidth = xInt.maxValue() - xInt.minValue();
      double ywidth = yInt.maxValue() - yInt.minValue();
      val = * ( zData
               + (int) ( (width-1) * ( x - xInt.minValue() ) / xwidth )
               + width * (int) ( (height-1) * ( y - yInt.minValue() ) / ywidth ));
    }
    return val;

  }

  QRectF pixelHint( const QRectF & ) const {
    const QwtInterval & xInt = interval(Qt::XAxis).normalized();
    const QwtInterval & yInt = interval(Qt::YAxis).normalized();
    return QRectF(xInt.minValue(), yInt.minValue(),
                  xInt.width()/width,
                  yInt.width()/height);
  }


};


























class PlotMap : public QwtPlotSpectrogram, public PlotData {
private:
  QwtMatrixRasterData * arrayData;

public :

  PlotMap(const QVector<double> & _zData, int _width,
          double xStart, double xEnd,
          double yStart, double yEnd) :
    QwtPlotSpectrogram(),
    PlotData(),
    arrayData(new QwtMatrixRasterData)
  {
    setRenderThreadCount(0); // use system specific thread count
    setColorMap(new QwtLinearColorMap);

    arrayData->setInterval(Qt::XAxis, QwtInterval(xStart, xEnd));
    arrayData->setInterval(Qt::YAxis, QwtInterval(yStart, yEnd));
    arrayData->setValueMatrix(_zData, _width);
    setData(arrayData);
    updateData();
    _data = const_cast<double *>( arrayData->valueMatrix().data() );
    _size = _zData.size();
  }

  ~PlotMap() {
//    delete arrayData;
  }

  void setPlotInterval(QwtInterval & interval) {
    arrayData->setInterval(Qt::ZAxis, interval);
  }

  void updateData() {
    PlotData::updateData();
  }

  double value(const QPointF & pos) {
    return arrayData->value(pos.x(),pos.y());
  }

};









class LogColorMap : public QwtLinearColorMap {

public:

  LogColorMap() {}

  unsigned char colorIndex (const QwtInterval &interval, double value) const {

    QwtInterval ival = interval;
    if (interval.maxValue() <= 0)
      ival.setInterval(0,0);
    else if (ival.minValue()<=0)
      ival.setMinValue(ival.maxValue()/1e-10);
    else
      ival.setInterval( log10(ival.minValue()), log10(ival.maxValue()) );

    return QwtLinearColorMap::colorIndex(ival, value <= 0.0 ? NAN : log10(value) );

  }

  QRgb rgb(const QwtInterval &interval, double value ) const {

    QwtInterval ival = interval;
    if (interval.maxValue() <= 0)
      ival.setInterval(0,0);
    else if (ival.minValue()<=0)
      ival.setMinValue(ival.maxValue()/1e-10);
    else
      ival.setInterval( log10(ival.minValue()), log10(ival.maxValue()) );

    return QwtLinearColorMap::rgb(ival, value <= 0.0 ? NAN : log10(value) );

  }

};







Graph::Graph(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Graph),
  pdata(0)
{

  ui->setupUi(this);
  ui->plot->setAutoReplot(false);

  MyPicker * zoomer = new MyPicker(ui->plot->canvas());
  connect(zoomer, SIGNAL(gimmeValue(QPointF)), SLOT(pick(QPointF)));
  connect(zoomer, SIGNAL(rightClicked(QPointF, double)), SIGNAL(rightClicked(QPointF, double)));
  ui->plot->axisWidget(QwtPlot::yRight)->setColorBarEnabled(true);

  connect(ui->autoMin, SIGNAL(toggled(bool)), SLOT(updateRange()));
  connect(ui->autoMax, SIGNAL(toggled(bool)), SLOT(updateRange()));
  connect(ui->min, SIGNAL(editingFinished()), SLOT(updateRange()));
  connect(ui->max, SIGNAL(editingFinished()), SLOT(updateRange()));
  connect(ui->showGrid, SIGNAL(toggled(bool)), SLOT(showGrid()));
  connect(ui->logY, SIGNAL(toggled(bool)), SLOT(setLogarithmic()));

}

Graph::~Graph(){
  delete ui;
}

void Graph::changePlot() {
  if( dynamic_cast<PlotLine*>(pdata) ) {
    dynamic_cast<PlotLine*>(pdata)->detach();
    delete dynamic_cast<PlotLine*>(pdata);
  } else if (dynamic_cast<PlotMap*>(pdata)) {
    dynamic_cast<PlotMap*>(pdata)->detach();
    delete dynamic_cast<PlotMap*>(pdata);
  }
}

double * Graph::changePlot(const QVector<double> & yData, double xStart, double xEnd) {
  changePlot();
  pdata = new PlotLine(yData, xStart, xEnd);
  ui->plot->enableAxis(QwtPlot::yRight, false);
  ui->plot->setAxisScale(QwtPlot::xBottom, xStart, xEnd);
  dynamic_cast<PlotLine*>(pdata)->attach(ui->plot);
  updateData();
  return pdata->data();
}



double * Graph::changePlot(const QVector<double> & zData, int width,
                       double xStart, double xEnd,
                       double yStart, double yEnd)  {
  if ( ! zData.size() || width <= 0 || zData.size()%width )
    throw_error("Bad data for map plot", "Graph");
  changePlot();
  pdata = new PlotMap(zData, width, xStart, xEnd, yStart, yEnd);
  ui->plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
  ui->plot->setAxisScale(QwtPlot::yLeft, yStart, yEnd );
  ui->plot->setAxisScale(QwtPlot::xBottom, xStart, xEnd );
  ui->plot->enableAxis(QwtPlot::yRight, true);
  dynamic_cast<PlotMap*>(pdata)->attach(ui->plot);
  updateData();
  return pdata->data();
}


void Graph::updateData() {
  if (!pdata)
    return;
  pdata->updateData();
  updateRange();
  ui->plot->replot();
}

void Graph::updateData(double point) {
  if (!pdata)
    return;
  if ( pdata->updateData(point) &&
       (ui->autoMin->isChecked() || ui->autoMax->isChecked()) )
    updateRange();
  else
    ui->plot->replot();
}


void  Graph::updateRange() {

  QwtInterval plotInterval = pdata->interval();

  if ( ui->autoMin->isChecked() )
    ui->min->setValue(plotInterval.minValue());
  else
    plotInterval.setMinValue(ui->min->value());

  if ( ui->autoMax->isChecked() )
    ui->max->setValue(plotInterval.maxValue());
  else
    plotInterval.setMaxValue(ui->max->value());

  if( dynamic_cast<PlotLine*>(pdata) ) {
    ui->plot->setAxisScale(QwtPlot::yLeft, plotInterval.minValue(), plotInterval.maxValue());
  } else if (dynamic_cast<PlotMap*>(pdata)) {
    dynamic_cast<PlotMap*>(pdata)->setPlotInterval(plotInterval);
    ui->plot->axisWidget(QwtPlot::yRight)
        ->setColorMap( plotInterval, ui->logY->isChecked() ? new LogColorMap : new QwtLinearColorMap);
    ui->plot->setAxisScale(QwtPlot::yRight, plotInterval.minValue(), plotInterval.maxValue());
  }

  ui->plot->replot();

  if (dynamic_cast<PlotMap*>(pdata) && ui->showGrid->isChecked())
    showGrid();

}

void Graph::showGrid() {
  if( dynamic_cast<PlotLine*>(pdata) ) {
    if (ui->showGrid->isChecked())
      dynamic_cast<PlotLine*>(pdata)->grid->attach(ui->plot);
    else
      dynamic_cast<PlotLine*>(pdata)->grid->detach();
  } else if (dynamic_cast<PlotMap*>(pdata)) {
    const QList<double> & contourLevels = ui->plot->axisScaleDiv(QwtPlot::yRight)
        ->ticks(QwtScaleDiv::MajorTick);
    dynamic_cast<PlotMap*>(pdata)->setContourLevels(contourLevels);
    dynamic_cast<PlotMap*>(pdata)->setDisplayMode(QwtPlotSpectrogram::ContourMode,
                                                  ui->showGrid->isChecked());
  }
  ui->plot->replot();
}


void Graph::setLogarithmic() {
  if( dynamic_cast<PlotLine*>(pdata) ) {
    if (ui->logY->isChecked())
      ui->plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine);
    else
      ui->plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
  } else if (dynamic_cast<PlotMap*>(pdata)) {
    QwtColorMap * cmap = ui->logY->isChecked() ? new LogColorMap : new QwtLinearColorMap;
    dynamic_cast<PlotMap*>(pdata)->setColorMap(cmap);
    if (ui->logY->isChecked())
      ui->plot->setAxisScaleEngine(QwtPlot::yRight, new QwtLog10ScaleEngine);
    else
      ui->plot->setAxisScaleEngine(QwtPlot::yRight, new QwtLinearScaleEngine);
  }
  updateRange();
  ui->plot->replot();
}


void Graph::print(QPrinter & printer) {
  QwtPlotRenderer renderer;
  if ( printer.colorMode() == QPrinter::GrayScale ) {
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardCanvasBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);
  }
  renderer.renderTo(ui->plot, printer);
}

void Graph::pick(const QPointF & point) {

  if ( ! dynamic_cast<MyPicker*>( sender() ) )
    return;

  double value=NAN;
  if (dynamic_cast<PlotMap*>(pdata))
    value = dynamic_cast<PlotMap*>(pdata)->value(point);
  else if (dynamic_cast<PlotLine*>(pdata))
    value = dynamic_cast<PlotLine*>(pdata)->value(point.x());

  dynamic_cast<MyPicker*>(sender())->setValue(value);

}

void Graph::setTitle(const QString & text) {
  ui->plot->setTitle(text);
}
