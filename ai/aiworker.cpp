#include "autoplayer.h"
#include "bitboard.h"
#include "evaluation/evaluation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

// AIWorker implementation
AIWorker::AIWorker(QObject* parent) : QThread(parent), abort(false), restart(false) {}

AIWorker::~AIWorker() {
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}

void AIWorker::requestMove(QVector<QVector<int>> const& board) {
    QMutexLocker locker(&mutex);

    // Copy the board
    currentBoard = board;

    // Set restart flag to true to process this new board
    restart = true;

    // If not running, start the thread
    if (!isRunning()) {
        start(QThread::LowPriority);
    } else {
        // Wake up the thread if it's waiting
        condition.wakeOne();
    }
}

void AIWorker::stopCalculation() {
    QMutexLocker locker(&mutex);
    abort = true;
    condition.wakeOne();
}

void AIWorker::run() {
    forever {
        // Get the current board to process
        mutex.lock();
        QVector<QVector<int>> board = currentBoard;
        restart                     = false;
        mutex.unlock();

        // Calculate the best move
        int bestMove = calculateBestMove(board);

        // Check if we should abort or restart
        mutex.lock();
        if (abort) {
            mutex.unlock();
            return;
        }

        if (!restart) {
            // Emit the result if we're not restarting
            emit moveCalculated(bestMove);
        }

        // Wait for more work or abort
        if (!restart) {
            condition.wait(&mutex);
        }

        if (abort) {
            mutex.unlock();
            return;
        }
        mutex.unlock();
    }
}

int AIWorker::calculateBestMove(QVector<QVector<int>> const& board) {
    BitBoard bitBoard = convertToBitBoard(board);

    // 清空缓存，避免缓存过大
    if (transpositionTable.size() > 500000) {
        transpositionTable.clear();
    }

    int bestDirection = -1;
    float bestScore   = -std::numeric_limits<float>::infinity();

    // Try each direction and choose the one with the highest score
    for (int direction = 0; direction < 4; ++direction) {
        // Check if we should abort
        mutex.lock();
        bool shouldAbort = abort || restart;
        mutex.unlock();

        if (shouldAbort) {
            return -1;
        }

        if (canMove(bitBoard, direction)) {
            BitBoard newBoard = makeMove(bitBoard, direction);
            float score       = 0.0f;
            // 使用自适应搜索深度
            expectimax(newBoard, 0, false, score);

            if (score > bestScore) {
                bestScore     = score;
                bestDirection = direction;
            }
        }
    }

    return bestDirection;
}

BitBoard AIWorker::convertToBitBoard(QVector<QVector<int>> const& board) {
    BitBoard result = 0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = board[i][j];
            int power = 0;

            // Convert the tile value to power of 2
            // 0 remains 0, 2 becomes 1, 4 becomes 2, etc.
            if (value > 0) {
                power = static_cast<int>(std::log2(value));
            }

            // Set the value in the bitboard
            result |= static_cast<BitBoard>(power & 0xF) << (4 * (4 * i + j));
        }
    }

    return result;
}

