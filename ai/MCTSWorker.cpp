#include "MCTSWorker.h"

#include <QElapsedTimer>
#include <QtConcurrent>
#include <algorithm>
#include <future>
#include <random>
#include <thread>

// 默认参数
int const DEFAULT_THREAD_COUNT     = 4;
int const DEFAULT_TIME_LIMIT       = 1000;   // 毫秒，设置为1秒，快速响应
int const DEFAULT_SIMULATION_LIMIT = 10000;  // 减少模拟次数以提高运行速度
bool const DEFAULT_USE_CACHE       = true;
int const DEFAULT_CACHE_SIZE       = 100000;

// 混合策略权重
float const MCTS_WEIGHT      = 1.0f;  // 默认使用纯粹MCTS
float const HEURISTIC_WEIGHT = 0.0f;  // 不使用启发式评估

MCTSWorker::MCTSWorker(QObject* parent)
    : QObject(parent),
      threadCount_(DEFAULT_THREAD_COUNT),
      timeLimit_(DEFAULT_TIME_LIMIT),
      simulationLimit_(DEFAULT_SIMULATION_LIMIT),
      useCache_(DEFAULT_USE_CACHE),
      cacheSize_(DEFAULT_CACHE_SIZE),
      stopRequested_(false),
      totalSimulations_(0),
      cacheHits_(0),
      cacheMisses_(0) {
    // 设置线程池参数
    threadPool_.setMaxThreadCount(threadCount_);
    threadPool_.setExpiryTimeout(60000);  // 60秒
}

MCTSWorker::~MCTSWorker() {
    // 等待所有线程完成
    threadPool_.waitForDone();
}

void MCTSWorker::setThreadCount(int threadCount) {
    threadCount_ = std::max(1, threadCount);
    threadPool_.setMaxThreadCount(threadCount_);
}

void MCTSWorker::setTimeLimit(int timeLimit) {
    timeLimit_ = std::max(10, timeLimit);  // 最小10毫秒
}

void MCTSWorker::setSimulationLimit(int simulationLimit) {
    simulationLimit_ = std::max(100, simulationLimit);  // 最小100次模拟
}

void MCTSWorker::setUseCache(bool useCache) {
    useCache_ = useCache;
}

void MCTSWorker::setCacheSize(int cacheSize) {
    cacheSize_ = std::max(100, cacheSize);  // 最小100项

    // 如果当前缓存大小超过新的限制，清除一些旧条目
    if (useCache_ && moveCache_.size() > static_cast<size_t>(cacheSize_)) {
        QWriteLocker locker(&cacheLock_);

        // 简单策略：保留最新的cacheSize_个条目
        if (moveCache_.size() > static_cast<size_t>(cacheSize_)) {
            size_t toRemove = moveCache_.size() - cacheSize_;
            auto it         = moveCache_.begin();
            for (size_t i = 0; i < toRemove && it != moveCache_.end(); ++i, ++it) {
                moveCache_.erase(it);
            }
        }
    }
}

void MCTSWorker::clearCache() {
    QWriteLocker locker(&cacheLock_);
    moveCache_.clear();
    cacheHits_   = 0;
    cacheMisses_ = 0;
}

BitBoard::Direction MCTSWorker::getBestMove(BitBoard const& board) {
    // 重置停止标志
    stopRequested_    = false;
    totalSimulations_ = 0;

    // 检查缓存
    BitBoard::Direction cachedDirection;
    if (useCache_ && getCachedMove(board, cachedDirection)) {
        return cachedDirection;
    }

    // 使用混合策略
    BitBoard::Direction bestDirection = hybridStrategy(board);

    // 将结果添加到缓存
    if (useCache_) {
        addToCache(board, bestDirection);
    }

    return bestDirection;
}

void MCTSWorker::stopSearch() {
    stopRequested_ = true;
}

void MCTSWorker::runMCTS(MCTSNode* rootNode, int threadId) {
    // 线程局部随机数生成器
    thread_local std::mt19937 rng(std::random_device{}() + threadId);

    // 执行MCTS搜索
    while (!stopRequested_ && totalSimulations_ < simulationLimit_) {
        // 选择阶段：从根节点开始，选择最有前途的节点
        MCTSNode* node = rootNode;
        while (!node->isLeaf() && !node->isTerminal()) {
            node = node->selectBestChild();
            if (node == nullptr) {
                break;
            }
        }

        if (node == nullptr) {
            continue;
        }

        // 扩展阶段：如果节点不是终止节点，扩展它
        if (!node->isTerminal()) {
            node->expand();

            // 如果扩展后有子节点，选择第一个子节点进行模拟
            if (!node->isLeaf()) {
                node = node->selectBestChild();
            }
        }

        // 模拟阶段：从选定节点开始模拟游戏
        float score = node->simulate();

        // 反向传播阶段：将模拟结果反向传播到根节点
        node->backpropagate(score);

        // 更新模拟次数
        totalSimulations_++;

        // 发送进度信号
        if (threadId == 0 && totalSimulations_ % 100 == 0) {
            int progress = static_cast<int>(100.0f * totalSimulations_ / simulationLimit_);
            emit searchProgress(progress);
        }
    }
}

