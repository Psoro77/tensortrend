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

    // Charger le modèle XGBoost
    QString modelPath = QCoreApplication::applicationDirPath() + "/../../../../data/models/final_xgb_3cat.json";

    qDebug() << "Loading model from:" << modelPath;

    if (!predictor.loadModel(modelPath.toStdString())) {
        QMessageBox::critical(this, "Error",
                              QString("Failed to load ML model from:\n%1").arg(modelPath));
    } else {
        qDebug() << "Model loaded successfully!";
        ui->statusbar->showMessage("Model loaded successfully - Ready for predictions");
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupChart() {
    // Créer un chart vide au démarrage
    chart = new QChart();
    chart->setTitle("Stock Price History");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setTheme(QChart::ChartThemeLight);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Ajouter dans le widget
    QVBoxLayout *layout = new QVBoxLayout(ui->chartWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
}

void MainWindow::setupConnections() {
    connect(ui->btnPredict, &QPushButton::clicked,
            this, &MainWindow::onPredictClicked);
    connect(ui->btnClear, &QPushButton::clicked,
            this, &MainWindow::onClearClicked);
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTimePeriodChanged);
}

void MainWindow::onPredictClicked() {
    qDebug() << "=== Predict button clicked ===";

    // 1. Vérifier la sélection
    QString selectedStock = ui->comboBoxStock->currentText();

    if (selectedStock == "Select a stock...") {
        QMessageBox::warning(this, "Warning", "Please select a stock first!");
        return;
    }

    QString stockFolder = getStockFolder();
    if (stockFolder.isEmpty()) {
        QMessageBox::warning(this, "Error", "Unknown stock selected!");
        return;
    }

    qDebug() << "Processing stock:" << stockFolder;
    ui->statusbar->showMessage(QString("Loading data for %1...").arg(stockFolder));

    // 2. Afficher le graphique des prix
    QString pricesCsvPath = QString("../../../../data/csv/%1/%1_prices.csv").arg(stockFolder);
    currentPriceData = readPriceCSV(pricesCsvPath);

    if (currentPriceData.isEmpty()) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to load price data from:\n%1").arg(pricesCsvPath));
        return;
    }

    // Réinitialiser le sélecteur de période
    ui->comboBox->setCurrentIndex(0);

    // Afficher le graphique (par défaut: tous les jours)
    displayChart(currentPriceData);
    qDebug() << "Chart displayed successfully";

    // 3. Faire la prédiction
    QString xgboostCsvPath = QString("../../../../data/csv/%1/XGBoostdata.csv").arg(stockFolder);

    qDebug() << "Reading features from:" << xgboostCsvPath;

    std::vector<float> features = predictor.readLastLineFromCSV(xgboostCsvPath.toStdString());

    if (features.empty()) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to read features from:\n%1").arg(xgboostCsvPath));
        return;
    }

    qDebug() << "Features read successfully:" << features.size();

    // 4. Obtenir la prédiction
    StockPredictor::PredictionResult result = predictor.predictWithProba(features);

    qDebug() << "Prediction:" << result.prediction;
    qDebug() << "Probabilities - DOWN:" << result.prob_down
             << "HOLD:" << result.prob_hold
             << "UP:" << result.prob_up;

    // 5. Afficher les résultats
    displayPrediction(result);

    ui->statusbar->showMessage(QString("✓ Prediction completed for %1").arg(selectedStock), 5000);
}

