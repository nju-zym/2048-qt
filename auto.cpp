#include "auto.h"

#include "opencl_manager.h"  // 确保OpenCLManager完整定义可用

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <random>

// 构造函数：初始化权重和其他成员变量
Auto::Auto(QObject* parent) : QObject(parent), stopTrainingFlag(false), openclManager(nullptr), useGPU(false) {
    // 初始化评估函数的权重
    weights = {
        10.0,  // 空格数量权重
        2.0,   // 单调性权重
        1.0,   // 平滑度权重
        2.0,   // 角落策略权重
        1.0    // 合并可能性权重
    };

    // 初始化训练进度对象
    trainingProgress = new TrainingProgress(this);

    // 尝试加载已保存的权重
    loadParameters();
}

// 析构函数
Auto::~Auto() {
    // 释放OpenCLManager
    if (openclManager != nullptr) {
        delete openclManager;
        openclManager = nullptr;
    }
}

// BitBoard操作：将QVector<QVector<int>>转换为BitBoard
BitBoard Auto::convertToBitBoard(QVector<QVector<int>> const& board) const {
    BitBoard result = 0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            // 计算当前位置的值
            int value = board[i][j];

            // 计算对应的位值（0表示空，1-15表示2^1到2^15）
            int bitValue = 0;
            if (value > 0) {
                bitValue = static_cast<int>(std::log2(value));
            }

            // 将值放入BitBoard的对应位置
            // 每个格子用4位表示，总共16个格子，需要64位
            result |= static_cast<BitBoard>(bitValue) << ((i * 4 + j) * 4);
        }
    }

    return result;
}

// BitBoard操作：将BitBoard转换为QVector<QVector<int>>
QVector<QVector<int>> Auto::convertFromBitBoard(BitBoard board) const {
    QVector<QVector<int>> result(4, QVector<int>(4, 0));

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            // 从BitBoard中提取当前位置的值
            int bitValue = (board >> ((i * 4 + j) * 4)) & 0xF;

            // 转换为实际值（2^bitValue）
            int value = 0;
            if (bitValue > 0) {
                value = 1 << bitValue;
            }

            result[i][j] = value;
        }
    }

    return result;
}

// 获取指定位置的值
int Auto::getTileValue(BitBoard board, int row, int col) const {
    int bitValue = (board >> ((row * 4 + col) * 4)) & 0xF;
    if (bitValue == 0) {
        return 0;
    }
    return 1 << bitValue;
}

// 设置指定位置的值
void Auto::setTileValue(BitBoard& board, int row, int col, int value) {
    // 清除原有位置的值
    board &= ~(static_cast<BitBoard>(0xF) << ((row * 4 + col) * 4));

    // 计算新值的位表示
    int bitValue = 0;
    if (value > 0) {
        bitValue = static_cast<int>(std::log2(value));
    }

    // 设置新值
    board |= static_cast<BitBoard>(bitValue) << ((row * 4 + col) * 4);
}

// 获取空格数量
int Auto::getEmptyTileCount(BitBoard board) const {
    int count = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (getTileValue(board, i, j) == 0) {
                count++;
            }
        }
    }
    return count;
}

// 获取最大方块值
int Auto::getMaxTile(BitBoard board) const {
    int maxTile = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            maxTile = std::max(maxTile, getTileValue(board, i, j));
        }
    }
    return maxTile;
}

// 检查是否可以向指定方向移动
bool Auto::canMove(BitBoard board, int direction) const {
    // 创建一个临时棋盘副本
    BitBoard newBoard = makeMove(board, direction);

    // 如果移动后的棋盘与原棋盘不同，则可以移动
    return newBoard != board;
}

// 检查游戏是否结束
bool Auto::isGameOver(BitBoard board) const {
    // 如果还有空格，游戏未结束
    if (getEmptyTileCount(board) > 0) {
        return false;
    }

    // 检查是否可以向任何方向移动
    for (int dir = 0; dir < 4; ++dir) {
        if (canMove(board, dir)) {
            return false;
        }
    }

    // 没有空格且不能移动，游戏结束
    return true;
}

