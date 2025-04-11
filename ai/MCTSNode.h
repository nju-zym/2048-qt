#ifndef MCTSNODE_H
#define MCTSNODE_H

#include "BitBoard.h"

#include <memory>
#include <mutex>
#include <random>
#include <unordered_map>
#include <vector>

/**
 * @brief 蒙特卡洛树搜索节点类
 * 
 * 实现蒙特卡洛树搜索算法的节点，用于2048游戏AI决策
 */
class MCTSNode {
public:
    /**
     * @brief 构造函数
     * @param board 当前棋盘状态
     * @param parent 父节点指针
     * @param move 从父节点到达此节点的移动
     * @param isChance 是否为机会节点（随机放置新方块的节点）
     */
    MCTSNode(BitBoard board, MCTSNode* parent = nullptr, int move = -1, bool isChance = false);

    /**
     * @brief 析构函数
     */
    ~MCTSNode();

    /**
     * @brief 选择最佳子节点
     * @return 最佳子节点指针
     */
    MCTSNode* selectBestChild();

    /**
     * @brief 扩展当前节点
     */
    void expand();

    /**
     * @brief 模拟游戏直到结束
     * @return 模拟得分
     */
    float simulate();

    /**
     * @brief 反向传播模拟结果
     * @param score 模拟得分
     */
    void backpropagate(float score);

    /**
     * @brief 获取最佳移动方向
     * @return 最佳移动方向
     */
    int getBestMove() const;

    /**
     * @brief 获取当前节点的UCB值
     * @param explorationConstant 探索常数
     * @return UCB值
     */
    float getUCB(float explorationConstant) const;

    /**
     * @brief 是否为叶节点
     * @return 是否为叶节点
     */
    bool isLeaf() const;

    /**
     * @brief 是否为终止节点
     * @return 是否为终止节点
     */
    bool isTerminal() const;

    /**
     * @brief 获取访问次数
     * @return 访问次数
     */
    int getVisitCount() const;

    /**
     * @brief 获取总得分
     * @return 总得分
     */
    float getTotalScore() const;

    /**
     * @brief 获取当前棋盘状态
     * @return 当前棋盘状态
     */
    BitBoard getBoard() const;

private:
    BitBoard board_;                      // 当前棋盘状态
    MCTSNode* parent_;                    // 父节点指针
    std::vector<MCTSNode*> children_;     // 子节点列表
    int visitCount_;                      // 访问次数
    float totalScore_;                    // 总得分
    int move_;                            // 从父节点到达此节点的移动
    bool isChance_;                       // 是否为机会节点
    bool isFullyExpanded_;                // 是否已完全扩展
    std::mutex mutex_;                    // 互斥锁，用于线程安全

    // 随机数生成器
    static thread_local std::mt19937 rng_;
};

#endif // MCTSNODE_H
