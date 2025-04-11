#ifndef MCTSWORKER_H
#define MCTSWORKER_H

#include "BitBoard.h"
#include "MCTSNode.h"

#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QThread>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include <unordered_map>

/**
 * @brief 蒙特卡洛树搜索工作器类
 * 
 * 使用蒙特卡洛树搜索算法为2048游戏提供AI决策
 */
class MCTSWorker : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit MCTSWorker(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MCTSWorker();

    /**
     * @brief 设置线程数
     * @param threadCount 线程数
     */
    void setThreadCount(int threadCount);

    /**
     * @brief 设置搜索时间限制
     * @param timeLimit 时间限制（毫秒）
     */
    void setTimeLimit(int timeLimit);

    /**
     * @brief 设置模拟次数限制
     * @param simulationLimit 模拟次数限制
     */
    void setSimulationLimit(int simulationLimit);

    /**
     * @brief 设置是否使用缓存
     * @param useCache 是否使用缓存
     */
    void setUseCache(bool useCache);

    /**
     * @brief 设置缓存大小
     * @param cacheSize 缓存大小
     */
    void setCacheSize(int cacheSize);

    /**
     * @brief 清除缓存
     */
    void clearCache();

    /**
     * @brief 获取最佳移动方向
     * @param board 当前棋盘状态
     * @return 最佳移动方向
     */
    BitBoard::Direction getBestMove(const BitBoard& board);

    /**
     * @brief 停止搜索
     */
    void stopSearch();

signals:
    /**
     * @brief 搜索进度信号
     * @param progress 进度（0-100）
     */
    void searchProgress(int progress);

    /**
     * @brief 搜索完成信号
     * @param direction 最佳移动方向
     */
    void searchComplete(int direction);

private:
    /**
     * @brief 执行MCTS搜索
     * @param rootNode 根节点
     * @param threadId 线程ID
     */
    void runMCTS(MCTSNode* rootNode, int threadId);

    /**
     * @brief 从缓存中获取最佳移动
     * @param board 当前棋盘状态
     * @param direction 最佳移动方向（输出参数）
     * @return 是否命中缓存
     */
    bool getCachedMove(const BitBoard& board, BitBoard::Direction& direction);

    /**
     * @brief 将最佳移动添加到缓存
     * @param board 当前棋盘状态
     * @param direction 最佳移动方向
     */
    void addToCache(const BitBoard& board, BitBoard::Direction direction);

    /**
     * @brief 混合策略：结合MCTS和启发式评估
     * @param board 当前棋盘状态
     * @return 最佳移动方向
     */
    BitBoard::Direction hybridStrategy(const BitBoard& board);

    /**
     * @brief 启发式评估函数
     * @param board 当前棋盘状态
     * @return 评估分数
     */
    float evaluateBoard(const BitBoard& board);

    int threadCount_;                      // 线程数
    int timeLimit_;                        // 时间限制（毫秒）
    int simulationLimit_;                  // 模拟次数限制
    bool useCache_;                        // 是否使用缓存
    int cacheSize_;                        // 缓存大小
    std::atomic<bool> stopRequested_;      // 是否请求停止搜索
    QThreadPool threadPool_;               // 线程池

    // 缓存：棋盘状态 -> 最佳移动方向
    std::unordered_map<uint64_t, BitBoard::Direction> moveCache_;
    QReadWriteLock cacheLock_;             // 缓存读写锁

    // 统计信息
    std::atomic<int> totalSimulations_;    // 总模拟次数
    std::atomic<int> cacheHits_;           // 缓存命中次数
    std::atomic<int> cacheMisses_;         // 缓存未命中次数
};

#endif // MCTSWORKER_H
