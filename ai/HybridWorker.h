#ifndef HYBRIDWORKER_H
#define HYBRIDWORKER_H

#include "BitBoard.h"
#include "MCTSWorker.h"
#include "ParallelExpectimaxWorker.h"

#include <QObject>
#include <QReadWriteLock>
#include <QThread>
#include <atomic>
#include <memory>
#include <unordered_map>

/**
 * @brief 混合策略工作器类
 * 
 * 结合MCTS和Expectimax算法为2048游戏提供AI决策
 */
class HybridWorker : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit HybridWorker(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~HybridWorker();

    /**
     * @brief 设置MCTS权重
     * @param weight MCTS权重（0-1）
     */
    void setMCTSWeight(float weight);

    /**
     * @brief 设置Expectimax权重
     * @param weight Expectimax权重（0-1）
     */
    void setExpectimaxWeight(float weight);

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
     * @brief 动态选择算法
     * @param board 当前棋盘状态
     * @return 最佳移动方向
     */
    BitBoard::Direction dynamicAlgorithmSelection(const BitBoard& board);

    float mctsWeight_;                     // MCTS权重
    float expectimaxWeight_;               // Expectimax权重
    int threadCount_;                      // 线程数
    int timeLimit_;                        // 时间限制（毫秒）
    bool useCache_;                        // 是否使用缓存
    int cacheSize_;                        // 缓存大小
    std::atomic<bool> stopRequested_;      // 是否请求停止搜索

    // 算法工作器
    std::unique_ptr<MCTSWorker> mctsWorker_;
    std::unique_ptr<ParallelExpectimaxWorker> expectimaxWorker_;

    // 缓存：棋盘状态 -> 最佳移动方向
    std::unordered_map<uint64_t, BitBoard::Direction> moveCache_;
    QReadWriteLock cacheLock_;             // 缓存读写锁

    // 统计信息
    std::atomic<int> cacheHits_;           // 缓存命中次数
    std::atomic<int> cacheMisses_;         // 缓存未命中次数
};

#endif // HYBRIDWORKER_H
