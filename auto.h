#ifndef AUTO_H
#define AUTO_H

#include "BitBoardTables.h"

// 位棋盘类型定义
using BitBoard = uint64_t;

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

    // 初始化位棋盘表格 - 公开方法供其他类调用
    void initTables();

    // 清除缓存的空方法（为了保持兼容性）
    void clearExpectimaxCache();

   private:
    // 策略参数
    QVector<double> strategyParams;
    QVector<double> defaultParams;  // 默认参数
    bool useLearnedParams;

    QMutex mutex;

    // 评估函数
    int evaluateBoard(QVector<QVector<int>> const& boardState);
    int evaluateBoardAdvanced(QVector<QVector<int>> const& boardState);
    int evaluateWithParams(QVector<QVector<int>> const& boardState, QVector<double> const& params);
    int evaluateAdvancedPattern(QVector<QVector<int>> const& boardState);
    static double calculateMergeScore(QVector<QVector<int>> const& boardState);

    // 位操作相关函数
    BitBoard convertToBitBoard(QVector<QVector<int>> const& boardState);
    QVector<QVector<int>> convertFromBitBoard(BitBoard board);
    int evaluateBitBoard(BitBoard board);
    int countEmptyTiles(BitBoard board);
    bool simulateMoveBitBoard(BitBoard& board, int direction, int& score);

    // 启发式剪枝相关函数
    int calculateMoveHeuristic(BitBoard board, int direction);
    int expectimaxBitBoard(BitBoard board, int depth, bool isMaxPlayer, int alpha = -1000000, int beta = 1000000);
    int expectimaxBitBoardWithPruning(BitBoard board, int depth, bool isMaxPlayer);

    int getBestMoveBitBoard(QVector<QVector<int>> const& boardState);
    bool isGameOver(QVector<QVector<int>> const& boardState);
    bool isGameOverBitBoard(BitBoard board);

    // 模拟和搜索
    bool simulateMove(QVector<QVector<int>>& boardState, int direction, int& score);
    int expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer);
};

#endif  // AUTO_H
