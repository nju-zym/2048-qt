#include "ParallelExpectimaxWorker.h"

#include "evaluation/CornerStrategyEval.h"
#include "evaluation/FreeTilesEval.h"
#include "evaluation/LargeNumbersConnectionEval.h"
#include "evaluation/MergeEval.h"
#include "evaluation/MonotonicityEval.h"
#include "evaluation/RiskEval.h"
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
    restart = true;
    condition.wakeOne();
    qDebug() << "ParallelExpectimaxWorker: Worker thread woken up";
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
    cacheSize = size;

    // 清除缓存
    QMutexLocker cacheLocker(&cacheMutex);
    if (cache.size() > cacheSize) {
        cache.clear();
    }
}

size_t ParallelExpectimaxWorker::getCacheSize() const {
    QMutexLocker locker(&mutex);
    return cacheSize;
}

void ParallelExpectimaxWorker::clearCache() {
    // 清除缓存
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

            // 如果没有任务或不需要重新开始，则等待
            while (!restart && !abort) {
                condition.wait(&mutex);
            }

            // 如果需要终止线程，则返回
            if (abort) {
                qDebug() << "ParallelExpectimaxWorker: Worker thread aborted";
                return;
            }

            // 重置重新开始标志
            restart = false;
            qDebug() << "ParallelExpectimaxWorker: Got new task";
        }

        // 计算最佳移动
        BitBoard board;
        int depth = 0;  // 初始化深度变量

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
    // 使用线程中断机制替代锁检查
    if (QThread::currentThread()->isInterruptionRequested()) {
        return DirectionScore();
    }

    // 执行移动
    BitBoard newBoard = board.move(static_cast<BitBoard::Direction>(direction));

    // 如果移动有效（棋盘发生了变化）
    if (newBoard != board) {
        // 计算期望得分
        float score = expectimax(newBoard,
                                 depth - 1,
                                 false,
                                 -std::numeric_limits<float>::infinity(),
                                 std::numeric_limits<float>::infinity());
        return DirectionScore(direction, score, true);
    }

    // 移动无效
    return DirectionScore(direction, -std::numeric_limits<float>::infinity(), false);
}

