#include "autoplayer.h"

#include "evaluation/merge.h"
#include "evaluation/monotonicity.h"
#include "evaluation/smoothness.h"
#include "evaluation/snake.h"
#include "evaluation/tile.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

// Constants for the AutoPlayer class only
static int const AUTO_MAX_DEPTH       = 7;  // 增加搜索深度，从5提高到7
static float const AUTO_PROBABILITY_2 = 0.9f;
static float const AUTO_PROBABILITY_4 = 0.1f;

// Default weights for evaluation functions - 基于研究调整的最佳权重值
float const DEFAULT_EMPTY_WEIGHT  = 2.7f;  // 空位的权重
float const DEFAULT_MONO_WEIGHT   = 1.0f;  // 单调性的权重
float const DEFAULT_SMOOTH_WEIGHT = 0.1f;  // 平滑性权重
float const DEFAULT_CORNER_WEIGHT = 2.0f;  // 角落策略的权重
float const DEFAULT_SNAKE_WEIGHT  = 4.0f;  // 蛇形模式权重（新增）
float const DEFAULT_MERGE_WEIGHT  = 1.0f;  // 合并机会权重（新增）
float const DEFAULT_TILE_WEIGHT   = 1.5f;  // 方块值权重（新增）

// BitBoard utility functions
uint64_t const ROW_MASK = 0xFF'FFULL;
uint64_t const COL_MASK = 0x00'0F'00'0F'00'0F'00'0FULL;

// Constructor
AutoPlayer::AutoPlayer(QObject* parent)
    : QObject(parent),
      autoPlaying(false),
      training(false),
      worker(nullptr),
      lastCalculatedMove(-1),
      emptyWeight(DEFAULT_EMPTY_WEIGHT),
      monoWeight(DEFAULT_MONO_WEIGHT),
      smoothWeight(DEFAULT_SMOOTH_WEIGHT),
      cornerWeight(DEFAULT_CORNER_WEIGHT),
      snakeWeight(DEFAULT_SNAKE_WEIGHT),
      mergeWeight(DEFAULT_MERGE_WEIGHT),
      tileWeight(DEFAULT_TILE_WEIGHT) {
    // Initialize random number generator
    std::random_device rd;
    rng = std::mt19937(rd());

    // Create worker thread
    worker = new AIWorker(this);
    connect(worker, &AIWorker::moveCalculated, this, &AutoPlayer::handleMoveCalculated);

    // Load parameters if available
    loadParameters();
}

// Destructor
AutoPlayer::~AutoPlayer() {
    // Save parameters when destroyed
    if (training) {
        saveParameters();
    }
}

// Start auto-play
void AutoPlayer::startAutoPlay() {
    autoPlaying = true;
}

// Stop auto-play
void AutoPlayer::stopAutoPlay() {
    autoPlaying = false;

    // Stop the worker thread
    if (worker) {
        worker->stopCalculation();
    }
}

// Check if auto-play is active
bool AutoPlayer::isAutoPlaying() const {
    return autoPlaying;
}

// Get the best move for the current board state
int AutoPlayer::getBestMove(QVector<QVector<int>> const& board) {
    // Request the worker to calculate the move
    worker->requestMove(board);

    // Return the last calculated move (will be updated when worker finishes)
    return lastCalculatedMove;
}

// Handle the move calculated by the worker thread
void AutoPlayer::handleMoveCalculated(int direction) {
    lastCalculatedMove = direction;
    emit moveDecided(direction);
}

