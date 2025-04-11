#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <random>
#include <mutex>
#include <memory>

// 使用 BitBoard 类型（与 auto.h 中定义一致）
using BitBoard = uint64_t;

// 置换表项的类型定义
enum class NodeType {
    EXACT,      // 精确值
    LOWER_BOUND, // 下界（Alpha 剪枝）
    UPPER_BOUND  // 上界（Beta 剪枝）
};

// 置换表项结构
struct TTEntry {
    int64_t hash;      // 完整的 Zobrist 哈希值（用于处理哈希冲突）
    int score;         // 节点的评分
    int depth;         // 搜索深度
    NodeType type;     // 节点类型
    bool isMaxPlayer;  // 是否为 MAX 节点
};

// 置换表类
class TranspositionTable {
public:
    // 单例模式获取实例
    static TranspositionTable& getInstance();

    // 初始化 Zobrist 哈希值
    void initialize();

    // 计算棋盘的 Zobrist 哈希值
    int64_t computeHash(BitBoard board);

    // 存储结果到置换表
    void store(BitBoard board, int score, int depth, NodeType type, bool isMaxPlayer);

    // 从置换表中查询结果
    bool lookup(BitBoard board, int depth, bool isMaxPlayer, int& score, NodeType& type);

    // 清空置换表
    void clear();

    // 获取置换表大小
    size_t size() const;

    // 获取置换表使用率
    double usageRate() const;

private:
    // 私有构造函数（单例模式）
    TranspositionTable();
    
    // 禁止拷贝和赋值
    TranspositionTable(const TranspositionTable&) = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;

    // Zobrist 哈希表 - 为每个位置和每个可能的值预先计算随机数
    // 4x4 棋盘，每个位置最多 16 种可能的值（0-15，对应 0 到 2^15）
    std::vector<std::vector<int64_t>> zobristTable;

    // 置换表 - 使用哈希值作为键，TTEntry 作为值
    std::unordered_map<int64_t, TTEntry> table;

    // 互斥锁，保护置换表的并发访问
    mutable std::mutex tableMutex;

    // 是否已初始化
    bool initialized;

    // 置换表的最大大小（可调整）
    static constexpr size_t MAX_TABLE_SIZE = 10000000; // 1000万项
};

#endif // TRANSPOSITION_TABLE_H
