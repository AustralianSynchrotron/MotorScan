#ifndef GRAPH_H
#define GRAPH_H

#include<ui_graph.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_zoomer.h>
#include <QDebug>





class CurveData: public QwtData {

private:

  const QwtArray<double> & data;
  const QwtDoubleRect & br;
  const QwtDoubleInterval & interval;

public:

  CurveData(const QwtArray<double> & _data, const QwtDoubleRect & _br,
            const QwtDoubleInterval & _interval):
      QwtData(),
      data(_data),
      br(_br),
      interval(_interval) { }

  virtual QwtData *copy() const {
    return new CurveData(data, br, interval) ;
  }

  virtual size_t 	size () const {
    return data.size();
  }

  virtual double 	x  (size_t i) const {
    return br.left() +
        i * ( br.width() ) / ( data.size() - 1 ) ;
  }

  virtual double 	y  (size_t i) const {
    return data[i];
  }

  double yx(double x) {
    int i = ( x - br.left() ) * ( data.size() - 1 ) / br.width() ;
    return  ( i >= 0 && i < data.size() )  ?  y( i )  :  NAN ;
  }

  virtual QwtDoubleRect boundingRect () const {
    return QwtDoubleRect(br.left(), interval.minValue(),
                         br.width(), interval.width() );
  }

};



class SpectrogramData: public QwtRasterData {

private:

  const QwtArray<double> & data;
  const QwtDoubleRect & br;
  const QwtDoubleInterval & interval;
  const QSize & points;

public:

    SpectrogramData(const QwtArray<double> & _data, const QwtDoubleRect & _br,
                    const QwtDoubleInterval & _interval, const QSize & _points):
        QwtRasterData(_br),
        data(_data),
        br(_br),
        interval(_interval),
        points(_points) {
    }

    virtual QwtRasterData *copy() const {
        return new SpectrogramData(data, br, interval, points);
    }

    virtual QwtDoubleInterval range() const {
        return interval;
    }

    virtual double value(double x, double y) const {
      x += 0.5 * qAbs(br.width()) / points.width();
      y -= 0.5 * qAbs(br.height()) / points.height();
      int point =  lround( (points.width()-1) * (x - br.left()) / br.width() ) +
                   points.width() * lround( (points.height()-1) * (y - br.top()) / br.height() );
      return  ( point >= 0 && point < data.size() )  ?  data[ point ] : NAN ;
    }

    virtual QSize rasterHint(const QwtDoubleRect &) const {
      return points;
    }

};





class MyZoomer: public QwtPlotZoomer {
  Q_OBJECT;
public:
    MyZoomer(QwtPlotCanvas *canvas):
        QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
                      QwtPicker::PointSelection,
                      QwtPicker::AlwaysOn, canvas) {
    }

    virtual QwtText trackerText(const QwtDoublePoint &pos) const {
      emit posCh(pos);
      return QwtText();
    }

  signals:
    void posCh(const QwtDoublePoint &pos) const;

};



class Graph : public QWidget {
  Q_OBJECT;

private:

  Ui::graph * ui;

public:
  explicit Graph(QWidget *parent = 0);

  QwtPlot * plot;
  QwtPlotZoomer* zoomer;

  void changePlot(size_t points, double start, double end);
  void changePlot(size_t xPoints, double xStart, double xEnd,
                  size_t yPoints, double yStart, double yEnd);
  double newData(int point, double nData);

private:

  QwtArray<double>  data;
  QwtDoubleRect  br;
  QwtDoubleInterval  interval;
  QSize  points;

  CurveData cData;
  SpectrogramData sData;
  QwtPlotCurve *curve;
  QwtPlotSpectrogram * spectrogram;
  QwtScaleWidget * scaleAxis;

private slots:

  void pick(QwtDoublePoint point);
  void linLog();

};



#endif // GRAPH_H
