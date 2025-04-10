#include "merge.h"

#include <algorithm>

namespace Merge {

// 从BitBoard中获取指定位置的值
inline int getBitBoardValue(BitBoard board, int row, int col) {
    int shift = 4 * (4 * row + col);
    return (board >> shift) & 0xF;
}

float calculateMergeScore(BitBoard board) {
    // 计算水平和垂直方向的合并机会总分
    return calculateHorizontalMergeScore(board) + calculateVerticalMergeScore(board);
}

float calculateHorizontalMergeScore(BitBoard board) {
    float score = 0.0F;

    // 检查水平合并机会
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            int current = getBitBoardValue(board, i, j);
            int next = getBitBoardValue(board, i, j + 1);

            if (current != 0 && current == next) {
                // 对于相同值的相邻方块，加分
                // 使用2的幂作为方块值，值越大的方块合并奖励越高
                score += (1 << current);
            }
        }
    }

    return score;
}

float calculateVerticalMergeScore(BitBoard board) {
    float score = 0.0F;

    // 检查垂直合并机会
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            int current = getBitBoardValue(board, i, j);
            int next = getBitBoardValue(board, i + 1, j);

            if (current != 0 && current == next) {
                // 对于相同值的相邻方块，加分
                score += (1 << current);
            }
        }
    }

    return score;
}

}  // namespace Merge
