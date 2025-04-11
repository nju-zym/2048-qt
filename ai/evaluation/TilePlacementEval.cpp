#include "TilePlacementEval.h"

#include <cmath>

// 位置权重矩阵，鼓励将高价值方块放在角落
// 左上角有最高权重，权重沿对角线递减
float const TilePlacementEval::POSITION_WEIGHTS[4][4]
    = {{4.0F, 3.0F, 1.0F, 0.0F}, {3.0F, 2.0F, 0.0F, -1.0F}, {1.0F, 0.0F, -2.0F, -3.0F}, {0.0F, -1.0F, -3.0F, -4.0F}};

float TilePlacementEval::evaluate(BitBoard const& board) {
    float score = 0.0F;

    // 获取棋盘上的最大值
    uint32_t maxTile = 0;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            uint32_t tileValue = board.getTile(row, col);
            if (tileValue > maxTile) {
                maxTile = tileValue;
            }
        }
    }

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = board.getTile(row, col);
            if (value > 0) {
                // 对数值越大的方块，位置权重的影响越大
                auto logValue = static_cast<float>(std::log2(value));

                // 如果是大数字，位置更重要
                float weightMultiplier = 1.0F;
                if (value >= maxTile / 2) {
                    weightMultiplier = 2.0F;  // 大数字的位置更重要
                } else if (value >= maxTile / 4) {
                    weightMultiplier = 1.5F;  // 中等大小的数字
                }

                score += POSITION_WEIGHTS[row][col] * logValue * weightMultiplier;
            }
        }
    }

    return score;
}
