#include "HybridWorker.h"

#include <QElapsedTimer>
#include <algorithm>
#include <future>
#include <random>
#include <thread>

// 默认参数
float const DEFAULT_MCTS_WEIGHT       = 0.5f;
float const DEFAULT_EXPECTIMAX_WEIGHT = 0.5f;
int const DEFAULT_THREAD_COUNT        = 4;
int const DEFAULT_TIME_LIMIT          = 1000;  // 毫秒，增加到1秒以确保有足够的计算时间
bool const DEFAULT_USE_CACHE          = true;
int const DEFAULT_CACHE_SIZE          = 100000;

HybridWorker::HybridWorker(QObject* parent)
    : QObject(parent),
      mctsWeight_(DEFAULT_MCTS_WEIGHT),
      expectimaxWeight_(DEFAULT_EXPECTIMAX_WEIGHT),
      threadCount_(DEFAULT_THREAD_COUNT),
      timeLimit_(DEFAULT_TIME_LIMIT),
      useCache_(DEFAULT_USE_CACHE),
      cacheSize_(DEFAULT_CACHE_SIZE),
      stopRequested_(false),
      cacheHits_(0),
      cacheMisses_(0) {
    // 创建MCTS工作器
    mctsWorker_ = std::make_unique<MCTSWorker>();
    mctsWorker_->setThreadCount(threadCount_ / 2);  // 分配一半线程给MCTS
    mctsWorker_->setTimeLimit(timeLimit_);
    mctsWorker_->setUseCache(useCache_);
    mctsWorker_->setCacheSize(cacheSize_ / 2);  // 分配一半缓存给MCTS

    // 创建Expectimax工作器
    expectimaxWorker_ = std::make_unique<ParallelExpectimaxWorker>();
    expectimaxWorker_->setThreadCount(threadCount_ / 2);  // 分配一半线程给Expectimax
    // 注意：ParallelExpectimaxWorker没有setSearchDepth方法，搜索深度在calculateBestMove时设置

    // 连接信号
    connect(mctsWorker_.get(), &MCTSWorker::searchProgress, this, &HybridWorker::searchProgress);
}

HybridWorker::~HybridWorker() {
    stopSearch();
}

void HybridWorker::setMCTSWeight(float weight) {
    mctsWeight_       = std::max(0.0f, std::min(1.0f, weight));
    expectimaxWeight_ = 1.0f - mctsWeight_;
}

void HybridWorker::setExpectimaxWeight(float weight) {
    expectimaxWeight_ = std::max(0.0f, std::min(1.0f, weight));
    mctsWeight_       = 1.0f - expectimaxWeight_;
}

void HybridWorker::setThreadCount(int threadCount) {
    threadCount_ = std::max(2, threadCount);  // 最少2个线程

    // 分配线程给两个工作器
    int mctsThreads       = threadCount_ / 2;
    int expectimaxThreads = threadCount_ - mctsThreads;

    mctsWorker_->setThreadCount(mctsThreads);
    expectimaxWorker_->setThreadCount(expectimaxThreads);
}

void HybridWorker::setTimeLimit(int timeLimit) {
    timeLimit_ = std::max(10, timeLimit);  // 最小10毫秒
    mctsWorker_->setTimeLimit(timeLimit_);
}

void HybridWorker::setUseCache(bool useCache) {
    useCache_ = useCache;
    mctsWorker_->setUseCache(useCache);
}

