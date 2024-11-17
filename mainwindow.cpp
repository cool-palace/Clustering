#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new Scene())
{
    ui->setupUi(this);
    scene->setSceneRect(0,0,ui->view->width(), ui->view->height());
    ui->view->setScene(scene);
    ui->view->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    ui->view->setAlignment(Qt::AlignCenter);
    connect(ui->load, &QAction::triggered, this, &MainWindow::load_data);
    connect(ui->save, &QAction::triggered, this, &MainWindow::save_clusters);
    connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::display_data);
    connect(ui->clustering, &QPushButton::clicked, this, &MainWindow::start_clustering);
}

MainWindow::~MainWindow() {
    delete ui;
    delete scene;
}

void MainWindow::load_data() {
    QString filepath = QFileDialog::getOpenFileName(nullptr, "Открыть файл",
                                                    QCoreApplication::applicationDirPath(),
                                                    "(*.dat)");
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Не удалось открыть файл.";
        return;
    }
    QTextStream in(&file);
    data.clear();
    clusters.clear();
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(" ", QString::SkipEmptyParts);
        QVector<double> row;
        row.reserve(values.size());
        for (const QString& value : values) {
            row.append(value.toDouble());
        }
        data.append(row);
    }
    file.close();
    ui->statusbar->showMessage("Данные успешно импортированы");
    ui->spinBox->setEnabled(!data.isEmpty());
    ui->spinBox->setValue(1);
    ui->spinBox->setMaximum(data.first().size());
    ui->num_clusters->setEnabled(!data.isEmpty());
    ui->clustering->setEnabled(!data.isEmpty());
    ui->centroids->setEnabled(!data.isEmpty());
    ui->save->setEnabled(!clusters.isEmpty());
    prepare_scene();
}

void MainWindow::save_clusters() {
    if (data.size() != clusters.size()) {
        qWarning() << "Нарушено соотношение данных, проведите кластеризацию повторно.";
        return;
    }
    QString filepath = QFileDialog::getSaveFileName(this, "Сохранить результут кластеризации",
                                                          QCoreApplication::applicationDirPath(),
                                                          "(*.dat)");
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Не удалось открыть файл для записи.";
        return;
    }

    QTextStream out(&file);
    for (int i = 0; i < data.size(); ++i) {
        if (data[i].size() >= 2) {
            out << data[i][0] << " " << data[i][1] << " " << clusters[i] << "\n";
        } else {
            qWarning() << "Строка " << i << " содержит меньше 2 значений.";
        }
    }
    file.close();
    ui->statusbar->showMessage("Результаты кластеризации сохранены.");
}

void MainWindow::find_minmax_ranges() {
    int row_size = data.first().size();
    QVector<double> minimums = QVector<double>(row_size, qInf());
    QVector<double> maximums = QVector<double>(row_size, -qInf());
    double x_step = qInf(), y_step = qInf();
    for (const auto& vector : data) {
        for (int i = 0; i < vector.size(); ++i) {
            double value = vector[i];
            if (value > maximums[i]) maximums[i] = value;
            else if (value < minimums[i]) minimums[i] = value;
        }
    }
    // Поиск величины шага дискретизации
    for (int i = 0; i < data.size(); ++i) {
        double x_diff = data[i][0] - minimums[0];
        if (x_diff > 0 && x_diff < x_step) x_step = x_diff;
        double y_diff = data[i][1] - minimums[1];
        if (y_diff > 0 && y_diff < y_step) y_step = y_diff;
    }
    scene->set_ranges(minimums, maximums, x_step, y_step);
}

void MainWindow::prepare_scene() {
    find_minmax_ranges();
    scene->display_data(data);
}

void MainWindow::display_data(int column) {
    scene->display_data(data, column-1);
}

