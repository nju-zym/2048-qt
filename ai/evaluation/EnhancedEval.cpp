#include "EnhancedEval.h"

#include "FreeTilesEval.h"
#include "MergeEval.h"
#include "MonotonicityEval.h"
#include "SmoothnessEval.h"
#include "TilePlacementEval.h"

#include <algorithm>
#include <cmath>

float EnhancedEval::evaluate(BitBoard const& board) {
    float score = 0.0f;

    // 单调性评估
    score += MONOTONICITY_WEIGHT * MonotonicityEval::evaluate(board);

    // 平滑性评估
    score += SMOOTHNESS_WEIGHT * SmoothnessEval::evaluate(board);

    // 空位数量评估
    score += FREE_TILES_WEIGHT * FreeTilesEval::evaluate(board);

    // 合并可能性评估
    score += MERGE_WEIGHT * MergeEval::evaluate(board);

    // 方块位置评估
    score += TILE_PLACEMENT_WEIGHT * TilePlacementEval::evaluate(board);

    // 角落最大值评估
    score += CORNER_MAX_WEIGHT * evaluateCornerMax(board);

    // 蛇形模式评估
    score += SNAKE_PATTERN_WEIGHT * evaluateSnakePattern(board);

    return score;
}

float EnhancedEval::evaluateCornerMax(BitBoard const& board) {
    // 找出最大值
    int maxValue = 0;
    int maxRow   = -1;
    int maxCol   = -1;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = board.getTile(row, col);
            if (value > maxValue) {
                maxValue = value;
                maxRow   = row;
                maxCol   = col;
            }
        }
    }

    // 如果最大值在角落，给予奖励
    if ((maxRow == 0 || maxRow == 3) && (maxCol == 0 || maxCol == 3)) {
        return 1.0f;
    }

    // 如果最大值在边缘，给予较小奖励
    if (maxRow == 0 || maxRow == 3 || maxCol == 0 || maxCol == 3) {
        return 0.5f;
    }

    // 如果最大值在中间，给予惩罚
    return -0.5f;
}

float EnhancedEval::evaluateSnakePattern(BitBoard const& board) {
    // 蛇形模式评估
    // 理想的蛇形模式：
    // 15 14 13 12
    // 8  9  10 11
    // 7  6  5  4
    // 0  1  2  3

    float score = 0.0f;

    // 检查是否符合蛇形模式
    // 第一行：从左到右递减
    bool row1Decreasing = true;
    for (int col = 0; col < 3; ++col) {
        if (board.getTile(0, col) < board.getTile(0, col + 1)) {
            row1Decreasing = false;
            break;
        }
    }

    // 第二行：从右到左递减
    bool row2Decreasing = true;
    for (int col = 3; col > 0; --col) {
        if (board.getTile(1, col) < board.getTile(1, col - 1)) {
            row2Decreasing = false;
            break;
        }
    }

    // 第三行：从左到右递减
    bool row3Decreasing = true;
    for (int col = 0; col < 3; ++col) {
        if (board.getTile(2, col) < board.getTile(2, col + 1)) {
            row3Decreasing = false;
            break;
        }
    }

    // 第四行：从右到左递减
    bool row4Decreasing = true;
    for (int col = 3; col > 0; --col) {
        if (board.getTile(3, col) < board.getTile(3, col - 1)) {
            row4Decreasing = false;
            break;
        }
    }

    // 计算符合蛇形模式的行数
    int matchingRows = 0;
    if (row1Decreasing) {
        matchingRows++;
    }
    if (row2Decreasing) {
        matchingRows++;
    }
    if (row3Decreasing) {
        matchingRows++;
    }
    if (row4Decreasing) {
        matchingRows++;
    }

    // 根据符合蛇形模式的行数给予奖励
    score = static_cast<float>(matchingRows) / 4.0f;

    return score;
}
