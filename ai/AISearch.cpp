#include "AISearch.h"

#include <QMutexLocker>  // 添加互斥锁头文件
#include <algorithm>
#include <cmath>
#include <random>  // 添加随机数头文件用于std::shuffle
#include <QDebug>  // 添加QDebug头文件

// 构造函数
AISearch::AISearch() : evaluator(std::make_unique<BoardEvaluator>()), expectimaxCache(std::make_shared<AICache>()) {
    // 使用智能指针创建评估器和缓存系统
}

// 清除缓存
void AISearch::clearCache() {
    QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护缓存清除操作
    expectimaxCache->clear();
}

// findBestMove: 找出最佳移动方向
int AISearch::findBestMove(QVector<QVector<int>> const& board) {
    // 清除缓存以确保最新的计算结果
    clearCache();
    int bestScore     = -1;
    int bestDirection = -1;

    // 获取当前棋盘空格数量和最大方块值
    int emptyCount = GameBoard::countEmptyCells(board);
    int maxValue   = GameBoard::getMaxTile(board);

    // 动态调整初始搜索深度，避免在复杂棋局时内存溢出
    int initialDepth = 5;  // 默认深度减少为5

    // 根据棋盘状态智能调整初始深度
    if (emptyCount <= 4 && maxValue >= 2048) {
        initialDepth = 4;  // 高级棋盘+非常少的空格：使用适中深度
    } else if (emptyCount <= 2) {
        initialDepth = 3;  // 极少空格时，使用更低深度以减少内存压力
    } else if (emptyCount >= 10) {
        initialDepth = 5;  // 空格较多时，使用适中深度减少内存消耗
    } else if (maxValue >= 4096) {
        initialDepth = 4;  // 对于接近胜利的棋盘，使用适中深度
    } else if (emptyCount >= 6 && maxValue >= 1024) {
        initialDepth = 5;  // 中期游戏且有中等大方块时
    } else {
        initialDepth = 4;  // 其他情况使用适中深度
    }

    // 尝试每个方向，计算移动后的棋盘评分
    for (int direction = 0; direction < 4; ++direction) {
        // 创建棋盘副本
        QVector<QVector<int>> boardCopy = board;

        // 模拟移动
        int moveScore = 0;
        bool moved    = GameBoard::simulateMove(boardCopy, direction, moveScore);

        // 如果这个方向可以移动，计算移动后的棋盘评分
        if (moved) {
            int score = 0;

            // 先进行基础评估
            score = evaluator->evaluateAdvancedPattern(boardCopy) + moveScore;

            // 使用expectimax算法进行深度搜索（使用动态深度）
            int simulationScore  = expectimax(boardCopy, initialDepth, false);
            score               += simulationScore;

            if (score > bestScore) {
                bestScore     = score;
                bestDirection = direction;
            }
        }
    }

    // 如果没有有效移动，随机选择一个方向
    if (bestDirection == -1) {
        // 使用更安全的随机数生成
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, 3);
        bestDirection = distrib(gen);
    }

    return bestDirection;
}

