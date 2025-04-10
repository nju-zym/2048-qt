#ifndef FREE_TILES_EVAL_H
#define FREE_TILES_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 空位数量评估类
 * 
 * 评估棋盘上的空位数量
 * 空位越多，游戏状态越好，有更多的操作空间
 */
class FreeTilesEval {
public:
    /**
     * @brief 评估棋盘的空位数量
     * @param board 位棋盘
     * @return 空位数量评分，值越高表示空位越多
     */
    static float evaluate(const BitBoard& board);
};

#endif // FREE_TILES_EVAL_H
