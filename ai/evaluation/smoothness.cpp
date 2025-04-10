#include "smoothness.h"

#include <algorithm>
#include <cmath>

namespace Smoothness {

// 从BitBoard中获取指定位置的值
inline int getBitBoardValue(BitBoard board, int row, int col) {
    int shift = 4 * (4 * row + col);
    return (board >> shift) & 0xF;
}

float calculateSmoothness(BitBoard board) {
    // 计算棋盘平滑度，即相邻格子数值的差异程度
    float smoothness = 0;

    // 计算水平方向平滑度
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row, col + 1);

            // 只计算非零数值之间的平滑度
            if (value1 != 0 && value2 != 0) {
                // 计算差异的幅度，取对数可以更好地反映相对差异
                float diff  = std::abs(value1 - value2);
                smoothness -= diff;
            }
        }
    }

    // 计算垂直方向平滑度
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row + 1, col);

            // 只计算非零数值之间的平滑度
            if (value1 != 0 && value2 != 0) {
                float diff  = std::abs(value1 - value2);
                smoothness -= diff;
            }
        }
    }

    return smoothness;
}

float calculateWeightedSmoothness(BitBoard board) {
    // 加权平滑度：方块值越大，平滑度影响越大
    float weightedSmoothness = 0;

    // 计算水平方向加权平滑度
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row, col + 1);

            // 只计算非零数值之间的平滑度
            if (value1 != 0 && value2 != 0) {
                // 差值越大，平滑度越差，乘以方块值使大数字的差异更受关注
                float weight        = std::max(value1, value2);
                float diff          = std::abs(value1 - value2) * weight;
                weightedSmoothness -= diff;
            }
        }
    }

    // 计算垂直方向加权平滑度
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row + 1, col);

            // 只计算非零数值之间的平滑度
            if (value1 != 0 && value2 != 0) {
                float weight        = std::max(value1, value2);
                float diff          = std::abs(value1 - value2) * weight;
                weightedSmoothness -= diff;
            }
        }
    }

    return weightedSmoothness;
}

float calculateLogSmoothness(BitBoard board) {
    // 对数平滑度：使用对数来衡量差异
    float logSmoothness = 0;

    // 计算水平方向平滑度
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row, col + 1);

            // 只计算非零数值之间的平滑度
            if (value1 != 0 && value2 != 0) {
                // 使用对数差异，更符合人类对数字大小的感知
                float logDiff  = std::abs(std::log2(value1) - std::log2(value2));
                logSmoothness -= logDiff;
            }
        }
    }

    // 计算垂直方向平滑度
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row + 1, col);

            // 只计算非零数值之间的平滑度
            if (value1 != 0 && value2 != 0) {
                float logDiff  = std::abs(std::log2(value1) - std::log2(value2));
                logSmoothness -= logDiff;
            }
        }
    }

    return logSmoothness;
}

}  // namespace Smoothness