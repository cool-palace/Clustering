#ifndef SCENE_H
#define SCENE_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QResizeEvent>

class Scene : public QGraphicsScene
{
    Q_OBJECT
public:
    Scene(QObject* parent = nullptr);
    ~Scene() override;
    void set_scale(double, double, double, double, double, double);
    void add_point(double, double);
    void set_point_color(int, int);
    void reset_point_colors();
    void set_ranges(const QVector<double>&, const QVector<double>&, double, double);
    void display_data(const QVector<QVector<double>>&);
    void display_data(const QVector<QVector<double>>&, int column);
    void display_data(const QVector<QVector<double>>&, const QVector<int>&, int);
signals:
    void update_point_color(int, int);
private:
    void reset();
    void clear_points();
    const int margin = 10;
    double scale;
    QPointF min;
    QPointF steps;
    QPen line_pen;
    QList<QGraphicsLineItem*> points;
    QVector<double> minimums;
    QVector<double> maximums;
    QVector<double> hue_step_sizes;
};

#endif // SCENE_H
