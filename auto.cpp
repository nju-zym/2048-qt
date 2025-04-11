#include "auto.h"

// Training worker include removed

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

// 构造函数：初始化策略参数
Auto::Auto()
    : strategyParams(5, 1.0),  // 初始化为5个参数，默认值为1.0
      defaultParams(5, 1.0),   // 初始化默认参数
      useLearnedParams(false),
      bestHistoricalScore(0) {  // 初始化历史最佳分数为0
    // 初始化默认参数为经过测试的有效值
    defaultParams[0] = 2.0;  // 空格数量权重
    defaultParams[1] = 2.0;  // 蛇形模式权重
    defaultParams[2] = 0.5;  // 平滑度权重
    defaultParams[3] = 4.5;  // 单调性权重
    defaultParams[4] = 1.0;  // 合并可能性权重

    // 初始化随机数生成器
    srand(time(nullptr));
}

// 析构函数
Auto::~Auto() {
    // 清除缓存
    expectimaxCache.clear();

    qDebug() << "Auto object destroyed successfully";
}

// findBestMove: 找出最佳移动方向
int Auto::findBestMove(QVector<QVector<int>> const& board) {
    // 清除缓存以确保最新的计算结果
    clearExpectimaxCache();
    int bestScore     = -1;
    int bestDirection = -1;

    // 尝试每个方向，计算移动后的棋盘评分
    for (int direction = 0; direction < 4; ++direction) {
        // 创建棋盘副本
        QVector<QVector<int>> boardCopy = board;

        // 模拟移动
        int moveScore = 0;
        bool moved    = simulateMove(boardCopy, direction, moveScore);

        // 如果这个方向可以移动，计算移动后的棋盘评分
        if (moved) {
            int score = 0;

            // 无论是否使用学习参数，都使用相同的评估方法，只是参数不同
            // 先进行基础评估 - 始终使用高级评估函数，包括处理大于2048的情况
            score = evaluateAdvancedPattern(boardCopy) + moveScore;

            // 使用expectimax算法进行深度为3的搜索
            int simulationScore  = expectimax(boardCopy, 3, false);
            score               += simulationScore;

            if (score > bestScore) {
                bestScore     = score;
                bestDirection = direction;
            }
        }
    }

    // 如果没有有效移动，随机选择一个方向
    if (bestDirection == -1) {
        bestDirection = rand() % 4;  // 随机选择一个方向
    }

    return bestDirection;
}

// 检查游戏是否结束
bool Auto::isGameOver(QVector<QVector<int>> const& boardState) {
    // 检查是否有空格
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                return false;  // 有空格，游戏未结束
            }
        }
    }

    // 检查是否有相邻相同的方块
    // 检查行
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] == boardState[i][j + 1]) {
                return false;  // 有相邻相同的方块，游戏未结束
            }
        }
    }

    // 检查列
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] == boardState[i + 1][j]) {
                return false;  // 有相邻相同的方块，游戏未结束
            }
        }
    }

    return true;  // 没有空格且没有相邻相同的方块，游戏结束
}

// 计算合并分数 - 评估棋盘上可能的合并操作
double Auto::calculateMergeScore(QVector<QVector<int>> const& boardState) {
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
int Auto::evaluateAdvancedPattern(QVector<QVector<int>> const& boardState) {
    int score    = 0;
    int maxValue = 0;

    // 找出最大值
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
            }
        }
    }

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
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
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
    if (isGameOver(boardState)) {
        score -= 500000;  // 大幅惩罚游戏结束状态
    }

    return score;
}

// evaluateBoardAdvanced: 高级评估棋盘状态
int Auto::evaluateBoardAdvanced(QVector<QVector<int>> const& boardState) {
    int score = 0;

    // 1. 空格数量权重 - 空格越多越好
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    // 空格数量的权重与棋盘填充程度相关，棋盘越满空格越重要
    score += emptyCount * (16 - emptyCount) * defaultParams[0];  // 使用空格数量权重参数

    // 2. 角落策略 - 使用权重矩阵
    // 定义权重矩阵，优先将大数放在角落，特别是左上角
    int const weightMatrix[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};

    // 计算权重得分
    int weightScore = 0;
    int maxValue    = 0;

    // 找出最大值
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
            }
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

// simulateMove: 模拟移动
bool Auto::simulateMove(QVector<QVector<int>>& boardState, int direction, int& score) {
    bool moved = false;
    score      = 0;

    if (direction == 0) {  // 向上
        for (int col = 0; col < 4; ++col) {
            for (int row = 1; row < 4; ++row) {
                if (boardState[row][col] != 0) {
                    int r = row;
                    while (r > 0 && boardState[r - 1][col] == 0) {
                        boardState[r - 1][col] = boardState[r][col];
                        boardState[r][col]     = 0;
                        r--;
                        moved = true;
                    }
                    if (r > 0 && boardState[r - 1][col] == boardState[r][col]) {
                        boardState[r - 1][col] *= 2;
                        score                  += boardState[r - 1][col];
                        boardState[r][col]      = 0;
                        moved                   = true;
                    }
                }
            }
        }
    } else if (direction == 1) {  // 向右
        for (int row = 0; row < 4; ++row) {
            for (int col = 2; col >= 0; --col) {
                if (boardState[row][col] != 0) {
                    int c = col;
                    while (c < 3 && boardState[row][c + 1] == 0) {
                        boardState[row][c + 1] = boardState[row][c];
                        boardState[row][c]     = 0;
                        c++;
                        moved = true;
                    }
                    if (c < 3 && boardState[row][c + 1] == boardState[row][c]) {
                        boardState[row][c + 1] *= 2;
                        score                  += boardState[row][c + 1];
                        boardState[row][c]      = 0;
                        moved                   = true;
                    }
                }
            }
        }
    } else if (direction == 2) {  // 向下
        for (int col = 0; col < 4; ++col) {
            for (int row = 2; row >= 0; --row) {
                if (boardState[row][col] != 0) {
                    int r = row;
                    while (r < 3 && boardState[r + 1][col] == 0) {
                        boardState[r + 1][col] = boardState[r][col];
                        boardState[r][col]     = 0;
                        r++;
                        moved = true;
                    }
                    if (r < 3 && boardState[r + 1][col] == boardState[r][col]) {
                        boardState[r + 1][col] *= 2;
                        score                  += boardState[r + 1][col];
                        boardState[r][col]      = 0;
                        moved                   = true;
                    }
                }
            }
        }
    } else if (direction == 3) {  // 向左
        for (int row = 0; row < 4; ++row) {
            for (int col = 1; col < 4; ++col) {
                if (boardState[row][col] != 0) {
                    int c = col;
                    while (c > 0 && boardState[row][c - 1] == 0) {
                        boardState[row][c - 1] = boardState[row][c];
                        boardState[row][c]     = 0;
                        c--;
                        moved = true;
                    }
                    if (c > 0 && boardState[row][c - 1] == boardState[row][c]) {
                        boardState[row][c - 1] *= 2;
                        score                  += boardState[row][c - 1];
                        boardState[row][c]      = 0;
                        moved                   = true;
                    }
                }
            }
        }
    }

    return moved;
}

