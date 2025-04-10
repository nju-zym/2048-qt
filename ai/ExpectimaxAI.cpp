#include "ExpectimaxAI.h"

#include "ExpectimaxWorker.h"

#include <QDebug>

ExpectimaxAI::ExpectimaxAI(int depth, QObject* parent)
    : AIInterface(parent), depth(depth), lastCalculatedMove(-1), moveReady(false), worker(nullptr) {
    // 创建工作线程
    worker = new ExpectimaxWorker();

    // 连接信号
    connect(worker, &ExpectimaxWorker::moveCalculated, this, &ExpectimaxAI::onMoveCalculated);
}

ExpectimaxAI::~ExpectimaxAI() {
    // 释放工作线程
    if (worker) {
        delete worker;
        worker = nullptr;
    }
}

int ExpectimaxAI::getBestMove(GameBoard const& board) {
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

void ExpectimaxAI::onMoveCalculated(int direction) {
    QMutexLocker locker(&mutex);
    lastCalculatedMove = direction;
    moveReady          = true;
    condition.wakeOne();

    // 发出移动决策信号
    emit moveDecided(direction);
}

void ExpectimaxAI::setDepth(int depth) {
    this->depth = depth;
}

int ExpectimaxAI::getDepth() const {
    return depth;
}

QString ExpectimaxAI::getName() const {
    return QString("Expectimax (深度 %1)").arg(depth);
}
