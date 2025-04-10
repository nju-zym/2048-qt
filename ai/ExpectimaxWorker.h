#ifndef EXPECTIMAX_WORKER_H
#define EXPECTIMAX_WORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include "BitBoard.h"

/**
 * @brief Expectimax算法工作线程类
 * 
 * 在单独的线程中执行Expectimax算法计算
 */
class ExpectimaxWorker : public QObject {
    Q_OBJECT

public:
    explicit ExpectimaxWorker(QObject* parent = nullptr);
    ~ExpectimaxWorker();

    /**
     * @brief 开始计算最佳移动
     * @param board 当前棋盘状态
     * @param depth 搜索深度
     */
    void calculateBestMove(const BitBoard& board, int depth);

    /**
     * @brief 停止当前计算
     */
    void stopCalculation();

signals:
    /**
     * @brief 当计算出最佳移动时发出的信号
     * @param direction 移动方向（0=上, 1=右, 2=下, 3=左）
     */
    void moveCalculated(int direction);

private:
    QThread workerThread;
    QMutex mutex;
    QWaitCondition condition;
    bool abort;
    bool restart;
    BitBoard currentBoard;
    int searchDepth;

    // 评估函数权重
    static constexpr float MONOTONICITY_WEIGHT = 1.0F;
    static constexpr float SMOOTHNESS_WEIGHT = 0.1F;
    static constexpr float FREE_TILES_WEIGHT = 2.7F;
    static constexpr float MERGE_WEIGHT = 1.0F;
    static constexpr float TILE_PLACEMENT_WEIGHT = 1.0F;

    /**
     * @brief 线程主函数
     */
    void run();

    /**
     * @brief Expectimax算法核心函数
     * @param board 当前棋盘状态
     * @param depth 当前搜索深度
     * @param maximizingPlayer 是否是最大化玩家的回合
     * @return 期望得分
     */
    float expectimax(const BitBoard& board, int depth, bool maximizingPlayer);

    /**
     * @brief 评估棋盘状态
     * @param board 棋盘状态
     * @return 评估分数
     */
    float evaluateBoard(const BitBoard& board);
};

#endif // EXPECTIMAX_WORKER_H
