#ifndef GRAPH_H
#define GRAPH_H

#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_canvas.h>
#include <QDebug>
#include <QPrinter>

#include <blitz/array.h>


//typedef blitz::Array<double,1> Line;
//typedef blitz::Array<double,2> Map;




class MyPicker: public QwtPlotPicker {
  Q_OBJECT;

private:
  double val;
  mutable QPointF latestPos;

public:
  #if QWT_VERSION >= 0x060100
  MyPicker(QWidget *canvas);
  #else
  MyPicker(QwtPlotCanvas *canvas);
  #endif

  QwtText trackerTextF(const QPointF &pos) const;

  void setValue(double _val) {val = _val;}
  double value() const {return val;}

signals:
  void gimmeValue(const QPointF &pos) const;
  void rightClicked(const QPointF & pos, double val) const;

protected:

virtual bool eventFilter(QObject * object, QEvent *event);

};






class PlotData;
namespace Ui {
class Graph;
}


class Graph : public QWidget {
  Q_OBJECT;

private:

  Ui::Graph * ui;
  PlotData * pdata;
  QwtPlotGrid * grid;

public:

  explicit Graph(QWidget *parent = 0);
  ~Graph();

  double * changePlot(const QVector<double> & yData, double xStart, double xEnd);
  double * changePlot(const QVector<double> & zData, int width,
                  double xStart, double xEnd,
                  double yStart, double yEnd);
  void updateData(double point);
  void updateData();
  void print(QPrinter & printer);
  void setTitle(const QString & text);

private slots:

  void changePlot();
  void updateRange();
  void showGrid();
  void setLogarithmic();
  void pick(const QPointF & point);

signals:

  void rightClicked(const QPointF & pos, double val) const;

};



#endif // GRAPH_H
