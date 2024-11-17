#include "scene.h"
#include <QDebug>
#include <QtConcurrent>
#include <QGraphicsView>

Scene::Scene(QObject* parent) : QGraphicsScene(parent)
    , line_pen(QBrush(Qt::black),2)
{
    connect(this, &Scene::update_point_color, this, &Scene::set_point_color);
}

Scene::~Scene() {
    for (auto point : points) {
        delete point;
    }
}

void Scene::set_scale(double x_max, double y_max, double x_min, double y_min, double x_step, double y_step) {
    min = QPointF(x_min, y_min);
    steps = QPointF(x_step, y_step) * 3;
    setSceneRect(QRectF(0, 0, abs(x_max-x_min)/steps.x(), abs(y_max-y_min)/steps.y()));
    auto view = views().first();
    view->fitInView(sceneRect(), Qt::KeepAspectRatio);
    view->ensureVisible(itemsBoundingRect(), 0, 0);
//    double x_scale = (x_max - x_min)/x_step;
//    double y_scale = (y_max - y_min)/y_step;
//    qDebug() << width() << height() << min << scale;
//    qDebug() << x_scale << y_scale << x_step << y_step;
}

void Scene::add_point(double x, double y) {
//    qDebug() << (x-min.x()) * scale + margin << height() - margin - (y-min.y()) * scale;
//    QPointF point = QPointF((x-min.x()) * scale + margin, height() - margin - (y-min.y()) * scale);
    QPointF point = QPointF((x-min.x())/steps.x(), height() - (y-min.y())/steps.y());
    auto line = QLineF(point, point);
    points.append(new QGraphicsLineItem(line));
    points.back()->setPen(line_pen);
    addItem(points.back());
}

void Scene::set_point_color(int index, int hue) {
    points[index]->setPen(QPen(QBrush(QColor::fromHsv(hue, 255, 255)),2));
}

void Scene::reset_point_colors() {
    for (int i = 0; i < points.size(); ++i) {
        points[i]->setPen(line_pen);
    }
}

void Scene::set_ranges(const QVector<double> & min, const QVector<double> & max, double x_step, double y_step) {
    reset();
    minimums = min;
    maximums = max;
    set_scale(maximums[0], maximums[1], minimums[0], minimums[1], x_step, y_step);
    hue_step_sizes = {0, 0};
    for (int i = 2; i < max.size(); ++i) {
        hue_step_sizes.push_back(300/(abs(maximums[i]-minimums[i])));
    }
}

void Scene::display_data(const QVector<QVector<double>>& data){
    points.reserve(data.size());
    for (int i = 0; i < data.size(); ++i) {
        add_point(data[i][0], data[i][1]);
    }
}

void Scene::display_data(const QVector<QVector<double>>& data, int column){
    if (column < 2)  {
        reset_point_colors();
        return;
    }
    QtConcurrent::run([this, data, column]() {
        for (int i = 0; i < data.size(); ++i) {
            int hue = abs(data[i][column] - minimums[column]) * hue_step_sizes[column];
            emit update_point_color(i, hue);
        }
    });
}

void Scene::display_data(const QVector<QVector<double>>& data, const QVector<int>& clusters, int k) {
    QtConcurrent::run([this, data, clusters, k]() {
        for (int i = 0; i < data.size(); ++i) {
            int hue = (clusters[i] * 300) / (k - 1);
            emit update_point_color(i, hue);
        }
    });
}

void Scene::reset() {
    clear_points();
    maximums.clear();
    minimums.clear();
    hue_step_sizes.clear();
}

void Scene::clear_points() {
    for (auto point : points) {
        delete point;
    }
    points.clear();
}
