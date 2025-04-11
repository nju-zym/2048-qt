#include "MonotonicityEval.h"

#include <algorithm>
#include <cmath>

float MonotonicityEval::evaluate(BitBoard const& board) {
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

    // 根据最大值调整单调性的重要性
    // 当最大值越大，单调性越重要
    float importanceFactor = 1.0F;
    if (maxTile >= 2048) {
        importanceFactor = 2.5F;
    } else if (maxTile >= 1024) {
        importanceFactor = 2.0F;
    } else if (maxTile >= 512) {
        importanceFactor = 1.5F;
    }

    // 计算行的单调性，优先考虑从左到右递减
    float rowScore = 0.0F;
    for (int row = 0; row < 4; ++row) {
        int a = board.getTile(row, 0) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(row, 0)));
        int b = board.getTile(row, 1) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(row, 1)));
        int c = board.getTile(row, 2) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(row, 2)));
        int d = board.getTile(row, 3) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(row, 3)));

        // 计算递减单调性（从左到右）
        float decreasingLeft = 0.0F;
        if (a > b) {
            decreasingLeft += static_cast<float>(a - b) * 2.5F;  // 增加权重
        }
        if (b > c) {
            decreasingLeft += static_cast<float>(b - c) * 2.0F;
        }
        if (c > d) {
            decreasingLeft += static_cast<float>(c - d) * 1.5F;
        }

        // 计算递减单调性（从右到左）
        float decreasingRight = 0.0F;
        if (d > c) {
            decreasingRight += static_cast<float>(d - c) * 2.5F;  // 增加权重
        }
        if (c > b) {
            decreasingRight += static_cast<float>(c - b) * 2.0F;
        }
        if (b > a) {
            decreasingRight += static_cast<float>(b - a) * 1.5F;
        }

        // 选择更好的单调性方向
        rowScore += std::max(decreasingLeft, decreasingRight);
    }

    // 计算列的单调性，优先考虑从上到下递减
    float colScore = 0.0F;
    for (int col = 0; col < 4; ++col) {
        int a = board.getTile(0, col) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(0, col)));
        int b = board.getTile(1, col) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(1, col)));
        int c = board.getTile(2, col) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(2, col)));
        int d = board.getTile(3, col) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(3, col)));

        // 计算递减单调性（从上到下）
        float decreasingTop = 0.0F;
        if (a > b) {
            decreasingTop += static_cast<float>(a - b) * 2.5F;  // 增加权重
        }
        if (b > c) {
            decreasingTop += static_cast<float>(b - c) * 2.0F;
        }
        if (c > d) {
            decreasingTop += static_cast<float>(c - d) * 1.5F;
        }

        // 计算递减单调性（从下到上）
        float decreasingBottom = 0.0F;
        if (d > c) {
            decreasingBottom += static_cast<float>(d - c) * 2.5F;  // 增加权重
        }
        if (c > b) {
            decreasingBottom += static_cast<float>(c - b) * 2.0F;
        }
        if (b > a) {
            decreasingBottom += static_cast<float>(b - a) * 1.5F;
        }

        // 选择更好的单调性方向
        colScore += std::max(decreasingTop, decreasingBottom);
    }

    // 特别奖励左上角的大数字
    float cornerBonus     = 0.0F;
    uint32_t topLeftValue = board.getTile(0, 0);
    if (topLeftValue > 0) {
        // 如果左上角是最大值，给予更高的奖励
        if (topLeftValue == maxTile) {
            cornerBonus = static_cast<float>(std::log2(topLeftValue)) * 5.0F;
        } else {
            cornerBonus = static_cast<float>(std::log2(topLeftValue)) * 3.0F;
        }
    }

    // 组合行列分数和角落奖励，并应用重要性因子
    return (rowScore * 1.5F + colScore + cornerBonus) * importanceFactor;
}
