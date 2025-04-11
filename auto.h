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

// 训练相关功能已移除

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
    // 主要功能
    int findBestMove(QVector<QVector<int>> const& board);
    // 训练相关方法已移除

    // 训练进度相关功能已移除

    // 缓存相关
    void clearExpectimaxCache();  // 清除缓存

    // 友元类声明已移除

   private:
    // 策略参数
    QVector<double> strategyParams;
    QVector<double> defaultParams;  // 默认参数，当不使用学习参数时使用
    bool useLearnedParams;
    int bestHistoricalScore;  // 历史最佳分数

    // 训练进度相关字段已移除
    QMutex mutex;

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

    // 评估函数
    int evaluateBoardAdvanced(QVector<QVector<int>> const& boardState);
    int evaluateAdvancedPattern(QVector<QVector<int>> const& boardState);
    static double calculateMergeScore(QVector<QVector<int>> const& boardState);

    // 游戏状态检查
    bool isGameOver(QVector<QVector<int>> const& boardState);

    // 模拟和搜索
    bool simulateMove(QVector<QVector<int>>& boardState, int direction, int& score);
    int expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer);

    // 遗传算法相关方法已移除
};

#endif  // AUTO_H
