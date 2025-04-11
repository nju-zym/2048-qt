#ifndef HYBRIDAI_H
#define HYBRIDAI_H

#include "AIInterface.h"
#include "HybridWorker.h"

#include <QObject>
#include <memory>

/**
 * @brief 混合策略AI类
 *
 * 使用混合策略（MCTS和Expectimax）为2048游戏提供AI决策
 */
class HybridAI : public AIInterface {
    Q_OBJECT

   public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit HybridAI(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~HybridAI();

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
    std::unique_ptr<HybridWorker> worker_;  // 混合策略工作器
};

#endif  // HYBRIDAI_H
