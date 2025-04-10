#include "snake.h"

#include <algorithm>

namespace Snake {

// 从BitBoard中获取指定位置的值
inline int getBitBoardValue(BitBoard board, int row, int col) {
    int shift = 4 * (4 * row + col);
    return (board >> shift) & 0xF;
}

float calculateSnakePattern(BitBoard board) {
    // 蛇形模式得分
    float score = 0;

    // 四种可能的蛇形路径权重
    score = std::max(std::max(calculateTopLeftSnake(board), calculateTopRightSnake(board)),
                     std::max(calculateBottomLeftSnake(board), calculateBottomRightSnake(board)));

    return score;
}

// 计算角落策略得分 - 鼓励高值方块放在角落
float calculateCornerScore(BitBoard board) {
    // 角落权重矩阵 - 角落权重高，中心权重低
    int const weights[4][4] = {{3, 2, 2, 3}, {2, 1, 1, 2}, {2, 1, 1, 2}, {3, 2, 2, 3}};

    float score = 0;

    // 计算每个位置的得分贡献
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getBitBoardValue(board, row, col);
            if (value > 0) {
                // 使用2的幂作为方块值
                score += (1 << value) * weights[row][col];
            }
        }
    }

    return score;
}

float calculateTopLeftSnake(BitBoard board) {
    // 从左上角开始的蛇形模式
    // 权重模式：
    // 15 14 13 12
    // 8  9  10 11
    // 7  6  5  4
    // 0  1  2  3
    float score = 0;

    // 定义蛇形权重矩阵
    int const weights[4][4] = {{15, 14, 13, 12}, {8, 9, 10, 11}, {7, 6, 5, 4}, {0, 1, 2, 3}};

    // 计算蛇形得分
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getBitBoardValue(board, row, col);
            if (value > 0) {
                // 乘以权重和方块值
                score += value * weights[row][col];
            }
        }
    }

    return score;
}

float calculateTopRightSnake(BitBoard board) {
    // 从右上角开始的蛇形模式
    // 权重模式：
    // 12 13 14 15
    // 11 10 9  8
    // 4  5  6  7
    // 3  2  1  0
    float score = 0;

    // 定义蛇形权重矩阵
    int const weights[4][4] = {{12, 13, 14, 15}, {11, 10, 9, 8}, {4, 5, 6, 7}, {3, 2, 1, 0}};

    // 计算蛇形得分
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getBitBoardValue(board, row, col);
            if (value > 0) {
                score += value * weights[row][col];
            }
        }
    }

    return score;
}

float calculateBottomLeftSnake(BitBoard board) {
    // 从左下角开始的蛇形模式
    // 权重模式：
    // 0  1  2  3
    // 7  6  5  4
    // 8  9  10 11
    // 15 14 13 12
    float score = 0;

    // 定义蛇形权重矩阵
    int const weights[4][4] = {{0, 1, 2, 3}, {7, 6, 5, 4}, {8, 9, 10, 11}, {15, 14, 13, 12}};

    // 计算蛇形得分
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getBitBoardValue(board, row, col);
            if (value > 0) {
                score += value * weights[row][col];
            }
        }
    }

    return score;
}

float calculateBottomRightSnake(BitBoard board) {
    // 从右下角开始的蛇形模式
    // 权重模式：
    // 3  2  1  0
    // 4  5  6  7
    // 11 10 9  8
    // 12 13 14 15
    float score = 0;

    // 定义蛇形权重矩阵
    int const weights[4][4] = {{3, 2, 1, 0}, {4, 5, 6, 7}, {11, 10, 9, 8}, {12, 13, 14, 15}};

    // 计算蛇形得分
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getBitBoardValue(board, row, col);
            if (value > 0) {
                score += value * weights[row][col];
            }
        }
    }

    return score;
}

}  // namespace Snake