// Start training with a specified number of iterations
void AutoPlayer::startTraining(int iterations) {
    if (training) {
        return;
    }

    training = true;

    // 参数搜索范围
    std::uniform_real_distribution<float> weightDist(0.1f, 5.0f);

    // 当前最佳权重和得分
    float bestEmptyWeight  = emptyWeight;
    float bestMonoWeight   = monoWeight;
    float bestSmoothWeight = smoothWeight;
    float bestCornerWeight = cornerWeight;
    float bestAverageScore = 0.0f;

    // 运行多次迭代以找到最佳权重组合
    for (int i = 0; i < iterations && training; ++i) {
        // 随机生成权重或在当前最佳权重附近搜索
        float testEmptyWeight, testMonoWeight, testSmoothWeight, testCornerWeight;

        if (i < iterations / 3 || bestAverageScore == 0.0f) {
            // 前1/3的迭代完全随机搜索
            testEmptyWeight  = weightDist(rng);
            testMonoWeight   = weightDist(rng);
            testSmoothWeight = weightDist(rng);
            testCornerWeight = weightDist(rng);
        } else {
            // 后期在当前最佳权重附近搜索，加入少量随机性
            std::normal_distribution<float> localDist(0.0f, 0.3f);
            testEmptyWeight  = std::max(0.1f, bestEmptyWeight + localDist(rng));
            testMonoWeight   = std::max(0.1f, bestMonoWeight + localDist(rng));
            testSmoothWeight = std::max(0.1f, bestSmoothWeight + localDist(rng));
            testCornerWeight = std::max(0.1f, bestCornerWeight + localDist(rng));
        }

        // 使用当前权重进行多次游戏模拟，计算平均分数
        float totalScore = 0.0f;
        int numGames     = 5;  // 每组权重测试5场游戏

        for (int game = 0; game < numGames; ++game) {
            // 模拟一场游戏，使用当前测试的权重
            float gameScore  = simulateGame(testEmptyWeight,
                                           testMonoWeight,
                                           testSmoothWeight,
                                           testCornerWeight,
                                           DEFAULT_SNAKE_WEIGHT,
                                           DEFAULT_MERGE_WEIGHT,
                                           DEFAULT_TILE_WEIGHT);
            totalScore      += gameScore;
        }

        float averageScore = totalScore / numGames;

        // 如果这组权重的表现更好，则更新最佳权重
        if (averageScore > bestAverageScore) {
            bestAverageScore = averageScore;
            bestEmptyWeight  = testEmptyWeight;
            bestMonoWeight   = testMonoWeight;
            bestSmoothWeight = testSmoothWeight;
            bestCornerWeight = testCornerWeight;
        }

        // 更新进度
        emit trainingProgress(i, iterations);
    }

    // 训练完成后更新权重
    if (training) {
        emptyWeight  = bestEmptyWeight;
        monoWeight   = bestMonoWeight;
        smoothWeight = bestSmoothWeight;
        cornerWeight = bestCornerWeight;

        // 保存参数
        saveParameters();
    }

    training = false;
    emit trainingComplete();
}

// 模拟一场游戏并返回得分
float AutoPlayer::simulateGame(
    float emptyW, float monoW, float smoothW, float cornerW, float snakeW, float mergeW, float tileW) {
    // 初始化游戏板
    BitBoard board = 0;

    // 添加两个初始方块
    board = addRandomTile(board);
    board = addRandomTile(board);

    // 游戏循环
    bool gameOver = false;
    int maxTile   = 0;
    int movesMade = 0;

    while (!gameOver && movesMade < 1000) {  // 设置最大移动次数以防止无限循环
        // 使用Expectimax算法确定最佳移动方向
        float score   = 0.0f;
        int direction = expectimax(board, 0, true, score, emptyW, monoW, smoothW, cornerW, snakeW, mergeW, tileW);

        // 如果没有有效移动，游戏结束
        if (direction == -1) {
            gameOver = true;
            continue;
        }

        // 执行移动
        BitBoard newBoard = makeMove(board, direction);

        // 如果棋盘没有变化，尝试其他方向
        if (newBoard == board) {
            // 尝试其他方向
            bool moved = false;
            for (int dir = 0; dir < 4; ++dir) {
                if (dir != direction) {
                    newBoard = makeMove(board, dir);
                    if (newBoard != board) {
                        moved = true;
                        break;
                    }
                }
            }

            if (!moved) {
                gameOver = true;
                continue;
            }
        }

        // 添加新的随机方块
        board = addRandomTile(newBoard);

        // 更新最大方块值
        maxTile = std::max(maxTile, getMaxTileValue(board));

        // 增加移动计数
        movesMade++;
    }

    // 计算游戏得分
    // 我们使用最大方块值和空格数量作为评分标准
    float gameScore = maxTile * 10 + countEmptyTiles(board) * 5;

    return gameScore;
}

