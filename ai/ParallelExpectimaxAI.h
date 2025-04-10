#ifndef PARALLEL_EXPECTIMAX_AI_H
#define PARALLEL_EXPECTIMAX_AI_H

#include "AIInterface.h"

#include <QMutex>
#include <QRandomGenerator>
#include <QTime>
#include <QWaitCondition>

// 前向声明
class ParallelExpectimaxWorker;

/**
 * @brief 并行Expectimax算法AI实现
 *
 * 使用多线程并行计算不同移动方向的Expectimax算法
 */
class ParallelExpectimaxAI : public AIInterface {
    Q_OBJECT

   public:
    /**
     * @brief 构造函数
     * @param depth 搜索深度
     * @param threadCount 线程数量，默认为系统核心数
     * @param parent 父对象
     */
    explicit ParallelExpectimaxAI(int depth = 3, int threadCount = -1, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ParallelExpectimaxAI();

    /**
     * @brief 获取下一步最佳移动方向
     * @param board 当前游戏棋盘
     * @return 最佳移动方向（0=上, 1=右, 2=下, 3=左）
     */
    int getBestMove(GameBoard const& board) override;

    /**
     * @brief 设置AI的思考深度
     * @param depth 搜索深度
     */
    void setDepth(int depth) override;

    /**
     * @brief 获取当前AI的思考深度
     * @return 当前搜索深度
     */
    int getDepth() const override;

    /**
     * @brief 获取AI算法名称
     * @return 算法名称
     */
    QString getName() const override;

    /**
     * @brief 设置线程数量
     * @param count 线程数量
     */
    void setThreadCount(int count);

    /**
     * @brief 获取线程数量
     * @return 线程数量
     */
    int getThreadCount() const;

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
     * @brief 设置是否使用动态深度调整
     * @param useDynamicDepth 是否使用动态深度调整
     */
    void setUseDynamicDepth(bool useDynamicDepth);

    /**
     * @brief 获取是否使用动态深度调整
     * @return 是否使用动态深度调整
     */
    bool getUseDynamicDepth() const;

    /**
     * @brief 设置最小搜索深度
     * @param minDepth 最小搜索深度
     */
    void setMinDepth(int minDepth);

    /**
     * @brief 获取最小搜索深度
     * @return 最小搜索深度
     */
    int getMinDepth() const;

    /**
     * @brief 设置最大搜索深度
     * @param maxDepth 最大搜索深度
     */
    void setMaxDepth(int maxDepth);

    /**
     * @brief 获取最大搜索深度
     * @return 最大搜索深度
     */
    int getMaxDepth() const;

   private slots:
    /**
     * @brief 处理工作线程计算出的移动
     * @param direction 移动方向
     */
    void onMoveCalculated(int direction);

   private:
    int depth;                 // 搜索深度
    int threadCount;           // 线程数量
    int lastCalculatedMove;    // 最后计算出的移动
    bool moveReady;            // 移动是否已经准备好
    mutable QMutex mutex;      // 互斥锁
    QWaitCondition condition;  // 条件变量

    // 动态深度调整相关
    bool useDynamicDepth;  // 是否使用动态深度调整
    int minDepth;          // 最小搜索深度
    int maxDepth;          // 最大搜索深度

    // 工作线程
    ParallelExpectimaxWorker* worker;

    /**
     * @brief 计算动态搜索深度
     * @param board 当前游戏棋盘
     * @return 动态搜索深度
     */
    int calculateDynamicDepth(GameBoard const& board) const;
};

#endif  // PARALLEL_EXPECTIMAX_AI_H
