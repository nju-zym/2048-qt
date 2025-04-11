#include "HybridAI.h"

#include "BitBoard.h"

HybridAI::HybridAI(QObject* parent) : AIInterface(parent), worker_(std::make_unique<HybridWorker>()) {
    // 连接信号
    connect(worker_.get(), &HybridWorker::searchProgress, this, &HybridAI::searchProgress);
    connect(worker_.get(), &HybridWorker::searchComplete, this, &HybridAI::searchComplete);
}

HybridAI::~HybridAI() = default;

int HybridAI::getBestMove(GameBoard const& board) {
    // 将GameBoard转换为BitBoard
    BitBoard bitBoard(board);

    // 获取最佳移动方向
    BitBoard::Direction bestDirection = worker_->getBestMove(bitBoard);

    // 将BitBoard::Direction转换为int（0=上, 1=右, 2=下, 3=左）
    switch (bestDirection) {
        case BitBoard::Direction::UP:
            return 0;  // 上
        case BitBoard::Direction::RIGHT:
            return 1;  // 右
        case BitBoard::Direction::DOWN:
            return 2;  // 下
        case BitBoard::Direction::LEFT:
            return 3;  // 左
        default:
            return 0;  // 默认方向（上）
    }
}

void HybridAI::setMCTSWeight(float weight) {
    worker_->setMCTSWeight(weight);
}

void HybridAI::setExpectimaxWeight(float weight) {
    worker_->setExpectimaxWeight(weight);
}

void HybridAI::setThreadCount(int threadCount) {
    worker_->setThreadCount(threadCount);
}

void HybridAI::setTimeLimit(int timeLimit) {
    worker_->setTimeLimit(timeLimit);
}

void HybridAI::setUseCache(bool useCache) {
    worker_->setUseCache(useCache);
}

void HybridAI::setCacheSize(int cacheSize) {
    worker_->setCacheSize(cacheSize);
}

void HybridAI::clearCache() {
    worker_->clearCache();
}

void HybridAI::setDepth(int depth) {
    // 在混合策略中，深度用于Expectimax部分
    // 这里我们可以将其传递给HybridWorker
    // 在当前实现中暂时忽略
    Q_UNUSED(depth);
}

int HybridAI::getDepth() const {
    // 在混合策略中，深度不是主要参数
    // 返回一个默认值
    return 5;
}

QString HybridAI::getName() const {
    // 根据MCTS权重返回不同的名称
    float mctsWeight       = 0.0F;
    float expectimaxWeight = 0.0F;

    // 尝试获取当前权重（这里只是一个示例，实际上我们可能需要修改HybridWorker类来提供这些信息）

    if (mctsWeight > 0.9F) {
        return "MCTS AI";
    }

    if (expectimaxWeight > 0.9F) {
        return "Expectimax AI";
    }

    return "Hybrid AI (MCTS + Expectimax)";
}