// 添加一个随机方块到棋盘
BitBoard AutoPlayer::addRandomTile(BitBoard board) {
    std::vector<int> emptyCells;

    // 找到所有空位置
    for (int i = 0; i < 16; ++i) {
        int shift = 4 * i;
        if (((board >> shift) & 0xF) == 0) {
            emptyCells.push_back(i);
        }
    }

    // 如果没有空位置，返回原始棋盘
    if (emptyCells.empty()) {
        return board;
    }

    // 随机选择一个空位置
    std::uniform_int_distribution<int> posDist(0, emptyCells.size() - 1);
    int randomPos = emptyCells[posDist(rng)];

    // 随机选择值（2或4）
    std::uniform_real_distribution<float> valueDist(0.0f, 1.0f);
    int value = (valueDist(rng) < 0.9f) ? 1 : 2;  // 1代表2，2代表4

    // 将新方块添加到棋盘
    return board | (static_cast<BitBoard>(value) << (4 * randomPos));
}

// 获取棋盘上的最大方块值（作为2的幂）
int AutoPlayer::getMaxTileValue(BitBoard board) {
    int maxValue = 0;

    for (int i = 0; i < 16; ++i) {
        int shift = 4 * i;
        int value = (board >> shift) & 0xF;
        maxValue  = std::max(maxValue, value);
    }

    return maxValue;
}

// Stop training
void AutoPlayer::stopTraining() {
    training = false;
}

// Check if training is active
bool AutoPlayer::isTraining() const {
    return training;
}

// Load parameters from file
bool AutoPlayer::loadParameters() {
    // Create data directory if it doesn't exist
    QDir dir;
    QString dataPath = "data";
    if (!dir.exists(dataPath)) {
        dir.mkpath(dataPath);
    }

    QFile file(dataPath + "/parameters.json");
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data   = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj   = doc.object();

    if (obj.contains("emptyWeight")) {
        emptyWeight = obj["emptyWeight"].toDouble();
    }
    if (obj.contains("monoWeight")) {
        monoWeight = obj["monoWeight"].toDouble();
    }
    if (obj.contains("smoothWeight")) {
        smoothWeight = obj["smoothWeight"].toDouble();
    }
    if (obj.contains("cornerWeight")) {
        cornerWeight = obj["cornerWeight"].toDouble();
    }
    if (obj.contains("snakeWeight")) {
        snakeWeight = obj["snakeWeight"].toDouble();
    }
    if (obj.contains("mergeWeight")) {
        mergeWeight = obj["mergeWeight"].toDouble();
    }
    if (obj.contains("tileWeight")) {
        tileWeight = obj["tileWeight"].toDouble();
    }

    file.close();
    return true;
}

// Save parameters to file
bool AutoPlayer::saveParameters() {
    QDir dir;
    QString dataPath = "data";
    if (!dir.exists(dataPath)) {
        dir.mkpath(dataPath);
    }

    QFile file(dataPath + "/parameters.json");
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonObject obj;
    obj["emptyWeight"]  = emptyWeight;
    obj["monoWeight"]   = monoWeight;
    obj["smoothWeight"] = smoothWeight;
    obj["cornerWeight"] = cornerWeight;
    obj["snakeWeight"]  = snakeWeight;
    obj["mergeWeight"]  = mergeWeight;
    obj["tileWeight"]   = tileWeight;

    QJsonDocument doc(obj);
    file.write(doc.toJson());
    file.close();
    return true;
}

// Convert QVector board to BitBoard representation
BitBoard AutoPlayer::convertToBitBoard(QVector<QVector<int>> const& board) {
    BitBoard result = 0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = board[i][j];
            int power = 0;

            // Convert value to power of 2 (0 remains 0)
            if (value > 0) {
                power = static_cast<int>(std::log2(value));
            }

            // Shift and insert into BitBoard
            result |= static_cast<BitBoard>(power) << (4 * (4 * i + j));
        }
    }

    return result;
}

// Convert BitBoard to QVector board
QVector<QVector<int>> AutoPlayer::convertFromBitBoard(BitBoard board) {
    QVector<QVector<int>> result(4, QVector<int>(4, 0));

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int shift = 4 * (4 * i + j);
            int power = (board >> shift) & 0xF;

            // Convert power back to value (0 remains 0)
            if (power > 0) {
                result[i][j] = 1 << power;
            } else {
                result[i][j] = 0;
            }
        }
    }

    return result;
}

