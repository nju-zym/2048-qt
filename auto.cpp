#include "auto.h"

// TrainingTask的run方法实现
void TrainingTask::run() {
    // 模拟多次游戏并计算平均分数
    int totalScore = 0;
    Auto autoPlayer;

    for (int i = 0; i < simulations; ++i) {
        totalScore += autoPlayer.simulateFullGame(params);
    }

    int avgScore = totalScore / simulations;

    // 调用回调函数返回分数
    callback(avgScore);
}

// 构造函数：初始化策略参数
Auto::Auto()
    : strategyParams(5, 1.0),  // 初始化为5个参数，默认值为1.0
      useLearnedParams(false),
      bestHistoricalScore(0),  // 初始化历史最佳分数为0
      trainingActive(false) {
    // 初始化随机数生成器
    srand(time(nullptr));
}

// 析构函数
Auto::~Auto() {
    // 确保训练线程已经结束
    if (trainingActive.load()) {
        trainingActive.store(false);
        QThreadPool::globalInstance()->waitForDone();
    }
}

// 设置是否使用学习到的参数
void Auto::setUseLearnedParams(bool use) {
    useLearnedParams = use;
}

// 获取是否使用学习到的参数
bool Auto::getUseLearnedParams() const {
    return useLearnedParams;
}

// 获取策略参数
QVector<double> Auto::getStrategyParams() const {
    return strategyParams;
}

// findBestMove: 找出最佳移动方向
int Auto::findBestMove(QVector<QVector<int>> const& board) {
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
            // 根据是否使用学习到的参数选择评估函数
            int score = 0;
            if (useLearnedParams) {
                // 使用学习到的参数进行评估
                score = evaluateWithParams(boardCopy, strategyParams) + moveScore;
            } else {
                // 使用高级评估函数评估棋盘状态
                score = evaluateBoardAdvanced(boardCopy) + moveScore;

                // 使用expectimax算法进行深度为5的搜索
                // 增加搜索深度可以提高AI能力，但会增加计算时间
                int simulationScore  = expectimax(boardCopy, 5, false);
                score               += simulationScore;
            }

            if (score > bestScore) {
                bestScore     = score;
                bestDirection = direction;
            }
        }
    }

    // 如果没有有效移动，随机选择一个方向
    if (bestDirection == -1) {
        bestDirection = rand() % 4;
    }

    return bestDirection;
}

// evaluateBoard: 评估棋盘状态
int Auto::evaluateBoard(QVector<QVector<int>> const& boardState) {
    int score = 0;

    // 策略：高分数方块尽量放在角落，并保持递减排列

    // 1. 计算空格数量，空格越多越好
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    score += emptyCount * 10;  // 每个空格给10分

    // 2. 奖励角落有高分数方块
    // 右下角最高分
    if (boardState[3][3] > 0) {
        score += boardState[3][3] * 2;
    }

    // 3. 奖励相邻方块数值递减排列
    // 检查行
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            // 如果相邻方块有递减关系，加分
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0 && boardState[i][j] >= boardState[i][j + 1]) {
                score += std::min(boardState[i][j], boardState[i][j + 1]);
            }
        }
    }

    // 检查列
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            // 如果相邻方块有递减关系，加分
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0 && boardState[i][j] >= boardState[i + 1][j]) {
                score += std::min(boardState[i][j], boardState[i + 1][j]);
            }
        }
    }

    return score;
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
                mergeScore       += tileValue * logValue;
            }
        }
    }

    // 检查列上的合并可能性
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i + 1][j]) {
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                mergeScore       += tileValue * logValue;
            }
        }
    }

    return mergeScore;
}

