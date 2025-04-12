#include "BoardEvaluator.h"

#include "GameBoard.h"

#include <cmath>

// 构造函数：初始化评估参数
BoardEvaluator::BoardEvaluator() : defaultParams(5, 1.0) {
    // 初始化默认参数为经过测试的有效值
    defaultParams[0] = 2.0;  // 空格数量权重
    defaultParams[1] = 2.0;  // 蛇形模式权重
    defaultParams[2] = 0.5;  // 平滑度权重
    defaultParams[3] = 4.5;  // 单调性权重
    defaultParams[4] = 1.0;  // 合并可能性权重
}

// 评估棋盘 - 选择合适的评估函数
int BoardEvaluator::evaluateBoard(QVector<QVector<int>> const& boardState) {
    return evaluateAdvancedPattern(boardState);
}

// 计算合并分数 - 评估棋盘上可能的合并操作
double BoardEvaluator::calculateMergeScore(QVector<QVector<int>> const& boardState) {
    double mergeScore = 0;

    // 检查行上的合并可能性
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i][j + 1]) {
                // 对于高值方块，给予更高的奖励
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                mergeScore       += tileValue * (logValue / 10.0) * 2.0;
            }
        }
    }

    // 检查列上的合并可能性
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i + 1][j]) {
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                mergeScore       += tileValue * (logValue / 10.0) * 2.0;
            }
        }
    }

    // 检查相邻的递增序列（如 2-4-8）
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 2; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0 && boardState[i][j + 2] > 0) {
                if (boardState[i][j + 1] == 2 * boardState[i][j] && boardState[i][j + 2] == 2 * boardState[i][j + 1]) {
                    // 奖励递增序列
                    mergeScore += boardState[i][j + 2] * 0.5;
                }
            }
        }
    }

    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 2; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0 && boardState[i + 2][j] > 0) {
                if (boardState[i + 1][j] == 2 * boardState[i][j] && boardState[i + 2][j] == 2 * boardState[i + 1][j]) {
                    // 奖励递增序列
                    mergeScore += boardState[i + 2][j] * 0.5;
                }
            }
        }
    }

    return mergeScore;
}

// 高级模式评估 - 专门处理超过2048的复杂情况
int BoardEvaluator::evaluateAdvancedPattern(QVector<QVector<int>> const& boardState) {
    int score    = 0;
    int maxValue = GameBoard::getMaxTile(boardState);

    // 如果最大值小于2048，使用普通评估函数
    if (maxValue < 2048) {
        return evaluateBoardAdvanced(boardState);
    }

    // 1. 蛇形模式权重矩阵 - 为了处理高级棋盘状态
    // 定义蛇形路径的权重矩阵，从左上角开始蛇形形式排列
    int const snakePattern[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};

    // 2. 计算蛇形模式得分
    int patternScore = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > 0) {
                // 对于高值方块，给予更高的权重
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                patternScore     += tileValue * snakePattern[i][j] * (logValue / 11.0);  // 11 = log2(2048)
            }
        }
    }
    score += patternScore * defaultParams[1];  // 使用蛇形模式权重参数

    // 3. 空格数量权重 - 对于高级棋盘更重要
    int emptyCount = GameBoard::countEmptyCells(boardState);
    // 空格数量的权重与棋盘填充程度相关，棋盘越满空格越重要
    score += emptyCount * (16 - emptyCount) * defaultParams[0] * 2.5;  // 使用空格数量权重参数，对于高级棋盘空格更重要

    // 4. 平滑度权重 - 相邻方块数值差异越小越好
    double smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                // 使用对数差异来减少大数值之间的差异惩罚
                double logVal1  = log2(boardState[i][j]);
                double logVal2  = log2(boardState[i][j + 1]);
                smoothness     -= std::abs(logVal1 - logVal2) * 4.0;
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                double logVal1  = log2(boardState[i][j]);
                double logVal2  = log2(boardState[i + 1][j]);
                smoothness     -= std::abs(logVal1 - logVal2) * 4.0;
            }
        }
    }
    score += static_cast<int>(smoothness * defaultParams[2] * 1.5);  // 使用平滑度权重参数，对于高级棋盘平滑度更重要

    // 5. 单调性权重 - 方块数值沿某个方向单调递增或递减
    double monotonicity = 0;

    // 检查行的单调性 - 使用对数值
    for (int i = 0; i < 4; ++i) {
        double current     = 0;
        double left_score  = 0;
        double right_score = 0;

        for (int j = 0; j < 4; ++j) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                if (current > 0) {
                    if (current > value) {
                        left_score += (current - value) * 2;  // 增加左向单调性权重
                    } else {
                        right_score += (value - current) * 2;  // 增加右向单调性权重
                    }
                }
                current = value;
            }
        }
        monotonicity += std::min(left_score, right_score);
    }

    // 检查列的单调性 - 使用对数值
    for (int j = 0; j < 4; ++j) {
        double current    = 0;
        double up_score   = 0;
        double down_score = 0;

        for (int i = 0; i < 4; ++i) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                if (current > 0) {
                    if (current > value) {
                        up_score += (current - value) * 2;  // 增加上向单调性权重
                    } else {
                        down_score += (value - current) * 2;  // 增加下向单调性权重
                    }
                }
                current = value;
            }
        }
        monotonicity += std::min(up_score, down_score);
    }

    score -= static_cast<int>(monotonicity * defaultParams[3] * 1.2);  // 使用单调性权重参数，单调性是负分，越小越好

    // 6. 合并可能性 - 奖励相邻相同值
    double mergeScore  = calculateMergeScore(boardState);
    score             += static_cast<int>(mergeScore * defaultParams[4] * 2.0);  // 使用合并可能性权重参数

    // 7. 游戏结束检测 - 如果游戏即将结束，给予大量惩罚
    if (GameBoard::isGameOver(boardState)) {
        score -= 500000;  // 大幅惩罚游戏结束状态
    }

    return score;
}