QString MainWindow::getStockFolder() {
    QString selectedStock = ui->comboBoxStock->currentText();

    // Mapping des noms affichés -> noms de dossiers
    if (selectedStock.contains("Coca-Cola")) return "Coca";
    if (selectedStock.contains("Tesla")) return "Tesla";
    if (selectedStock.contains("Google")) return "Google";
    if (selectedStock.contains("NVIDIA")) return "Nvidia";
    if (selectedStock.contains("Johnson and Johnson")) return "Johnson and Johnson";

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

int MainWindow::getDaysFromTimePeriod(int index) {
    switch(index) {
    case 0: return -1;     // Toutes les données disponibles
    case 1: return 7;      // 1 semaine
    case 2: return 30;     // 1 mois
    case 3: return 180;    // 6 mois
    case 4: return 365;    // 1 an
    case 5: return 1825;   // 5 ans
    default: return -1;    // Par défaut: tout afficher
    }
}

void MainWindow::displayChart(const QVector<PriceData> &data, int daysToShow) {
    if (data.isEmpty()) {
        qWarning() << "No data to display";
        return;
    }

    // 1. Nettoyer le chart existant
    chart->removeAllSeries();

    // Supprimer les axes existants
    foreach (QAbstractAxis *axis, chart->axes()) {
        chart->removeAxis(axis);
    }

    // 2. Créer une nouvelle série
    QLineSeries *series = new QLineSeries();
    series->setName("Close Price");

    // 3. Calculer l'index de départ
    int startIndex;
    if (daysToShow == -1) {
        // Afficher tous les jours (limité à 365 pour la performance)
        startIndex = 0;
    } else {
        startIndex = qMax(0, data.size() - daysToShow);
    }

    // 4. Remplir la série avec les données
    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::min();

    for (int i = startIndex; i < data.size(); ++i) {
        series->append(data[i].date.toMSecsSinceEpoch(), data[i].close);
        minPrice = qMin(minPrice, data[i].close);
        maxPrice = qMax(maxPrice, data[i].close);
    }

    // 5. Ajouter la série au chart
    chart->addSeries(series);

    // 6. Créer les axes
    QDateTimeAxis *axisX = new QDateTimeAxis();

    // Format de date adapté selon la période affichée
    int displayedDays = data.size() - startIndex;
    if (displayedDays <= 31) {
        axisX->setFormat("MMM dd");  // Pour 1 mois ou moins
    } else if (displayedDays <= 180) {
        axisX->setFormat("MMM yyyy");  // Pour 6 mois ou moins
    } else {
        axisX->setFormat("MMM yyyy");  // Pour 1 an et plus
    }

    axisX->setTitleText("Date");
    axisX->setLabelsAngle(-45);
    axisX->setTickCount(qMin(10, displayedDays));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Price ($)");
    axisY->setLabelFormat("%.2f");

    // Ajouter une marge de 5% aux prix min/max pour une meilleure visualisation
    double margin = (maxPrice - minPrice) * 0.05;
    double minY = qMax(0.0, minPrice - margin);
    axisY->setRange(minY, maxPrice + margin);

    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 7. Mettre à jour le titre
    QString stockName = ui->comboBoxStock->currentText();
    displayedDays = data.size() - startIndex;

    QString periodText;
    if (daysToShow == -1) {
        periodText = QString("All Available Data (%1 days)").arg(displayedDays);
    } else {
        periodText = QString("Last %1 Days").arg(displayedDays);
    }

    chart->setTitle(QString("%1 - %2").arg(stockName, periodText));

    qDebug() << "Chart updated successfully! Showing" << displayedDays << "days of data";
}

void MainWindow::displayPrediction(const StockPredictor::PredictionResult &result) {
    // 1. Texte de prédiction
    QString predictionText;
    QString emoji;

    if (result.prediction == -1) {
        predictionText = "📉 The model predicts a DECLINE for the next trading day.";
        emoji = "📉";
    } else if (result.prediction == 0) {
        predictionText = "➡️ The model predicts STABILITY for the next trading day.";
        emoji = "➡️";
    } else {
        predictionText = "📈 The model predicts an INCREASE for the next trading day.";
        emoji = "📈";
    }

    predictionText += QString("\nConfidence: %1%").arg(result.confidence * 100, 0, 'f', 1);

    ui->lblPredictionText->setText(predictionText);

    // 2. Mettre à jour les probabilités
    ui->lblDownValue->setText(QString("%1%").arg(result.prob_down * 100, 0, 'f', 1));
    ui->lblHoldValue->setText(QString("%1%").arg(result.prob_hold * 100, 0, 'f', 1));
    ui->lblUpValue->setText(QString("%1%").arg(result.prob_up * 100, 0, 'f', 1));

    // 3. Mettre en évidence la prédiction principale
    // Reset tous les styles
    ui->frameDown->setStyleSheet("QFrame { background-color: #ffebee; border-radius: 8px; border: 2px solid #ef5350; }");
    ui->frameHold->setStyleSheet("QFrame { background-color: #fff3e0; border-radius: 8px; border: 2px solid #ff9800; }");
    ui->frameUp->setStyleSheet("QFrame { background-color: #e8f5e9; border-radius: 8px; border: 2px solid #66bb6a; }");

    // Mettre en évidence la prédiction
    if (result.prediction == -1) {
        ui->frameDown->setStyleSheet("QFrame { background-color: #ffcdd2; border-radius: 8px; border: 3px solid #d32f2f; box-shadow: 0 4px 8px rgba(211, 47, 47, 0.3); }");
    } else if (result.prediction == 0) {
        ui->frameHold->setStyleSheet("QFrame { background-color: #ffe0b2; border-radius: 8px; border: 3px solid #f57c00; box-shadow: 0 4px 8px rgba(245, 124, 0, 0.3); }");
    } else {
        ui->frameUp->setStyleSheet("QFrame { background-color: #c8e6c9; border-radius: 8px; border: 3px solid #388e3c; box-shadow: 0 4px 8px rgba(56, 142, 60, 0.3); }");
    }
}

void MainWindow::onTimePeriodChanged(int index) {
    // Ne rien faire si aucune donnée n'est chargée
    if (currentPriceData.isEmpty()) {
        ui->statusbar->showMessage("Please select a stock and click Predict first", 3000);
        return;
    }

    // Obtenir le nombre de jours à afficher (y compris pour index 0)
    int daysToShow = getDaysFromTimePeriod(index);

    // Afficher le graphique avec la période sélectionnée
    displayChart(currentPriceData, daysToShow);

    // Mettre à jour la barre de statut
    QString periodName = ui->comboBox->currentText();
    ui->statusbar->showMessage(QString("Chart updated to show %1").arg(periodName), 3000);

    qDebug() << "Time period changed to:" << periodName << "(" << daysToShow << "days)";
}

void MainWindow::onClearClicked() {
    // Réinitialiser le texte de prédiction
    ui->lblPredictionText->setText("No prediction available. Select a stock and click Predict.");

    // Réinitialiser les probabilités
    ui->lblDownValue->setText("---%");
    ui->lblHoldValue->setText("---%");
    ui->lblUpValue->setText("---%");

    // Réinitialiser les styles
    ui->frameDown->setStyleSheet("QFrame { background-color: #ffebee; border-radius: 8px; border: 2px solid #ef5350; }");
    ui->frameHold->setStyleSheet("QFrame { background-color: #fff3e0; border-radius: 8px; border: 2px solid #ff9800; }");
    ui->frameUp->setStyleSheet("QFrame { background-color: #e8f5e9; border-radius: 8px; border: 2px solid #66bb6a; }");

    // Nettoyer le chart
    chart->removeAllSeries();
    foreach (QAbstractAxis *axis, chart->axes()) {
        chart->removeAxis(axis);
    }
    chart->setTitle("Stock Price History");

    // Réinitialiser la sélection des combos
    ui->comboBoxStock->setCurrentIndex(0);
    ui->comboBox->setCurrentIndex(0);

    // Vider le cache de données
    currentPriceData.clear();

    ui->statusbar->showMessage("All results cleared - Ready for new prediction");

    qDebug() << "All data cleared successfully";
}
