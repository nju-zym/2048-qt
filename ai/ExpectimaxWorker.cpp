#include "ExpectimaxWorker.h"
#include "evaluation/MonotonicityEval.h"
#include "evaluation/SmoothnessEval.h"
#include "evaluation/FreeTilesEval.h"
#include "evaluation/MergeEval.h"
#include "evaluation/TilePlacementEval.h"
#include <limits>
#include <algorithm>
#include <QDebug>

ExpectimaxWorker::ExpectimaxWorker(QObject* parent)
    : QObject(parent), abort(false), restart(false), searchDepth(3) {
    // 将worker移动到工作线程
    moveToThread(&workerThread);
    
    // 连接线程启动信号到run槽
    connect(&workerThread, &QThread::started, this, &ExpectimaxWorker::run);
    
    // 启动工作线程
    workerThread.start();
}

ExpectimaxWorker::~ExpectimaxWorker() {
    // 停止线程
    {
        QMutexLocker locker(&mutex);
        abort = true;
        condition.wakeOne();
    }
    
    workerThread.quit();
    workerThread.wait();
}

void ExpectimaxWorker::calculateBestMove(const BitBoard& board, int depth) {
    QMutexLocker locker(&mutex);
    
    // 保存当前任务参数
    currentBoard = board;
    searchDepth = depth;
    
    // 如果线程已经在运行，设置restart标志
    if (!restart) {
        restart = true;
        condition.wakeOne();
    }
}

void ExpectimaxWorker::stopCalculation() {
    QMutexLocker locker(&mutex);
    restart = false;
}

void ExpectimaxWorker::run() {
    forever {
        // 等待任务
        {
            QMutexLocker locker(&mutex);
            while (!restart && !abort) {
                condition.wait(&mutex);
            }
            
            if (abort) {
                return;
            }
            
            restart = false;
        }
        
        // 计算最佳移动
        BitBoard board;
        int depth;
        
        {
            QMutexLocker locker(&mutex);
            board = currentBoard;
            depth = searchDepth;
        }
        
        // 如果游戏已经结束，返回任意方向
        if (board.isGameOver()) {
            emit moveCalculated(0);  // 上
            continue;
        }
        
        float bestScore = -std::numeric_limits<float>::infinity();
        int bestDirection = -1;
        
        // 尝试所有四个方向
        for (int direction = 0; direction < 4; ++direction) {
            // 检查是否需要重新开始或中止
            {
                QMutexLocker locker(&mutex);
                if (restart || abort) {
                    break;
                }
            }
            
            BitBoard newBoard = board.move(static_cast<BitBoard::Direction>(direction));
            
            // 如果移动有效（棋盘发生了变化）
            if (newBoard != board) {
                float score = expectimax(newBoard, depth - 1, false);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestDirection = direction;
                }
            }
        }
        
        // 检查是否需要重新开始或中止
        {
            QMutexLocker locker(&mutex);
            if (restart || abort) {
                continue;
            }
        }
        
        // 如果没有找到有效移动，返回任意方向
        if (bestDirection == -1) {
            bestDirection = 0;  // 上
        }
        
        // 发出移动计算完成信号
        emit moveCalculated(bestDirection);
    }
}

float ExpectimaxWorker::expectimax(const BitBoard& board, int depth, bool maximizingPlayer) {
    // 检查是否需要重新开始或中止
    {
        QMutexLocker locker(&mutex);
        if (restart || abort) {
            return 0.0f;
        }
    }
    
    // 达到搜索深度限制或游戏结束
    if (depth == 0 || board.isGameOver()) {
        return evaluateBoard(board);
    }
    
    if (maximizingPlayer) {
        // 玩家回合，尝试所有可能的移动
        float bestScore = -std::numeric_limits<float>::infinity();
        
        for (int direction = 0; direction < 4; ++direction) {
            // 检查是否需要重新开始或中止
            {
                QMutexLocker locker(&mutex);
                if (restart || abort) {
                    return 0.0f;
                }
            }
            
            BitBoard newBoard = board.move(static_cast<BitBoard::Direction>(direction));
            
            // 如果移动有效（棋盘发生了变化）
            if (newBoard != board) {
                float score = expectimax(newBoard, depth - 1, false);
                bestScore = std::max(bestScore, score);
            }
        }
        
        // 如果没有有效移动，返回当前棋盘的评估分数
        if (bestScore == -std::numeric_limits<float>::infinity()) {
            return evaluateBoard(board);
        }
        
        return bestScore;
    } else {
        // 计算机回合，随机放置新方块
        float expectedScore = 0.0f;
        std::vector<BitBoard::Position> emptyPositions = board.getEmptyPositions();
        
        if (emptyPositions.empty()) {
            return evaluateBoard(board);
        }
        
        float probability = 1.0f / emptyPositions.size();
        
        for (const auto& pos : emptyPositions) {
            // 检查是否需要重新开始或中止
            {
                QMutexLocker locker(&mutex);
                if (restart || abort) {
                    return 0.0f;
                }
            }
            
            // 90%概率放置2
            BitBoard boardWith2 = board.placeNewTile(pos, 2);
            expectedScore += 0.9f * probability * expectimax(boardWith2, depth - 1, true);
            
            // 10%概率放置4
            BitBoard boardWith4 = board.placeNewTile(pos, 4);
            expectedScore += 0.1f * probability * expectimax(boardWith4, depth - 1, true);
        }
        
        return expectedScore;
    }
}

float ExpectimaxWorker::evaluateBoard(const BitBoard& board) {
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
    
    return score;
}