// 清除expectimax缓存
void Auto::clearExpectimaxCache() {
    expectimaxCache.clear();
}

// expectimax: 期望最大算法 - 高效版本
int Auto::expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer) {
    // 检查缓存
    BoardState state{boardState, depth, isMaxPlayer};
    auto cacheIt = expectimaxCache.find(state);
    if (cacheIt != expectimaxCache.end()) {
        return cacheIt->second;
    }

    // 绝对深度限制 - 防止过深递归
    static int const MAX_ABSOLUTE_DEPTH = 5;  // 降低最大深度以提高性能
    if (depth > MAX_ABSOLUTE_DEPTH) {
        depth = MAX_ABSOLUTE_DEPTH;
    }

    // 如果到达最大深度，返回评估分数
    if (depth <= 0) {
        int score              = evaluateAdvancedPattern(boardState);
        expectimaxCache[state] = score;  // 缓存结果
        return score;
    }

    // 检测游戏是否结束
    if (isGameOver(boardState)) {
        int score              = -500000;  // 游戏结束给予大量惩罚
        expectimaxCache[state] = score;
        return score;
    }

    // 快速检测最大值和空格数
    int maxValue   = 0;
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
            }
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }

    // 对于高级棋盘，减少搜索深度以提高性能
    int extraDepth = 0;
    if (maxValue >= 2048) {
        // 对于高级棋盘，根据空格数量动态调整搜索深度
        if (emptyCount <= 4) {
            depth = std::min(depth, 3);  // 空格很少时限制深度
        } else {
            extraDepth = 1;  // 空格较多时增加深度
        }
    }

    int result = 0;
    if (isMaxPlayer) {
        // MAX节点：选择最佳移动
        int bestScore = -1;

        // 尝试所有可能的移动
        for (int direction = 0; direction < 4; ++direction) {
            QVector<QVector<int>> boardCopy = boardState;
            int moveScore                   = 0;

            bool moved = simulateMove(boardCopy, direction, moveScore);

            if (moved) {
                // 递归计算期望分数
                int score = moveScore + expectimax(boardCopy, depth - 1 + extraDepth, false);
                bestScore = std::max(bestScore, score);
            }
        }

        result = bestScore > 0 ? bestScore : 0;
    } else {
        // CHANCE节点：随机生成新方块
        // 如果没有空格，返回评估分数
        if (emptyCount == 0) {
            int score              = evaluateAdvancedPattern(boardState);
            expectimaxCache[state] = score;
            return score;
        }

        // 优化：对于高级棋盘，只模拟一个空格以提高性能
        int tilesToSimulate = 1;
        if (maxValue < 2048 && emptyCount > 1) {
            tilesToSimulate = std::min(2, emptyCount);
        }

        double totalScore = 0.0;

        // 优化空格选择策略
        QVector<QPair<int, int>> emptyPositions;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (boardState[i][j] == 0) {
                    emptyPositions.append(qMakePair(i, j));
                    if (emptyPositions.size() >= tilesToSimulate) {
                        break;
                    }
                }
            }
            if (emptyPositions.size() >= tilesToSimulate) {
                break;
            }
        }

        for (int i = 0; i < tilesToSimulate; ++i) {
            int row = emptyPositions[i].first;
            int col = emptyPositions[i].second;

            // 模拟生成2和4两种情况
            QVector<QVector<int>> boardWith2 = boardState;
            boardWith2[row][col]             = 2;

            // 优化：对于高级棋盘，只考虑生成2的情况
            if (maxValue >= 4096) {
                totalScore += expectimax(boardWith2, depth - 1, true);
            } else {
                QVector<QVector<int>> boardWith4 = boardState;
                boardWith4[row][col]             = 4;

                totalScore += 0.9 * expectimax(boardWith2, depth - 1, true);  // 90%概率生成2
                totalScore += 0.1 * expectimax(boardWith4, depth - 1, true);  // 10%概率生成4
            }
        }

        result = static_cast<int>(totalScore / tilesToSimulate);
    }

    // 缓存结果
    expectimaxCache[state] = result;

    // 限制缓存大小以防止内存溢出
    if (expectimaxCache.size() > 10000) {
        // 当缓存过大时清除
        expectimaxCache.clear();
    }

    return result;
}
