#ifndef MONOTONICITY_EVAL_H
#define MONOTONICITY_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 单调性评估类
 * 
 * 评估棋盘的单调性，即方块值在行和列上的递增或递减程度
 * 单调性好的棋盘更容易合并方块
 */
class MonotonicityEval {
public:
    /**
     * @brief 评估棋盘的单调性
     * @param board 位棋盘
     * @return 单调性评分，值越高表示单调性越好
     */
    static float evaluate(const BitBoard& board);

private:
    // 辅助函数，计算一行或一列的单调性
    static float calculateMonotonicity(int a, int b, int c, int d);
};

#endif // MONOTONICITY_EVAL_H
