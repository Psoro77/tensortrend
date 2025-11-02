#ifndef STOCKPREDICTOR_H
#define STOCKPREDICTOR_H

#include <vector>
#include <string>
#include <xgboost/c_api.h>

class StockPredictor {
public:
    struct PredictionResult {
        int prediction;        // -1 = DOWN, 0 = HOLD, 1 = UP
        float prob_down;
        float prob_hold;
        float prob_up;
        float confidence;
    };

    StockPredictor();
    ~StockPredictor();

    bool loadModel(const std::string& model_path);
    PredictionResult predictWithProba(const std::vector<float>& features);
    std::vector<float> readLastLineFromCSV(const std::string& csv_path);

private:
    BoosterHandle booster;
    static const int NUM_FEATURES = 60;
    static const int NUM_CLASSES = 3;
};

#endif