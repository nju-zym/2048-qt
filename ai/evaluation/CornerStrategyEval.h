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
    static float evaluate(BitBoard const& board);

   private:
    /**
     * @brief 计算角落权重
     *
     * 鼓励将大数字放在角落，并形成递减序列
     *
     * @param board 棋盘
     * @return 角落权重分数
     */
    static float calculateCornerWeight(BitBoard const& board);

    /**
     * @brief 计算梯度权重
     *
     * 鼓励数字形成梯度，从角落开始逐渐减小
     *
     * @param board 棋盘
     * @return 梯度权重分数
     */
    static float calculateGradientWeight(BitBoard const& board);
};

#endif  // CORNER_STRATEGY_EVAL_H
