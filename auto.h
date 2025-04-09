#ifndef AUTO_H
#define AUTO_H

#include <QAtomicInt>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QRandomGenerator>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <random>
#include <unordered_map>
#include <vector>

// Include OpenCLManager
#include "opencl_manager.h"

// Training progress class
class TrainingProgress : public QObject {
    Q_OBJECT

   public:
    explicit TrainingProgress(QObject* parent = nullptr) : QObject(parent) {}

    // 训练进度信号
    Q_SIGNAL void progressUpdated(int generation,
                                  int totalGenerations,
                                  int bestScore,
                                  QVector<double> const& bestParams);
    Q_SIGNAL void trainingCompleted(int finalScore, QVector<double> const& finalParams);
    Q_SIGNAL void simulationUpdated(int currentSim, int totalSims, int avgScore, int totalProgress);
};

// BitBoard representation for efficient game state handling
using BitBoard = uint64_t;

class Auto : public QObject {
    Q_OBJECT

   public:
    explicit Auto(QObject* parent = nullptr);
    ~Auto();

    // Main interface methods
    int findBestMove(QVector<QVector<int>> const& board);
    void learnParameters(int populationSize, int generations, int simulationsPerEval);
    void stopTraining();
    bool saveParameters();
    bool loadParameters();
    void clearExpectimaxCache();
    void resetParameters();
    void resetBestHistoricalScore();
    TrainingProgress* getTrainingProgress();

    // GPU acceleration methods
    bool initializeGPU();
    void enableGPU(bool enable);
    bool isGPUEnabled() const;  // 移除内联实现，只保留声明

    // 注意：训练状态信号已移至TrainingProgress类

   private:
    // BitBoard operations
    BitBoard convertToBitBoard(QVector<QVector<int>> const& board) const;
    QVector<QVector<int>> convertFromBitBoard(BitBoard board) const;
    BitBoard makeMove(BitBoard board, int direction) const;
    bool canMove(BitBoard board, int direction) const;
    int getEmptyTileCount(BitBoard board) const;
    bool isGameOver(BitBoard board) const;
    bool hasWon(BitBoard board) const;
    int getScore(BitBoard board) const;
    BitBoard addRandomTile(BitBoard board) const;

    // AI algorithms
    double expectimax(BitBoard board,
                      int depth,
                      double probability,
                      bool isMaximizingPlayer,
                      double alpha = -std::numeric_limits<double>::infinity(),
                      double beta  = std::numeric_limits<double>::infinity());
    double evaluateBoard(BitBoard board) const;
    int getSearchDepth(BitBoard board) const;

    // Heuristic evaluation functions
    double scoreHeuristic(BitBoard board) const;
    double emptyTilesHeuristic(BitBoard board) const;
    double monotonicity(BitBoard board) const;
    double smoothness(BitBoard board) const;
    double cornerHeuristic(BitBoard board) const;
    double patternHeuristic(BitBoard board) const;
    double mergeHeuristic(BitBoard board) const;

    // Learning and optimization
    double runSimulation(QVector<double> const& params);
    QVector<double> crossover(QVector<double> const& parent1, QVector<double> const& parent2);
    QVector<double> mutate(QVector<double> const& individual, double mutationRate);
    int tournamentSelect(std::vector<QVector<double>> const& population,
                         std::vector<double> const& fitness,
                         int tournamentSize);

    // Utility functions
    int getTileValue(BitBoard board, int row, int col) const;
    static void setTileValue(BitBoard& board, int row, int col, int value);
    int getMaxTile(BitBoard board) const;
    QString getDataPath() const;
    bool moveLine(int line[4]) const;

    // GPU acceleration methods (私有实现部分)
    double evaluateBoardGPU(BitBoard board) const;
    QVector<double> evaluateBoardsGPU(QVector<BitBoard> const& boards) const;
    QVector<BitBoard> makeMovesGPU(QVector<BitBoard> const& boards, int direction, QVector<bool>& moveResults) const;
    QVector<double> runSimulationsGPU(QVector<double> const& params, int simulationCount);

    // Member variables
    QVector<double> weights;           // Weights for different heuristics
    double learningRate        = 0.1;  // 学习率
    int patienceCounter        = 0;    // 早期停止计数器
    double bestHistoricalScore = 0.0;  // 历史最佳分数
    std::atomic<bool> stopTrainingFlag;
    QMutex cacheMutex;
    std::unordered_map<BitBoard, double> expectimaxCache;
    TrainingProgress* trainingProgress = nullptr;  // 训练进度对象
    OpenCLManager* openclManager       = nullptr;  // OpenCL manager for GPU acceleration
    bool useGPU                        = false;    // Flag to control GPU usage

    // Constants
    static constexpr int MAX_DEPTH        = 5;    // Maximum depth for expectimax search
    static constexpr double PROBABILITY_2 = 0.9;  // Probability of spawning a 2
    static constexpr double PROBABILITY_4 = 0.1;  // Probability of spawning a 4
};

#endif  // AUTO_H
