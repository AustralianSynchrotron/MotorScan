#ifndef GRAPH_H
#define GRAPH_H


#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_picker.h>
#include <QDebug>

#include <blitz/array.h>


typedef blitz::Array<double,1> Line;
typedef blitz::Array<double,2> Map;




class MyPicker: public QwtPlotPicker {
  Q_OBJECT;

private:
  double val;

public:
  MyPicker(QwtPlotCanvas *canvas);

  QwtText trackerTextF(const QPointF &pos) const;

  void setValue(double _val) { val = _val; }

signals:
  void gimmeValue(const QPointF &pos) const;
  void rightClicked(QPoint pos) const;

private:

  bool eventFilter(QObject * object, QEvent *event);

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

  void changePlot(const double * xData, const double * yData, int size);
  void changePlot(const double * zData, int width, int height,
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


};



#endif // GRAPH_H