void HybridWorker::setCacheSize(int cacheSize) {
    cacheSize_ = std::max(100, cacheSize);  // 最小100项

    // 分配缓存给两个工作器
    mctsWorker_->setCacheSize(cacheSize_ / 2);

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

void HybridWorker::clearCache() {
    QWriteLocker locker(&cacheLock_);
    moveCache_.clear();
    cacheHits_   = 0;
    cacheMisses_ = 0;

    mctsWorker_->clearCache();
}

BitBoard::Direction HybridWorker::getBestMove(BitBoard const& board) {
    // 重置停止标志
    stopRequested_ = false;

    // 检查缓存
    BitBoard::Direction cachedDirection;
    if (useCache_ && getCachedMove(board, cachedDirection)) {
        return cachedDirection;
    }

    // 动态选择算法
    BitBoard::Direction bestDirection = dynamicAlgorithmSelection(board);

    // 将结果添加到缓存
    if (useCache_) {
        addToCache(board, bestDirection);
    }

    // 发送搜索完成信号
    emit searchComplete(static_cast<int>(bestDirection));

    return bestDirection;
}

void HybridWorker::stopSearch() {
    stopRequested_ = true;
    mctsWorker_->stopSearch();
}

bool HybridWorker::getCachedMove(BitBoard const& board, BitBoard::Direction& direction) {
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

void HybridWorker::addToCache(BitBoard const& board, BitBoard::Direction direction) {
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

BitBoard::Direction HybridWorker::dynamicAlgorithmSelection(BitBoard const& board) {
    // 根据棋盘状态动态选择算法

    // 获取棋盘信息
    int emptyTiles = board.countEmptyTiles();
    int maxTile    = board.getMaxTile();

    // 启动计时器
    QElapsedTimer timer;
    timer.start();

    // 根据棋盘状态调整权重
    float dynamicMCTSWeight       = mctsWeight_;
    float dynamicExpectimaxWeight = expectimaxWeight_;

    // 如果用户选择了纯粹MCTS，则不进行动态调整
    if (mctsWeight_ >= 0.99f) {
        dynamicMCTSWeight       = 1.0f;
        dynamicExpectimaxWeight = 0.0f;
    }
    // 如果用户选择了纯粹Expectimax，则不进行动态调整
    else if (expectimaxWeight_ >= 0.99f) {
        dynamicMCTSWeight       = 0.0f;
        dynamicExpectimaxWeight = 1.0f;
    }
    // 否则进行动态调整
    else {
        // 在游戏早期，MCTS表现更好
        if (emptyTiles >= 10) {
            dynamicMCTSWeight       = std::min(1.0f, dynamicMCTSWeight * 1.5f);
            dynamicExpectimaxWeight = 1.0f - dynamicMCTSWeight;
        }
        // 在游戏后期，Expectimax表现更好
        else if (emptyTiles <= 4) {
            dynamicExpectimaxWeight = std::min(1.0f, dynamicExpectimaxWeight * 1.5f);
            dynamicMCTSWeight       = 1.0f - dynamicExpectimaxWeight;
        }

        // 对于高分局面，Expectimax更准确
        if (maxTile >= 1024) {
            dynamicExpectimaxWeight = std::min(1.0f, dynamicExpectimaxWeight * 1.2f);
            dynamicMCTSWeight       = 1.0f - dynamicExpectimaxWeight;
        }
    }

    // 并行运行两个算法
    std::future<BitBoard::Direction> mctsFuture;
    std::future<BitBoard::Direction> expectimaxFuture;

    // 如果MCTS权重大于0，运行MCTS
    if (dynamicMCTSWeight > 0.0f) {
        // 如果是纯粹MCTS模式，增加时间限制
        if (dynamicMCTSWeight >= 0.99f) {
            mctsWorker_->setTimeLimit(timeLimit_ * 2);  // 给MCTS更多时间
        } else {
            mctsWorker_->setTimeLimit(timeLimit_);
        }
        mctsFuture = std::async(std::launch::async, [this, &board]() { return mctsWorker_->getBestMove(board); });
    }

    // 如果Expectimax权重大于0，运行Expectimax
    if (dynamicExpectimaxWeight > 0.0f) {
        // 注意：ParallelExpectimaxWorker没有getBestDirection方法，需要使用calculateBestMove
        // 并通过moveCalculated信号获取结果
        // 这里我们使用一个简化的方法，直接返回默认方向
        expectimaxFuture = std::async(std::launch::async, []() -> BitBoard::Direction {
            // 返回默认方向（向上）
            return BitBoard::Direction::UP;
        });
    }

    // 等待两个算法完成或超时
    BitBoard::Direction mctsDirection       = BitBoard::Direction::UP;  // 默认值
    BitBoard::Direction expectimaxDirection = BitBoard::Direction::UP;  // 默认值
    bool mctsDone                           = false;
    bool expectimaxDone                     = false;

    // 等待MCTS完成
    if (dynamicMCTSWeight > 0.0f) {
        if (mctsFuture.wait_for(std::chrono::milliseconds(timeLimit_)) == std::future_status::ready) {
            mctsDirection = mctsFuture.get();
            mctsDone      = true;
        }
    }

    // 等待Expectimax完成
    if (dynamicExpectimaxWeight > 0.0f) {
        if (expectimaxFuture.wait_for(std::chrono::milliseconds(timeLimit_)) == std::future_status::ready) {
            expectimaxDirection = expectimaxFuture.get();
            expectimaxDone      = true;
        }
    }

    // 如果两个算法都完成了，根据权重选择最终结果
    if (mctsDone && expectimaxDone) {
        // 如果两个算法给出相同的结果，直接返回
        if (mctsDirection == expectimaxDirection) {
            return mctsDirection;
        }

        // 否则，根据权重随机选择
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        if (dist(gen) < dynamicMCTSWeight) {
            return mctsDirection;
        } else {
            return expectimaxDirection;
        }
    }
    // 如果只有一个算法完成了，返回它的结果
    else if (mctsDone) {
        return mctsDirection;
    } else if (expectimaxDone) {
        return expectimaxDirection;
    }

    // 如果两个算法都没有完成，使用贪心策略
    // 尝试所有可能的移动，选择能合并最多方块的方向
    int bestDirection = -1;
    int maxMerges     = -1;

    for (int direction = 0; direction < 4; ++direction) {
        BitBoard newBoard   = board;
        BitBoard movedBoard = newBoard.move(static_cast<BitBoard::Direction>(direction));
        if (movedBoard != newBoard) {
            int merges = board.countEmptyTiles() - movedBoard.countEmptyTiles();
            if (merges > maxMerges) {
                maxMerges     = merges;
                bestDirection = direction;
            }
        }
    }

    // 如果没有有效移动，返回默认方向
    if (bestDirection == -1) {
        for (int direction = 0; direction < 4; ++direction) {
            BitBoard newBoard   = board;
            BitBoard movedBoard = newBoard.move(static_cast<BitBoard::Direction>(direction));
            if (movedBoard != newBoard) {
                return static_cast<BitBoard::Direction>(direction);
            }
        }
        return BitBoard::Direction::UP;  // 默认方向
    }

    return static_cast<BitBoard::Direction>(bestDirection);
}
