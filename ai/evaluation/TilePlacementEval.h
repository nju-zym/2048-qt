#ifndef TILE_PLACEMENT_EVAL_H
#define TILE_PLACEMENT_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 方块位置评估类
 * 
 * 评估方块在棋盘上的位置，鼓励将高价值方块放在角落
 */
class TilePlacementEval {
public:
    /**
     * @brief 评估方块的位置
     * @param board 位棋盘
     * @return 位置评分，值越高表示位置越好
     */
    static float evaluate(const BitBoard& board);

private:
    // 位置权重矩阵，鼓励将高价值方块放在角落
    static const float POSITION_WEIGHTS[4][4];
};

#endif // TILE_PLACEMENT_EVAL_H
