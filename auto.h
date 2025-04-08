#ifndef AUTO_H
#define AUTO_H

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <QVector>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <random>

// 训练任务类，用于多线程训练
class TrainingTask : public QRunnable {
   public:
    TrainingTask(QVector<double> params, int simulations, std::function<void(int)> callback)
        : params(std::move(params)), simulations(simulations), callback(std::move(callback)) {}

    void run() override;

   private:
    QVector<double> params;
    int simulations;
    std::function<void(int)> callback;
};

// 训练进度信号类
class TrainingProgress : public QObject {
    Q_OBJECT

   public:
    TrainingProgress() = default;

   signals:
    void progressUpdated(int generation, int totalGenerations, int bestScore, QVector<double> const& bestParams);
    void trainingCompleted(int finalScore, QVector<double> const& finalParams);
};

class Auto : public QObject {
    Q_OBJECT

   public:
    Auto();
    ~Auto() override;

    // 禁用复制和移动
    Auto(Auto const&)            = delete;
    Auto& operator=(Auto const&) = delete;
    Auto(Auto&&)                 = delete;
    Auto& operator=(Auto&&)      = delete;

    // 设置和获取参数
    void setUseLearnedParams(bool use);
    [[nodiscard]] bool getUseLearnedParams() const;
    [[nodiscard]] QVector<double> getStrategyParams() const;

    // 主要功能
    int findBestMove(QVector<QVector<int>> const& board);
    void learnParameters(int populationSize = 50, int generations = 30, int simulations = 15);
    int simulateFullGame(QVector<double> const& params);
    int evaluateParameters(QVector<double> const& params, int simulations = 30);  // 更全面地评估参数

    // 保存和加载参数
    bool saveParameters(QString const& filename = "");
    bool loadParameters(QString const& filename = "");

    // 获取训练进度信号对象
    TrainingProgress* getTrainingProgress() {
        return &trainingProgress;
    }

   private:
    // 策略参数
    QVector<double> strategyParams;
    bool useLearnedParams;
    int bestHistoricalScore;  // 历史最佳分数

    // 训练进度
    TrainingProgress trainingProgress;
    QMutex mutex;
    std::atomic<bool> trainingActive;

    // 评估函数
    int evaluateBoard(QVector<QVector<int>> const& boardState);
    int evaluateBoardAdvanced(QVector<QVector<int>> const& boardState);
    int evaluateWithParams(QVector<QVector<int>> const& boardState, QVector<double> const& params);
    int evaluateAdvancedPattern(QVector<QVector<int>> const& boardState);
    double calculateMergeScore(QVector<QVector<int>> const& boardState);
    bool isGameOver(QVector<QVector<int>> const& boardState);

    // 模拟和搜索
    bool simulateMove(QVector<QVector<int>>& boardState, int direction, int& score);
    int monteCarloSimulation(QVector<QVector<int>> const& boardState, int depth);
    int expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer);

    // 遗传算法相关
    QVector<int> findTopIndices(QVector<int> const& scores, int count);
    int tournamentSelection(QVector<int> const& scores);
    QVector<double> crossover(QVector<double> const& parent1, QVector<double> const& parent2);
    void mutate(QVector<double>& params);
};

#endif  // AUTO_H
