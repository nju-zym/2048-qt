#include "ParallelExpectimaxWorker.h"

#include "evaluation/FreeTilesEval.h"
#include "evaluation/MergeEval.h"
#include "evaluation/MonotonicityEval.h"
#include "evaluation/SmoothnessEval.h"
#include "evaluation/TilePlacementEval.h"

#include <QDebug>
#include <QThread>
#include <algorithm>
#include <limits>

ParallelExpectimaxWorker::ParallelExpectimaxWorker(QObject* parent)
    : QObject(parent),
      abort(false),
      restart(false),
      searchDepth(3),
      threadCount(QThread::idealThreadCount() * 2),
      upWatcher(),
      rightWatcher(),
      downWatcher(),
      leftWatcher() {
    // 设置线程池大小
    threadPool.setMaxThreadCount(threadCount);

    // 将worker移动到工作线程
    moveToThread(&workerThread);

    // 连接线程启动信号到run槽
    connect(&workerThread, &QThread::started, this, &ParallelExpectimaxWorker::run);

    // 启动工作线程
    workerThread.start();

    qDebug() << "ParallelExpectimaxWorker created with" << threadCount << "threads";
}

ParallelExpectimaxWorker::~ParallelExpectimaxWorker() {
    // 停止线程
    {
        QMutexLocker locker(&mutex);
        abort = true;
        condition.wakeOne();
    }

    // 等待线程池中的所有任务完成
    threadPool.waitForDone();

    // 退出工作线程
    workerThread.quit();
    workerThread.wait();
}

void ParallelExpectimaxWorker::calculateBestMove(BitBoard const& board, int depth) {
    QMutexLocker locker(&mutex);

    qDebug() << "ParallelExpectimaxWorker: Calculating best move, depth:" << depth;

    // 保存当前任务参数
    currentBoard = board;
    searchDepth  = depth;

    // 如果线程已经在运行，设置restart标志
    if (!restart) {
        restart = true;
        condition.wakeOne();
        qDebug() << "ParallelExpectimaxWorker: Worker thread woken up";
    } else {
        qDebug() << "ParallelExpectimaxWorker: Worker thread already running";
    }
}

void ParallelExpectimaxWorker::stopCalculation() {
    QMutexLocker locker(&mutex);
    restart = false;
}

void ParallelExpectimaxWorker::setThreadCount(int count) {
    if (count > 0) {
        QMutexLocker locker(&mutex);
        threadCount = count;
        threadPool.setMaxThreadCount(count);
    }
}

int ParallelExpectimaxWorker::getThreadCount() const {
    QMutexLocker locker(&mutex);
    return threadCount;
}

void ParallelExpectimaxWorker::setUseAlphaBeta(bool useAlphaBeta) {
    QMutexLocker locker(&mutex);
    this->useAlphaBeta = useAlphaBeta;
}

bool ParallelExpectimaxWorker::getUseAlphaBeta() const {
    QMutexLocker locker(&mutex);
    return useAlphaBeta;
}

void ParallelExpectimaxWorker::setUseCache(bool useCache) {
    QMutexLocker locker(&mutex);
    this->useCache = useCache;
}

bool ParallelExpectimaxWorker::getUseCache() const {
    QMutexLocker locker(&mutex);
    return useCache;
}

void ParallelExpectimaxWorker::setCacheSize(size_t size) {
    QMutexLocker locker(&mutex);
    QMutexLocker cacheLocker(&cacheMutex);

    cacheSize = size;

    // 如果缓存已满，清除一半
    if (cache.size() > cacheSize) {
        cache.clear();
    }
}

size_t ParallelExpectimaxWorker::getCacheSize() const {
    QMutexLocker locker(&mutex);
    return cacheSize;
}

void ParallelExpectimaxWorker::clearCache() {
    QMutexLocker cacheLocker(&cacheMutex);
    cache.clear();
}

void ParallelExpectimaxWorker::setUseEnhancedEval(bool useEnhancedEval) {
    QMutexLocker locker(&mutex);
    this->useEnhancedEval = useEnhancedEval;
}

bool ParallelExpectimaxWorker::getUseEnhancedEval() const {
    QMutexLocker locker(&mutex);
    return useEnhancedEval;
}