// 检查是否达到2048（获胜）
bool Auto::hasWon(BitBoard board) const {
    // 检查是否有任何方块达到2048
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (getTileValue(board, i, j) >= 2048) {
                return true;
            }
        }
    }
    return false;
}

// 计算当前棋盘的分数
int Auto::getScore(BitBoard board) const {
    int score = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = getTileValue(board, i, j);
            if (value > 0) {
                // 分数计算：每个方块的值 * log2(值)
                score += value * (std::log2(value) - 1);
            }
        }
    }
    return score;
}

// 向指定方向移动棋盘
BitBoard Auto::makeMove(BitBoard board, int direction) const {
    BitBoard newBoard = board;
    bool moved        = false;

    // 根据方向执行移动
    switch (direction) {
        case 0:  // 上
            for (int j = 0; j < 4; ++j) {
                // 提取当前列
                int col[4] = {getTileValue(newBoard, 0, j),
                              getTileValue(newBoard, 1, j),
                              getTileValue(newBoard, 2, j),
                              getTileValue(newBoard, 3, j)};

                // 移动并合并
                bool colMoved = moveLine(col);

                // 更新棋盘
                if (colMoved) {
                    moved = true;
                    for (int i = 0; i < 4; ++i) {
                        setTileValue(newBoard, i, j, col[i]);
                    }
                }
            }
            break;

        case 1:  // 右
            for (int i = 0; i < 4; ++i) {
                // 提取当前行
                int row[4] = {getTileValue(newBoard, i, 0),
                              getTileValue(newBoard, i, 1),
                              getTileValue(newBoard, i, 2),
                              getTileValue(newBoard, i, 3)};

                // 反转行，以便使用相同的移动逻辑
                std::reverse(row, row + 4);

                // 移动并合并
                bool rowMoved = moveLine(row);

                // 反转回来
                std::reverse(row, row + 4);

                // 更新棋盘
                if (rowMoved) {
                    moved = true;
                    for (int j = 0; j < 4; ++j) {
                        setTileValue(newBoard, i, j, row[j]);
                    }
                }
            }
            break;

        case 2:  // 下
            for (int j = 0; j < 4; ++j) {
                // 提取当前列
                int col[4] = {getTileValue(newBoard, 0, j),
                              getTileValue(newBoard, 1, j),
                              getTileValue(newBoard, 2, j),
                              getTileValue(newBoard, 3, j)};

                // 反转列，以便使用相同的移动逻辑
                std::reverse(col, col + 4);

                // 移动并合并
                bool colMoved = moveLine(col);

                // 反转回来
                std::reverse(col, col + 4);

                // 更新棋盘
                if (colMoved) {
                    moved = true;
                    for (int i = 0; i < 4; ++i) {
                        setTileValue(newBoard, i, j, col[i]);
                    }
                }
            }
            break;

        case 3:  // 左
            for (int i = 0; i < 4; ++i) {
                // 提取当前行
                int row[4] = {getTileValue(newBoard, i, 0),
                              getTileValue(newBoard, i, 1),
                              getTileValue(newBoard, i, 2),
                              getTileValue(newBoard, i, 3)};

                // 移动并合并
                bool rowMoved = moveLine(row);

                // 更新棋盘
                if (rowMoved) {
                    moved = true;
                    for (int j = 0; j < 4; ++j) {
                        setTileValue(newBoard, i, j, row[j]);
                    }
                }
            }
            break;
        default:
            // 无效方向，不做任何操作
            break;
    }

    // 如果没有移动，返回原始棋盘
    if (!moved) {
        return board;
    }

    return newBoard;
}

