#include<graph.h>


Graph::Graph(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::graph),
    plot(new QwtPlot(this)),

    data(),
    br(0,0,1,1),
    interval(0,0),
    points(2,2),

    cData(data, br, interval),
    sData(data, br, interval, points),

    curve(new QwtPlotCurve),
    spectrogram(new QwtPlotSpectrogram),

    scaleAxis( plot->axisWidget(QwtPlot::yRight) )

{
  ui->setupUi(this);

  ui->placePlot->insertWidget(0, plot);


  zoomer = new MyZoomer(plot->canvas());

  connect(zoomer, SIGNAL(posCh(QwtDoublePoint)), SLOT(pick(QwtDoublePoint)));

  connect(ui->scaleModel, SIGNAL(buttonClicked(int)), SLOT(linLog()));

  curve->setPen(QPen(QColor(255,0,0)));
  //curve->setStyle(QwtPlotCurve::Dots);

  curve->setData(cData);
  spectrogram->setData(sData);

  scaleAxis->setColorBarEnabled(true);


}



void Graph::changePlot(size_t points, double start, double end){

  spectrogram->detach();
  curve->detach();
  interval.setInterval(0.0, 0.0);
  plot->replot();

  ui->y->setVisible(false);
  ui->label_y->setVisible(false);

  br.setLeft(start);
  br.setRight(end);

  data.fill(NAN, points);
  curve->setData(cData);

  plot->enableAxis(QwtPlot::yRight, false);
  plot->setAxisScale(QwtPlot::xBottom, qMin(start, end), qMax(start, end) );

  curve->attach(plot);
  ui->lin->setChecked(true);
  ui->lin->setEnabled(true);
  ui->log->setEnabled(true);
  plot->replot();
  zoomer->setZoomBase();

}



void Graph::changePlot(size_t xPoints, double xStart, double xEnd,
                       size_t yPoints, double yStart, double yEnd) {

  spectrogram->detach();
  curve->detach();
  interval.setInterval(0.0, 0.0);
  plot->replot();

  ui->y->setVisible(true);
  ui->label_y->setVisible(true);

  br.setLeft(xStart);
  br.setRight(xEnd);
  br.setTop(yStart);
  br.setBottom(yEnd);

  points = QSize(xPoints, yPoints);

  data.fill(NAN, xPoints*yPoints);
  sData.setBoundingRect(br);
  spectrogram->setData(sData);

  scaleAxis->setColorMap(spectrogram->data().range(), spectrogram->colorMap());
  plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
  plot->setAxisScale(QwtPlot::yLeft, qMin(yStart, yEnd), qMax(yStart, yEnd) );
  plot->setAxisScale(QwtPlot::xBottom, qMin(xStart, xEnd), qMax(xStart, xEnd) );
  plot->setAxisScale(QwtPlot::yRight,
                     spectrogram->data().range().minValue(),
                     spectrogram->data().range().maxValue() );
  plot->enableAxis(QwtPlot::yRight, true);

  spectrogram->attach(plot);
  ui->lin->setChecked(true);
  ui->lin->setEnabled(false);
  ui->log->setEnabled(false);
  plot->replot();

}


double Graph::newData(int point, double nData) {

  if ( point >= 0  &&  point < data.size() )
    data[point] = nData;

  bool updateRange = true;

  if ( ! point )
    interval.setInterval(nData,nData);
  else if ( nData < interval.minValue() )
    interval.setMinValue(nData);
  else if ( nData > interval.maxValue() )
    interval.setMaxValue(nData);
  else
    updateRange = false;


  if ( updateRange ) {
    if ( plot->axisEnabled(QwtPlot::yRight) ) {
      scaleAxis->setColorMap(spectrogram->data().range(), spectrogram->colorMap());
      plot->setAxisScale(QwtPlot::yRight,
                         spectrogram->data().range().minValue(),
                         spectrogram->data().range().maxValue() );
    } else {
      zoomer->setZoomBase();
    }
  }

  return nData;
}


void Graph::pick(QwtDoublePoint point) {
  ui->x->setText(QString::number(point.x()));
  ui->y->setText(QString::number(point.y()));

  double value;
  if (curve->plot())
    value = cData.yx(point.x());
  else if (spectrogram->plot())
    value = sData.value(point.x(), point.y());
  else
    value = NAN;

  if ( isnan(value) )
    ui->value->setText("NaN");
  else
    ui->value->setText(QString::number(value));
}

void Graph::linLog() {

  QwtScaleEngine * scale = ( ui->scaleModel->checkedButton() == ui->log ) ?
                   (QwtScaleEngine *) new QwtLog10ScaleEngine  :
                   (QwtScaleEngine *) new QwtLinearScaleEngine;

  if (curve->plot())
    plot->setAxisScaleEngine(QwtPlot::yLeft, scale);

  plot->replot();

}