float ParallelExpectimaxWorker::expectimax(
    BitBoard const& board, int depth, bool maximizingPlayer, float alpha, float beta) {
    // 在递归开始时检查一次是否需要重新开始或中止
    // 但不在每个递归层级都检查，减少锁竞争
    static thread_local int checkCounter = 0;

    // 每10层递归才检查一次
    if ((checkCounter++ % 10) == 0) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            return 0.0f;
        }
    }

    // 检查缓存
    if (useCache) {
        CacheKey key = {board.hash(), depth, maximizingPlayer};

        // 查找缓存
        QMutexLocker cacheLocker(&cacheMutex);
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it->second;
        }
    }

    // 达到搜索深度限制或游戏结束
    if (depth == 0 || board.isGameOver()) {
        return evaluateBoard(board);
    }

    float result = 0.0f;

    if (maximizingPlayer) {
        // 玩家回合，尝试所有可能的移动
        float bestScore = -std::numeric_limits<float>::infinity();

        // 优化：预先计算所有可能的移动并排序
        std::vector<std::pair<int, BitBoard>> validMoves;
        for (int direction = 0; direction < 4; ++direction) {
            BitBoard newBoard = board.move(static_cast<BitBoard::Direction>(direction));
            if (newBoard != board) {
                validMoves.emplace_back(direction, newBoard);
            }
        }

        // 按启发式评估分数降序排序，先尝试有希望的移动
        std::sort(validMoves.begin(), validMoves.end(), [this](auto const& a, auto const& b) {
            return evaluateBoard(a.second) > evaluateBoard(b.second);
        });

        for (auto const& [direction, newBoard] : validMoves) {
            // 使用线程中断机制替代锁检查
            if ((checkCounter++ % 10) == 0 && QThread::currentThread()->isInterruptionRequested()) {
                return 0.0f;
            }

            // 使用Alpha-Beta剪枝
            float score = expectimax(newBoard, depth - 1, false, alpha, beta);
            bestScore   = std::max(bestScore, score);

            // Beta剪枝
            alpha = std::max(alpha, bestScore);
            if (beta <= alpha && useAlphaBeta) {
                break;  // Beta剪枝
            }
        }

        // 如果没有有效移动，返回当前棋盘的评估分数
        if (validMoves.empty()) {
            return evaluateBoard(board);
        }

        result = bestScore;
    } else {
        // 计算机回合，随机放置新方块
        float expectedScore                            = 0.0f;
        std::vector<BitBoard::Position> emptyPositions = board.getEmptyPositions();

        if (emptyPositions.empty()) {
            return evaluateBoard(board);
        }

        float probability = 1.0f / emptyPositions.size();

        // 优化：如果空位太多，只随机选择一部分进行模拟
        int const MAX_EMPTY_POSITIONS = 6;
        if (emptyPositions.size() > MAX_EMPTY_POSITIONS && depth > 1) {
            // 随机选择一部分空位
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(emptyPositions.begin(), emptyPositions.end(), g);

            // 如果空位过多，只保留前 MAX_EMPTY_POSITIONS 个
            if (emptyPositions.size() > static_cast<size_t>(MAX_EMPTY_POSITIONS)) {
                emptyPositions.erase(emptyPositions.begin() + MAX_EMPTY_POSITIONS, emptyPositions.end());
            }

            // 调整概率
            probability = 1.0f / static_cast<float>(emptyPositions.size());
        }

        for (auto const& pos : emptyPositions) {
            // 使用线程中断机制替代锁检查
            if ((checkCounter++ % 10) == 0 && QThread::currentThread()->isInterruptionRequested()) {
                return 0.0f;
            }

            // 90%概率放置2
            BitBoard boardWith2  = board.placeNewTile(pos, 2);
            float scoreWith2     = expectimax(boardWith2, depth - 1, true, alpha, beta);
            expectedScore       += 0.9f * probability * scoreWith2;

            // Alpha-Beta剪枝优化
            if (useAlphaBeta) {
                beta = std::min(beta, expectedScore);
                if (beta <= alpha) {
                    break;  // Alpha剪枝
                }
            }

            // 10%概率放置4
            BitBoard boardWith4  = board.placeNewTile(pos, 4);
            float scoreWith4     = expectimax(boardWith4, depth - 1, true, alpha, beta);
            expectedScore       += 0.1f * probability * scoreWith4;

            // Alpha-Beta剪枝优化
            if (useAlphaBeta) {
                beta = std::min(beta, expectedScore);
                if (beta <= alpha) {
                    break;  // Alpha剪枝
                }
            }
        }

        result = expectedScore;
    }

    // 更新缓存
    if (useCache) {
        CacheKey key = {board.hash(), depth, maximizingPlayer};

        // 更新缓存
        QMutexLocker cacheLocker(&cacheMutex);

        // 检查缓存大小
        if (cache.size() >= cacheSize) {
            // 如果缓存已满，清除一半
            auto it = cache.begin();
            for (size_t i = 0; i < cache.size() / 2; ++i) {
                it = cache.erase(it);
            }
        }

        cache[key] = result;
    }

    return result;
}

float ParallelExpectimaxWorker::expectimax(BitBoard const& board, int depth, bool maximizingPlayer) {
    // 调用带Alpha-Beta剪枝的Expectimax算法
    return expectimax(board,
                      depth,
                      maximizingPlayer,
                      -std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::infinity());
}

float ParallelExpectimaxWorker::evaluateBoard(BitBoard const& board) const {
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

    // 使用增强评估函数
    if (useEnhancedEval) {
        // 角落策略评估
        score += CORNER_STRATEGY_WEIGHT * CornerStrategyEval::evaluate(board);

        // 大数字连接评估
        score += LARGE_NUMBERS_CONNECTION_WEIGHT * LargeNumbersConnectionEval::evaluate(board);

        // 风险评估
        score += RISK_WEIGHT * RiskEval::evaluate(board);
    }

    return score;
}