// 高级模式评估 - 专门处理超过2048的复杂情况
int Auto::evaluateAdvancedPattern(QVector<QVector<int>> const& boardState) {
    int score    = 0;
    int maxValue = 0;
    int maxRow = 0, maxCol = 0;

    // 找出最大值及其位置
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
                maxRow   = i;
                maxCol   = j;
            }
        }
    }

    // 如果最大值小于2048，使用普通评估函数
    if (maxValue < 2048) {
        return evaluateBoardAdvanced(boardState);
    }

    // 对于超过2048的情况，使用更复杂的评估策略

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
    score += patternScore * 2;  // 增加模式权重

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
    score += emptyCount * (16 - emptyCount) * 50;  // 大幅增加空格权重

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
    score += static_cast<int>(smoothness * 40);  // 增加平滑度权重

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

    score -= static_cast<int>(monotonicity * 60);  // 单调性是负分，越小越好

    // 6. 合并可能性 - 奖励相邻相同值
    double mergeScore  = calculateMergeScore(boardState);
    score             += static_cast<int>(mergeScore * 20);

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
    score += emptyCount * (16 - emptyCount) * 20;  // 增加空格权重

    // 2. 角落策略 - 使用权重矩阵
    // 定义权重矩阵，优先将大数放在角落，特别是左上角
    int const weightMatrix[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};

    // 计算权重得分
    int weightScore = 0;
    int maxValue    = 0;
    int maxRow = 0, maxCol = 0;

    // 找出最大值及其位置
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
                maxRow   = i;
                maxCol   = j;
            }
            // 根据权重矩阵计算得分
            if (boardState[i][j] > 0) {
                weightScore += boardState[i][j] * weightMatrix[i][j];
            }
        }
    }

    // 如果最大值在左上角，给予额外奖励
    if (maxRow == 0 && maxCol == 0) {
        weightScore += maxValue * 4;
    }
    // 如果最大值在任意角落，也给予奖励
    else if ((maxRow == 0 || maxRow == 3) && (maxCol == 0 || maxCol == 3)) {
        weightScore += maxValue * 2;
    }

    score += weightScore;

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
    score += smoothness * 30;  // 增加平滑度权重

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

    score -= monotonicity * 50;  // 单调性是负分，越小越好

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
    score += mergeScore * 10;

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

// monteCarloSimulation: 蒙特卡洛模拟
int Auto::monteCarloSimulation(QVector<QVector<int>> const& boardState, int depth) {
    if (depth <= 0) {
        return evaluateBoardAdvanced(boardState);
    }

    int bestScore = -1;

    // 尝试每个方向
    for (int direction = 0; direction < 4; ++direction) {
        QVector<QVector<int>> boardCopy = boardState;
        int moveScore                   = 0;

        // 模拟移动
        bool moved = simulateMove(boardCopy, direction, moveScore);

        if (moved) {
            // 计算当前移动的分数
            int currentScore = moveScore;

            // 找出空格
            QVector<QPair<int, int>> emptyTiles;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (boardCopy[i][j] == 0) {
                        emptyTiles.append(qMakePair(i, j));
                    }
                }
            }

            if (!emptyTiles.isEmpty()) {
                // 对于每个空格，模拟生成新方块
                int totalFutureScore = 0;
                int simCount         = 0;

                // 为了提高效率，只随机选择一部分空格进行模拟
                int maxSimulations = std::min(3, static_cast<int>(emptyTiles.size()));
                for (int sim = 0; sim < maxSimulations; sim++) {
                    // 随机选择一个空位置
                    int randomIndex = rand() % emptyTiles.size();
                    int row         = emptyTiles[randomIndex].first;
                    int col         = emptyTiles[randomIndex].second;

                    // 模拟生成2和4两种情况
                    for (int newTile : {2, 4}) {
                        QVector<QVector<int>> newBoardCopy = boardCopy;
                        newBoardCopy[row][col]             = newTile;

                        // 递归模拟下一步最佳移动
                        int futureScore = expectimax(newBoardCopy, depth - 1, false);

                        // 加权平均：2出现90%的概率，4出现10%的概率
                        double probability  = (newTile == 2) ? 0.9 : 0.1;
                        totalFutureScore   += static_cast<int>(futureScore * probability);
                        simCount++;
                    }
                }

                // 计算平均未来分数
                if (simCount > 0) {
                    currentScore += totalFutureScore / simCount;
                }
            }

            // 更新最佳分数
            if (currentScore > bestScore) {
                bestScore = currentScore;
            }
        }
    }

    return bestScore > 0 ? bestScore : 0;
}

