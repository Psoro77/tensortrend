// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts>
#include <QVector>
#include <QDateTime>
#include "StockPredictor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPredictClicked();  // Slot pour le bouton Predict
    void onClearClicked();
private:
    Ui::MainWindow *ui;
    QChartView *chartView;
    QChart *chart;
    StockPredictor predictor;
    // Structure pour stocker les données de prix
    struct PriceData {
        QDateTime date;
        double close;
    };

    void setupChart();
    void setupConnections();  // Pour connecter les signaux
    QString getStockFolder();  // Récupérer le nom du dossier selon le stock sélectionné
    QVector<PriceData> readPriceCSV(const QString &csvPath);  // Lire le CSV
    void displayChart(const QVector<PriceData> &data);  // Afficher le chart
    void displayPrediction(const StockPredictor::PredictionResult &result);  // ← Nouvelle fonction
};

#endif
