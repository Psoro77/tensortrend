// MainWindow.cpp
#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chartView(nullptr)
    , chart(nullptr)
{
    ui->setupUi(this);

    // Créer le chart au démarrage
    setupChart();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupChart() {
    // 1. Créer le chart
    chart = new QChart();
    chart->setTitle("Stock Price History");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // 2. Créer une série de données (pour tester)
    QLineSeries *series = new QLineSeries();
    series->setName("Test Data");

    // Ajouter quelques points de test
    series->append(0, 100);
    series->append(1, 105);
    series->append(2, 103);
    series->append(3, 110);
    series->append(4, 108);
    series->append(5, 115);

    // 3. Ajouter la série au chart
    chart->addSeries(series);

    // 4. Créer les axes
    chart->createDefaultAxes();

    // 5. Créer la vue du chart
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // 6. Ajouter la vue dans le widget chartWidget de votre UI
    QVBoxLayout *layout = new QVBoxLayout(ui->chartWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
}