// expectimax: 期望最大算法
int Auto::expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer) {
    // 如果到达最大深度，返回评估分数
    if (depth <= 0) {
        // 使用高级模式评估函数处理复杂情况
        return evaluateAdvancedPattern(boardState);
    }

    // 检测游戏是否结束
    if (isGameOver(boardState)) {
        return -500000;  // 游戏结束给予大量惩罚
    }

    // 自适应搜索深度 - 对于高级棋盘状态增加搜索深度
    int maxValue = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            maxValue = std::max(maxValue, boardState[i][j]);
        }
    }

    // 如果最大值超过2048，增加搜索深度
    int extraDepth = 0;
    if (maxValue >= 2048) {
        extraDepth = 1;  // 增加一层搜索深度
    }
    if (maxValue >= 4096) {
        extraDepth = 2;  // 再增加一层搜索深度
    }

    if (isMaxPlayer) {
        // MAX节点：玩家选择最佳移动
        int bestScore = -1;

        // 尝试每个方向
        for (int direction = 0; direction < 4; ++direction) {
            QVector<QVector<int>> boardCopy = boardState;
            int moveScore                   = 0;

            // 模拟移动
            bool moved = simulateMove(boardCopy, direction, moveScore);

            if (moved) {
                // 递归计算期望分数
                int score = moveScore + expectimax(boardCopy, depth - 1 + extraDepth, false);

                if (score > bestScore) {
                    bestScore = score;
                }
            }
        }

        return bestScore > 0 ? bestScore : 0;
    } else {
        // CHANCE节点：随机生成新方块
        // 找出所有空格
        QVector<QPair<int, int>> emptyTiles;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (boardState[i][j] == 0) {
                    emptyTiles.append(qMakePair(i, j));
                }
            }
        }

        if (emptyTiles.isEmpty()) {
            return evaluateBoardAdvanced(boardState);
        }

        // 计算期望分数
        double expectedScore = 0;

        // 为了提高效率，只随机选择一部分空格
        // 对于高级棋盘状态，增加模拟空格数量
        int maxTiles = std::min(2 + extraDepth, static_cast<int>(emptyTiles.size()));
        for (int i = 0; i < maxTiles; i++) {
            int randomIndex = rand() % emptyTiles.size();
            int row         = emptyTiles[randomIndex].first;
            int col         = emptyTiles[randomIndex].second;

            // 模拟生成2和4两种情况
            QVector<QVector<int>> boardWith2 = boardState;
            boardWith2[row][col]             = 2;

            QVector<QVector<int>> boardWith4 = boardState;
            boardWith4[row][col]             = 4;

            // 递归计算期望分数，加权平均
            expectedScore += 0.9 * expectimax(boardWith2, depth - 1 + extraDepth, true);  // 90%概率生成2
            expectedScore += 0.1 * expectimax(boardWith4, depth - 1 + extraDepth, true);  // 10%概率生成4

            // 从列表中移除已使用的空格
            emptyTiles.removeAt(randomIndex);
        }

        // 返回平均期望分数
        return static_cast<int>(expectedScore / maxTiles);
    }
}

