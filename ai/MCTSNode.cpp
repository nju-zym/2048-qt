#include "MCTSNode.h"

#include <algorithm>
#include <cmath>
#include <limits>

// 初始化线程局部随机数生成器
thread_local std::mt19937 MCTSNode::rng_(std::random_device{}());

MCTSNode::MCTSNode(BitBoard board, MCTSNode* parent, int move, bool isChance)
    : board_(board),
      parent_(parent),
      visitCount_(0),
      totalScore_(0.0f),
      move_(move),
      isChance_(isChance),
      isFullyExpanded_(false) {}

MCTSNode::~MCTSNode() {
    // 释放所有子节点
    for (auto child : children_) {
        delete child;
    }
}

MCTSNode* MCTSNode::selectBestChild() {
    if (children_.empty()) {
        return nullptr;
    }

    // 使用UCB公式选择最佳子节点
    float bestUCB       = -std::numeric_limits<float>::infinity();
    MCTSNode* bestChild = nullptr;

    // 探索常数，可以调整以平衡探索和利用
    float const explorationConstant = 1.414f;  // sqrt(2)

    for (auto child : children_) {
        float ucb = child->getUCB(explorationConstant);
        if (ucb > bestUCB) {
            bestUCB   = ucb;
            bestChild = child;
        }
    }

    return bestChild;
}

void MCTSNode::expand() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (isTerminal()) {
        isFullyExpanded_ = true;
        return;
    }

    if (isChance_) {
        // 机会节点：随机放置新方块
        std::vector<BitBoard::Position> emptyPositions = board_.getEmptyPositions();

        // 如果没有空位，则无法扩展
        if (emptyPositions.empty()) {
            isFullyExpanded_ = true;
            return;
        }

        // 随机选择一个空位
        std::uniform_int_distribution<size_t> posDist(0, emptyPositions.size() - 1);
        BitBoard::Position pos = emptyPositions[posDist(rng_)];

        // 90%概率放置2，10%概率放置4
        std::uniform_real_distribution<float> valueDist(0.0f, 1.0f);
        int value = (valueDist(rng_) < 0.9f) ? 2 : 4;

        // 创建新棋盘并放置新方块
        BitBoard newBoard = board_;
        newBoard.setTile(pos.row, pos.col, value);

        // 创建子节点（非机会节点）
        children_.push_back(new MCTSNode(newBoard, this, -1, false));
    } else {
        // 决策节点：尝试所有可能的移动
        for (int direction = 0; direction < 4; ++direction) {
            BitBoard newBoard   = board_;
            BitBoard movedBoard = newBoard.move(static_cast<BitBoard::Direction>(direction));
            if (movedBoard != newBoard) {
                // 移动有效，创建子节点（机会节点）
                children_.push_back(new MCTSNode(newBoard, this, direction, true));
            }
        }
    }

    // 如果没有有效移动，则标记为完全扩展
    if (children_.empty()) {
        isFullyExpanded_ = true;
    }
}

float MCTSNode::simulate() {
    // 从当前状态开始模拟游戏直到结束
    BitBoard simBoard = board_;
    int moveCount     = 0;
    int maxTile       = 0;

    // 最大模拟步数，减少步数以提高性能
    int const MAX_SIMULATION_STEPS = 100;  // 从1000减少到100

    while (!simBoard.isGameOver() && moveCount < MAX_SIMULATION_STEPS) {
        if (isChance_) {
            // 机会节点：随机放置新方块
            std::vector<BitBoard::Position> emptyPositions = simBoard.getEmptyPositions();
            if (emptyPositions.empty()) {
                break;
            }

            // 随机选择一个空位
            std::uniform_int_distribution<size_t> posDist(0, emptyPositions.size() - 1);
            BitBoard::Position pos = emptyPositions[posDist(rng_)];

            // 90%概率放置2，10%概率放置4
            std::uniform_real_distribution<float> valueDist(0.0f, 1.0f);
            int value = (valueDist(rng_) < 0.9f) ? 2 : 4;

            simBoard.setTile(pos.row, pos.col, value);
        } else {
            // 决策节点：随机选择一个有效移动
            std::vector<int> validMoves;
            for (int direction = 0; direction < 4; ++direction) {
                BitBoard testBoard  = simBoard;
                BitBoard movedBoard = testBoard.move(static_cast<BitBoard::Direction>(direction));
                if (movedBoard != testBoard) {
                    validMoves.push_back(direction);
                }
            }

            if (validMoves.empty()) {
                break;
            }

            // 随机选择一个有效移动
            std::uniform_int_distribution<size_t> moveDist(0, validMoves.size() - 1);
            int direction = validMoves[moveDist(rng_)];
            simBoard.move(static_cast<BitBoard::Direction>(direction));
        }

        // 更新最大方块值
        maxTile = std::max(maxTile, simBoard.getMaxTile());
        moveCount++;
    }

    // 简化得分计算，只考虑最大方块值，提高性能
    float score = static_cast<float>(maxTile);

    // 如果游戏结束且没有空位，给予惩罚
    if (simBoard.isGameOver()) {
        score *= 0.5f;
    }

    return score;
}

void MCTSNode::backpropagate(float score) {
    MCTSNode* current = this;
    while (current != nullptr) {
        std::lock_guard<std::mutex> lock(current->mutex_);
        current->visitCount_++;
        current->totalScore_ += score;
        current               = current->parent_;
    }
}

int MCTSNode::getBestMove() const {
    if (children_.empty()) {
        return -1;
    }

    // 选择访问次数最多的子节点
    MCTSNode* bestChild = nullptr;
    int maxVisits       = -1;

    for (auto child : children_) {
        if (child->visitCount_ > maxVisits) {
            maxVisits = child->visitCount_;
            bestChild = child;
        }
    }

    // 返回从当前节点到最佳子节点的移动
    return bestChild ? bestChild->move_ : -1;
}

float MCTSNode::getUCB(float explorationConstant) const {
    if (visitCount_ == 0) {
        return std::numeric_limits<float>::infinity();
    }

    // UCB公式：平均得分 + 探索项
    float exploitation = totalScore_ / static_cast<float>(visitCount_);
    float exploration
        = explorationConstant
          * std::sqrt(std::log(static_cast<float>(parent_->visitCount_)) / static_cast<float>(visitCount_));

    return exploitation + exploration;
}

bool MCTSNode::isLeaf() const {
    return children_.empty();
}

bool MCTSNode::isTerminal() const {
    return board_.isGameOver();
}

int MCTSNode::getVisitCount() const {
    return visitCount_;
}

float MCTSNode::getTotalScore() const {
    return totalScore_;
}

BitBoard MCTSNode::getBoard() const {
    return board_;
}
