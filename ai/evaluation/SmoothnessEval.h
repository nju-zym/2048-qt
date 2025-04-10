#ifndef SMOOTHNESS_EVAL_H
#define SMOOTHNESS_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 平滑性评估类
 * 
 * 评估棋盘的平滑性，即相邻方块之间值的差异程度
 * 平滑性好的棋盘更容易合并方块
 */
class SmoothnessEval {
public:
    /**
     * @brief 评估棋盘的平滑性
     * @param board 位棋盘
     * @return 平滑性评分，值越高表示平滑性越好
     */
    static float evaluate(const BitBoard& board);
};

#endif // SMOOTHNESS_EVAL_H
