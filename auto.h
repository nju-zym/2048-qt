#ifndef AUTO_H
#define AUTO_H

#include <QApplication>
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
#include <unordered_map>

// 位棋盘类型定义
typedef uint64_t BitBoard;

// 位棋盘状态结构体
struct BitBoardState {
    BitBoard board;
    int depth;
    bool isMaxPlayer;

    bool operator==(BitBoardState const& other) const {
        return board == other.board && depth == other.depth && isMaxPlayer == other.isMaxPlayer;
    }
};

// 位棋盘状态的哈希函数
namespace std {
template <>
struct hash<BitBoardState> {
    std::size_t operator()(BitBoardState const& state) const {
        std::size_t h1 = std::hash<BitBoard>{}(state.board);
        std::size_t h2 = std::hash<int>{}(state.depth);
        std::size_t h3 = std::hash<bool>{}(state.isMaxPlayer);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
}  // namespace std

// 训练任务类，用于多线程训练
class TrainingTask : public QRunnable {
   public:
    TrainingTask(QVector<double> params,
                 int simulations,
                 std::function<void(int)> finalCallback,
                 std::function<void(int, int, int)> progressCallback = nullptr)
        : params(std::move(params)),
          simulations(simulations),
          finalCallback(std::move(finalCallback)),
          progressCallback(std::move(progressCallback)) {}

    void run() override;

   private:
    QVector<double> params;
    int simulations;
    std::function<void(int)> finalCallback;               // 回调函数，返回最终分数
    std::function<void(int, int, int)> progressCallback;  // 进度回调，参数：当前模拟次数、总模拟次数、当前分数
};

// 训练工作线程类
class TrainingWorker;

// 训练进度信号类
class TrainingProgress : public QObject {
    Q_OBJECT

   public:
    TrainingProgress() = default;

   signals:
    // 代代进度更新
    void progressUpdated(int generation, int totalGenerations, int bestScore, QVector<double> const& bestParams);
    // 单次模拟进度更新
    void simulationUpdated(int currentSim, int totalSims, int avgScore, int totalProgress);
    // 训练完成
    void trainingCompleted(int finalScore, QVector<double> const& finalParams);
};

// 前向声明
class TrainingWorker;

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

    // 停止训练
    void stopTraining() {
        trainingActive.store(false);
    }

    // 主要功能
    int findBestMove(QVector<QVector<int>> const& board);
    void learnParameters(int populationSize = 150, int generations = 100, int simulations = 50);
    int simulateFullGame(QVector<double> const& params);
    void simulateFullGameDetailed(QVector<double> const& params, int& score, int& maxTile);
    int evaluateParameters(QVector<double> const& params, int simulations = 50);  // 更全面地评估参数

    // 初始化位棋盘表格 - 公开方法供其他类调用
    void initTables();

    // 保存和加载参数
    bool saveParameters(QString const& filename = "");
    bool savePersistentData(QJsonDocument const& doc);
    bool loadParameters(QString const& filename = "");
    bool loadPersistentData();
    bool processJsonData(QByteArray const& data);

    // 重置历史最佳分数
    void resetBestHistoricalScore();

    // 获取训练进度信号对象
    TrainingProgress* getTrainingProgress() {
        return &trainingProgress;
    }

    // 缓存相关
    void clearExpectimaxCache();  // 清除缓存

    // 友元类声明
    friend class TrainingWorker;

   private:
    // 策略参数
    QVector<double> strategyParams;
    QVector<double> defaultParams;  // 默认参数，当不使用学习参数时使用
    bool useLearnedParams;
    int bestHistoricalScore;  // 历史最佳分数

    // 训练进度
    TrainingProgress trainingProgress;
    QMutex mutex;
    std::atomic<bool> trainingActive;

    // 缓存相关
    struct BoardState {
        QVector<QVector<int>> board;
        int depth;
        bool isMaxPlayer;

        bool operator==(BoardState const& other) const {
            return board == other.board && depth == other.depth && isMaxPlayer == other.isMaxPlayer;
        }
    };

    struct BoardStateHash {
        std::size_t operator()(BoardState const& state) const {
            std::size_t hash = 0;
            for (auto const& row : state.board) {
                for (int value : row) {
                    hash = hash * 31 + value;
                }
            }
            hash = hash * 31 + state.depth;
            hash = hash * 31 + (state.isMaxPlayer ? 1 : 0);
            return hash;
        }
    };

    std::unordered_map<BoardState, int, BoardStateHash> expectimaxCache;

    // 使用位操作优化的棋盘表示
    typedef uint64_t BitBoard;

    struct BitBoardState {
        BitBoard board;
        int depth;
        bool isMaxPlayer;

        bool operator==(BitBoardState const& other) const {
            return board == other.board && depth == other.depth && isMaxPlayer == other.isMaxPlayer;
        }
    };

    struct BitBoardStateHash {
        std::size_t operator()(BitBoardState const& state) const {
            return state.board ^ (static_cast<uint64_t>(state.depth) << 32) ^ (state.isMaxPlayer ? 0x1ULL << 63 : 0);
        }
    };

    // 位棋盘缓存
    std::unordered_map<BitBoardState, int, BitBoardStateHash> bitboardCache;

    // 评估函数
    int evaluateBoard(QVector<QVector<int>> const& boardState);
    int evaluateBoardAdvanced(QVector<QVector<int>> const& boardState);
    int evaluateWithParams(QVector<QVector<int>> const& boardState, QVector<double> const& params);
    int evaluateAdvancedPattern(QVector<QVector<int>> const& boardState);
    static double calculateMergeScore(QVector<QVector<int>> const& boardState);

    // 数据目录路径
    QString getDataDirPath() const;

    // 位操作相关函数
    BitBoard convertToBitBoard(QVector<QVector<int>> const& boardState);
    QVector<QVector<int>> convertFromBitBoard(BitBoard board);
    int evaluateBitBoard(BitBoard board);
    int countEmptyTiles(BitBoard board);
    bool simulateMoveBitBoard(BitBoard& board, int direction, int& score);
    int expectimaxBitBoard(BitBoard board, int depth, bool isMaxPlayer);
    int getBestMoveBitBoard(QVector<QVector<int>> const& boardState);
    bool isGameOver(QVector<QVector<int>> const& boardState);
    bool isGameOverBitBoard(BitBoard board);

    // 模拟和搜索
    bool simulateMove(QVector<QVector<int>>& boardState, int direction, int& score);
    int monteCarloSimulation(QVector<QVector<int>> const& boardState, int depth);
    int expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer);

    // 遗传算法相关
    QVector<int> findTopIndices(QVector<int> const& scores, int count);
    int tournamentSelection(QVector<int> const& scores);
    QVector<double> crossover(QVector<double> const& parent1, QVector<double> const& parent2);
    void mutate(QVector<double>& params, double mutationRate = 0.2);
};

#endif  // AUTO_H