// evaluateWithParams: 使用给定参数评估棋盘
int Auto::evaluateWithParams(QVector<QVector<int>> const& boardState, QVector<double> const& params) {
    int score = 0;

    // 确保参数数量足够
    if (params.size() < 5) {
        return evaluateBoardAdvanced(boardState);  // 如果参数不足，使用默认评估函数
    }

    // 1. 空格数量权重
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    score += static_cast<int>(emptyCount * params[0] * 10);

    // 2. 蛇形模式权重
    int const snakePattern[4][4] = {{1, 2, 3, 4}, {8, 7, 6, 5}, {9, 10, 11, 12}, {16, 15, 14, 13}};
    int snakeScore               = 0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > 0) {
                snakeScore += boardState[i][j] * snakePattern[i][j];
            }
        }
    }
    score += static_cast<int>(snakeScore * params[1]);

    // 3. 平滑度权重
    int smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                int diff    = std::abs(boardState[i][j] - boardState[i][j + 1]);
                smoothness -= diff;
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                int diff    = std::abs(boardState[i][j] - boardState[i + 1][j]);
                smoothness -= diff;
            }
        }
    }
    score += static_cast<int>(smoothness * params[2]);

    // 4. 单调性权重
    int monotonicity = 0;
    for (int i = 0; i < 4; ++i) {
        bool increasing = true;
        bool decreasing = true;
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                if (boardState[i][j] > boardState[i][j + 1]) {
                    increasing = false;
                }
                if (boardState[i][j] < boardState[i][j + 1]) {
                    decreasing = false;
                }
            }
        }
        if (increasing || decreasing) {
            monotonicity += 1;
        }
    }
    for (int j = 0; j < 4; ++j) {
        bool increasing = true;
        bool decreasing = true;
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                if (boardState[i][j] > boardState[i + 1][j]) {
                    increasing = false;
                }
                if (boardState[i][j] < boardState[i + 1][j]) {
                    decreasing = false;
                }
            }
        }
        if (increasing || decreasing) {
            monotonicity += 1;
        }
    }
    score += static_cast<int>(monotonicity * params[3] * 40);

    // 5. 最大值位置权重
    int maxValue = 0;
    int maxRow = 0, maxCol = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
                maxRow   = i;
                maxCol   = j;
            }
        }
    }
    // 如果最大值在角落，给予额外奖励
    if ((maxRow == 0 || maxRow == 3) && (maxCol == 0 || maxCol == 3)) {
        score += static_cast<int>(maxValue * params[4]);
    }

    return score;
}

// simulateFullGame: 模拟完整游戏
int Auto::simulateFullGame(QVector<double> const& params) {
    // 初始化模拟棋盘
    QVector<QVector<int>> simBoard(4, QVector<int>(4, 0));
    int simScore = 0;

    // 随机生成两个初始方块
    QVector<QPair<int, int>> emptyTiles;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            emptyTiles.append(qMakePair(i, j));
        }
    }

    // 随机选择两个位置生成初始方块
    for (int i = 0; i < 2; ++i) {
        int randomIndex    = rand() % emptyTiles.size();
        int row            = emptyTiles[randomIndex].first;
        int col            = emptyTiles[randomIndex].second;
        simBoard[row][col] = (rand() % 10 < 9) ? 2 : 4;  // 90%的概率生成2，10%生成4
        emptyTiles.removeAt(randomIndex);
    }

    // 模拟游戏过程，直到游戏结束
    bool gameOver = false;
    int moveCount = 0;
    int maxMoves  = 1000;  // 防止无限循环

    while (!gameOver && moveCount < maxMoves) {
        // 找出最佳移动方向
        int bestDirection = -1;
        int bestScore     = -1;

        // 尝试每个方向
        for (int direction = 0; direction < 4; ++direction) {
            QVector<QVector<int>> boardCopy = simBoard;
            int moveScore                   = 0;

            // 模拟移动
            bool moved = simulateMove(boardCopy, direction, moveScore);

            if (moved) {
                // 使用参数化的评估函数
                int score = evaluateWithParams(boardCopy, params) + moveScore;

                if (score > bestScore) {
                    bestScore     = score;
                    bestDirection = direction;
                }
            }
        }

        // 如果没有可用的移动，游戏结束
        if (bestDirection == -1) {
            gameOver = true;
            continue;
        }

        // 执行最佳移动
        int moveScore = 0;
        simulateMove(simBoard, bestDirection, moveScore);
        simScore += moveScore;
        moveCount++;

        // 随机生成新方块
        emptyTiles.clear();
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (simBoard[i][j] == 0) {
                    emptyTiles.append(qMakePair(i, j));
                }
            }
        }

        if (emptyTiles.isEmpty()) {
            gameOver = true;
            continue;
        }

        int randomIndex    = rand() % emptyTiles.size();
        int row            = emptyTiles[randomIndex].first;
        int col            = emptyTiles[randomIndex].second;
        simBoard[row][col] = (rand() % 10 < 9) ? 2 : 4;  // 90%的概率生成2，10%生成4
    }

    return simScore;
}