int AIWorker::expectimax(BitBoard board, int depth, bool maximizingPlayer, float& score) {
    // Check if we should abort
    mutex.lock();
    bool shouldAbort = abort || restart;
    mutex.unlock();

    if (shouldAbort) {
        score = 0.0f;
        return -1;
    }

    // 检查缓存
    if (depth > 0 && transpositionTable.contains(board)) {
        CacheEntry entry = transpositionTable.value(board);
        score            = entry.score;
        return entry.bestMove;
    }

    // 获取当前的搜索深度限制
    int maxDepth = 5;  // 默认搜索深度

    // 计算空位数量，用于自适应搜索深度
    if (depth == 0) {
        int emptyCount = 0;
        for (int i = 0; i < 16; ++i) {
            int shift = 4 * i;
            int value = (board >> shift) & 0xF;
            if (value == 0) {
                emptyCount++;
            }
        }

        // 根据空位数量动态调整搜索深度
        if (emptyCount <= 2) {
            maxDepth = 8;  // 空位很少，可以搜索很深
        } else if (emptyCount <= 4) {
            maxDepth = 7;  // 空位较少，可以搜索深一点
        } else if (emptyCount <= 6) {
            maxDepth = 6;  // 标准搜索深度
        } else if (emptyCount <= 10) {
            maxDepth = 5;  // 空位较多，搜索浅一点
        } else {
            maxDepth = 4;  // 空位很多，搜索浅一点
        }
    }

    // Maximum depth reached, evaluate the board
    if (depth >= maxDepth) {
        score = evaluateBoard(board);
        return -1;
    }

    if (maximizingPlayer) {
        // Max player (the AI) tries to maximize the score
        float bestScore   = -std::numeric_limits<float>::infinity();
        int bestDirection = -1;

        // Try each direction
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

        // 将结果存入缓存
        CacheEntry entry;
        entry.score    = bestScore;
        entry.bestMove = bestDirection;
        transpositionTable.insert(board, entry);

        score = bestScore;
        return bestDirection;
    } else {
        // Chance player (the random tile spawn) tries to minimize the score
        float expectedScore = 0.0f;
        int emptyTiles      = 0;
        std::vector<int> emptyPositions;

        // Count empty tiles and store their positions
        for (int i = 0; i < 16; ++i) {
            int shift = 4 * i;
            int value = (board >> shift) & 0xF;
            if (value == 0) {
                emptyTiles++;
                emptyPositions.push_back(i);
            }
        }

        // 优化：如果空位过多，只考虑一部分空位
        // 这样可以显著减少计算量，同时保持足够的准确性
        int const MAX_POSITIONS_TO_CONSIDER = 4;  // 最多考虑四个空位
        if (emptyTiles > MAX_POSITIONS_TO_CONSIDER && depth > 1) {
            // 随机选择一部分空位进行考虑
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(emptyPositions.begin(), emptyPositions.end(), g);
            emptyPositions.resize(MAX_POSITIONS_TO_CONSIDER);
            emptyTiles = MAX_POSITIONS_TO_CONSIDER;
        }

        // 处理选定的空位
        for (int pos : emptyPositions) {
            int shift = 4 * pos;

            // Try placing a 2 (90% chance)
            BitBoard newBoard2 = board | (static_cast<BitBoard>(1) << shift);
            float score2       = 0.0f;
            expectimax(newBoard2, depth + 1, true, score2);

            // Try placing a 4 (10% chance)
            BitBoard newBoard4 = board | (static_cast<BitBoard>(2) << shift);
            float score4       = 0.0f;
            expectimax(newBoard4, depth + 1, true, score4);

            // Weighted average based on spawn probabilities
            expectedScore += (0.9f * score2 + 0.1f * score4) / emptyTiles;
        }

        score = expectedScore;
        return -1;
    }
}

float AIWorker::evaluateBoard(BitBoard board) {
    // 计算游戏阶段，根据最大的瓦片和空位数量
    int maxTile    = 0;
    int emptyCount = 0;

    for (int i = 0; i < 16; ++i) {
        int shift = 4 * i;
        int value = (board >> shift) & 0xF;

        if (value == 0) {
            emptyCount++;
        } else if (value > maxTile) {
            maxTile = value;
        }
    }

    // 根据游戏阶段动态调整权重
    float emptyWeight  = 10.0f;  // 显著增加空位的重要性
    float monoWeight   = 4.0f;   // 显著增加单调性的重要性
    float smoothWeight = 0.1f;   // 减少平滑性的重要性
    float cornerWeight = 50.0f;  // 显著增加角落策略的重要性
    float snakeWeight  = 30.0f;  // 显著增加蛇形策略的重要性
    float mergeWeight  = 1.0f;   // 标准合并权重
    float tileWeight   = 4.0f;   // 增加瓦片值的重要性
    float edgeWeight   = 25.0f;  // 边缘策略权重

    // 早期阶段（最大瓦片 < 8，即256）
    if (maxTile < 8) {
        emptyWeight  = 8.0f;   // 更重视空位
        monoWeight   = 1.0f;   // 减少单调性的重要性
        smoothWeight = 0.3f;   // 增加平滑性的重要性
        cornerWeight = 20.0f;  // 增加角落策略的重要性
        snakeWeight  = 10.0f;  // 增加蛇形策略的重要性
        mergeWeight  = 2.0f;   // 增加合并的重要性
        tileWeight   = 2.0f;   // 增加瓦片值的重要性
        edgeWeight   = 15.0f;  // 边缘策略权重
    }
    // 中期阶段（最大瓦片 8-10，即256-1024）
    else if (maxTile < 11) {
        emptyWeight  = 10.0f;  // 更重视空位
        monoWeight   = 2.0f;   // 增加单调性的重要性
        smoothWeight = 0.1f;   // 标准平滑性权重
        cornerWeight = 25.0f;  // 增加角落策略的重要性
        snakeWeight  = 15.0f;  // 增加蛇形策略的重要性
        mergeWeight  = 1.5f;   // 标准合并权重
        tileWeight   = 3.0f;   // 增加瓦片值的重要性
        edgeWeight   = 20.0f;  // 边缘策略权重
    }
    // 后期阶段（最大瓦片 >= 11，即>=2048）
    else {
        emptyWeight  = 12.0f;  // 显著增加空位的重要性
        monoWeight   = 6.0f;   // 显著增加单调性的重要性
        smoothWeight = 0.05f;  // 减少平滑性的重要性
        cornerWeight = 60.0f;  // 显著增加角落策略的重要性
        snakeWeight  = 40.0f;  // 显著增加蛇形策略的重要性
        mergeWeight  = 1.0f;   // 减少合并的重要性
        tileWeight   = 5.0f;   // 增加瓦片值的重要性
        edgeWeight   = 30.0f;  // 边缘策略权重
    }

    // 如果空位很少，显著增加空位的权重
    if (emptyCount <= 3) {
        emptyWeight *= 2.5f;
    } else if (emptyCount <= 5) {
        emptyWeight *= 1.8f;
    }

    // 使用调整后的权重进行评估
    return Evaluation::evaluateBoard(
        board, emptyWeight, monoWeight, smoothWeight, cornerWeight, snakeWeight, mergeWeight, tileWeight, edgeWeight);
}