void ParallelExpectimaxWorker::run() {
    qDebug() << "ParallelExpectimaxWorker: Worker thread started";

    forever {
        // 等待任务
        {
            QMutexLocker locker(&mutex);
            qDebug() << "ParallelExpectimaxWorker: Waiting for task";

            while (!restart && !abort) {
                condition.wait(&mutex);
            }

            if (abort) {
                qDebug() << "ParallelExpectimaxWorker: Worker thread aborted";
                return;
            }

            restart = false;
            qDebug() << "ParallelExpectimaxWorker: Got new task";
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

        // 并行评估四个方向
        QFuture<DirectionScore> upFuture;
        QFuture<DirectionScore> rightFuture;
        QFuture<DirectionScore> downFuture;
        QFuture<DirectionScore> leftFuture;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        upFuture = QtConcurrent::run(&threadPool, &ParallelExpectimaxWorker::evaluateDirection, this, board, 0, depth);
        rightFuture
            = QtConcurrent::run(&threadPool, &ParallelExpectimaxWorker::evaluateDirection, this, board, 1, depth);
        downFuture
            = QtConcurrent::run(&threadPool, &ParallelExpectimaxWorker::evaluateDirection, this, board, 2, depth);
        leftFuture
            = QtConcurrent::run(&threadPool, &ParallelExpectimaxWorker::evaluateDirection, this, board, 3, depth);
#else
        upFuture
            = QtConcurrent::run(&threadPool, [this, board, depth]() { return evaluateDirection(board, 0, depth); });

        rightFuture
            = QtConcurrent::run(&threadPool, [this, board, depth]() { return evaluateDirection(board, 1, depth); });

        downFuture
            = QtConcurrent::run(&threadPool, [this, board, depth]() { return evaluateDirection(board, 2, depth); });

        leftFuture
            = QtConcurrent::run(&threadPool, [this, board, depth]() { return evaluateDirection(board, 3, depth); });
#endif

        // 断开之前的连接
        disconnect(&upWatcher, nullptr, this, nullptr);
        disconnect(&rightWatcher, nullptr, this, nullptr);
        disconnect(&downWatcher, nullptr, this, nullptr);
        disconnect(&leftWatcher, nullptr, this, nullptr);

        // 定义处理完成的函数
        auto processResults = [this]() {
            // 检查是否所有方向都评估完成
            if (upWatcher.isFinished() && rightWatcher.isFinished() && downWatcher.isFinished()
                && leftWatcher.isFinished()) {
                // 获取评估结果
                DirectionScore upScore    = upWatcher.result();
                DirectionScore rightScore = rightWatcher.result();
                DirectionScore downScore  = downWatcher.result();
                DirectionScore leftScore  = leftWatcher.result();

                // 找出最佳方向
                DirectionScore bestScore = upScore;
                if (rightScore.valid && rightScore.score > bestScore.score) {
                    bestScore = rightScore;
                }
                if (downScore.valid && downScore.score > bestScore.score) {
                    bestScore = downScore;
                }
                if (leftScore.valid && leftScore.score > bestScore.score) {
                    bestScore = leftScore;
                }

                // 如果没有找到有效移动，返回任意方向
                int bestDirection = bestScore.valid ? bestScore.direction : 0;

                qDebug() << "ParallelExpectimaxWorker: Best direction:" << bestDirection;

                // 发出移动计算完成信号
                emit moveCalculated(bestDirection);
            }
        };

        // 连接所有四个 watcher 的信号
        connect(&upWatcher, &QFutureWatcher<DirectionScore>::finished, this, processResults, Qt::DirectConnection);
        connect(&rightWatcher, &QFutureWatcher<DirectionScore>::finished, this, processResults, Qt::DirectConnection);
        connect(&downWatcher, &QFutureWatcher<DirectionScore>::finished, this, processResults, Qt::DirectConnection);
        connect(&leftWatcher, &QFutureWatcher<DirectionScore>::finished, this, processResults, Qt::DirectConnection);

        // 开始监视Future
        upWatcher.setFuture(upFuture);
        rightWatcher.setFuture(rightFuture);
        downWatcher.setFuture(downFuture);
        leftWatcher.setFuture(leftFuture);
    }
}

DirectionScore ParallelExpectimaxWorker::evaluateDirection(BitBoard const& board, int direction, int depth) {
    // 检查是否需要重新开始或中止
    {
        QMutexLocker locker(&mutex);
        if (restart || abort) {
            return DirectionScore();
        }
    }

    // 执行移动
    BitBoard newBoard = board.move(static_cast<BitBoard::Direction>(direction));

    // 如果移动有效（棋盘发生了变化）
    if (newBoard != board) {
        // 计算期望得分
        float score = expectimax(newBoard, depth - 1, false);
        return DirectionScore(direction, score, true);
    }

    // 移动无效
    return DirectionScore(direction, -std::numeric_limits<float>::infinity(), false);
}

float ParallelExpectimaxWorker::expectimax(BitBoard const& board, int depth, bool maximizingPlayer) {
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
                bestScore   = std::max(bestScore, score);
            }
        }

        // 如果没有有效移动，返回当前棋盘的评估分数
        if (bestScore == -std::numeric_limits<float>::infinity()) {
            return evaluateBoard(board);
        }

        return bestScore;
    } else {
        // 计算机回合，随机放置新方块
        float expectedScore                            = 0.0f;
        std::vector<BitBoard::Position> emptyPositions = board.getEmptyPositions();

        if (emptyPositions.empty()) {
            return evaluateBoard(board);
        }

        float probability = 1.0f / emptyPositions.size();

        for (auto const& pos : emptyPositions) {
            // 检查是否需要重新开始或中止
            {
                QMutexLocker locker(&mutex);
                if (restart || abort) {
                    return 0.0f;
                }
            }

            // 90%概率放置2
            BitBoard boardWith2  = board.placeNewTile(pos, 2);
            expectedScore       += 0.9f * probability * expectimax(boardWith2, depth - 1, true);

            // 10%概率放置4
            BitBoard boardWith4  = board.placeNewTile(pos, 4);
            expectedScore       += 0.1f * probability * expectimax(boardWith4, depth - 1, true);
        }

        return expectedScore;
    }
}

float ParallelExpectimaxWorker::evaluateBoard(BitBoard const& board) {
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