// 移动并合并一行或一列
bool Auto::moveLine(int line[4]) const {
    bool moved = false;

    // 移动非零元素到前面
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            if (line[i] == 0 && line[j] != 0) {
                line[i] = line[j];
                line[j] = 0;
                moved   = true;
                j       = i;  // 重新检查当前位置
            }
        }
    }

    // 合并相同的元素
    for (int i = 0; i < 3; ++i) {
        if (line[i] != 0 && line[i] == line[i + 1]) {
            line[i]     *= 2;
            line[i + 1]  = 0;
            moved        = true;
        }
    }

    // 再次移动非零元素到前面
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            if (line[i] == 0 && line[j] != 0) {
                line[i] = line[j];
                line[j] = 0;
                moved   = true;
                j       = i;  // 重新检查当前位置
            }
        }
    }

    return moved;
}

// 添加随机方块（90%概率为2，10%概率为4）
BitBoard Auto::addRandomTile(BitBoard board) const {
    // 获取所有空格位置
    QVector<QPair<int, int>> emptyTiles;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (getTileValue(board, i, j) == 0) {
                emptyTiles.append(qMakePair(i, j));
            }
        }
    }

    // 如果没有空格，返回原始棋盘
    if (emptyTiles.isEmpty()) {
        return board;
    }

    // 随机选择一个空格
    int randomIndex = QRandomGenerator::global()->bounded(emptyTiles.size());
    int row         = emptyTiles[randomIndex].first;
    int col         = emptyTiles[randomIndex].second;

    // 90%概率生成2，10%概率生成4
    int value = QRandomGenerator::global()->bounded(10) < 9 ? 2 : 4;

    // 更新棋盘
    BitBoard newBoard = board;
    setTileValue(newBoard, row, col, value);

    return newBoard;
}

// 查找最佳移动方向
int Auto::findBestMove(QVector<QVector<int>> const& board) {
    // 将棋盘转换为BitBoard
    BitBoard bitBoard = convertToBitBoard(board);

    // 清除缓存
    QMutexLocker locker(&cacheMutex);
    expectimaxCache.clear();
    locker.unlock();

    // 先计算每个方向的初始评估值，以便按照有希望的顺序搜索
    std::vector<std::pair<int, double>> directions;
    for (int dir = 0; dir < 4; ++dir) {
        if (canMove(bitBoard, dir)) {
            BitBoard newBoard = makeMove(bitBoard, dir);
            // 使用简单的启发式评估
            double initialScore = emptyTilesHeuristic(newBoard) + monotonicity(newBoard);
            directions.push_back({dir, initialScore});
        }
    }

    // 如果没有有效移动，返回-1
    if (directions.empty()) {
        return -1;
    }

    // 按照评估值降序排序，先搜索有希望的移动
    std::sort(directions.begin(), directions.end(), [](auto const& a, auto const& b) { return a.second > b.second; });

    // 尝试每个方向，找出最佳移动
    double bestScore = -std::numeric_limits<double>::infinity();
    int bestMove     = directions[0].first;  // 默认使用最有希望的移动

    // 获取动态深度，但在这里不直接使用
    // 而是在expectimax函数中使用

    for (auto const& [dir, _] : directions) {
        // 执行移动
        BitBoard newBoard = makeMove(bitBoard, dir);

        // 计算该移动的期望分数，使用alpha-beta剪枝
        double score = expectimax(
            newBoard, 0, 1.0, false, -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());

        // 更新最佳移动
        if (score > bestScore) {
            bestScore = score;
            bestMove  = dir;
        }

        // 输出调试信息
        qDebug() << "Direction:" << dir << "Score:" << score;
    }

    qDebug() << "Best move:" << bestMove << "Score:" << bestScore;
    return bestMove;
}

// 清除Expectimax缓存
void Auto::clearExpectimaxCache() {
    QMutexLocker locker(&cacheMutex);
    expectimaxCache.clear();
}

// 获取数据文件路径
QString Auto::getDataPath() const {
    // 创建数据目录（如果不存在）
    QDir dir;
    QString dataPath = QDir::currentPath() + "/data";
    if (!dir.exists(dataPath)) {
        dir.mkpath(dataPath);
    }
    return dataPath + "/ai_parameters.txt";
}

