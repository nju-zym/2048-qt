#include "ParallelExpectimaxAI.h"

#include "BitBoard.h"
#include "ParallelExpectimaxWorker.h"

#include <QDebug>
#include <QThread>

ParallelExpectimaxAI::ParallelExpectimaxAI(int depth, int threadCount, QObject* parent)
    : AIInterface(parent),
      depth(depth),
      threadCount(threadCount <= 0 ? QThread::idealThreadCount() * 2 : threadCount),
      lastCalculatedMove(-1),
      moveReady(false),
      useDynamicDepth(false),
      minDepth(2),
      maxDepth(5),
      worker(nullptr) {
    // 创建工作线程
    worker = new ParallelExpectimaxWorker();

    // 设置线程数量
    worker->setThreadCount(this->threadCount);

    // 连接信号
    connect(worker, &ParallelExpectimaxWorker::moveCalculated, this, &ParallelExpectimaxAI::onMoveCalculated);

    qDebug() << "ParallelExpectimaxAI created with depth" << depth << "and" << this->threadCount << "threads";
}

ParallelExpectimaxAI::~ParallelExpectimaxAI() {
    // 释放工作线程
    if (worker) {
        delete worker;
        worker = nullptr;
    }
}

int ParallelExpectimaxAI::getBestMove(GameBoard const& board) {
    // 如果游戏已经结束，返回任意方向
    if (board.isGameOver()) {
        return 0;  // 上
    }

    // 创建BitBoard
    BitBoard bitBoard(board);

    // 检查是否有有效移动
    bool anyValidMove = false;
    for (int dir = 0; dir < 4; dir++) {
        BitBoard tempBoard = bitBoard;
        if (tempBoard.move(static_cast<BitBoard::Direction>(dir)) != bitBoard) {
            anyValidMove = true;
            break;
        }
    }

    // 如果没有有效移动，返回任意方向
    if (!anyValidMove) {
        return 0;  // 上
    }

    // 如果使用动态深度调整，计算动态深度
    int searchDepth = useDynamicDepth ? calculateDynamicDepth(board) : depth;

    // 启动工作线程计算
    worker->calculateBestMove(bitBoard, searchDepth);

    // 返回上一次计算的移动，或者选择一个最佳方向
    QMutexLocker locker(&mutex);
    if (moveReady) {
        int move = lastCalculatedMove;
        return move;
    } else {
        // 如果还没有计算结果，进行简单的启发式计算
        // 计算每个方向的空格数量
        int bestDir       = -1;
        int maxEmptyTiles = -1;

        for (int dir = 0; dir < 4; dir++) {
            BitBoard tempBoard = bitBoard;
            BitBoard newBoard  = tempBoard.move(static_cast<BitBoard::Direction>(dir));

            if (newBoard != bitBoard) {
                // 计算移动后的空格数量
                int emptyTiles = newBoard.countEmptyTiles();

                // 更新最佳方向
                if (emptyTiles > maxEmptyTiles) {
                    maxEmptyTiles = emptyTiles;
                    bestDir       = dir;
                }
            }
        }

        // 如果找到有效方向，返回最佳方向
        if (bestDir != -1) {
            return bestDir;
        }

        // 如果没有有效方向，返回任意方向
        return 0;  // 上
    }
}

void ParallelExpectimaxAI::onMoveCalculated(int direction) {
    QMutexLocker locker(&mutex);
    lastCalculatedMove = direction;
    moveReady          = true;
    condition.wakeOne();

    qDebug() << "ParallelExpectimaxAI: Move calculated:" << direction;

    // 发出移动决策信号
    emit moveDecided(direction);
}

void ParallelExpectimaxAI::setDepth(int depth) {
    this->depth = depth;
}

int ParallelExpectimaxAI::getDepth() const {
    return depth;
}

QString ParallelExpectimaxAI::getName() const {
    return QString("并行Expectimax (深度 %1, 线程 %2)").arg(depth).arg(threadCount);
}

void ParallelExpectimaxAI::setThreadCount(int count) {
    if (count > 0) {
        threadCount = count;
        if (worker) {
            worker->setThreadCount(count);
        }
    }
}

