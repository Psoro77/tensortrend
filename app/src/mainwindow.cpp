// MainWindow.cpp
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chartView(nullptr)
    , chart(nullptr)
{
    ui->setupUi(this);

    setupChart();
    setupConnections();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupChart() {
    // Créer un chart vide au démarrage
    chart = new QChart();
    chart->setTitle("Stock Price History");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Ajouter dans le widget
    QVBoxLayout *layout = new QVBoxLayout(ui->chartWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
}

void MainWindow::setupConnections() {
    // Connecter le bouton Predict
    connect(ui->btnPredict, &QPushButton::clicked,
            this, &MainWindow::onPredictClicked);
}

void MainWindow::onPredictClicked() {
    qDebug() << "Predict button clicked!";
    qDebug() << "Current working directory:" << QDir::currentPath();
    // 1. Récupérer le stock sélectionné
    QString selectedStock = ui->comboBoxStock->currentText();

    if (selectedStock == "Select a stock...") {
        QMessageBox::warning(this, "Warning", "Please select a stock first!");
        return;
    }

    // 2. Récupérer le nom du dossier
    QString stockFolder = getStockFolder();

    if (stockFolder.isEmpty()) {
        QMessageBox::warning(this, "Error", "Unknown stock selected!");
        return;
    }

    qDebug() << "Loading data for:" << stockFolder;

    // 3. Construire le chemin du CSV
    QString csvPath = QString("../../../../data/csv/%1/%1_prices.csv").arg(stockFolder);

    qDebug() << "CSV Path:" << csvPath;

    // 4. Lire les données
    QVector<PriceData> priceData = readPriceCSV(csvPath);

    if (priceData.isEmpty()) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to load data from:\n%1").arg(csvPath));
        return;
    }

    qDebug() << "Data loaded successfully! Total rows:" << priceData.size();

    // 5. Afficher le chart
    displayChart(priceData);

    // 6. Message de succès
    ui->statusbar->showMessage(QString("Loaded %1 price data (%2 days)")
                                   .arg(stockFolder)
                                   .arg(priceData.size()));
}

QString MainWindow::getStockFolder() {
    QString selectedStock = ui->comboBoxStock->currentText();

    // Mapping des noms affichés -> noms de dossiers
    if (selectedStock.contains("Coca-Cola")) return "Coca";
    if (selectedStock.contains("Tesla")) return "Tesla";
    if (selectedStock.contains("Google")) return "Google";
    if (selectedStock.contains("NVIDIA")) return "Nvidia";
    if (selectedStock.contains("Dick's")) return "DSG";

    return "";
}

QVector<MainWindow::PriceData> MainWindow::readPriceCSV(const QString &csvPath) {
    QVector<PriceData> data;

    QFile file(csvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file:" << csvPath;
        return data;
    }

    QTextStream in(&file);

    // Lire l'en-tête
    QString header = in.readLine();
    qDebug() << "CSV Header:" << header;

    // Lire les lignes de données
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList fields = line.split(',');

        if (fields.size() < 2) {
            qWarning() << "Invalid line (not enough columns):" << line;
            continue;
        }

        PriceData pd;

        // Parser la date (format: YYYY-MM-DD)
        pd.date = QDateTime::fromString(fields[0].trimmed(), "yyyy-MM-dd");

        if (!pd.date.isValid()) {
            qWarning() << "Invalid date:" << fields[0];
            continue;
        }

        // Parser le prix de clôture (colonne 2)
        bool ok;
        pd.close = fields[1].trimmed().toDouble(&ok);

        if (!ok) {
            qWarning() << "Invalid price:" << fields[1];
            continue;
        }

        data.append(pd);
    }

    file.close();

    qDebug() << "Successfully read" << data.size() << "rows from CSV";

    return data;
}

void MainWindow::displayChart(const QVector<PriceData> &data) {
    // 1. Nettoyer le chart existant
    chart->removeAllSeries();

    // Supprimer les axes existants
    foreach (QAbstractAxis *axis, chart->axes()) {
        chart->removeAxis(axis);
    }

    // 2. Créer une nouvelle série
    QLineSeries *series = new QLineSeries();
    series->setName("Close Price");

    // 3. Remplir la série avec les données
    // (Limiter aux 90 derniers jours pour la lisibilité)
    int startIndex = qMax(0, data.size() - 90);

    for (int i = startIndex; i < data.size(); ++i) {
        series->append(data[i].date.toMSecsSinceEpoch(), data[i].close);
    }

    // 4. Ajouter la série au chart
    chart->addSeries(series);

    // 5. Créer les axes
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("MMM dd");
    axisX->setTitleText("Date");
    axisX->setLabelsAngle(-45);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Price ($)");
    axisY->setLabelFormat("%.2f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 6. Mettre à jour le titre
    QString stockName = getStockFolder();
    chart->setTitle(QString("%1 - Last %2 Days").arg(stockName).arg(data.size() - startIndex));

    qDebug() << "Chart updated successfully!";
}