// 保存参数
bool Auto::saveParameters() {
    QFile file(getDataPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件保存参数:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    for (double weight : weights) {
        out << weight << "\n";
    }

    file.close();
    return true;
}

// 加载参数
bool Auto::loadParameters() {
    QFile file(getDataPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件加载参数:" << file.errorString();
        return false;
    }

    QTextStream in(&file);
    QVector<double> loadedWeights;

    while (!in.atEnd()) {
        QString line  = in.readLine();
        bool ok       = false;
        double weight = line.toDouble(&ok);
        if (ok) {
            loadedWeights.append(weight);
        }
    }

    file.close();

    // 确保加载的权重数量正确
    if (loadedWeights.size() == weights.size()) {
        weights = loadedWeights;
        return true;
    }

    return false;
}

// 停止训练
void Auto::stopTraining() {
    stopTrainingFlag = true;
}

// 重置参数
void Auto::resetParameters() {
    // 重置为默认权重
    weights = {
        10.0,  // 空格数量权重
        2.0,   // 单调性权重
        1.0,   // 平滑度权重
        2.0,   // 角落策略权重
        1.0    // 合并可能性权重
    };
}

// 重置历史最佳分数
void Auto::resetBestHistoricalScore() {
    // 重置参数
    resetParameters();

    // 清除缓存
    clearExpectimaxCache();

    // 删除保存的参数文件
    QFile file(getDataPath());
    if (file.exists()) {
        file.remove();
    }

    qDebug() << "历史最佳分数和参数已重置";
}

// 获取训练进度对象
TrainingProgress* Auto::getTrainingProgress() {
    return trainingProgress;
}

// 获取搜索深度
int Auto::getSearchDepth(BitBoard board) const {
    // 根据空格数量动态调整搜索深度
    int emptyCount = getEmptyTileCount(board);

    // 空格越少，搜索深度越大
    if (emptyCount <= 2) {
        return 6;  // 极少空格时搜索更深
    }
    if (emptyCount <= 4) {
        return 5;
    }
    if (emptyCount <= 6) {
        return 4;
    }
    if (emptyCount <= 8) {
        return 3;
    }
    return 2;  // 空格多时搜索浅一些
}

// Expectimax算法实现
double Auto::expectimax(
    BitBoard board, int depth, double probability, bool isMaximizingPlayer, double alpha, double beta) {
    // 如果概率太小或者达到最大深度，返回评估值
    int maxDepth = getSearchDepth(board);
    if (probability < 0.0001 || depth >= maxDepth) {
        return evaluateBoard(board);
    }

    // 检查缓存
    QMutexLocker locker(&cacheMutex);
    auto it = expectimaxCache.find(board);
    if (it != expectimaxCache.end()) {
        return it->second;
    }
    locker.unlock();

    // 如果游戏结束，返回评估值
    if (isGameOver(board)) {
        return evaluateBoard(board);
    }

    double value = 0.0;

    if (isMaximizingPlayer) {
        // 玩家回合，尝试所有可能的移动
        value                = -std::numeric_limits<double>::infinity();
        bool validMoveExists = false;

        // 先计算每个方向的初始评估值，以便按照有希望的顺序搜索
        std::vector<std::pair<int, double>> directions;
        for (int dir = 0; dir < 4; ++dir) {
            if (canMove(board, dir)) {
                BitBoard newBoard = makeMove(board, dir);
                // 使用简单的启发式评估
                double initialScore = emptyTilesHeuristic(newBoard) + monotonicity(newBoard);
                directions.push_back({dir, initialScore});
            }
        }

        // 按照评估值降序排序，先搜索有希望的移动
        std::sort(
            directions.begin(), directions.end(), [](auto const& a, auto const& b) { return a.second > b.second; });

        for (auto const& [dir, _] : directions) {
            validMoveExists   = true;
            BitBoard newBoard = makeMove(board, dir);
            double moveValue  = expectimax(newBoard, depth + 1, probability, false, alpha, beta);
            value             = std::max(value, moveValue);

            // Alpha-Beta剪枝
            alpha = std::max(alpha, value);
            if (alpha >= beta) {
                break;  // Beta剪枝
            }
        }

        // 如果没有有效移动，返回评估值
        if (!validMoveExists) {
            value = evaluateBoard(board);
        }
    } else {
        // 电脑回合，随机生成新方块
        value          = 0.0;
        int emptyCount = getEmptyTileCount(board);

        if (emptyCount == 0) {
            return evaluateBoard(board);
        }

        // 计算每个空格生成2或4的期望值

        // 优化：如果空格太多，只随机选择一部分进行计算
        std::vector<std::pair<int, int>> emptyPositions;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (getTileValue(board, i, j) == 0) {
                    emptyPositions.push_back({i, j});
                }
            }
        }

        // 如果空格超过4个，随机选择4个进行计算
        if (emptyPositions.size() > 4 && depth > 0) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(emptyPositions.begin(), emptyPositions.end(), gen);
            emptyPositions.resize(4);
        }

        for (auto const& [i, j] : emptyPositions) {
            // 尝试在此位置放置2
            BitBoard newBoard = board;
            setTileValue(newBoard, i, j, 2);
            double newProb = probability * PROBABILITY_2 / emptyCount;
            double val2    = expectimax(newBoard, depth, newProb, true, alpha, beta);

            // 尝试在此位置放置4
            newBoard = board;
            setTileValue(newBoard, i, j, 4);
            newProb     = probability * PROBABILITY_4 / emptyCount;
            double val4 = expectimax(newBoard, depth, newProb, true, alpha, beta);

            // 加权平均
            double positionValue  = (PROBABILITY_2 * val2 + PROBABILITY_4 * val4);
            value                += positionValue / emptyPositions.size();

            // 对于最小化玩家，我们希望找到最小的值
            // 但在这里我们实际上是在计算期望值，所以不进行剪枝
        }
    }

    // 更新缓存
    locker.relock();
    expectimaxCache[board] = value;

    return value;
}

