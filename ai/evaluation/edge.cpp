#include "edge.h"

#include <algorithm>
#include <cmath>

// Forward declaration from bitboard.h
extern int getBitBoardValue(BitBoard board, int row, int col);

namespace Edge {

float calculateEdgeScore(BitBoard board) {
    // 计算边缘策略得分，鼓励将大数字放在棋盘边缘
    float edgeScore = 0.0f;

    // 边缘权重矩阵 - 角落权重最高, 边缘权重高，中心权重低
    // 5 3 3 5
    // 3 1 1 3
    // 3 1 1 3
    // 5 3 3 5
    int const weights[4][4] = {{5, 3, 3, 5}, {3, 1, 1, 3}, {3, 1, 1, 3}, {5, 3, 3, 5}};

    // 获取最大值
    int maxValue = 0;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getBitBoardValue(board, row, col);
            maxValue  = std::max(maxValue, value);
        }
    }

    // 计算每个位置的得分贡献
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getBitBoardValue(board, row, col);
            if (value > 0) {
                // 使用2的幂作为方块值，并乘以位置权重
                // 大数字在边缘得分更高
                float tileScore = (1 << value) * weights[row][col];

                // 如果是最大值或接近最大值，额外奖励放在边缘和角落
                if (value >= maxValue - 1) {
                    // 如果在角落，给予更高的奖励
                    if ((row == 0 || row == 3) && (col == 0 || col == 3)) {
                        tileScore *= 2.5f;
                    }
                    // 如果在边缘但不在角落
                    else if (row == 0 || row == 3 || col == 0 || col == 3) {
                        tileScore *= 1.8f;
                    }
                }
                // 对于其他大值瓦片，也鼓励放在边缘
                else if (value >= 8) {  // 256及以上
                    if (row == 0 || row == 3 || col == 0 || col == 3) {
                        tileScore *= 1.5f;
                    }
                }

                edgeScore += tileScore;
            }
        }
    }

    return edgeScore;
}

}  // namespace Edge
