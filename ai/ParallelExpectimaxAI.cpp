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

    // 启动工作线程计算
    worker->calculateBestMove(bitBoard, depth);

    // 返回上一次计算的移动，或者随机选择一个方向
    QMutexLocker locker(&mutex);
    if (moveReady) {
        int move = lastCalculatedMove;
        return move;
    } else {
        // 如果还没有计算结果，随机选择一个方向
        return QRandomGenerator::global()->bounded(4);
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
