#ifndef AICACHE_H
#define AICACHE_H

#include <QMutex>
#include <QVector>
#include <cstdint>
#include <memory>  // 添加智能指针支持
#include <unordered_map>

class AICache {
   public:
    // 棋盘状态结构体，用于缓存键
    struct BoardState {
        // 使用共享指针存储棋盘状态，避免深拷贝和不必要的内存分配
        std::shared_ptr<QVector<QVector<int>>> board;
        int depth        = 0;
        bool isMaxPlayer = false;

        BoardState() : board(std::make_shared<QVector<QVector<int>>>()) {}

        BoardState(QVector<QVector<int>> const& b, int d, bool isMax)
            : board(std::make_shared<QVector<QVector<int>>>(b)), depth(d), isMaxPlayer(isMax) {}

        bool operator==(BoardState const& other) const noexcept {
            try {
                // 安全检查
                if (!board || !other.board) {
                    return false;
                }

                if (board->size() != other.board->size()) {
                    return false;
                }

                // 首先检查简单的字段
                if (depth != other.depth || isMaxPlayer != other.isMaxPlayer) {
                    return false;
                }

                // 然后逐行逐列比较棋盘
                for (int i = 0; i < board->size(); ++i) {
                    if ((*board)[i].size() != (*other.board)[i].size()) {
                        return false;
                    }

                    for (int j = 0; j < (*board)[i].size(); ++j) {
                        if ((*board)[i][j] != (*other.board)[i][j]) {
                            return false;
                        }
                    }
                }

                return true;
            } catch (...) {
                // 捕获所有异常，确保不会崩溃
                return false;
            }
        }
    };

    // 哈希函数
    struct BoardStateHash {
        std::size_t operator()(BoardState const& state) const noexcept {
            try {
                // 安全检查
                if (!state.board || state.board->isEmpty()) {
                    return 0;
                }

                // 使用 FNV-1a 哈希算法 - 比简单累加更少冲突
                std::size_t hash            = 14695981039346656037ULL;  // FNV 偏移量基数
                std::size_t const FNV_prime = 1099511628211ULL;

                // 对棋盘状态进行哈希
                for (int i = 0; i < state.board->size(); ++i) {
                    auto const& row = (*state.board)[i];
                    for (int j = 0; j < row.size(); ++j) {
                        // 结合位置信息以减少冲突
                        std::size_t value  = (i * 4 + j) * 101 + static_cast<std::size_t>(row[j]);
                        hash              ^= value;
                        hash              *= FNV_prime;
                    }
                }

                // 组合其他字段
                hash ^= static_cast<std::size_t>(state.depth);
                hash *= FNV_prime;
                hash ^= (state.isMaxPlayer ? 1ULL : 0ULL);
                hash *= FNV_prime;

                return hash;
            } catch (...) {
                // 捕获所有异常，确保不会崩溃
                return 0;
            }
        }
    };

    // 构造函数
    explicit AICache(int maxSize = MAX_CACHE_SIZE);

    // 禁止拷贝构造和拷贝赋值，确保缓存的唯一性
    AICache(AICache const&)            = delete;
    AICache& operator=(AICache const&) = delete;

    // 缓存操作
    void clear() const;
    void insert(BoardState const& key, int score) const;
    bool contains(BoardState const& key) const;
    int get(BoardState const& key) const;
    void updateAccessTime(BoardState const& key) const;

    // 限制缓存大小
    void checkSize() const;

    // 获取统计信息
    int getHitCount() const;
    int getMissCount() const;
    size_t size() const;

   private:
    // 使用线程安全的小缓存大小，降低内存压力
    static constexpr int MAX_CACHE_SIZE = 50000;  // 降低为50000

    mutable std::unordered_map<BoardState, int, BoardStateHash> cache;
    mutable std::unordered_map<BoardState, uint64_t, BoardStateHash> accessTimes;

    // 添加互斥锁以保护缓存操作
    mutable QMutex cacheMutex;

    // 缓存修剪策略
    void pruneCache() const;

    // 计数器
    int maxCacheSize;
    mutable int hitCount           = 0;
    mutable int missCount          = 0;
    mutable uint64_t accessCounter = 0;
};

#endif  // AICACHE_H