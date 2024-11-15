#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>


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
    connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::display_data);
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
        qWarning() << "Не удалось открыть файл!";
        return;
    }
    QTextStream in(&file);
    data.clear();
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
    prepare_scene();
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
