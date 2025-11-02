#include "StockPredictor.h"
#include <iostream>
#include <fstream>
#include <sstream>

StockPredictor::StockPredictor() : booster(nullptr) {}

StockPredictor::~StockPredictor() {
    if (booster) {
        XGBoosterFree(booster);
    }
}

bool StockPredictor::loadModel(const std::string& model_path) {
    int result = XGBoosterCreate(nullptr, 0, &booster);
    if (result != 0) {
        std::cerr << "Error creating booster" << std::endl;
        return false;
    }

    result = XGBoosterLoadModel(booster, model_path.c_str());
    if (result != 0) {
        std::cerr << "Error loading model: " << model_path << std::endl;
        return false;
    }

    std::cout << "Model loaded: " << model_path << std::endl;
    return true;
}

StockPredictor::PredictionResult StockPredictor::predictWithProba(const std::vector<float>& features) {
    PredictionResult result = { 0, 0.0f, 0.0f, 0.0f, 0.0f };

    if (features.size() != NUM_FEATURES) {
        std::cerr << "Error: Expected " << NUM_FEATURES << " features, got " << features.size() << std::endl;
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

    result.prediction = predicted_class - 1;  // 0->-1, 1->0, 2->1

    return result;
}

std::vector<float> StockPredictor::readLastLineFromCSV(const std::string& csv_path) {
    std::ifstream file(csv_path);
    std::vector<float> features;

    if (!file.is_open()) {
        std::cerr << "Error opening CSV: " << csv_path << std::endl;
        return features;
    }

    std::string line, last_line;

    // Ignore header
    std::getline(file, line);

    // Read all lines to find the last one
    while (std::getline(file, line)) {
        if (!line.empty()) {
            last_line = line;
        }
    }

    file.close();

    if (last_line.empty()) {
        std::cerr << "CSV is empty" << std::endl;
        return features;
    }

    // Parse the last line (60 features, ignore 61st column if it's the target)
    std::stringstream ss(last_line);
    std::string value;

    for (int i = 0; i < NUM_FEATURES; i++) {
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

    if (features.size() != NUM_FEATURES) {
        std::cerr << "Error: Read " << features.size() << " features instead of " << NUM_FEATURES << std::endl;
        features.clear();
    }

    return features;
}