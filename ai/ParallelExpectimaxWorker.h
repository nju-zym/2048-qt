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

    /**
     * @brief 设置是否使用动态深度调整
     * @param useDynamicDepth 是否使用动态深度调整
     */
    void setUseDynamicDepth(bool useDynamicDepth);

    /**
     * @brief 获取是否使用动态深度调整
     * @return 是否使用动态深度调整
     */
    bool getUseDynamicDepth() const;

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
    bool useDynamicDepth = true;      // 是否使用动态深度调整
    bool useWorkStealing = true;      // 是否使用工作窃取
    size_t cacheSize     = 10000000;  // 增加缓存大小从1M到10M
    size_t batchSize     = 0;         // 批处理大小，0表示自动计算

    // 缓存
    struct CacheKey {
        uint64_t boardHash;
        int depth;
        bool isMaximizingPlayer;
        float alpha;  // 添加Alpha值
        float beta;   // 添加Beta值

        bool operator==(CacheKey const& other) const {
            return boardHash == other.boardHash && depth == other.depth
                   && isMaximizingPlayer == other.isMaximizingPlayer && alpha == other.alpha && beta == other.beta;
        }
    };

    struct CacheKeyHash {
        std::size_t operator()(CacheKey const& key) const {
            // 使用更好的哈希函数
            std::size_t h1 = std::hash<uint64_t>{}(key.boardHash);
            std::size_t h2 = std::hash<int>{}(key.depth);
            std::size_t h3 = std::hash<bool>{}(key.isMaximizingPlayer);

            // 组合哈希值
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    // 缓存条目
    struct CacheEntry {
        float value;           // 缓存的值
        uint32_t accessCount;  // 访问计数
        uint32_t generation;   // 世代标记
    };

    // 缓存相关变量
    std::unordered_map<CacheKey, CacheEntry, CacheKeyHash> cache;
    uint32_t currentGeneration = 0;  // 当前缓存世代
    QMutex cacheMutex;
    QReadWriteLock cacheRWLock;  // 缓存读写锁，减少锁竞争

    // Future watchers
    QFutureWatcher<DirectionScore> upWatcher;
    QFutureWatcher<DirectionScore> rightWatcher;
    QFutureWatcher<DirectionScore> downWatcher;
    QFutureWatcher<DirectionScore> leftWatcher;

    // 评估函数权重 - 优化后的权重
    static constexpr float MONOTONICITY_WEIGHT             = 5.0F;  // 进一步增加单调性权重，这是2048最重要的策略
    static constexpr float SMOOTHNESS_WEIGHT               = 0.5F;  // 增加平滑性权重，有助于合并
    static constexpr float FREE_TILES_WEIGHT               = 3.0F;  // 增加空位权重，避免棋盘填满
    static constexpr float MERGE_WEIGHT                    = 2.0F;  // 增加合并权重，鼓励合并操作
    static constexpr float TILE_PLACEMENT_WEIGHT           = 2.0F;  // 增加方块位置权重，鼓励大数字在角落
    static constexpr float CORNER_STRATEGY_WEIGHT          = 5.0F;  // 进一步增加角落策略权重，这是高分的关键
    static constexpr float LARGE_NUMBERS_CONNECTION_WEIGHT = 4.0F;  // 增加大数字连接权重，有助于形成更大的数字
    static constexpr float RISK_WEIGHT                     = 3.0F;  // 增加风险评估权重，避免危险局面

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
     * @brief 带Alpha-Beta剪枝的Expectimax算法核心函数
     * @param board 当前棋盘状态
     * @param depth 当前搜索深度
     * @param maximizingPlayer 是否是最大化玩家的回合
     * @param alpha Alpha值
     * @param beta Beta值
     * @return 期望得分
     */
    float expectimax(BitBoard const& board, int depth, bool maximizingPlayer, float alpha, float beta);

    /**
     * @brief 评估棋盘状态
     * @param board 棋盘状态
     * @return 评估分数
     */
    float evaluateBoard(BitBoard const& board) const;

    /**
     * @brief 预热缓存，提前计算常见棋面
     */
    void preheateCache();

    /**
     * @brief 设置是否使用工作窃取
     * @param useWorkStealing 是否使用工作窃取
     */
    void setUseWorkStealing(bool useWorkStealing);

    /**
     * @brief 获取是否使用工作窃取
     * @return 是否使用工作窃取
     */
    bool getUseWorkStealing() const;

    /**
     * @brief 设置批处理大小
     * @param batchSize 批处理大小
     */
    void setBatchSize(size_t batchSize);

    /**
     * @brief 获取批处理大小
     * @return 批处理大小
     */
    size_t getBatchSize() const;
};

#endif  // PARALLEL_EXPECTIMAX_WORKER_H