// Helper function to check if a move is valid
bool AIWorker::canMove(BitBoard board, int direction) {
    BitBoard newBoard = makeMove(board, direction);
    return newBoard != board;
}

// Helper function to make a move
BitBoard AIWorker::makeMove(BitBoard board, int direction) {
    BitBoard result = board;

    switch (direction) {
        case 0:  // Up
            for (int col = 0; col < 4; ++col) {
                BitBoard column = 0;
                for (int row = 0; row < 4; ++row) {
                    int shift  = 4 * (4 * row + col);
                    column    |= ((board >> shift) & 0xF) << (4 * row);
                }

                column = processTiles(column);

                for (int row = 0; row < 4; ++row) {
                    int shift  = 4 * (4 * row + col);
                    result    &= ~(static_cast<BitBoard>(0xF) << shift);
                    result    |= ((column >> (4 * row)) & 0xF) << shift;
                }
            }
            break;

        case 1:  // Right
            for (int row = 0; row < 4; ++row) {
                int shift        = 4 * 4 * row;
                BitBoard rowBits = (board >> shift) & 0xFF'FF;
                rowBits          = reverseRow(rowBits);
                rowBits          = processTiles(rowBits);
                rowBits          = reverseRow(rowBits);

                result &= ~(static_cast<BitBoard>(0xFF'FF) << shift);
                result |= rowBits << shift;
            }
            break;

        case 2:  // Down
            for (int col = 0; col < 4; ++col) {
                BitBoard column = 0;
                for (int row = 0; row < 4; ++row) {
                    int shift  = 4 * (4 * row + col);
                    column    |= ((board >> shift) & 0xF) << (4 * row);
                }

                column = reverseRow(column);
                column = processTiles(column);
                column = reverseRow(column);

                for (int row = 0; row < 4; ++row) {
                    int shift  = 4 * (4 * row + col);
                    result    &= ~(static_cast<BitBoard>(0xF) << shift);
                    result    |= ((column >> (4 * row)) & 0xF) << shift;
                }
            }
            break;

        case 3:  // Left
            for (int row = 0; row < 4; ++row) {
                int shift        = 4 * 4 * row;
                BitBoard rowBits = (board >> shift) & 0xFF'FF;
                rowBits          = processTiles(rowBits);

                result &= ~(static_cast<BitBoard>(0xFF'FF) << shift);
                result |= rowBits << shift;
            }
            break;

        default:  // Invalid direction, return original board
            break;
    }

    return result;
}

// Helper function to process a row or column of tiles
BitBoard AIWorker::processTiles(BitBoard row) {
    BitBoard result = 0;
    int resultIndex = 0;
    int values[4]   = {0};

    // Extract values
    for (int i = 0; i < 4; ++i) {
        values[i] = (row >> (4 * i)) & 0xF;
    }

    // Move and merge
    for (int i = 0; i < 4; ++i) {
        if (values[i] == 0) {
            continue;
        }

        if (resultIndex > 0 && values[i] == ((result >> (4 * (resultIndex - 1))) & 0xF)) {
            // Merge
            result += static_cast<BitBoard>(1) << (4 * (resultIndex - 1));
        } else {
            // Move
            result |= static_cast<BitBoard>(values[i]) << (4 * resultIndex);
            resultIndex++;
        }
    }

    return result;
}

// Helper function to reverse a row or column
BitBoard AIWorker::reverseRow(BitBoard row) {
    BitBoard result = 0;

    for (int i = 0; i < 4; ++i) {
        result |= ((row >> (4 * i)) & 0xF) << (4 * (3 - i));
    }

    return result;
}