// Expectimax algorithm implementation
int AutoPlayer::expectimax(BitBoard board, int depth, bool maximizingPlayer, float& score) {
    // Terminal conditions
    if (depth >= AUTO_MAX_DEPTH) {
        score = evaluateBoard(board);
        return -1;
    }

    if (maximizingPlayer) {
        // Player's turn - choose the best move
        float bestScore   = -std::numeric_limits<float>::infinity();
        int bestDirection = -1;

        for (int direction = 0; direction < 4; ++direction) {
            if (canMove(board, direction)) {
                BitBoard newBoard  = makeMove(board, direction);
                float currentScore = 0.0f;
                expectimax(newBoard, depth + 1, false, currentScore);

                if (currentScore > bestScore) {
                    bestScore     = currentScore;
                    bestDirection = direction;
                }
            }
        }

        score = bestScore;
        return bestDirection;
    } else {
        // Computer's turn - average of all possible tile placements
        auto possibleNewTiles = getPossibleNewTiles(board);
        if (possibleNewTiles.empty()) {
            score = evaluateBoard(board);
            return -1;
        }

        float expectedScore = 0.0f;
        for (auto const& [newBoard, probability] : possibleNewTiles) {
            float currentScore = 0.0f;
            expectimax(newBoard, depth + 1, true, currentScore);
            expectedScore += probability * currentScore;
        }

        score = expectedScore;
        return -1;
    }
}

// 重载的expectimax函数实现，带有权重参数
int AutoPlayer::expectimax(BitBoard board,
                           int depth,
                           bool maximizingPlayer,
                           float& score,
                           float emptyWeight,
                           float monoWeight,
                           float smoothWeight,
                           float cornerWeight,
                           float snakeWeight,
                           float mergeWeight,
                           float tileWeight) {
    // 缓存当前权重
    float originalEmptyWeight  = this->emptyWeight;
    float originalMonoWeight   = this->monoWeight;
    float originalSmoothWeight = this->smoothWeight;
    float originalCornerWeight = this->cornerWeight;
    float originalSnakeWeight  = this->snakeWeight;
    float originalMergeWeight  = this->mergeWeight;
    float originalTileWeight   = this->tileWeight;

    // 临时设置指定的权重
    this->emptyWeight  = emptyWeight;
    this->monoWeight   = monoWeight;
    this->smoothWeight = smoothWeight;
    this->cornerWeight = cornerWeight;
    this->snakeWeight  = snakeWeight;
    this->mergeWeight  = mergeWeight;
    this->tileWeight   = tileWeight;

    // 实现expectimax算法
    int result = -1;

    // Terminal conditions
    if (depth >= AUTO_MAX_DEPTH) {
        score  = evaluateBoard(board);
        result = -1;
    } else if (maximizingPlayer) {
        // Player's turn - choose the best move
        float bestScore   = -std::numeric_limits<float>::infinity();
        int bestDirection = -1;

        for (int direction = 0; direction < 4; ++direction) {
            if (canMove(board, direction)) {
                BitBoard newBoard  = makeMove(board, direction);
                float currentScore = 0.0f;
                expectimax(newBoard,
                           depth + 1,
                           false,
                           currentScore,
                           emptyWeight,
                           monoWeight,
                           smoothWeight,
                           cornerWeight,
                           snakeWeight,
                           mergeWeight,
                           tileWeight);

                if (currentScore > bestScore) {
                    bestScore     = currentScore;
                    bestDirection = direction;
                }
            }
        }

        score  = bestScore;
        result = bestDirection;
    } else {
        // Computer's turn - average of all possible tile placements
        auto possibleNewTiles = getPossibleNewTiles(board);
        if (possibleNewTiles.empty()) {
            score  = evaluateBoard(board);
            result = -1;
        } else {
            float expectedScore = 0.0f;
            for (auto const& [newBoard, probability] : possibleNewTiles) {
                float currentScore = 0.0f;
                expectimax(newBoard,
                           depth + 1,
                           true,
                           currentScore,
                           emptyWeight,
                           monoWeight,
                           smoothWeight,
                           cornerWeight,
                           snakeWeight,
                           mergeWeight,
                           tileWeight);
                expectedScore += probability * currentScore;
            }

            score  = expectedScore;
            result = -1;
        }
    }

    // 恢复原始权重
    this->emptyWeight  = originalEmptyWeight;
    this->monoWeight   = originalMonoWeight;
    this->smoothWeight = originalSmoothWeight;
    this->cornerWeight = originalCornerWeight;
    this->snakeWeight  = originalSnakeWeight;
    this->mergeWeight  = originalMergeWeight;
    this->tileWeight   = originalTileWeight;

    return result;
}