// 评估参数性能 - 更全面地评估参数的效果
int Auto::evaluateParameters(QVector<double> const& params, int simulations) {
    int totalScore = 0;

    // 运行多次模拟游戏并计算平均分数
    for (int i = 0; i < simulations; ++i) {
        totalScore += simulateFullGame(params);
    }

    return totalScore / simulations;
}

// 保存参数到文件
bool Auto::saveParameters(QString const& filename) {
    QString saveFilename = filename;
    if (saveFilename.isEmpty()) {
        // 生成默认文件名，包含时间戳
        QDateTime now = QDateTime::currentDateTime();
        saveFilename  = QString("2048_ai_params_%1.json").arg(now.toString("yyyyMMdd_hhmmss"));
    }

    // 创建 JSON 对象
    QJsonObject paramsObj;
    QJsonArray paramsArray;

    for (double param : strategyParams) {
        paramsArray.append(param);
    }

    paramsObj["parameters"] = paramsArray;
    paramsObj["score"]      = bestHistoricalScore;  // 存储该参数集的最高分
    paramsObj["timestamp"]  = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(paramsObj);
    QFile file(saveFilename);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    file.close();

    return true;
}

// 从文件加载参数
bool Auto::loadParameters(QString const& filename) {
    QString loadFilename = filename;
    if (loadFilename.isEmpty()) {
        // 如果没有指定文件名，尝试加载默认文件
        loadFilename = "2048_ai_params_default.json";
    }

    QFile file(loadFilename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("parameters") || !obj["parameters"].isArray()) {
        return false;
    }

    QJsonArray paramsArray = obj["parameters"].toArray();
    QVector<double> loadedParams;

    for (auto const& value : paramsArray) {
        if (value.isDouble()) {
            loadedParams.append(value.toDouble());
        }
    }

    // 确保参数数量正确
    if (loadedParams.size() < 5) {
        return false;
    }

    // 加载历史最佳分数
    if (obj.contains("score")) {
        bestHistoricalScore = obj["score"].toInt();
    } else {
        bestHistoricalScore = 0;
    }

    // 更新参数
    strategyParams   = loadedParams;
    useLearnedParams = true;

    return true;
}

