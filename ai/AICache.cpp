#include "AICache.h"

#include <QDebug>
#include <QMutexLocker>
#include <algorithm>
#include <cstdint>

// 构造函数：初始化缓存和计数器
AICache::AICache(int maxSize) {
    this->maxCacheSize  = maxSize;
    this->hitCount      = 0;
    this->missCount     = 0;
    this->accessCounter = 0;
}

// 检查缓存中是否包含指定的棋盘状态
bool AICache::contains(BoardState const& key) const {
    if (!key.board || key.board->isEmpty()) {
        return false;
    }

    try {
        QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护
        return this->cache.find(key) != this->cache.end();
    } catch (std::exception const& e) {
        return false;
    }
}

// 从缓存中获取指定棋盘状态的评分
int AICache::get(BoardState const& key) const {
    // 首先检查棋盘是否为空
    if (!key.board || key.board->isEmpty()) {
        QMutexLocker locker(&cacheMutex);  // 保护计数器更新
        ++this->missCount;
        return 0;
    }

    try {
        QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护
        auto it = this->cache.find(key);
        if (it != this->cache.end()) {
            // 缓存命中，增加计数
            ++this->hitCount;
            return it->second;
        }
        // 缓存未命中，增加计数
        ++this->missCount;
    } catch (std::exception const& e) {
        // 如果发生异常，安全地处理
        QMutexLocker locker(&cacheMutex);  // 保护计数器更新
        ++this->missCount;
        return 0;
    }

    return 0;  // 没找到返回0
}

// 更新访问时间
void AICache::updateAccessTime(BoardState const& key) const {
    if (!key.board || key.board->isEmpty()) {
        return;
    }

    try {
        QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护
        this->accessTimes[key] = ++this->accessCounter;
    } catch (std::exception const& e) {
        // 如果发生异常，安全地处理
    }
}

// 将棋盘状态及其评分插入缓存
void AICache::insert(BoardState const& key, int score) const {
    if (!key.board || key.board->isEmpty()) {
        return;
    }

    try {
        QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护

        // 检查缓存大小是否接近上限 - 预防性修剪
        if (cache.size() >= maxCacheSize * 0.95) {
            pruneCache();
        }

        // 创建一个新的BoardState副本，避免持有对原始key的引用
        BoardState safeCopy(*key.board, key.depth, key.isMaxPlayer);

        // 插入新数据
        cache[safeCopy] = score;

        // 更新访问时间
        accessTimes[safeCopy] = ++accessCounter;
    } catch (std::exception const& e) {
        // 如果发生异常，安全地处理并记录错误
        qDebug() << "Error in cache insert: " << e.what();
    } catch (...) {
        // 捕获所有其他类型的异常
        qDebug() << "Unknown error in cache insert";
    }
}

// 检查缓存大小并在需要时修剪
void AICache::checkSize() const {
    try {
        // 注意：此方法已在insert中被锁保护，不需要再加锁

        // 检查缓存大小是否达到上限
        if (this->cache.size() >= this->maxCacheSize) {
            // 改进：不再简单地清空整个缓存，而是采用智能的缓存替换策略
            this->pruneCache();
        }
    } catch (std::exception const& e) {
        // 如果发生异常，安全地处理
    }
}

// 清空整个缓存
void AICache::clear() const {
    try {
        QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护
        this->cache.clear();
        this->accessTimes.clear();
        this->hitCount      = 0;
        this->missCount     = 0;
        this->accessCounter = 0;
    } catch (std::exception const& e) {
        // 如果发生异常，安全地处理
    }
}

// 缓存修剪策略
void AICache::pruneCache() const {
    try {
        // 注意：此方法已在checkSize或insert中被锁保护，不需要再加锁

        // 如果缓存小于最大大小，不需要修剪
        if (cache.size() < maxCacheSize) {
            return;
        }

        // 直接清除一半缓存以简化操作，避免复杂排序可能导致的问题
        size_t targetSize = maxCacheSize / 2;

        if (cache.size() <= targetSize) {
            return;  // 安全检查
        }

        // 创建局部副本以避免迭代器失效问题
        std::vector<BoardState> keysToRemove;
        keysToRemove.reserve(cache.size() - targetSize);

        size_t currentCount = 0;
        for (auto const& pair : cache) {
            if (currentCount >= targetSize) {
                keysToRemove.push_back(pair.first);
            }
            currentCount++;
        }

        // 安全移除多余的缓存项
        for (auto const& key : keysToRemove) {
            cache.erase(key);
            accessTimes.erase(key);
        }

        // 记录缓存修剪操作
        qDebug() << "Cache pruned: removed" << keysToRemove.size() << "items";
    } catch (std::exception const& e) {
        // 如果发生异常，安全地处理并记录错误
        qDebug() << "Error in pruneCache: " << e.what();

        // 出现问题时，清空所有缓存以恢复到安全状态
        cache.clear();
        accessTimes.clear();
    } catch (...) {
        // 捕获所有其他类型的异常
        qDebug() << "Unknown error in pruneCache";

        // 出现未知问题时，清空所有缓存以恢复到安全状态
        cache.clear();
        accessTimes.clear();
    }
}

// 获取缓存命中次数
int AICache::getHitCount() const {
    try {
        QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护
        return this->hitCount;
    } catch (std::exception const& e) {
        return 0;
    }
}

// 获取缓存未命中次数
int AICache::getMissCount() const {
    try {
        QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护
        return this->missCount;
    } catch (std::exception const& e) {
        return 0;
    }
}

// 返回缓存当前大小
size_t AICache::size() const {
    try {
        QMutexLocker locker(&cacheMutex);  // 添加互斥锁保护
        return this->cache.size();
    } catch (std::exception const& e) {
        return 0;
    }
}