int ParallelExpectimaxAI::getThreadCount() const {
    return threadCount;
}

void ParallelExpectimaxAI::setUseAlphaBeta(bool useAlphaBeta) {
    if (worker) {
        worker->setUseAlphaBeta(useAlphaBeta);
    }
}

bool ParallelExpectimaxAI::getUseAlphaBeta() const {
    return worker ? worker->getUseAlphaBeta() : false;
}

void ParallelExpectimaxAI::setUseCache(bool useCache) {
    if (worker) {
        worker->setUseCache(useCache);
    }
}

bool ParallelExpectimaxAI::getUseCache() const {
    return worker ? worker->getUseCache() : false;
}

void ParallelExpectimaxAI::setCacheSize(size_t size) {
    if (worker) {
        worker->setCacheSize(size);
    }
}

size_t ParallelExpectimaxAI::getCacheSize() const {
    return worker ? worker->getCacheSize() : 0;
}

void ParallelExpectimaxAI::clearCache() {
    if (worker) {
        worker->clearCache();
    }
}

void ParallelExpectimaxAI::setUseEnhancedEval(bool useEnhancedEval) {
    if (worker) {
        worker->setUseEnhancedEval(useEnhancedEval);
    }
}

bool ParallelExpectimaxAI::getUseEnhancedEval() const {
    return worker ? worker->getUseEnhancedEval() : false;
}

void ParallelExpectimaxAI::setUseDynamicDepth(bool useDynamicDepth) {
    this->useDynamicDepth = useDynamicDepth;
}

bool ParallelExpectimaxAI::getUseDynamicDepth() const {
    return useDynamicDepth;
}

void ParallelExpectimaxAI::setMinDepth(int minDepth) {
    if (minDepth > 0 && minDepth <= maxDepth) {
        this->minDepth = minDepth;
    }
}

int ParallelExpectimaxAI::getMinDepth() const {
    return minDepth;
}

void ParallelExpectimaxAI::setMaxDepth(int maxDepth) {
    if (maxDepth >= minDepth) {
        this->maxDepth = maxDepth;
    }
}

int ParallelExpectimaxAI::getMaxDepth() const {
    return maxDepth;
}

int ParallelExpectimaxAI::calculateDynamicDepth(GameBoard const& board) const {
    // 获取棋盘上的空格数量
    int emptyTiles = 0;
    for (int row = 0; row < board.getSize(); ++row) {
        for (int col = 0; col < board.getSize(); ++col) {
            if (board.isTileEmpty(row, col)) {
                emptyTiles++;
            }
        }
    }

    // 获取棋盘上的最大数字
    int maxTile = 0;
    for (int row = 0; row < board.getSize(); ++row) {
        for (int col = 0; col < board.getSize(); ++col) {
            if (!board.isTileEmpty(row, col)) {
                maxTile = std::max(maxTile, board.getTileValue(row, col));
            }
        }
    }

    // 根据空格数量和最大数字动态调整搜索深度
    int dynamicDepth = minDepth;  // 默认使用最小深度

    // 当空格数量少时，使用更深的搜索
    if (emptyTiles <= 4) {
        dynamicDepth = maxDepth;
    } else if (emptyTiles <= 8) {
        dynamicDepth = (minDepth + maxDepth) / 2 + 1;
    } else {
        dynamicDepth = minDepth;
    }

    // 当最大数字较大时，使用更深的搜索
    if (maxTile >= 2048) {
        dynamicDepth = std::max(dynamicDepth, maxDepth - 1);
    } else if (maxTile >= 1024) {
        dynamicDepth = std::max(dynamicDepth, (minDepth + maxDepth) / 2);
    }

    // 确保深度在有效范围内
    dynamicDepth = std::max(minDepth, std::min(dynamicDepth, maxDepth));

    qDebug() << "Dynamic depth:" << dynamicDepth << "(empty tiles:" << emptyTiles << ", max tile:" << maxTile << ")";

    return dynamicDepth;
}