bool MCTSWorker::getCachedMove(BitBoard const& board, BitBoard::Direction& direction) {
    if (!useCache_) {
        return false;
    }

    uint64_t boardHash = board.getBoard();

    QReadLocker locker(&cacheLock_);
    auto it = moveCache_.find(boardHash);
    if (it != moveCache_.end()) {
        direction = it->second;
        cacheHits_++;
        return true;
    }

    cacheMisses_++;
    return false;
}

void MCTSWorker::addToCache(BitBoard const& board, BitBoard::Direction direction) {
    if (!useCache_) {
        return;
    }

    uint64_t boardHash = board.getBoard();

    QWriteLocker locker(&cacheLock_);

    // 如果缓存已满，移除一个随机条目
    if (moveCache_.size() >= static_cast<size_t>(cacheSize_)) {
        auto it = moveCache_.begin();
        std::advance(it, std::rand() % moveCache_.size());
        moveCache_.erase(it);
    }

    moveCache_[boardHash] = direction;
}

BitBoard::Direction MCTSWorker::hybridStrategy(BitBoard const& board) {
    // 创建根节点
    MCTSNode rootNode(board);

    // 启动计时器
    QElapsedTimer timer;
    timer.start();

    // 并行执行MCTS搜索
    QVector<QFuture<void>> futures;
    for (int i = 0; i < threadCount_; ++i) {
        futures.push_back(QtConcurrent::run(&threadPool_, [this, &rootNode, i]() { runMCTS(&rootNode, i); }));
    }

    // 等待时间限制或模拟次数限制
    while (timer.elapsed() < timeLimit_ && totalSimulations_ < simulationLimit_ && !stopRequested_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 停止搜索
    stopRequested_ = true;

    // 等待所有线程完成
    for (auto& future : futures) {
        future.waitForFinished();
    }

    // 获取MCTS的最佳移动
    int mctsMove = rootNode.getBestMove();

    // 如果MCTS没有找到有效移动，使用启发式评估
    if (mctsMove == -1) {
        // 尝试所有可能的移动，选择评估分数最高的
        float bestScore   = -std::numeric_limits<float>::infinity();
        int bestDirection = -1;

        for (int direction = 0; direction < 4; ++direction) {
            BitBoard newBoard   = board;
            BitBoard movedBoard = newBoard.move(static_cast<BitBoard::Direction>(direction));
            if (movedBoard != newBoard) {
                float score = evaluateBoard(newBoard);
                if (score > bestScore) {
                    bestScore     = score;
                    bestDirection = direction;
                }
            }
        }

        return static_cast<BitBoard::Direction>(bestDirection);
    }

    // 混合策略：结合MCTS和启发式评估
    // 由于我们已经将HEURISTIC_WEIGHT设置为0，这部分代码不会执行
    if (false) {  // 始终不执行混合策略
        // 计算每个方向的启发式评估分数
        std::vector<float> heuristicScores(4, -std::numeric_limits<float>::infinity());

        for (int direction = 0; direction < 4; ++direction) {
            BitBoard newBoard   = board;
            BitBoard movedBoard = newBoard.move(static_cast<BitBoard::Direction>(direction));
            if (movedBoard != newBoard) {
                heuristicScores[direction] = evaluateBoard(newBoard);
            }
        }

        // 归一化启发式分数
        float maxHeuristicScore = *std::max_element(heuristicScores.begin(), heuristicScores.end());
        if (maxHeuristicScore > -std::numeric_limits<float>::infinity()) {
            for (int direction = 0; direction < 4; ++direction) {
                if (heuristicScores[direction] > -std::numeric_limits<float>::infinity()) {
                    heuristicScores[direction] /= maxHeuristicScore;
                } else {
                    heuristicScores[direction] = 0.0f;
                }
            }
        }

        // 如果MCTS的最佳移动与启发式评估的最佳移动不同，考虑混合策略
        int heuristicBestDirection
            = std::distance(heuristicScores.begin(), std::max_element(heuristicScores.begin(), heuristicScores.end()));

        if (heuristicBestDirection != mctsMove && heuristicScores[heuristicBestDirection] > 0.0f) {
            // 如果启发式评估的最佳移动比MCTS的最佳移动好很多，考虑使用启发式评估的结果
            if (heuristicScores[heuristicBestDirection] > 0.8f && heuristicScores[mctsMove] < 0.5f) {
                return static_cast<BitBoard::Direction>(heuristicBestDirection);
            }

            // 否则，使用MCTS的结果
            return static_cast<BitBoard::Direction>(mctsMove);
        }
    }

    // 默认使用MCTS的结果
    return static_cast<BitBoard::Direction>(mctsMove);
}

float MCTSWorker::evaluateBoard(BitBoard const& board) {
    // 启发式评估函数，考虑多个因素

    // 1. 空位数量
    float emptyTilesScore = static_cast<float>(board.countEmptyTiles()) * 10.0f;

    // 2. 最大方块值
    float maxTileScore = static_cast<float>(board.getMaxTile());

    // 3. 单调性（方块值沿某个方向递增或递减）
    float monotonicity = 0.0f;

    // 水平方向的单调性
    for (int row = 0; row < 4; ++row) {
        // 从左到右递增
        float increasingH = 0.0f;
        // 从左到右递减
        float decreasingH = 0.0f;

        for (int col = 0; col < 3; ++col) {
            int current = board.getTile(row, col);
            int next    = board.getTile(row, col + 1);

            if (current > 0 && next > 0) {
                if (current > next) {
                    decreasingH += std::log2(current) - std::log2(next);
                } else if (current < next) {
                    increasingH += std::log2(next) - std::log2(current);
                }
            }
        }

        monotonicity += std::max(increasingH, decreasingH);
    }

    // 垂直方向的单调性
    for (int col = 0; col < 4; ++col) {
        // 从上到下递增
        float increasingV = 0.0f;
        // 从上到下递减
        float decreasingV = 0.0f;

        for (int row = 0; row < 3; ++row) {
            int current = board.getTile(row, col);
            int next    = board.getTile(row + 1, col);

            if (current > 0 && next > 0) {
                if (current > next) {
                    decreasingV += std::log2(current) - std::log2(next);
                } else if (current < next) {
                    increasingV += std::log2(next) - std::log2(current);
                }
            }
        }

        monotonicity += std::max(increasingV, decreasingV);
    }

    // 4. 平滑性（相邻方块值的差异）
    float smoothness = 0.0f;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = board.getTile(row, col);
            if (value > 0) {
                float logValue = std::log2(value);

                // 检查右侧方块
                if (col < 3) {
                    int rightValue = board.getTile(row, col + 1);
                    if (rightValue > 0) {
                        smoothness -= std::abs(logValue - std::log2(rightValue));
                    }
                }

                // 检查下方方块
                if (row < 3) {
                    int belowValue = board.getTile(row + 1, col);
                    if (belowValue > 0) {
                        smoothness -= std::abs(logValue - std::log2(belowValue));
                    }
                }
            }
        }
    }

    // 5. 合并机会（相邻相同方块的数量）
    float mergeOpportunities = 0.0f;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int current = board.getTile(row, col);
            int next    = board.getTile(row, col + 1);

            if (current > 0 && current == next) {
                mergeOpportunities += std::log2(current);
            }
        }
    }

    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int current = board.getTile(row, col);
            int next    = board.getTile(row + 1, col);

            if (current > 0 && current == next) {
                mergeOpportunities += std::log2(current);
            }
        }
    }

    // 6. 角落策略（大方块在角落）
    float cornerStrategy = 0.0f;

    // 检查四个角落
    int topLeft     = board.getTile(0, 0);
    int topRight    = board.getTile(0, 3);
    int bottomLeft  = board.getTile(3, 0);
    int bottomRight = board.getTile(3, 3);

    // 找出最大的角落方块
    int maxCorner = std::max({topLeft, topRight, bottomLeft, bottomRight});

    if (maxCorner > 0) {
        // 如果最大方块在角落，给予奖励
        if (maxCorner == board.getMaxTile()) {
            cornerStrategy += std::log2(maxCorner) * 2.0f;
        } else {
            cornerStrategy += std::log2(maxCorner);
        }
    }

    // 组合所有因素，权重可以根据经验调整
    float score = emptyTilesScore * 2.0f + maxTileScore * 1.0f + monotonicity * 1.0f + smoothness * 0.1f
                  + mergeOpportunities * 1.0f + cornerStrategy * 1.5f;

    return score;
}