void MainWindow::start_clustering() {
    if (data.empty()) return;
    int k = ui->num_clusters->value();
    bool plus_centroids = ui->centroids->currentIndex() == 1 ? true : false;
    ui->statusbar->showMessage("Начата кластеризация, пожалуйста, подождите...");
    ui->spinBox->setEnabled(false);
    ui->clustering->setEnabled(false);
    QFutureWatcher<QVector<int>>* watcher = new QFutureWatcher<QVector<int>>(this);
    auto future = QtConcurrent::run([this, k, plus_centroids]() {
        return k_means_clustering(data, k, plus_centroids);
    });
    connect(watcher, &QFutureWatcher<QVector<int>>::finished, this, [this, watcher]() {
        clusters = watcher->result();
        scene->display_data(data, clusters, ui->num_clusters->value());
        ui->statusbar->showMessage("Кластеризация проведена.");
        ui->spinBox->setEnabled(true);
        ui->clustering->setEnabled(true);
        ui->save->setEnabled(true);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

double MainWindow::euclidean_distance(const QVector<double> &point1, const QVector<double> &point2) {
    double sum = 0;
    for (int i = 2; i < point1.size(); ++i) {
        sum += std::pow(point1[i] - point2[i], 2);
    }
    return std::sqrt(sum);
}

QVector<int> MainWindow::k_means_clustering(const QVector<QVector<double>> &data, int k, bool plus_centroids) {
    int num_points = data.size();
    int num_features = data[0].size();

    QVector<QVector<double>> centroids;
    if (plus_centroids) {
        // Поиск центроидов по алгоритму k-means++
        centroids = k_means_plusplus_centroids(data, k);
    } else {
        // Инициализация центроидов случайным образом
        centroids = QVector<QVector<double>>(k, QVector<double>(num_features));
        srand(time(0));
        for (int i = 0; i < k; ++i) {
            centroids[i] = data[rand() % num_points];
        }
    }
    QVector<int> labels(num_points, -1);
    bool converged = false;

    while (!converged) {
        converged = true;
        // Присвоение точек к ближайшим центроидам
        for (int i = 0; i < num_points; ++i) {
            double min_distance = std::numeric_limits<double>::max();
            int best_centroid = -1;
            for (int j = 0; j < k; ++j) {
                double distance = euclidean_distance(data[i], centroids[j]);
                if (distance < min_distance) {
                    min_distance = distance;
                    best_centroid = j;
                }
            }
            if (labels[i] != best_centroid) {
                labels[i] = best_centroid;
                converged = false;
            }
        }
        // Обновление центроидов
        QVector<QVector<double>> new_centroids(k, QVector<double>(num_features, 0));
        QVector<int> points_per_cluster(k, 0);
        for (int i = 0; i < num_points; ++i) {
            int cluster = labels[i];
            for (int j = 0; j < num_features; ++j) {
                new_centroids[cluster][j] += data[i][j];
            }
            points_per_cluster[cluster]++;
        }
        for (int i = 0; i < k; ++i) {
            if (points_per_cluster[i] > 0) {
                for (int j = 0; j < num_features; ++j) {
                    new_centroids[i][j] /= points_per_cluster[i];
                }
            } else {
                new_centroids[i] = data[rand() % num_points]; // Случайная инициализация для пустого кластера
            }
        }
        centroids = new_centroids;
    }
    return labels;
}

QVector<QVector<double>> MainWindow::k_means_plusplus_centroids(const QVector<QVector<double>>& data, int k) {
    int num_points = data.size();
    QVector<QVector<double>> centroids;
    srand(time(0));
    // Случайный выбор первого центра
    centroids.push_back(data[rand() % num_points]);

    // Выбор оставшихся центров
    for (int i = 1; i < k; ++i) {
        QVector<double> distances(num_points);
        double total_distance = 0.0;
        // Вычисление расстояний до ближайшего центра
        for (int j = 0; j < num_points; ++j) {
            double min_distance = std::numeric_limits<double>::max();
            for (const auto& centroid : centroids) {
                double distance = euclidean_distance(data[j], centroid);
                if (distance < min_distance) {
                    min_distance = distance;
                }
            }
            distances[j] = min_distance;
            total_distance += min_distance * min_distance; // Общая сумма квадратов расстояний
        }
        // Выбор следующего центра с вероятностью пропорционально квадрату расстояния
        double rand_val = static_cast<double>(rand()) / RAND_MAX; // Случайное число от 0 до 1
        double cumulative_prob = 0.0;
        for (int j = 0; j < num_points; ++j) {
            cumulative_prob += (distances[j] * distances[j]) / total_distance;
            if (cumulative_prob >= rand_val) {
                centroids.push_back(data[j]);
                break;
            }
        }
    }
    return centroids;
}
