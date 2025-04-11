#include "transposition_table.h"
#include <chrono>
#include <algorithm>

// 单例模式获取实例
TranspositionTable& TranspositionTable::getInstance() {
    static TranspositionTable instance;
    return instance;
}

// 构造函数
TranspositionTable::TranspositionTable() : initialized(false) {
    // 初始化 Zobrist 哈希表
    initialize();
}

// 初始化 Zobrist 哈希表
void TranspositionTable::initialize() {
    if (initialized) {
        return;
    }

    // 使用当前时间作为随机数种子，确保每次运行生成不同的哈希值
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    
    // 为每个位置和每个可能的值生成随机数
    zobristTable.resize(16); // 4x4 棋盘有 16 个位置
    for (int pos = 0; pos < 16; ++pos) {
        zobristTable[pos].resize(16); // 每个位置有 16 种可能的值（0-15）
        for (int val = 0; val < 16; ++val) {
            zobristTable[pos][val] = rng();
        }
    }

    initialized = true;
}

// 计算棋盘的 Zobrist 哈希值
int64_t TranspositionTable::computeHash(BitBoard board) {
    int64_t hash = 0;

    // 遍历棋盘的每个位置
    for (int pos = 0; pos < 16; ++pos) {
        // 提取该位置的值（4位）
        uint64_t shift = static_cast<uint64_t>(pos * 4);
        uint64_t value = (board >> shift) & 0xFULL;
        
        // 如果值不为 0，异或对应的随机数
        if (value > 0) {
            hash ^= zobristTable[pos][static_cast<int>(value)];
        }
    }

    return hash;
}

// 存储结果到置换表
void TranspositionTable::store(BitBoard board, int score, int depth, NodeType type, bool isMaxPlayer) {
    // 计算哈希值
    int64_t hash = computeHash(board);
    
    // 创建置换表项
    TTEntry entry;
    entry.hash = hash;
    entry.score = score;
    entry.depth = depth;
    entry.type = type;
    entry.isMaxPlayer = isMaxPlayer;
    
    // 加锁保护并发访问
    std::lock_guard<std::mutex> lock(tableMutex);
    
    // 如果置换表已满，移除一些旧条目
    if (table.size() >= MAX_TABLE_SIZE) {
        // 简单策略：随机移除 10% 的条目
        size_t entriesToRemove = MAX_TABLE_SIZE / 10;
        auto it = table.begin();
        for (size_t i = 0; i < entriesToRemove && it != table.end(); ++i) {
            it = table.erase(it);
        }
    }
    
    // 存储条目
    table[hash] = entry;
}

// 从置换表中查询结果
bool TranspositionTable::lookup(BitBoard board, int depth, bool isMaxPlayer, int& score, NodeType& type) {
    // 计算哈希值
    int64_t hash = computeHash(board);
    
    // 加锁保护并发访问
    std::lock_guard<std::mutex> lock(tableMutex);
    
    // 查找哈希值
    auto it = table.find(hash);
    if (it == table.end()) {
        return false; // 未找到
    }
    
    // 找到条目
    const TTEntry& entry = it->second;
    
    // 验证是否是同一个棋盘状态（通过完整哈希值比较）
    if (entry.hash != hash) {
        return false; // 哈希冲突
    }
    
    // 验证是否是同一类型的节点（MAX 或 CHANCE）
    if (entry.isMaxPlayer != isMaxPlayer) {
        return false; // 节点类型不匹配
    }
    
    // 验证深度是否足够
    if (entry.depth < depth) {
        return false; // 存储的深度不够
    }
    
    // 返回存储的值
    score = entry.score;
    type = entry.type;
    return true;
}

// 清空置换表
void TranspositionTable::clear() {
    std::lock_guard<std::mutex> lock(tableMutex);
    table.clear();
}

// 获取置换表大小
size_t TranspositionTable::size() const {
    std::lock_guard<std::mutex> lock(tableMutex);
    return table.size();
}

// 获取置换表使用率
double TranspositionTable::usageRate() const {
    std::lock_guard<std::mutex> lock(tableMutex);
    return static_cast<double>(table.size()) / MAX_TABLE_SIZE;
}