// 评估棋盘状态
double Auto::evaluateBoard(BitBoard board) const {
    // 如果启用了GPU加速，使用GPU版本
    if (useGPU && openclManager != nullptr) {
        return evaluateBoardGPU(board);
    }

    // 组合多种启发式方法
    double score = 0.0;

    // 空格数量评估
    score += weights[0] * emptyTilesHeuristic(board);

    // 单调性评估
    score += weights[1] * monotonicity(board);

    // 平滑度评估
    score += weights[2] * smoothness(board);

    // 角落策略评估
    score += weights[3] * cornerHeuristic(board);

    // 合并可能性评估
    score += weights[4] * mergeHeuristic(board);

    // 超过2048后，考虑草形模式
    int maxTile = getMaxTile(board);
    if (maxTile >= 2048) {
        // 添加草形模式评估，权重随最大方块值增加
        double patternWeight  = std::log2(maxTile) / 11.0;  // 2048的log2是11
        score                += patternWeight * patternHeuristic(board);
    }

    return score;
}

// 空格数量启发式
double Auto::emptyTilesHeuristic(BitBoard board) const {
    int emptyCount = getEmptyTileCount(board);
    return emptyCount;
}

// 单调性启发式（确保数值沿某方向单调增加/减少）
double Auto::monotonicity(BitBoard board) const {
    // 计算四个方向的单调性
    double monotonicityScore = 0.0;

    // 水平方向单调性
    for (int i = 0; i < 4; ++i) {
        // 从左到右单调递增
        int leftToRight = 0;
        // 从右到左单调递增
        int rightToLeft = 0;

        for (int j = 0; j < 3; ++j) {
            int current = getTileValue(board, i, j);
            int next    = getTileValue(board, i, j + 1);

            if (current != 0 && next != 0) {
                if (current > next) {
                    leftToRight += std::log2(current / next);
                } else if (next > current) {
                    rightToLeft += std::log2(next / current);
                }
            }
        }

        // 取较小的惩罚值
        monotonicityScore -= std::min(leftToRight, rightToLeft);
    }

    // 垂直方向单调性
    for (int j = 0; j < 4; ++j) {
        // 从上到下单调递增
        int topToBottom = 0;
        // 从下到上单调递增
        int bottomToTop = 0;

        for (int i = 0; i < 3; ++i) {
            int current = getTileValue(board, i, j);
            int next    = getTileValue(board, i + 1, j);

            if (current != 0 && next != 0) {
                if (current > next) {
                    topToBottom += std::log2(current / next);
                } else if (next > current) {
                    bottomToTop += std::log2(next / current);
                }
            }
        }

        // 取较小的惩罚值
        monotonicityScore -= std::min(topToBottom, bottomToTop);
    }

    return monotonicityScore;
}

