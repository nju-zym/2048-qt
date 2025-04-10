#ifndef CORNER_STRATEGY_EVAL_H
#define CORNER_STRATEGY_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 角落策略评估类
 * 
 * 评估棋盘上的数字是否形成了一个以角落为中心的稳定结构
 */
class CornerStrategyEval {
public:
    /**
     * @brief 评估棋盘
     * @param board 棋盘
     * @return 评估分数
     */
    static float evaluate(const BitBoard& board);
    
private:
    /**
     * @brief 计算蛇形模式的分数
     * 
     * 蛇形模式是指数字按照从大到小的顺序排列，形成一个蛇形
     * 例如：
     * 16 8  4  2
     * 32 64 128 4
     * 2  4  8  16
     * 4  2  4  8
     * 
     * @param board 棋盘
     * @return 蛇形模式的分数
     */
    static float calculateSnakePattern(const BitBoard& board);
    
    /**
     * @brief 计算角落权重
     * 
     * 鼓励将大数字放在角落
     * 
     * @param board 棋盘
     * @return 角落权重分数
     */
    static float calculateCornerWeight(const BitBoard& board);
    
    /**
     * @brief 计算梯度权重
     * 
     * 鼓励数字形成梯度，从角落开始逐渐减小
     * 
     * @param board 棋盘
     * @return 梯度权重分数
     */
    static float calculateGradientWeight(const BitBoard& board);
};

#endif // CORNER_STRATEGY_EVAL_H
