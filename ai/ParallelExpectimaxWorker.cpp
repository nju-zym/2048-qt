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
      searchDepth(5),                                              // 增加默认搜索深度从3到5
      threadCount(std::min(QThread::idealThreadCount() * 2, 32)),  // 限制最大线程数为32，避免过多线程导致的开销
      useDynamicDepth(true),                                       // 默认启用动态深度调整
      upWatcher(),
      rightWatcher(),
      downWatcher(),
      leftWatcher() {
    // 设置线程池大小
    threadPool.setMaxThreadCount(threadCount);
    threadPool.setExpiryTimeout(60000);  // 线程闲置60秒后过期，避免长时间占用资源

    // 设置线程池优先级
    QThreadPool::globalInstance()->setThreadPriority(QThread::HighPriority);

    // 预热线程池
    QVector<QFuture<void>> futures;
    for (int i = 0; i < threadCount; ++i) {
        futures.append(QtConcurrent::run(&threadPool, []() {
            // 线程初始化
            QThread::currentThread()->setPriority(QThread::HighPriority);
        }));
    }

    // 等待所有线程初始化完成
    for (auto& future : futures) {
        future.waitForFinished();
    }

    // 预热缓存
    preheateCache();

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

        // 重新预热线程池
        threadPool.clear();

        // 预热线程池
        QVector<QFuture<void>> futures;
        for (int i = 0; i < threadCount; ++i) {
            futures.append(QtConcurrent::run(&threadPool, []() {
                // 线程初始化
                QThread::currentThread()->setPriority(QThread::HighPriority);
            }));
        }

        // 等待所有线程初始化完成
        for (auto& future : futures) {
            future.waitForFinished();
        }

        qDebug() << "ParallelExpectimaxWorker: Thread count set to" << count;
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

void ParallelExpectimaxWorker::setUseDynamicDepth(bool useDynamicDepth) {
    QMutexLocker locker(&mutex);
    this->useDynamicDepth = useDynamicDepth;
}

bool ParallelExpectimaxWorker::getUseDynamicDepth() const {
    QMutexLocker locker(&mutex);
    return useDynamicDepth;
}

void ParallelExpectimaxWorker::setUseWorkStealing(bool useWorkStealing) {
    QMutexLocker locker(&mutex);
    this->useWorkStealing = useWorkStealing;
}

bool ParallelExpectimaxWorker::getUseWorkStealing() const {
    QMutexLocker locker(&mutex);
    return useWorkStealing;
}

void ParallelExpectimaxWorker::setBatchSize(size_t batchSize) {
    QMutexLocker locker(&mutex);
    this->batchSize = batchSize;
}

size_t ParallelExpectimaxWorker::getBatchSize() const {
    QMutexLocker locker(&mutex);
    return batchSize;
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
        // 动态深度调整
        int adjustedDepth = depth;

        if (useDynamicDepth) {
            // 获取棋盘上的空位数量
            int emptyTiles = newBoard.getEmptyPositions().size();

            // 根据空位数量动态调整深度
            if (emptyTiles <= 2) {
                // 空位很少，增加深度以获得更精确的计算
                adjustedDepth += 2;
            } else if (emptyTiles <= 4) {
                // 空位较少，增加深度
                adjustedDepth += 1;
            } else if (emptyTiles >= 10) {
                // 空位很多，减少深度以加快计算
                adjustedDepth = std::max(2, adjustedDepth - 1);
            }

            // 获取棋盘上的最大值
            uint32_t maxTile = 0;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    if (newBoard.getTile(row, col) > maxTile) {
                        maxTile = newBoard.getTile(row, col);
                    }
                }
            }

            // 如果最大值足够大，增加深度
            if (maxTile >= 2048) {
                adjustedDepth += 1;
            }
        }

        // 计算期望得分，使用调整后的深度
        float score = expectimax(newBoard,
                                 adjustedDepth - 1,
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
            return 0.0F;
        }
    }

    // 检查缓存
    if (useCache) {
        CacheKey key = {board.hash(), depth, maximizingPlayer, alpha, beta};

        // 使用读写锁优化缓存访问
        {
            // 首先尝试读锁，多个线程可以同时读取
            QReadLocker readLocker(&cacheRWLock);
            auto it = cache.find(key);
            if (it != cache.end()) {
                // 读取缓存值
                float cachedValue = it->second.value;
                // 释放读锁
                readLocker.unlock();

                // 获取写锁更新访问计数和世代
                QWriteLocker writeLocker(&cacheRWLock);
                it = cache.find(key);  // 重新查找，因为可能已经被其他线程修改
                if (it != cache.end()) {
                    it->second.accessCount++;
                    it->second.generation = currentGeneration;
                }
                return cachedValue;
            }
        }
    }

    // 达到搜索深度限制或游戏结束
    if (depth == 0 || board.isGameOver()) {
        return evaluateBoard(board);
    }

    // 早期终止机制
    // 如果棋盘上有非常大的数字，可以提前终止搜索
    uint32_t maxTile = 0;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (board.getTile(row, col) > maxTile) {
                maxTile = board.getTile(row, col);
            }
        }
    }

    // 如果最大数字足够大，直接返回评估分数
    if (maxTile >= 16384 && depth <= 3) {  // 2^14
        return evaluateBoard(board);
    }

    float result = 0.0F;

    if (maximizingPlayer) {
        // 玩家回合，尝试所有可能的移动
        float bestScore = -std::numeric_limits<float>::infinity();

        // 优化：预先计算所有可能的移动并排序
        std::vector<int> validDirections;
        std::vector<BitBoard> validBoards;
        std::vector<float> boardScores;

        for (int direction = 0; direction < 4; ++direction) {
            BitBoard newBoard = board.move(static_cast<BitBoard::Direction>(direction));
            if (newBoard != board) {
                validDirections.push_back(direction);
                validBoards.push_back(newBoard);
                boardScores.push_back(evaluateBoard(newBoard));
            }
        }

        // 按启发式评估分数降序排序，先尝试有希望的移动
        // 使用已经计算好的评估分数排序
        // 创建索引数组
        std::vector<size_t> indices(validDirections.size());
        std::iota(indices.begin(), indices.end(), 0);  // 填充 0, 1, 2, ...

        // 按评估分数降序排序索引
        std::sort(indices.begin(), indices.end(), [&boardScores](size_t a, size_t b) {
            return boardScores[a] > boardScores[b];
        });

        // 使用排序后的索引重新排列数组
        std::vector<int> sortedDirections;
        std::vector<BitBoard> sortedBoards;
        std::vector<float> sortedScores;

        for (size_t idx : indices) {
            sortedDirections.push_back(validDirections[idx]);
            sortedBoards.push_back(validBoards[idx]);
            sortedScores.push_back(boardScores[idx]);
        }

        // 替换原数组
        validDirections = std::move(sortedDirections);
        validBoards     = std::move(sortedBoards);
        boardScores     = std::move(sortedScores);

        // 如果深度足够大，使用并行计算
        if (depth >= 3 && validBoards.size() > 1) {
            // 如果启用工作窃取和批处理
            // 计算实际批处理大小
            size_t actualBatchSize = batchSize;
            if (actualBatchSize == 0) {
                // 动态计算批处理大小
                // 根据线程数和移动数量计算最佳批处理大小
                actualBatchSize = std::max(1UL, std::min(validBoards.size() / 2, static_cast<size_t>(threadCount / 2)));
            }

            if (useWorkStealing && validBoards.size() >= actualBatchSize) {
                // 分批处理
                size_t batchCount = (validBoards.size() + actualBatchSize - 1) / actualBatchSize;
                std::vector<QFuture<std::vector<float>>> batchFutures;

                for (size_t batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
                    // 计算批的起始和结束索引
                    size_t startIdx = batchIndex * actualBatchSize;
                    size_t endIdx   = std::min(startIdx + actualBatchSize, validBoards.size());

                    // 创建当前批的移动列表
                    std::vector<BitBoard> batchBoards;
                    for (size_t i = startIdx; i < endIdx; ++i) {
                        batchBoards.push_back(validBoards[i]);
                    }

                    // 并行处理整个批
                    batchFutures.push_back(
                        QtConcurrent::run(&threadPool, [this, batchBoards, depth, alpha, beta]() -> std::vector<float> {
                            std::vector<float> batchScores;
                            batchScores.reserve(batchBoards.size());

                            for (auto const& board : batchBoards) {
                                float score = expectimax(board, depth - 1, false, alpha, beta);
                                batchScores.push_back(score);
                            }

                            return batchScores;
                        }));
                }

                // 收集所有批的结果
                size_t resultIndex = 0;
                for (auto& future : batchFutures) {
                    std::vector<float> batchResults = future.result();

                    for (float score : batchResults) {
                        bestScore = std::max(bestScore, score);

                        // Beta剪枝
                        alpha = std::max(alpha, bestScore);
                        if (beta <= alpha && useAlphaBeta) {
                            break;  // Beta剪枝
                        }

                        resultIndex++;
                        if (resultIndex >= validBoards.size()) {
                            break;
                        }
                    }

                    if (beta <= alpha && useAlphaBeta) {
                        break;  // Beta剪枝
                    }
                }
            } else {
                // 原始并行计算方式
                std::vector<QFuture<float>> futures;
                std::vector<float> scores(validBoards.size());

                // 使用索引访问而不是结构化绑定
                for (size_t i = 0; i < validBoards.size(); ++i) {
                    // 直接复制棋盘状态
                    BitBoard boardCopy = validBoards[i];

                    // 使用线程池并行计算
                    futures.push_back(QtConcurrent::run(&threadPool,
                                                        // 使用值捕获而不是引用捕获
                                                        [this, boardCopy, depth, alpha, beta]() {
                                                            return expectimax(boardCopy, depth - 1, false, alpha, beta);
                                                        }));
                }

                // 等待所有计算完成并获取结果
                for (size_t i = 0; i < futures.size(); ++i) {
                    scores[i] = futures[i].result();
                    bestScore = std::max(bestScore, scores[i]);

                    // Beta剪枝
                    alpha = std::max(alpha, bestScore);
                    if (beta <= alpha && useAlphaBeta) {
                        break;  // Beta剪枝
                    }
                }
            }
        } else {
            // 使用串行计算
            for (size_t i = 0; i < validDirections.size(); ++i) {
                // 使用线程中断机制替代锁检查
                if ((checkCounter++ % 10) == 0 && QThread::currentThread()->isInterruptionRequested()) {
                    return 0.0F;
                }

                // 不使用方向信息，只需要棋盘状态
                BitBoard newBoard = validBoards[i];

                // 使用Alpha-Beta剪枝
                float score = expectimax(newBoard, depth - 1, false, alpha, beta);
                bestScore   = std::max(bestScore, score);

                // Beta剪枝
                alpha = std::max(alpha, bestScore);
                if (beta <= alpha && useAlphaBeta) {
                    break;  // Beta剪枝
                }
            }
        }

        // 如果没有有效移动，返回当前棋盘的评估分数
        if (validBoards.empty()) {
            return evaluateBoard(board);
        }

        result = bestScore;
    } else {
        // 计算机回合，随机放置新方块
        float expectedScore                            = 0.0F;
        std::vector<BitBoard::Position> emptyPositions = board.getEmptyPositions();

        if (emptyPositions.empty()) {
            return evaluateBoard(board);
        }

        float probability = 1.0F / static_cast<float>(emptyPositions.size());

        // 优化：如果空位太多，智能选择一部分进行模拟
        int MAX_EMPTY_POSITIONS = 8;  // 默认最大空位数量

        // 根据深度和空位数量动态调整最大空位数量
        // 深度越大，选择的空位越少，以减少计算量
        if (depth >= 5) {
            MAX_EMPTY_POSITIONS = 4;  // 深度很大时显著减少空位数量
        } else if (depth >= 4) {
            MAX_EMPTY_POSITIONS = 5;  // 深度较大时减少空位数量
        } else if (depth >= 3) {
            MAX_EMPTY_POSITIONS = 6;  // 中等深度
        } else if (depth == 2) {
            MAX_EMPTY_POSITIONS = 8;  // 深度较小
        } else {
            MAX_EMPTY_POSITIONS = 12;  // 深度很小时考虑更多空位
        }

        // 根据空位数量进一步调整
        // 当空位很多时，可以更激进地减少采样数量
        int emptyCount = static_cast<int>(emptyPositions.size());
        if (emptyCount > 12) {
            // 空位非常多时，进一步减少采样数量
            MAX_EMPTY_POSITIONS = std::min(MAX_EMPTY_POSITIONS, emptyCount / 2);
        } else if (emptyCount > 8) {
            // 空位较多时，适度减少采样数量
            MAX_EMPTY_POSITIONS = std::min(MAX_EMPTY_POSITIONS, (emptyCount * 2) / 3);
        }

        if (emptyPositions.size() > MAX_EMPTY_POSITIONS && depth > 1) {
            // 计算每个空位的价值
            std::vector<std::pair<BitBoard::Position, float>> positionValues;
            positionValues.reserve(emptyPositions.size());  // 预分配空间提高性能

            // 创建随机数生成器
            static thread_local std::mt19937 g(std::random_device{}());
            std::uniform_real_distribution<float> dist(-0.5F, 0.5F);

            // 获取棋盘上的最大值
            uint32_t maxTile = 0;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    uint32_t tileValue = board.getTile(row, col);
                    if (tileValue > maxTile) {
                        maxTile = tileValue;
                    }
                }
            }

            for (auto const& pos : emptyPositions) {
                float value = 0.0F;
                int row     = pos.row;
                int col     = pos.col;

                // 1. 考虑空位周围的方块值
                // 周围方块值越大，该空位越重要
                float surroundingValue    = 0.0F;
                int surroundingCount      = 0;
                float maxSurroundingValue = 0.0F;  // 记录周围最大值

                // 检查四个方向
                if (row > 0 && board.getTile(row - 1, col) > 0) {
                    float tileValue      = static_cast<float>(std::log2(board.getTile(row - 1, col)));
                    surroundingValue    += tileValue;
                    maxSurroundingValue  = std::max(maxSurroundingValue, tileValue);
                    surroundingCount++;
                }
                if (row < 3 && board.getTile(row + 1, col) > 0) {
                    float tileValue      = static_cast<float>(std::log2(board.getTile(row + 1, col)));
                    surroundingValue    += tileValue;
                    maxSurroundingValue  = std::max(maxSurroundingValue, tileValue);
                    surroundingCount++;
                }
                if (col > 0 && board.getTile(row, col - 1) > 0) {
                    float tileValue      = static_cast<float>(std::log2(board.getTile(row, col - 1)));
                    surroundingValue    += tileValue;
                    maxSurroundingValue  = std::max(maxSurroundingValue, tileValue);
                    surroundingCount++;
                }
                if (col < 3 && board.getTile(row, col + 1) > 0) {
                    float tileValue      = static_cast<float>(std::log2(board.getTile(row, col + 1)));
                    surroundingValue    += tileValue;
                    maxSurroundingValue  = std::max(maxSurroundingValue, tileValue);
                    surroundingCount++;
                }

                // 如果周围有方块，计算平均值
                if (surroundingCount > 0) {
                    float avgValue = surroundingValue / static_cast<float>(surroundingCount);

                    // 同时考虑平均值和最大值
                    value += avgValue * 1.5F + maxSurroundingValue * 1.0F;

                    // 如果周围有大数字，给予额外奖励
                    if (maxSurroundingValue >= static_cast<float>(std::log2(maxTile / 2))) {
                        value += 3.0F;  // 大数字附近的空位更重要
                    }
                }

                // 2. 考虑空位的位置
                // 角落的空位不太重要，因为我们希望大数字在角落
                if ((row == 0 && col == 0) || (row == 0 && col == 3) || (row == 3 && col == 0)
                    || (row == 3 && col == 3)) {
                    value -= 2.0F;  // 增加角落空位的惩罚
                } else if (row == 0 || row == 3 || col == 0 || col == 3) {
                    // 边缘空位也不太重要，但比角落好一点
                    value -= 1.0F;
                }

                // 3. 考虑空位周围的相等方块
                // 如果空位周围有相等的方块，该空位更重要
                if (row > 0 && row < 3 && board.getTile(row - 1, col) == board.getTile(row + 1, col)
                    && board.getTile(row - 1, col) > 0) {
                    auto logValue  = static_cast<float>(std::log2(board.getTile(row - 1, col)));
                    value         += logValue * 2.0F;  // 增加权重，鼓励合并

                    // 如果是大数字，给予额外奖励
                    if (board.getTile(row - 1, col) >= maxTile / 4) {
                        value += logValue * 1.0F;  // 大数字的合并更重要
                    }
                }
                if (col > 0 && col < 3 && board.getTile(row, col - 1) == board.getTile(row, col + 1)
                    && board.getTile(row, col - 1) > 0) {
                    auto logValue  = static_cast<float>(std::log2(board.getTile(row, col - 1)));
                    value         += logValue * 2.0F;  // 增加权重，鼓励合并

                    // 如果是大数字，给予额外奖励
                    if (board.getTile(row, col - 1) >= maxTile / 4) {
                        value += logValue * 1.0F;  // 大数字的合并更重要
                    }
                }

                // 4. 考虑空位周围的合并机会
                // 如果空位周围有相邻的相等方块，该空位更重要
                int mergeOpportunities = 0;     // 记录合并机会数量
                float mergeValue       = 0.0F;  // 记录合并机会的总价值

                // 检查所有可能的合并机会
                if (row > 0 && col > 0 && board.getTile(row - 1, col) == board.getTile(row, col - 1)
                    && board.getTile(row - 1, col) > 0) {
                    auto logValue  = static_cast<float>(std::log2(board.getTile(row - 1, col)));
                    mergeValue    += logValue;
                    mergeOpportunities++;
                }
                if (row > 0 && col < 3 && board.getTile(row - 1, col) == board.getTile(row, col + 1)
                    && board.getTile(row - 1, col) > 0) {
                    auto logValue  = static_cast<float>(std::log2(board.getTile(row - 1, col)));
                    mergeValue    += logValue;
                    mergeOpportunities++;
                }
                if (row < 3 && col > 0 && board.getTile(row + 1, col) == board.getTile(row, col - 1)
                    && board.getTile(row + 1, col) > 0) {
                    auto logValue  = static_cast<float>(std::log2(board.getTile(row + 1, col)));
                    mergeValue    += logValue;
                    mergeOpportunities++;
                }
                if (row < 3 && col < 3 && board.getTile(row + 1, col) == board.getTile(row, col + 1)
                    && board.getTile(row + 1, col) > 0) {
                    auto logValue  = static_cast<float>(std::log2(board.getTile(row + 1, col)));
                    mergeValue    += logValue;
                    mergeOpportunities++;
                }

                // 如果有多个合并机会，给予更高的奖励
                if (mergeOpportunities > 0) {
                    // 平均合并价值
                    float avgMergeValue = mergeValue / static_cast<float>(mergeOpportunities);

                    // 基础奖励
                    value += avgMergeValue * 1.5F;

                    // 根据合并机会数量给予额外奖励
                    if (mergeOpportunities >= 2) {
                        value += avgMergeValue * 1.0F * static_cast<float>(mergeOpportunities);
                    }
                }

                // 5. 添加随机性因素，避免每次选择相同的空位
                // 使用更智能的随机性因素，根据空位的价值调整随机性的影响
                // 价值越高，随机性影响越小
                float randomFactor = 0.0F;

                // 如果空位数量很多，增加随机性以提高多样性
                if (emptyPositions.size() > 8) {
                    randomFactor = dist(g) * 1.0F;
                } else if (emptyPositions.size() > 4) {
                    randomFactor = dist(g) * 0.7F;
                } else {
                    // 空位很少时，减少随机性影响，更依赖价值评估
                    randomFactor = dist(g) * 0.3F;
                }

                // 如果空位的价值很高，减少随机性的影响
                if (value > 10.0F) {
                    randomFactor *= 0.5F;  // 高价值空位的随机性影响更小
                }

                value += randomFactor;

                positionValues.emplace_back(pos, value);
            }

            // 按价值降序排序
            std::sort(positionValues.begin(), positionValues.end(), [](auto const& a, auto const& b) {
                return a.second > b.second;
            });

            // 只保留前 MAX_EMPTY_POSITIONS 个空位
            if (positionValues.size() > static_cast<size_t>(MAX_EMPTY_POSITIONS)) {
                positionValues.erase(positionValues.begin() + MAX_EMPTY_POSITIONS, positionValues.end());
            }

            // 提取选中的空位
            emptyPositions.clear();
            for (auto const& [pos, _] : positionValues) {
                emptyPositions.push_back(pos);
            }

            // 调整概率
            probability = 1.0F / static_cast<float>(emptyPositions.size());
        }

        // 如果深度足够大且空位足够多，使用并行计算
        if (depth >= 3 && emptyPositions.size() > 1) {
            // 如果启用工作窃取和批处理
            if (useWorkStealing && emptyPositions.size() >= batchSize) {
                // 分批处理
                size_t batchCount = (emptyPositions.size() + batchSize - 1) / batchSize;
                std::vector<QFuture<std::vector<std::pair<float, float>>>> batchFutures;

                for (size_t batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
                    // 计算批的起始和结束索引
                    size_t startIdx = batchIndex * batchSize;
                    size_t endIdx   = std::min(startIdx + batchSize, emptyPositions.size());

                    // 创建当前批的空位列表
                    std::vector<BitBoard::Position> batchPositions;
                    for (size_t i = startIdx; i < endIdx; ++i) {
                        batchPositions.push_back(emptyPositions[i]);
                    }

                    // 并行处理整个批
                    batchFutures.push_back(QtConcurrent::run(
                        &threadPool,
                        [this, board, batchPositions, depth, alpha, beta]() -> std::vector<std::pair<float, float>> {
                            std::vector<std::pair<float, float>> batchScores;
                            batchScores.reserve(batchPositions.size());

                            for (auto const& pos : batchPositions) {
                                // 90%概率放置2
                                BitBoard boardWith2 = board.placeNewTile(pos, 2);
                                float scoreWith2    = expectimax(boardWith2, depth - 1, true, alpha, beta);

                                // 10%概率放置4
                                BitBoard boardWith4 = board.placeNewTile(pos, 4);
                                float scoreWith4    = expectimax(boardWith4, depth - 1, true, alpha, beta);

                                batchScores.emplace_back(scoreWith2, scoreWith4);
                            }

                            return batchScores;
                        }));
                }

                // 收集所有批的结果
                size_t resultIndex = 0;
                for (auto& future : batchFutures) {
                    std::vector<std::pair<float, float>> batchResults = future.result();

                    for (auto const& [scoreWith2, scoreWith4] : batchResults) {
                        expectedScore += 0.9F * probability * scoreWith2 + 0.1F * probability * scoreWith4;

                        // Alpha-Beta剪枝优化
                        if (useAlphaBeta) {
                            beta = std::min(beta, expectedScore);
                            if (beta <= alpha) {
                                break;  // Alpha剪枝
                            }
                        }

                        resultIndex++;
                        if (resultIndex >= emptyPositions.size()) {
                            break;
                        }
                    }

                    if (useAlphaBeta && beta <= alpha) {
                        break;  // Alpha剪枝
                    }
                }
            } else {
                // 原始并行计算方式
                std::vector<QFuture<float>> futuresWith2;
                std::vector<QFuture<float>> futuresWith4;

                for (auto const& pos : emptyPositions) {
                    // 90%概率放置2
                    BitBoard boardWith2 = board.placeNewTile(pos, 2);
                    futuresWith2.push_back(QtConcurrent::run(&threadPool, [this, boardWith2, depth, alpha, beta]() {
                        return expectimax(boardWith2, depth - 1, true, alpha, beta);
                    }));

                    // 10%概率放置4
                    BitBoard boardWith4 = board.placeNewTile(pos, 4);
                    futuresWith4.push_back(QtConcurrent::run(&threadPool, [this, boardWith4, depth, alpha, beta]() {
                        return expectimax(boardWith4, depth - 1, true, alpha, beta);
                    }));
                }

                // 等待所有计算完成并获取结果
                for (size_t i = 0; i < emptyPositions.size(); ++i) {
                    float scoreWith2 = futuresWith2[i].result();
                    float scoreWith4 = futuresWith4[i].result();

                    expectedScore += 0.9F * probability * scoreWith2 + 0.1F * probability * scoreWith4;

                    // Alpha-Beta剪枝优化
                    if (useAlphaBeta) {
                        beta = std::min(beta, expectedScore);
                        if (beta <= alpha) {
                            break;  // Alpha剪枝
                        }
                    }
                }
            }
        } else {
            // 使用串行计算
            for (auto const& pos : emptyPositions) {
                // 使用线程中断机制替代锁检查
                if ((checkCounter++ % 10) == 0 && QThread::currentThread()->isInterruptionRequested()) {
                    return 0.0F;
                }

                // 90%概率放置2
                BitBoard boardWith2  = board.placeNewTile(pos, 2);
                float scoreWith2     = expectimax(boardWith2, depth - 1, true, alpha, beta);
                expectedScore       += 0.9F * probability * scoreWith2;

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
                expectedScore       += 0.1F * probability * scoreWith4;

                // Alpha-Beta剪枝优化
                if (useAlphaBeta) {
                    beta = std::min(beta, expectedScore);
                    if (beta <= alpha) {
                        break;  // Alpha剪枝
                    }
                }
            }
        }

        result = expectedScore;
    }

    // 更新缓存
    if (useCache) {
        CacheKey key = {board.hash(), depth, maximizingPlayer, alpha, beta};

        // 使用写锁更新缓存
        QWriteLocker writeLocker(&cacheRWLock);

        // 检查缓存大小
        if (cache.size() >= cacheSize) {
            // 如果缓存已满，使用更智能的缓存替换策略
            // 1. 删除旧世代的条目
            // 2. 删除访问计数最少的条目

            // 当前世代的阈值，删除比这个值小的条目
            uint32_t generationThreshold = currentGeneration > 5 ? currentGeneration - 5 : 0;

            // 首先删除旧世代的条目
            for (auto it = cache.begin(); it != cache.end();) {
                if (it->second.generation < generationThreshold) {
                    it = cache.erase(it);
                } else {
                    ++it;
                }
            }

            // 如果还是太大，删除访问计数最少的条目
            if (cache.size() >= cacheSize) {
                // 收集所有条目的访问计数
                std::vector<std::pair<CacheKey, uint32_t>> accessCounts;
                accessCounts.reserve(cache.size());  // 预分配空间提高性能

                for (auto const& [k, v] : cache) {
                    accessCounts.emplace_back(k, v.accessCount);
                }

                // 按访问计数升序排序
                std::sort(accessCounts.begin(), accessCounts.end(), [](auto const& a, auto const& b) {
                    return a.second < b.second;
                });

                // 删除访问计数最少的 20% 条目
                size_t toRemove = static_cast<size_t>(cache.size() * 0.2);
                for (size_t i = 0; i < toRemove && i < accessCounts.size(); ++i) {
                    cache.erase(accessCounts[i].first);
                }
            }
        }

        // 增加缓存世代
        currentGeneration++;

        // 存储结果
        cache[key] = CacheEntry{result, 1, currentGeneration};
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
    float score = 0.0F;

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

void ParallelExpectimaxWorker::preheateCache() {
    qDebug() << "Preheating cache...";

    // 创建一个空棋盘
    BitBoard emptyBoard;

    // 创建一些常见的初始棋面
    std::vector<BitBoard> initialBoards;

    // 添加空棋盘
    initialBoards.push_back(emptyBoard);

    // 添加一些常见的初始棋面（一个2和一个4）
    for (int row1 = 0; row1 < 4; ++row1) {
        for (int col1 = 0; col1 < 4; ++col1) {
            for (int row2 = 0; row2 < 4; ++row2) {
                for (int col2 = 0; col2 < 4; ++col2) {
                    if (row1 == row2 && col1 == col2) {
                        continue;  // 跳过相同位置
                    }

                    BitBoard board = emptyBoard;
                    board          = board.placeNewTile({row1, col1}, 2);
                    board          = board.placeNewTile({row2, col2}, 2);
                    initialBoards.push_back(board);

                    board = emptyBoard;
                    board = board.placeNewTile({row1, col1}, 2);
                    board = board.placeNewTile({row2, col2}, 4);
                    initialBoards.push_back(board);
                }
            }
        }
    }

    // 限制预热的棋面数量，避免耗时过长
    size_t maxBoards = std::min(initialBoards.size(), static_cast<size_t>(100));
    initialBoards.resize(maxBoards);

    qDebug() << "Preheating cache with" << maxBoards << "initial boards...";

    // 预热缓存，使用多线程
    QVector<QFuture<void>> futures;

    for (auto const& board : initialBoards) {
        futures.append(QtConcurrent::run(&threadPool, [this, board]() {
            // 对每个初始棋面进行浅层搜索
            for (int depth = 1; depth <= 2; ++depth) {
                for (int direction = 0; direction < 4; ++direction) {
                    // 预热移动评估
                    evaluateDirection(board, direction, depth);
                }

                // 预热 Expectimax 搜索
                expectimax(board,
                           depth,
                           true,
                           -std::numeric_limits<float>::infinity(),
                           std::numeric_limits<float>::infinity());
            }
        }));
    }

    // 等待所有预热任务完成
    for (auto& future : futures) {
        future.waitForFinished();
    }

    qDebug() << "Cache preheating completed. Cache size:" << cache.size();
}
