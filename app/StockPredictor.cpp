#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <xgboost/c_api.h>

class StockPredictor {
private:
    BoosterHandle booster;
    const int NUM_FEATURES = 60;
    const int NUM_CLASSES = 3;

public:
    StockPredictor() : booster(nullptr) {}

    ~StockPredictor() {
        if (booster) {
            XGBoosterFree(booster);
        }
    }

    bool loadModel(const std::string& model_path) {
        int result = XGBoosterCreate(nullptr, 0, &booster);
        if (result != 0) {
            std::cerr << "Error creation booster" << std::endl;
            return false;
        }

        result = XGBoosterLoadModel(booster, model_path.c_str());
        if (result != 0) {
            std::cerr << "Error chargement modele: " << model_path << std::endl;
            return false;
        }

        std::cout << "Modele charged: " << model_path << std::endl;
        return true;
    }

    int predict(const std::vector<float>& features) {
        if (features.size() != NUM_FEATURES) {
            std::cerr << "Error: " << features.size() << " features instead of  " << NUM_FEATURES << std::endl;
            return 0;
        }

        DMatrixHandle dmat;
        bst_ulong nrows = 1;
        bst_ulong ncols = NUM_FEATURES;

        XGDMatrixCreateFromMat(features.data(), nrows, ncols, -1, &dmat);

        bst_ulong out_len;
        const float* out_result;

        XGBoosterPredict(booster, dmat, 0, 0, 0, &out_len, &out_result);
        XGDMatrixFree(dmat);

        int predicted_class = 0;
        float max_prob = out_result[0];

        for (int i = 1; i < NUM_CLASSES; i++) {
            if (out_result[i] > max_prob) {
                max_prob = out_result[i];
                predicted_class = i;
            }
        }

        return predicted_class - 1;  // 0->-1, 1->0, 2->1
    }

    struct PredictionResult {
        int prediction;
        float prob_down;
        float prob_hold;
        float prob_up;
        float confidence;
    };

    PredictionResult predictWithProba(const std::vector<float>& features) {
        PredictionResult result = { 0, 0.0f, 0.0f, 0.0f, 0.0f };

        if (features.size() != NUM_FEATURES) {
            std::cerr << "Error features" << std::endl;
            return result;
        }

        DMatrixHandle dmat;
        XGDMatrixCreateFromMat(features.data(), 1, NUM_FEATURES, -1, &dmat);

        bst_ulong out_len;
        const float* out_result;

        XGBoosterPredict(booster, dmat, 0, 0, 0, &out_len, &out_result);
        XGDMatrixFree(dmat);

        result.prob_down = out_result[0];
        result.prob_hold = out_result[1];
        result.prob_up = out_result[2];

        int predicted_class = 0;
        result.confidence = out_result[0];

        if (out_result[1] > result.confidence) {
            predicted_class = 1;
            result.confidence = out_result[1];
        }
        if (out_result[2] > result.confidence) {
            predicted_class = 2;
            result.confidence = out_result[2];
        }

        result.prediction = predicted_class - 1;

        return result;
    }
};

// read last line of CSV (without the target)
std::vector<float> readLastLineFromCSV(const std::string& csv_path) {
    std::ifstream file(csv_path);
    std::vector<float> features;

    if (!file.is_open()) {
        std::cerr << "Error ouverture CSV: " << csv_path << std::endl;
        return features;
    }

    std::string line, last_line;

    // Ignore the header
    std::getline(file, line);

    // Lire toutes les lignes pour trouver la dernière
    while (std::getline(file, line)) {
        if (!line.empty()) {
            last_line = line;
        }
    }

    file.close();

    if (last_line.empty()) {
        std::cerr << "CSV vide" << std::endl;
        return features;
    }

    // Parse the last line
    std::stringstream ss(last_line);
    std::string value;

    // Lire the 60 first column (ignore the 61ème that is target)
    for (int i = 0; i < 60; i++) {
    
        if (std::getline(ss, value, ',')) {
            try {
                features.push_back(std::stof(value));
            }
            catch (...) {
                std::cerr << "Error parsing value: " << value << std::endl;
                features.clear();
                return features;
            }
        }
    }

    if (features.size() != 60) {
        std::cerr << "Error: " << features.size() << " features read au lieu de 60" << std::endl;
        
        features.clear();
    }

    return features;
}

int main() {
    StockPredictor predictor;

    // Charge the model
    if (!predictor.loadModel("final_xgb.json")) {
        std::cerr << "echec chargement modèle" << std::endl;
        return 1;
    }

    // read last line of the csv
    std::string csv_path = "../../ML/data/XGBoostdata.csv";
    std::vector<float> features = readLastLineFromCSV(csv_path);

    if (features.empty()) {
        std::cerr << "echec lecture CSV" << std::endl;
        return 1;
    }

    std::cout << "\n" << features.size() << " features read of CSV" << std::endl;

    // simple prediction
    int prediction = predictor.predict(features);
    std::cout << "\n== PREDICTION FOR THE NEXT DAY ==" << std::endl;
    std::cout << "Direction: " << prediction;
    if (prediction == -1) std::cout << " (DOWN - Expected decline)";
    else if (prediction == 0) std::cout << " (HOLD - Expected Stability)";
    else std::cout << " (UP - Expected increase)";
    std::cout << std::endl;

    //detailled prediction
    auto result = predictor.predictWithProba(features);
    std::cout << "\n=== PROBABILITIES ===" << std::endl;
    std::cout << "  DOWN:  " << (result.prob_down * 100.0f) << "%" << std::endl;
    std::cout << "  HOLD:  " << (result.prob_hold * 100.0f) << "%" << std::endl;
    std::cout << "  UP:    " << (result.prob_up * 100.0f) << "%" << std::endl;
    std::cout << "  Confidence: " << (result.confidence * 100.0f) << "%" << std::endl;

    return 0;
}