// 平滑度启发式（相邻方块数值差异越小越好）
double Auto::smoothness(BitBoard board) const {
    double smoothnessScore = 0.0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = getTileValue(board, i, j);

            if (value != 0) {
                // 检查右侧方块
                if (j < 3) {
                    int rightValue = getTileValue(board, i, j + 1);
                    if (rightValue != 0) {
                        smoothnessScore -= std::abs(std::log2(value) - std::log2(rightValue));
                    }
                }

                // 检查下方方块
                if (i < 3) {
                    int belowValue = getTileValue(board, i + 1, j);
                    if (belowValue != 0) {
                        smoothnessScore -= std::abs(std::log2(value) - std::log2(belowValue));
                    }
                }
            }
        }
    }

    return smoothnessScore;
}

// 角落策略启发式（大数值放在角落）
double Auto::cornerHeuristic(BitBoard board) const {
    double cornerScore = 0.0;

    // 检查四个角落
    int topLeft     = getTileValue(board, 0, 0);
    int topRight    = getTileValue(board, 0, 3);
    int bottomLeft  = getTileValue(board, 3, 0);
    int bottomRight = getTileValue(board, 3, 3);

    // 找出最大值
    int maxCorner = std::max(std::max(topLeft, topRight), std::max(bottomLeft, bottomRight));

    // 如果最大值在角落，给予奖励
    if (maxCorner > 0) {
        cornerScore += std::log2(maxCorner);
    }

    return cornerScore;
}

// 合并可能性启发式（相邻相同数值的方块数量）
double Auto::mergeHeuristic(BitBoard board) const {
    double mergeScore = 0.0;

    // 检查水平方向可合并的方块
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (getTileValue(board, i, j) != 0 && getTileValue(board, i, j) == getTileValue(board, i, j + 1)) {
                mergeScore += std::log2(getTileValue(board, i, j));
            }
        }
    }

    // 检查垂直方向可合并的方块
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (getTileValue(board, i, j) != 0 && getTileValue(board, i, j) == getTileValue(board, i + 1, j)) {
                mergeScore += std::log2(getTileValue(board, i, j));
            }
        }
    }

    return mergeScore;
}

// 模式启发式（奖励特定的棋盘模式）
double Auto::patternHeuristic(BitBoard board) const {
    // 这里实现一个简单的蛇形模式评估
    // 蛇形模式：大数值按照蛇形路径排列
    double patternScore = 0.0;

    // 定义蛇形路径的权重
    double const weights[4][4] = {{15, 14, 13, 12}, {8, 9, 10, 11}, {7, 6, 5, 4}, {0, 1, 2, 3}};

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = getTileValue(board, i, j);
            if (value > 0) {
                patternScore += weights[i][j] * std::log2(value);
            }
        }
    }

    return patternScore;
}