// evaluateBoardAdvanced: 高级评估棋盘状态
int BoardEvaluator::evaluateBoardAdvanced(QVector<QVector<int>> const& boardState) {
    int score = 0;

    // 1. 空格数量权重 - 空格越多越好
    int emptyCount = GameBoard::countEmptyCells(boardState);
    // 空格数量的权重与棋盘填充程度相关，棋盘越满空格越重要
    score += emptyCount * (16 - emptyCount) * defaultParams[0];  // 使用空格数量权重参数

    // 2. 角落策略 - 使用权重矩阵
    // 定义权重矩阵，优先将大数放在角落，特别是左上角
    int const weightMatrix[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};

    // 计算权重得分
    int weightScore = 0;
    int maxValue    = GameBoard::getMaxTile(boardState);

    // 计算权重得分
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            // 根据权重矩阵计算得分
            if (boardState[i][j] > 0) {
                weightScore += boardState[i][j] * weightMatrix[i][j];
            }
        }
    }

    // 奖励角落有高分数方块
    if (boardState[0][0] == maxValue) {
        weightScore += maxValue * 4;  // 左上角最高分
    }
    // 如果最大值在任意角落，也给予奖励
    else if (boardState[0][3] == maxValue || boardState[3][0] == maxValue || boardState[3][3] == maxValue) {
        weightScore += maxValue * 2;
    }

    score += weightScore * defaultParams[1];  // 使用蛇形模式权重参数

    // 3. 平滑度权重 - 相邻方块数值差异越小越好
    int smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                // 使用对数差异来减少大数值之间的差异惩罚
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i][j + 1] > 0 ? log2(boardState[i][j + 1]) : 0;
                smoothness     -= abs(logVal1 - logVal2) * 2.0;
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                // 使用对数差异
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i + 1][j] > 0 ? log2(boardState[i + 1][j]) : 0;
                smoothness     -= abs(logVal1 - logVal2) * 2.0;
            }
        }
    }
    score += smoothness * defaultParams[2];  // 使用平滑度权重参数

    // 4. 单调性权重 - 方块数值沿某个方向单调递增或递减
    double monotonicity = 0;

    // 检查行的单调性 - 使用对数值
    for (int i = 0; i < 4; ++i) {
        double current     = 0;
        double left_score  = 0;
        double right_score = 0;

        for (int j = 0; j < 4; ++j) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                // 计算左向单调性
                if (current > value) {
                    left_score += current - value;
                } else {
                    right_score += value - current;
                }
                current = value;
            }
        }
        monotonicity += std::min(left_score, right_score);
    }

    // 检查列的单调性 - 使用对数值
    for (int j = 0; j < 4; ++j) {
        double current    = 0;
        double up_score   = 0;
        double down_score = 0;

        for (int i = 0; i < 4; ++i) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                // 计算上向单调性
                if (current > value) {
                    up_score += current - value;
                } else {
                    down_score += value - current;
                }
                current = value;
            }
        }
        monotonicity += std::min(up_score, down_score);
    }

    score -= monotonicity * defaultParams[3];  // 使用单调性权重参数，单调性是负分，越小越好

    // 5. 合并可能性 - 奖励相邻相同值
    int mergeScore = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i][j + 1]) {
                mergeScore += boardState[i][j];
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i + 1][j]) {
                mergeScore += boardState[i][j];
            }
        }
    }
    score += mergeScore * defaultParams[4];  // 使用合并可能性权重参数

    // 6. 大数值聚集 - 奖励大数值聚集在一起
    int clusterScore = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            int cellValue = boardState[i][j];
            if (cellValue > 0) {
                // 检查右边和下边的方块
                if (boardState[i][j + 1] > 0) {
                    clusterScore += std::min(cellValue, boardState[i][j + 1]);
                }
                if (boardState[i + 1][j] > 0) {
                    clusterScore += std::min(cellValue, boardState[i + 1][j]);
                }
            }
        }
    }
    score += clusterScore;

    return score;
}