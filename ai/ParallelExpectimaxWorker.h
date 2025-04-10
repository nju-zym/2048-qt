#ifndef PARALLEL_EXPECTIMAX_WORKER_H
#define PARALLEL_EXPECTIMAX_WORKER_H

#include "BitBoard.h"

#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <QWaitCondition>
#include <QtGlobal>
#include <array>
#include <unordered_map>

// 包含 QtConcurrent
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#    include <QtConcurrent>
#else
#    include <QtConcurrent/QtConcurrentRun>
#endif

/**
 * @brief 方向评估结果结构体
 */
struct DirectionScore {
    int direction;
    float score;
    bool valid;

    DirectionScore() : direction(-1), score(-std::numeric_limits<float>::infinity()), valid(false) {}
    DirectionScore(int dir, float s, bool v) : direction(dir), score(s), valid(v) {}
};

/**
 * @brief 并行Expectimax算法工作线程类
 *
 * 使用多线程并行计算不同移动方向的Expectimax算法
 */
class ParallelExpectimaxWorker : public QObject {
    Q_OBJECT

   public:
    explicit ParallelExpectimaxWorker(QObject* parent = nullptr);
    ~ParallelExpectimaxWorker();

    /**
     * @brief 开始计算最佳移动
     * @param board 当前棋盘状态
     * @param depth 搜索深度
     */
    void calculateBestMove(BitBoard const& board, int depth);

    /**
     * @brief 停止当前计算
     */
    void stopCalculation();

    /**
     * @brief 设置线程数量
     * @param count 线程数量
     */
    void setThreadCount(int count);

    /**
     * @brief 设置是否使用Alpha-Beta剪枝
     * @param useAlphaBeta 是否使用Alpha-Beta剪枝
     */
    void setUseAlphaBeta(bool useAlphaBeta);

    /**
     * @brief 获取是否使用Alpha-Beta剪枝
     * @return 是否使用Alpha-Beta剪枝
     */
    bool getUseAlphaBeta() const;

    /**
     * @brief 设置是否使用缓存
     * @param useCache 是否使用缓存
     */
    void setUseCache(bool useCache);

    /**
     * @brief 获取是否使用缓存
     * @return 是否使用缓存
     */
    bool getUseCache() const;

    /**
     * @brief 设置缓存大小
     * @param size 缓存大小
     */
    void setCacheSize(size_t size);

    /**
     * @brief 获取缓存大小
     * @return 缓存大小
     */
    size_t getCacheSize() const;

    /**
     * @brief 清除缓存
     */
    void clearCache();

    /**
     * @brief 设置是否使用增强评估函数
     * @param useEnhancedEval 是否使用增强评估函数
     */
    void setUseEnhancedEval(bool useEnhancedEval);

    /**
     * @brief 获取是否使用增强评估函数
     * @return 是否使用增强评估函数
     */
    bool getUseEnhancedEval() const;

    /**
     * @brief 获取线程数量
     * @return 线程数量
     */
    int getThreadCount() const;

   signals:
    /**
     * @brief 当计算出最佳移动时发出的信号
     * @param direction 移动方向（0=上, 1=右, 2=下, 3=左）
     */
    void moveCalculated(int direction);

   private:
    QThread workerThread;
    mutable QMutex mutex;
    QWaitCondition condition;
    bool abort;
    bool restart;
    BitBoard currentBoard;
    int searchDepth;
    int threadCount;
    QThreadPool threadPool;

    // 算法配置
    bool useAlphaBeta    = true;
    bool useCache        = true;
    bool useEnhancedEval = true;
    size_t cacheSize     = 1000000;

    // 缓存
    struct CacheKey {
        uint64_t boardHash;
        int depth;
        bool isMaximizingPlayer;

        bool operator==(CacheKey const& other) const {
            return boardHash == other.boardHash && depth == other.depth
                   && isMaximizingPlayer == other.isMaximizingPlayer;
        }
    };

    struct CacheKeyHash {
        std::size_t operator()(CacheKey const& key) const {
            return key.boardHash ^ (key.depth << 1) ^ key.isMaximizingPlayer;
        }
    };

    std::unordered_map<CacheKey, float, CacheKeyHash> cache;
    QMutex cacheMutex;

    // Future watchers
    QFutureWatcher<DirectionScore> upWatcher;
    QFutureWatcher<DirectionScore> rightWatcher;
    QFutureWatcher<DirectionScore> downWatcher;
    QFutureWatcher<DirectionScore> leftWatcher;

    // 评估函数权重
    static constexpr float MONOTONICITY_WEIGHT   = 1.0F;
    static constexpr float SMOOTHNESS_WEIGHT     = 0.1F;
    static constexpr float FREE_TILES_WEIGHT     = 2.7F;
    static constexpr float MERGE_WEIGHT          = 1.0F;
    static constexpr float TILE_PLACEMENT_WEIGHT = 1.0F;

    /**
     * @brief 线程主函数
     */
    void run();

    /**
     * @brief 评估单个方向
     * @param board 当前棋盘状态
     * @param direction 移动方向
     * @param depth 搜索深度
     * @return 方向评估结果
     */
    DirectionScore evaluateDirection(BitBoard const& board, int direction, int depth);

    /**
     * @brief Expectimax算法核心函数
     * @param board 当前棋盘状态
     * @param depth 当前搜索深度
     * @param maximizingPlayer 是否是最大化玩家的回合
     * @return 期望得分
     */
    float expectimax(BitBoard const& board, int depth, bool maximizingPlayer);

    /**
     * @brief 评估棋盘状态
     * @param board 棋盘状态
     * @return 评估分数
     */
    float evaluateBoard(BitBoard const& board);
};

#endif  // PARALLEL_EXPECTIMAX_WORKER_H