// learnParameters: 学习最佳参数
void Auto::learnParameters(int populationSize, int generations, int simulations) {
    // 如果已经在训练中，则返回
    if (trainingActive.load()) {
        return;
    }

    // 设置训练状态
    trainingActive.store(true);

    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 10.0);

    // 初始化种群
    QVector<QVector<double>> population;
    QVector<double> bestParams(5, 1.0);  // 最佳参数
    int bestScore = 0;

    // 初始化种群中的每个个体
    for (int i = 0; i < populationSize; ++i) {
        QVector<double> params(5);
        for (int j = 0; j < 5; ++j) {
            params[j] = dis(gen);  // 生成 0-10 之间的随机浮点数
        }
        population.append(params);
    }

    // 进化多代
    for (int gen = 0; gen < generations && trainingActive.load(); ++gen) {
        QVector<int> scores(populationSize, 0);
        QVector<bool> evaluated(populationSize, false);
        int evaluatedCount = 0;

        // 创建线程池
        QThreadPool* threadPool = QThreadPool::globalInstance();
        threadPool->setMaxThreadCount(QThread::idealThreadCount());

        // 对每组参数创建任务
        for (int i = 0; i < populationSize; ++i) {
            // 创建任务
            auto task = new TrainingTask(
                population[i], simulations, [this, i, &scores, &evaluated, &evaluatedCount](int score) {
                    // 使用互斥锁保护共享数据
                    QMutexLocker locker(&this->mutex);
                    scores[i]    = score;
                    evaluated[i] = true;
                    evaluatedCount++;
                });

            // 设置任务自动删除
            task->setAutoDelete(true);

            // 提交任务到线程池
            threadPool->start(task);
        }

        // 等待所有任务完成
        while (evaluatedCount < populationSize && trainingActive.load()) {
            QThread::msleep(100);  // 等待100毫秒
        }

        // 如果训练被取消，则返回
        if (!trainingActive.load()) {
            break;
        }

        // 找出当前代的最佳参数
        int genBestScore = 0;

        for (int i = 0; i < populationSize; ++i) {
            if (scores[i] > genBestScore) {
                genBestScore = scores[i];
            }

            // 更新全局最佳参数
            if (scores[i] > bestScore) {
                bestScore  = scores[i];
                bestParams = population[i];
            }
        }

        // 发送进度更新信号
        emit trainingProgress.progressUpdated(gen + 1, generations, bestScore, bestParams);

        // 创建新一代
        QVector<QVector<double>> newPopulation;

        // 精英选择 - 保留最佳的五个个体
        QVector<int> indices = findTopIndices(scores, 5);
        for (int idx : indices) {
            newPopulation.append(population[idx]);
        }

        // 交叉和变异生成新一代
        while (newPopulation.size() < populationSize) {
            // 选择两个父代进行交叉
            int parent1 = tournamentSelection(scores);
            int parent2 = tournamentSelection(scores);

            // 交叉
            QVector<double> child = crossover(population[parent1], population[parent2]);

            // 变异
            mutate(child);

            // 添加到新种群
            newPopulation.append(child);
        }

        // 替换旧种群
        population = newPopulation;
    }

    // 评估最终参数的性能
    int finalScore = evaluateParameters(bestParams, 30);  // 运行30次模拟游戏进行更全面的评估

    // 只有当新参数比历史最佳参数更好时才更新
    if (finalScore > bestHistoricalScore) {
        // 保存最佳参数
        strategyParams      = bestParams;
        bestHistoricalScore = finalScore;
        useLearnedParams    = true;

        qDebug() << "New best parameters found with score:" << finalScore;
    } else {
        qDebug() << "Training did not improve parameters. Best score:" << finalScore
                 << "Historical best:" << bestHistoricalScore;
    }

    // 发送训练完成信号
    emit trainingProgress.trainingCompleted(finalScore, bestParams);

    // 重置训练状态
    trainingActive.store(false);
}

// findTopIndices: 找出最高分数的索引
QVector<int> Auto::findTopIndices(QVector<int> const& scores, int count) {
    QVector<int> indices(scores.size());
    for (int i = 0; i < scores.size(); ++i) {
        indices[i] = i;
    }

    // 根据分数排序索引
    std::sort(indices.begin(), indices.end(), [&scores](int a, int b) { return scores[a] > scores[b]; });

    // 返回前 count 个索引
    return indices.mid(0, count);
}

// tournamentSelection: 选择算法
int Auto::tournamentSelection(QVector<int> const& scores) {
    // 随机选择3个个体进行比赛
    int size = scores.size();
    int a    = rand() % size;
    int b    = rand() % size;
    int c    = rand() % size;

    // 返回分数最高的
    if (scores[a] >= scores[b] && scores[a] >= scores[c]) {
        return a;
    }
    if (scores[b] >= scores[a] && scores[b] >= scores[c]) {
        return b;
    }
    return c;
}

// crossover: 交叉算法
QVector<double> Auto::crossover(QVector<double> const& parent1, QVector<double> const& parent2) {
    QVector<double> child(parent1.size());

    // 均匀交叉
    for (int i = 0; i < parent1.size(); ++i) {
        // 50%的概率从父代1继承，50%的概率从父代2继承
        child[i] = (rand() % 2 == 0) ? parent1[i] : parent2[i];
    }

    return child;
}

// mutate: 变异算法
void Auto::mutate(QVector<double>& params) {
    // 每个参数有 20% 的概率发生变异
    for (int i = 0; i < params.size(); ++i) {
        if (rand() % 5 == 0) {
            // 变异幅度为当前值的 -30% 到 +30%
            double change  = params[i] * (rand() % 61 - 30) / 100.0;
            params[i]     += change;

            // 确保参数不为负
            if (params[i] < 0) {
                params[i] = 0;
            }
        }
    }
}
