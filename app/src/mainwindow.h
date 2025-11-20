#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts>
#include <QVector>
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include "StockPredictor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Slots principaux
    void onPredictClicked();        // Slot pour le bouton Predict
    void onClearClicked();          // Slot pour le bouton Clear
    void onTimePeriodChanged(int index);  // Slot pour changement de période

private:
    Ui::MainWindow *ui;

    // Composants Chart
    QChartView *chartView;
    QChart *chart;

    // Prédicteur ML
    StockPredictor predictor;

    // Structure pour stocker les données de prix
    struct PriceData {
        QDateTime date;
        double close;
    };

    // Cache des données actuelles (pour éviter de recharger le CSV)
    QVector<PriceData> currentPriceData;

    // Méthodes de configuration
    void setupChart();              // Initialiser le graphique
    void setupConnections();        // Connecter les signaux et slots

    // Méthodes utilitaires
    QString getStockFolder();       // Récupérer le nom du dossier selon le stock sélectionné
    QVector<PriceData> readPriceCSV(const QString &csvPath);  // Lire le CSV des prix
    void displayChart(const QVector<PriceData> &data, int daysToShow = -1);  // Afficher le chart
    void displayPrediction(const StockPredictor::PredictionResult &result);  // Afficher la prédiction
    int getDaysFromTimePeriod(int index);  // Convertir l'index du combo en nombre de jours
};

#endif // MAINWINDOW_H