// Evaluate the board state using multiple heuristics
float AutoPlayer::evaluateBoard(BitBoard board) {
    // 检查缓存，避免重复计算
    if (evaluationCache.find(board) != evaluationCache.end()) {
        return evaluationCache[board];
    }

    float score = 0.0f;

    // 基本评估函数
    score += emptyWeight * countEmptyTiles(board);
    score += monoWeight * calculateMonotonicity(board);
    score += smoothWeight * calculateSmoothness(board);
    score += cornerWeight * cornerStrategy(board);
    score += snakeWeight * snakePatternStrategy(board);
    score += mergeWeight * mergeOpportunityScore(board);
    score += tileWeight * weightedTileScore(board);

    // 缓存评估结果
    evaluationCache[board] = score;

    return score;
}

// Count the number of empty tiles
float AutoPlayer::countEmptyTiles(BitBoard board) {
    int count = 0;

    for (int i = 0; i < 16; ++i) {
        int shift = 4 * i;
        int value = (board >> shift) & 0xF;

        if (value == 0) {
            count++;
        }
    }

    return static_cast<float>(count);
}

// Calculate monotonicity (tiles arranged in order)
float AutoPlayer::calculateMonotonicity(BitBoard board) {
    // Use the monotonicity evaluation function from the evaluation module
    return Monotonicity::calculateMonotonicity(board);
}

// Calculate smoothness (adjacent tiles have similar values)
float AutoPlayer::calculateSmoothness(BitBoard board) {
    // Use the smoothness evaluation function from the evaluation module
    return Smoothness::calculateSmoothness(board);
}

// Corner strategy (high-value tiles in corners)
float AutoPlayer::cornerStrategy(BitBoard board) {
    // This function is now implemented in the evaluation module
    return Snake::calculateCornerScore(board);
}

// 蛇形排列策略 - 鼓励大数值沿特定路径排列（"蛇形"）
float AutoPlayer::snakePatternStrategy(BitBoard board) {
    // 使用Snake评估模块中的蛇形路径评估函数
    return Snake::calculateSnakePattern(board);
}

// 合并机会评分 - 评估相邻方块可能合并的机会
float AutoPlayer::mergeOpportunityScore(BitBoard board) {
    // 使用Merge评估模块中的合并机会评估函数
    return Merge::calculateMergeScore(board);
}

// 加权方块评分 - 根据方块值给予非线性加权，促进合成更高的方块
float AutoPlayer::weightedTileScore(BitBoard board) {
    // 使用Tile评估模块中的加权方块评分函数
    return Tile::calculateWeightedTileScore(board);
}

