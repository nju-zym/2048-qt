#include "TilePlacementEval.h"
#include <cmath>

// 位置权重矩阵，鼓励将高价值方块放在角落
// 左上角有最高权重，权重沿对角线递减
const float TilePlacementEval::POSITION_WEIGHTS[4][4] = {
    {3.0f, 2.0f, 1.0f, 0.0f},
    {2.0f, 1.0f, 0.0f, -1.0f},
    {1.0f, 0.0f, -1.0f, -2.0f},
    {0.0f, -1.0f, -2.0f, -3.0f}
};

float TilePlacementEval::evaluate(const BitBoard& board) {
    float score = 0.0f;
    
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = board.getTile(row, col);
            if (value > 0) {
                // 对数值越大的方块，位置权重的影响越大
                float logValue = std::log2(value);
                score += POSITION_WEIGHTS[row][col] * logValue;
            }
        }
    }
    
    return score;
}