// 学习参数
void Auto::learnParameters(int populationSize, int generations, int simulationsPerEval) {
    // 重置停止标志
    stopTrainingFlag = false;

    // 重置早期停止计数器
    patienceCounter = 0;

    // 初始化学习率
    learningRate = 0.1;

    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());

    // 初始化种群
    std::vector<QVector<double>> population;
    std::vector<double> fitness;

    // 生成初始种群
    for (int i = 0; i < populationSize; ++i) {
        // 基于当前权重生成变异
        QVector<double> individual = weights;

        // 添加随机变异
        individual = mutate(individual, 0.5);

        population.push_back(individual);
        fitness.push_back(0.0);
    }

    // 确保当前权重也在种群中
    population[0] = weights;

    // 记录最佳个体
    QVector<double> bestIndividual = weights;
    double bestFitness             = 0.0;

    // 进化多代
    for (int gen = 0; gen < generations; ++gen) {
        // 检查是否要停止训练
        if (stopTrainingFlag) {
            qDebug() << "训练被用户中断";
            break;
        }

// 评估每个个体
#pragma omp parallel for
        for (int i = 0; i < populationSize; ++i) {
            // 如果这个个体已经评估过，跳过
            if (fitness[i] > 0) {
                continue;
            }

            // 运行多次模拟并取平均分数
            double totalScore = 0.0;

            // 如果启用了GPU加速，使用GPU版本
            if (useGPU && openclManager != nullptr) {
                // 临时设置权重
                QVector<double> oldWeights = weights;
                weights                    = population[i];

                // 使用GPU运行批量模拟
                QVector<double> scores = runSimulationsGPU(weights, simulationsPerEval);

                // 计算总分
                for (double score : scores) {
                    totalScore += score;
                }

                // 恢复原来的权重
                weights = oldWeights;
            } else {
                // 使用CPU运行模拟
                for (int sim = 0; sim < simulationsPerEval; ++sim) {
                    // 临时设置权重
                    QVector<double> oldWeights = weights;
                    weights                    = population[i];

                    // 运行模拟
                    double score  = runSimulation(weights);
                    totalScore   += score;

                    // 恢复原来的权重
                    weights = oldWeights;
                }
            }

            // 计算平均分数
            fitness[i] = totalScore / simulationsPerEval;
        }

        // 找出最佳个体
        int bestIndex = 0;
        for (int i = 0; i < populationSize; ++i) {
            if (fitness[i] > fitness[bestIndex]) {
                bestIndex = i;
            }
        }

        // 获取当前代的最佳适应度
        // 直接使用fitness[bestIndex]进行比较

        // 更新全局最佳个体
        if (fitness[bestIndex] > bestFitness) {
            bestFitness    = fitness[bestIndex];
            bestIndividual = population[bestIndex];

            // 发送进度信号
            trainingProgress->progressUpdated(gen + 1, generations, static_cast<int>(bestFitness), bestIndividual);

            qDebug() << "第" << gen + 1 << "代最佳分数:" << bestFitness;
            qDebug() << "最佳参数:" << bestIndividual;

            // 重置早期停止计数器
            patienceCounter = 0;

            // 如果超过历史最佳分数，更新并保存
            if (bestFitness > bestHistoricalScore) {
                bestHistoricalScore = bestFitness;
                saveParameters();
            }
        } else {
            // 如果没有改进，增加计数器
            patienceCounter++;

            // 调整学习率
            if (patienceCounter > 3) {
                learningRate *= 0.8;  // 减小学习率
                qDebug() << "调整学习率为:" << learningRate;
            }

            // 早期停止检查
            if (patienceCounter > 10) {
                qDebug() << "连续10代没有改进，提前停止训练";
                break;
            }
        }

        // 创建新一代
        std::vector<QVector<double>> newPopulation;
        std::vector<double> newFitness;

        // 精英主义：保留最佳个体
        newPopulation.push_back(bestIndividual);
        newFitness.push_back(bestFitness);

        // 生成新一代
        while (newPopulation.size() < populationSize) {
            // 选择父代（锁定赛选择）
            int parent1Index = tournamentSelect(population, fitness, 3);
            int parent2Index = tournamentSelect(population, fitness, 3);

            // 交叉
            QVector<double> child = crossover(population[parent1Index], population[parent2Index]);

            // 变异，使用自适应学习率
            child = mutate(child, 0.1 * learningRate);

            // 添加到新种群
            newPopulation.push_back(child);
            newFitness.push_back(0.0);
        }

        // 替换种群
        population = newPopulation;
        fitness    = newFitness;
    }

    // 训练结束，更新权重
    weights = bestIndividual;

    // 发送训练完成信号
    trainingProgress->trainingCompleted(static_cast<int>(bestFitness), bestIndividual);

    // 保存参数
    saveParameters();

    qDebug() << "训练完成，最终分数:" << bestFitness;
}