// Make a move in the specified direction
BitBoard AutoPlayer::makeMove(BitBoard board, int direction) {
    BitBoard newBoard = board;
    BitBoard row;

    // Process each row or column based on direction
    switch (direction) {
        case 0:  // Up
            for (int col = 0; col < 4; ++col) {
                // Extract column
                row = 0;
                for (int i = 0; i < 4; ++i) {
                    int value  = (board >> (4 * (4 * i + col))) & 0xF;
                    row       |= static_cast<BitBoard>(value) << (4 * i);
                }

                // Process column
                row = processTiles(row);

                // Put column back
                for (int i = 0; i < 4; ++i) {
                    // Clear the position in the new board
                    newBoard &= ~(static_cast<BitBoard>(0xF) << (4 * (4 * i + col)));
                    // Set the new value
                    int value  = (row >> (4 * i)) & 0xF;
                    newBoard  |= static_cast<BitBoard>(value) << (4 * (4 * i + col));
                }
            }
            break;

        case 1:  // Right
            for (int row_idx = 0; row_idx < 4; ++row_idx) {
                // Extract row
                row = (board >> (16 * row_idx)) & 0xFF'FF;

                // Process row (reverse for right direction)
                row = reverseRow(row);
                row = processTiles(row);
                row = reverseRow(row);

                // Put row back
                newBoard &= ~(static_cast<BitBoard>(0xFF'FF) << (16 * row_idx));
                newBoard |= row << (16 * row_idx);
            }
            break;

        case 2:  // Down
            for (int col = 0; col < 4; ++col) {
                // Extract column
                row = 0;
                for (int i = 0; i < 4; ++i) {
                    int value  = (board >> (4 * (4 * i + col))) & 0xF;
                    row       |= static_cast<BitBoard>(value) << (4 * i);
                }

                // Process column (reverse for down direction)
                row = reverseRow(row);
                row = processTiles(row);
                row = reverseRow(row);

                // Put column back
                for (int i = 0; i < 4; ++i) {
                    // Clear the position in the new board
                    newBoard &= ~(static_cast<BitBoard>(0xF) << (4 * (4 * i + col)));
                    // Set the new value
                    int value  = (row >> (4 * i)) & 0xF;
                    newBoard  |= static_cast<BitBoard>(value) << (4 * (4 * i + col));
                }
            }
            break;

        case 3:  // Left
            for (int row_idx = 0; row_idx < 4; ++row_idx) {
                // Extract row
                row = (board >> (16 * row_idx)) & 0xFF'FF;

                // Process row
                row = processTiles(row);

                // Put row back
                newBoard &= ~(static_cast<BitBoard>(0xFF'FF) << (16 * row_idx));
                newBoard |= row << (16 * row_idx);
            }
            break;
    }

    return newBoard;
}

// Helper function to process a row or column of tiles
BitBoard AutoPlayer::processTiles(BitBoard row) {
    BitBoard result = 0;
    int position    = 0;
    int prev_tile   = 0;

    // Move all non-zero tiles to the left and merge if possible
    for (int i = 0; i < 4; ++i) {
        int current = (row >> (4 * i)) & 0xF;
        if (current == 0) {
            continue;
        }

        if (prev_tile == 0) {
            // First tile or after a merge
            prev_tile = current;
        } else if (prev_tile == current) {
            // Merge tiles
            result |= static_cast<BitBoard>(prev_tile + 1) << (4 * position);
            position++;
            prev_tile = 0;
        } else {
            // Different tiles, can't merge
            result |= static_cast<BitBoard>(prev_tile) << (4 * position);
            position++;
            prev_tile = current;
        }
    }

    // Don't forget the last tile if there is one
    if (prev_tile != 0) {
        result |= static_cast<BitBoard>(prev_tile) << (4 * position);
    }

    return result;
}

// Helper function to reverse a row or column
BitBoard AutoPlayer::reverseRow(BitBoard row) {
    BitBoard result = 0;
    for (int i = 0; i < 4; ++i) {
        int value  = (row >> (4 * i)) & 0xF;
        result    |= static_cast<BitBoard>(value) << (4 * (3 - i));
    }
    return result;
}

// Check if a move is possible in the specified direction
bool AutoPlayer::canMove(BitBoard board, int direction) {
    // A move is possible if the board changes after making the move
    return board != makeMove(board, direction);
}

// Get all possible new tile placements with their probabilities
std::vector<std::pair<BitBoard, float>> AutoPlayer::getPossibleNewTiles(BitBoard board) {
    std::vector<std::pair<BitBoard, float>> result;

    // Find all empty cells
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int shift = 4 * (4 * i + j);
            int value = (board >> shift) & 0xF;

            if (value == 0) {
                // This cell is empty, add a 2 and a 4 with appropriate probabilities
                BitBoard newBoard2 = board | (static_cast<BitBoard>(1) << shift);  // Add a 2 (2^1)
                BitBoard newBoard4 = board | (static_cast<BitBoard>(2) << shift);  // Add a 4 (2^2)

                result.push_back(std::make_pair(newBoard2, 0.9f));  // 90% chance for a 2
                result.push_back(std::make_pair(newBoard4, 0.1f));  // 10% chance for a 4
            }
        }
    }

    return result;
}