// expectimax: 期望最大算法 - 高效版本
int AISearch::expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer) {
    try {
        // 检查缓存
        AICache::BoardState state(boardState, depth, isMaxPlayer);
        {
            QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护缓存读取操作
            if (expectimaxCache->contains(state)) {
                return expectimaxCache->get(state);
            }
        }

        // 绝对深度限制 - 防止过深递归，降低到5以确保安全
        static int const MAX_ABSOLUTE_DEPTH = 5;
        if (depth > MAX_ABSOLUTE_DEPTH) {
            depth = MAX_ABSOLUTE_DEPTH;
        }

        // 如果到达最大深度，返回评估分数
        if (depth <= 0) {
            int score = evaluator->evaluateAdvancedPattern(boardState);
            {
                QMutexLocker locker(&cacheMutex);       // 添加互斥锁保护缓存写入操作
                expectimaxCache->insert(state, score);  // 缓存结果
            }
            return score;
        }

        // 检测游戏是否结束
        if (GameBoard::isGameOver(boardState)) {
            int score = -500000;  // 游戏结束给予大量惩罚
            {
                QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护缓存写入操作
                expectimaxCache->insert(state, score);
            }
            return score;
        }

        // 快速检测最大值和空格数
        int maxValue   = GameBoard::getMaxTile(boardState);
        int emptyCount = GameBoard::countEmptyCells(boardState);

        // 针对即将满的棋盘特别优化，避免内存爆炸
        if (emptyCount <= 3) {
            // 当空格非常少时，减少深度更多，避免内存溢出
            depth = std::min(depth, 2);  // 减少到2，进一步防止栈溢出
        }

        // 对于任何情况，额外深度都不超过1
        int extraDepth = 0;

        // 只在空格较多且方块值不太大的情况下增加额外深度
        if (emptyCount >= 8 && maxValue < 1024) {
            extraDepth = 1;
        }

        // 应用额外深度
        depth += extraDepth;

        // 限制最终深度不超过5
        depth = std::min(depth, 5);

        int result = 0;
        if (isMaxPlayer) {
            // MAX节点：选择最佳移动
            int bestScore = -1;

            // 尝试所有可能的移动
            for (int direction = 0; direction < 4; ++direction) {
                QVector<QVector<int>> boardCopy = boardState;
                int moveScore                   = 0;

                bool moved = GameBoard::simulateMove(boardCopy, direction, moveScore);

                if (moved) {
                    // 递归计算期望分数，如果深度已经很低，避免过多递归
                    int score;
                    if (depth <= 2) {
                        // 对于较低深度，直接使用评估函数，不再递归
                        score = moveScore + evaluator->evaluateAdvancedPattern(boardCopy);
                    } else {
                        score = moveScore + expectimax(boardCopy, depth - 1, false);
                    }
                    bestScore = std::max(bestScore, score);
                }
            }

            result = bestScore > 0 ? bestScore : 0;
        } else {
            // CHANCE节点：随机生成新方块
            // 如果没有空格，返回评估分数
            if (emptyCount == 0) {
                int score = evaluator->evaluateAdvancedPattern(boardState);
                {
                    QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护缓存写入操作
                    expectimaxCache->insert(state, score);
                }
                return score;
            }

            // 始终只模拟一个空格以减少内存使用
            int tilesToSimulate = 1;

            // 确保不会模拟太多空格，避免内存溢出
            tilesToSimulate = std::min(tilesToSimulate, emptyCount);

            double totalScore = 0.0;

            // 只考虑一个随机位置，而不是收集所有空格
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, emptyCount - 1);
            int randomEmptyIndex = distrib(gen);

            // 找到第n个空位
            int currentEmpty = 0;
            int row = -1, col = -1;
            for (int i = 0; i < 4 && row == -1; ++i) {
                for (int j = 0; j < 4 && row == -1; ++j) {
                    if (boardState[i][j] == 0) {
                        if (currentEmpty == randomEmptyIndex) {
                            row = i;
                            col = j;
                        }
                        currentEmpty++;
                    }
                }
            }

            // 如果找到了有效的空位
            if (row != -1 && col != -1) {
                // 模拟生成2的情况
                QVector<QVector<int>> boardWith2 = boardState;
                boardWith2[row][col]             = 2;

                // 对于较低深度，直接使用评估函数避免过多递归
                if (depth <= 2) {
                    totalScore = evaluator->evaluateAdvancedPattern(boardWith2);
                } else {
                    // 对于更高深度，继续递归
                    totalScore = expectimax(boardWith2, depth - 1, true);
                }

                result = static_cast<int>(totalScore);
            } else {
                // 安全措施：如果无法找到空位，返回基础评估分数
                result = evaluator->evaluateAdvancedPattern(boardState);
            }
        }

        // 缓存结果
        {
            QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护缓存写入操作
            expectimaxCache->insert(state, result);
        }

        return result;
    } catch (std::exception const& e) {
        qDebug() << "Error in expectimax:" << QString::fromStdString(e.what());
        qDebug() << "Error in expectimax: " << e.what();
        return evaluator->evaluateAdvancedPattern(boardState);
    } catch (...) {
        // 捕获所有其他异常，确保函数始终返回值
        qDebug() << "Unknown error in expectimax";
        return 0;
    }
}
