
#include <QPrinter>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_raster_data.h>
#include <qwt_symbol.h>
#include <qwt_interval.h>
#include <qwt_plot_renderer.h>
#include <qwt_color_map.h>

#include "graph.h"
#include "ui_graph.h"
#include "error.h"




class PlotData {
protected:

  double _min;
  double _max;

  PlotData() : _min(NAN), _max(NAN) {}

  void updateData(const double * data, int size) {
    _min = NAN;
    _max = NAN;
    for (int idx=0 ; idx<size ; idx++) {
      double point = *(data+idx);
      if ( ! isnan(point) ) {
        if ( isnan(_min) || point < _min) _min = point;
        if ( isnan(_max) || point > _max) _max = point;
      }
    }
  }

public:

  double min() const {return _min;}
  double max() const {return _max;}

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

  virtual void updateData() = 0;

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
  int size;
  const double * xData;
  const double * yData;

public :

  QwtPlotGrid * grid;

  PlotLine(const double * _xData, const double * _yData, int _size) :
    QwtPlotCurve(),
    PlotData(),
    size(_size),
    xData(_xData),
    yData(_yData)
  {

    if ( ! _xData || ! _yData || ! _size )
      throw_error("Zero sized or nonequal XY-data", "Graph");

    setPen(QPen(QColor(255,0,0)));
    setStyle(QwtPlotCurve::Lines);
    QwtSymbol * symbol = new QwtSymbol(QwtSymbol::Ellipse);
    symbol->setSize(8);
    setSymbol(symbol);
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setPaintAttribute(QwtPlotCurve::CacheSymbols);

    updateData();

    grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMajPen(Qt::DashLine);
    grid->setMinPen(Qt::DotLine);

  }


  ~PlotLine() {
    grid->detach();
    delete grid;
  }

  void updateData() {
    PlotData::updateData(yData, size);
    int idx=0;
    while ( idx < size && ! isnan(*(yData+idx)) )
      idx++;
    setRawSamples(xData, yData, idx);
  }

  bool updateData(double point) {
    bool ret = PlotData::updateData(point);
    int idx=0;
    while ( idx < size && ! isnan(*(yData+idx)) )
      idx++;
    ret |= ( (int) dataSize() != idx );
    if (ret)
      setRawSamples(xData, yData, idx);
    return ret;
  }

};







class MapRasterData : public QwtRasterData {

public:

  int width;
  int height;
  const double * zData;

  MapRasterData(const double * _zData, int _width, int _height,
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
  MapRasterData * rastData;
public :

  PlotMap(const double * _zData, int _width, int _height,
          double xStart, double xEnd,
          double yStart, double yEnd) :
    QwtPlotSpectrogram(),
    PlotData(),
    rastData(new MapRasterData(_zData, _width, _height,
                               xStart, xEnd, yStart, yEnd) )
  {
    setRenderThreadCount(0); // use system specific thread count
    setColorMap(new QwtLinearColorMap);
    setData(rastData);
    updateData();
  }

  void setPlotInterval(QwtInterval & interval) {
    rastData->setInterval(Qt::ZAxis, interval);
  }

  void updateData() {
    PlotData::updateData(rastData->zData, rastData->width * rastData->height);
  }

  double value(const QPointF & pos) {
    return rastData->value(pos.x(),pos.y());
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

  MyZoomer * zoomer = new MyZoomer(ui->plot->canvas());
  connect(zoomer, SIGNAL(gimmeValue(QPointF)), SLOT(pick(QPointF)));
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

void Graph::changePlot(const double * xData, const double * yData, int size) {
  if ( ! xData || ! yData )
    throw_error("Bad data for curve plot", "Graph");
  changePlot();
  PlotLine * curve = new PlotLine(xData, yData, size);
  pdata = curve;
  ui->plot->enableAxis(QwtPlot::yRight, false);
  ui->plot->setAxisScale(QwtPlot::xBottom, *xData, *(xData+size-1) );
  curve->attach(ui->plot);
  updateData();
}



void Graph::changePlot(const double * zData, int width, int height,
                       double xStart, double xEnd,
                       double yStart, double yEnd)  {
  if ( ! zData || width <= 0 || height <= 0 )
    throw_error("Bad data for map plot", "Graph");
  changePlot();
  PlotMap * map = new PlotMap(zData, width, height, xStart, xEnd, yStart, yEnd);
  pdata = map;
  ui->plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
  ui->plot->setAxisScale(QwtPlot::yLeft, yStart, yEnd );
  ui->plot->setAxisScale(QwtPlot::xBottom, xStart, xEnd );
  ui->plot->enableAxis(QwtPlot::yRight, true);
  map->attach(ui->plot);
  updateData();
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

  if ( ! dynamic_cast<MyZoomer*>( sender() ) )
    return;

  double value = NAN;
  if (dynamic_cast<PlotMap*>(pdata))
    value = dynamic_cast<PlotMap*>(pdata)->value(point);

  dynamic_cast<MyZoomer*>(sender())->setValue(value);

}

void Graph::setTitle(const QString & text) {
  ui->plot->setTitle(text);
}