// 锁定赛选择
int Auto::tournamentSelect(std::vector<QVector<double>> const& population,
                           std::vector<double> const& fitness,
                           int tournamentSize) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, population.size() - 1);

    int bestIndex      = dist(gen);
    double bestFitness = fitness[bestIndex];

    for (int i = 1; i < tournamentSize; ++i) {
        int index = dist(gen);
        if (fitness[index] > bestFitness) {
            bestIndex   = index;
            bestFitness = fitness[index];
        }
    }

    return bestIndex;
}

// 运行模拟
double Auto::runSimulation(QVector<double> const& params) {
    // 如果启用了GPU加速，使用GPU版本
    if (useGPU && openclManager != nullptr) {
        // 运行单个模拟，取平均值
        QVector<double> scores = runSimulationsGPU(params, 1);
        if (!scores.isEmpty()) {
            return scores[0];
        }
    }

    // 保存原始权重
    QVector<double> oldWeights = weights;

    // 设置新权重
    weights = params;

    // 初始化棋盘
    BitBoard board = 0;

    // 添加两个初始方块
    board = addRandomTile(board);
    board = addRandomTile(board);

    // 记录分数
    int score   = 0;
    int moves   = 0;
    int maxTile = 0;

    // 运行游戏直到结束
    while (!isGameOver(board) && moves < 1000) {
        // 找出最佳移动
        int bestMove     = -1;
        double bestScore = -std::numeric_limits<double>::infinity();

        for (int dir = 0; dir < 4; ++dir) {
            if (canMove(board, dir)) {
                BitBoard newBoard = makeMove(board, dir);
                double moveScore  = expectimax(newBoard, 0, 1.0, false);

                if (moveScore > bestScore) {
                    bestScore = moveScore;
                    bestMove  = dir;
                }
            }
        }

        // 如果没有有效移动，结束游戏
        if (bestMove == -1) {
            break;
        }

        // 执行移动
        board = makeMove(board, bestMove);

        // 添加新方块
        board = addRandomTile(board);

        // 更新分数和移动次数
        score = getScore(board);
        moves++;

        // 更新最大方块
        maxTile = std::max(maxTile, getMaxTile(board));
    }

    // 恢复原始权重
    weights = oldWeights;

    // 返回分数，考虑最大方块和移动次数
    return score + (std::log2(maxTile) * 1000) + (moves * 10);
}

// 交叉操作
QVector<double> Auto::crossover(QVector<double> const& parent1, QVector<double> const& parent2) {
    QVector<double> child(parent1.size());

    // 单点交叉
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, parent1.size() - 1);

    int crossoverPoint = dist(gen);

    for (int i = 0; i < parent1.size(); ++i) {
        if (i < crossoverPoint) {
            child[i] = parent1[i];
        } else {
            child[i] = parent2[i];
        }
    }

    return child;
}

// 变异操作
QVector<double> Auto::mutate(QVector<double> const& individual, double mutationRate) {
    QVector<double> mutated = individual;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0.0, 1.0);
    std::normal_distribution<> normal(0.0, 1.0);

    for (int i = 0; i < mutated.size(); ++i) {
        // 按照变异率决定是否变异
        if (dist(gen) < mutationRate) {
            // 添加正态分布的随机值
            mutated[i] += normal(gen) * mutated[i] * 0.2;

            // 确保权重为正值
            if (mutated[i] < 0.01) {
                mutated[i] = 0.01;
            }
        }
    }

    return mutated;
}

// 移除重复的isGPUEnabled实现，该函数已在auto_gpu.cpp中定义
