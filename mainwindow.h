#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include "scene.h"
#include <cmath>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Scene* scene;
    QVector<QVector<double>> data;
    QVector<int> clusters;
    void load_data();
    void save_clusters();
    void find_minmax_ranges();
    void prepare_scene();
    void display_data(int column = 0);
    void start_clustering();

    static double euclidean_distance(const QVector<double>&, const QVector<double>&);
    static QVector<int> k_means_clustering(const QVector<QVector<double>>&, int, bool);
    static QVector<QVector<double>> k_means_plusplus_centroids(const QVector<QVector<double>>&, int);
};
#endif // MAINWINDOW_H
