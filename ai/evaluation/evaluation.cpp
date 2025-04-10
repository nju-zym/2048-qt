#include "evaluation.h"

#include "edge.h"
#include "monotonicity.h"
#include "smoothness.h"
#include "snake.h"

#include <cmath>  // for std::log10

// Forward declarations from bitboard.h
extern int countEmptyTiles(BitBoard board);
extern int getBitBoardValue(BitBoard board, int row, int col);

namespace Evaluation {

// Forward declarations of functions in this file
float calculateEmptyTilesScore(BitBoard board);
float calculateCornerPotential(BitBoard board);
float calculateMergePotential(BitBoard board);
float calculateTileValuesScore(BitBoard board);

float evaluateBoard(BitBoard board,
                    float emptyWeight,
                    float monoWeight,
                    float smoothWeight,
                    float cornerWeight,
                    float snakeWeight,
                    float mergeWeight,
                    float tileWeight,
                    float edgeWeight) {
    // 计算空位分数
    float emptyScore = emptyWeight * calculateEmptyTilesScore(board);

    // 计算单调性分数
    float monoScore = monoWeight * Monotonicity::calculateMonotonicity(board);

    // 计算平滑性分数
    float smoothScore = smoothWeight * Smoothness::calculateSmoothness(board);

    // 计算角落策略分数
    float cornerScore = cornerWeight * calculateCornerPotential(board);

    // 计算蛇形排列分数
    float snakeScore = snakeWeight * Snake::calculateSnakePattern(board);

    // 计算合并机会分数
    float mergeScore = mergeWeight * calculateMergePotential(board);

    // 计算方块权重分数
    float tileScore = tileWeight * calculateTileValuesScore(board);

    // 计算边缘策略分数
    float edgeScore = edgeWeight * Edge::calculateEdgeScore(board);

    // 返回综合评分
    return emptyScore + monoScore + smoothScore + cornerScore + snakeScore + mergeScore + tileScore + edgeScore;
}

float calculateEmptyTilesScore(BitBoard board) {
    // 统计空位数量，每个空位都有积极意义
    int emptyCount = countEmptyTiles(board);

    // 空位计算方式：log(空位数量 + 1) * 10
    // 这样的函数具有边际递减性质，随着空位增多，每个额外空位的价值递减
    // +1是为了避免log(0)的情况
    // 使用log10而非log2，因为log10在标准库中更常见
    return emptyCount > 0 ? static_cast<float>(std::log10(emptyCount + 1) * 10.0) : 0.0f;
}

float calculateCornerPotential(BitBoard board) {
    // 计算角落策略分数，鼓励将大数字放在棋盘角落

    // 获取角落的值
    int topLeft     = getBitBoardValue(board, 0, 0);
    int topRight    = getBitBoardValue(board, 0, 3);
    int bottomLeft  = getBitBoardValue(board, 3, 0);
    int bottomRight = getBitBoardValue(board, 3, 3);

    // 计算最大角落值
    int maxCorner = topLeft;
    if (topRight > maxCorner) {
        maxCorner = topRight;
    }
    if (bottomLeft > maxCorner) {
        maxCorner = bottomLeft;
    }
    if (bottomRight > maxCorner) {
        maxCorner = bottomRight;
    }

    // 如果最大值在角落，则加分
    float cornerScore = 0.0f;

    if (topLeft == maxCorner) {
        cornerScore += static_cast<float>(topLeft);
    }
    if (topRight == maxCorner) {
        cornerScore += static_cast<float>(topRight);
    }
    if (bottomLeft == maxCorner) {
        cornerScore += static_cast<float>(bottomLeft);
    }
    if (bottomRight == maxCorner) {
        cornerScore += static_cast<float>(bottomRight);
    }

    // 额外惩罚小数值出现在角落的情况
    float penalty = 0.0f;
    if (topLeft > 0 && topLeft < 3) {
        penalty -= 10.0f;
    }
    if (topRight > 0 && topRight < 3) {
        penalty -= 10.0f;
    }
    if (bottomLeft > 0 && bottomLeft < 3) {
        penalty -= 10.0f;
    }
    if (bottomRight > 0 && bottomRight < 3) {
        penalty -= 10.0f;
    }

    return cornerScore + penalty;
}

float calculateMergePotential(BitBoard board) {
    // 计算合并潜力，即相邻相同数字的数量
    float mergePotential = 0.0f;

    // 检查水平合并可能
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row, col + 1);

            if (value1 > 0 && value1 == value2) {
                // 相邻相同数字，增加合并潜力
                // 值越大，合并收益越高
                mergePotential += static_cast<float>(value1);
            }
        }
    }

    // 检查垂直合并可能
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row + 1, col);

            if (value1 > 0 && value1 == value2) {
                mergePotential += static_cast<float>(value1);
            }
        }
    }

    return mergePotential;
}

float calculateTileValuesScore(BitBoard board) {
    // 计算方块值的得分，较大的方块有更高的权重
    float tileScore = 0.0f;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getBitBoardValue(board, row, col);
            if (value > 0) {
                // 更高的方块值给予更多分数
                // 使用2的幂来计算，使得高值更有价值
                tileScore += static_cast<float>(1U << value);
            }
        }
    }

    return tileScore;
}

}  // namespace Evaluation