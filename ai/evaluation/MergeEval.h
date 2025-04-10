#ifndef MERGE_EVAL_H
#define MERGE_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 合并可能性评估类
 * 
 * 评估棋盘上相邻相同值方块的数量，即潜在的合并可能性
 */
class MergeEval {
public:
    /**
     * @brief 评估棋盘的合并可能性
     * @param board 位棋盘
     * @return 合并可能性评分，值越高表示合并可能性越大
     */
    static float evaluate(const BitBoard& board);
};

#endif // MERGE_EVAL_H
