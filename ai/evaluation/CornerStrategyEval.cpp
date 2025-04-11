#include "CornerStrategyEval.h"

#include <algorithm>
#include <cmath>

float CornerStrategyEval::evaluate(BitBoard const& board) {
    // 简化评估，只使用角落权重和梯度权重
    float cornerScore   = calculateCornerWeight(board);
    float gradientScore = calculateGradientWeight(board);

    // 加权组合，增加角落权重
    return cornerScore * 3.0f + gradientScore * 1.5f;
}

float CornerStrategyEval::calculateCornerWeight(BitBoard const& board) {
    float score = 0.0f;

    // 获取棋盘上的最大值
    uint32_t maxTile = 0;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (board.getTile(row, col) > maxTile) {
                maxTile = board.getTile(row, col);
            }
        }
    }

    // 左上角策略（主要策略）
    uint32_t topLeft = board.getTile(0, 0);
    if (topLeft > 0) {
        // 如果最大值在左上角，给予很高的奖励
        if (topLeft == maxTile) {
            score += static_cast<float>(std::log2(topLeft)) * 5.0f;
        } else {
            score += static_cast<float>(std::log2(topLeft)) * 3.0f;
        }

        // 检查相邻方块是否形成递减序列
        uint32_t right = board.getTile(0, 1);
        uint32_t down  = board.getTile(1, 0);

        // 如果右边的方块小于左上角，给予奖励
        if (right > 0 && right < topLeft) {
            score += static_cast<float>(std::log2(right)) * 2.0f;

            // 检查是否连续递减
            uint32_t rightRight = board.getTile(0, 2);
            if (rightRight > 0 && rightRight < right) {
                score += static_cast<float>(std::log2(rightRight)) * 1.5f;
            }
        }

        // 如果下边的方块小于左上角，给予奖励
        if (down > 0 && down < topLeft) {
            score += static_cast<float>(std::log2(down)) * 2.0f;

            // 检查是否连续递减
            uint32_t downDown = board.getTile(2, 0);
            if (downDown > 0 && downDown < down) {
                score += static_cast<float>(std::log2(downDown)) * 1.5f;
            }
        }
    }

    // 其他角落策略（次要策略）
    uint32_t topRight    = board.getTile(0, 3);
    uint32_t bottomLeft  = board.getTile(3, 0);
    uint32_t bottomRight = board.getTile(3, 3);

    // 如果其他角落有大值，也给予一定的奖励，但低于左上角
    if (topRight > 0) {
        if (topRight == maxTile) {
            score += static_cast<float>(std::log2(topRight)) * 2.5f;
        } else {
            score += static_cast<float>(std::log2(topRight)) * 1.5f;
        }
    }

    if (bottomLeft > 0) {
        if (bottomLeft == maxTile) {
            score += static_cast<float>(std::log2(bottomLeft)) * 2.5f;
        } else {
            score += static_cast<float>(std::log2(bottomLeft)) * 1.5f;
        }
    }

    if (bottomRight > 0) {
        if (bottomRight == maxTile) {
            score += static_cast<float>(std::log2(bottomRight)) * 2.0f;
        } else {
            score += static_cast<float>(std::log2(bottomRight)) * 1.0f;
        }
    }

    // 惩罚中间的大数字
    for (int row = 1; row < 3; ++row) {
        for (int col = 1; col < 3; ++col) {
            uint32_t value = board.getTile(row, col);
            if (value > 0) {
                // 中间的大数字会被惩罚
                if (value >= maxTile / 2) {
                    score -= static_cast<float>(std::log2(value)) * 2.0f;
                }
            }
        }
    }

    return score;
}

float CornerStrategyEval::calculateGradientWeight(BitBoard const& board) {
    // 定义四个角落的梯度权重矩阵
    float const gradientWeights[4][4][4]
        = {// 左上角梯度
           {{4.0f, 3.0f, 2.0f, 1.0f}, {3.0f, 2.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 0.0f, -1.0f}, {1.0f, 0.0f, -1.0f, -2.0f}},
           // 右上角梯度
           {{1.0f, 2.0f, 3.0f, 4.0f}, {0.0f, 1.0f, 2.0f, 3.0f}, {-1.0f, 0.0f, 1.0f, 2.0f}, {-2.0f, -1.0f, 0.0f, 1.0f}},
           // 左下角梯度
           {{1.0f, 0.0f, -1.0f, -2.0f}, {2.0f, 1.0f, 0.0f, -1.0f}, {3.0f, 2.0f, 1.0f, 0.0f}, {4.0f, 3.0f, 2.0f, 1.0f}},
           // 右下角梯度
           {{-2.0f, -1.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 1.0f, 2.0f}, {0.0f, 1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f, 4.0f}}};

    // 计算每个角落的梯度分数
    float gradientScores[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    for (int corner = 0; corner < 4; ++corner) {
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                int value = board.getTile(row, col);
                if (value > 0) {
                    gradientScores[corner] += gradientWeights[corner][row][col] * std::log2(value);
                }
            }
        }
    }

    // 返回最高的梯度分数
    return *std::max_element(gradientScores, gradientScores + 4);
}
