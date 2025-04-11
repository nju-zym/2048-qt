#ifndef AUTO_H
#define AUTO_H

#include "bitboard_tables.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <QVector>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <random>
#include <unordered_map>

// 位棋盘状态结构体
struct BitBoardState {
    BitBoard board;
    int depth;
    bool isMaxPlayer;

    bool operator==(BitBoardState const& other) const {
        return board == other.board && depth == other.depth && isMaxPlayer == other.isMaxPlayer;
    }
};

// 位棋盘状态的哈希函数
namespace std {
template <>
struct hash<BitBoardState> {
    std::size_t operator()(BitBoardState const& state) const {
        std::size_t h1 = std::hash<BitBoard>{}(state.board);
        std::size_t h2 = std::hash<int>{}(state.depth);
        std::size_t h3 = std::hash<bool>{}(state.isMaxPlayer);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
}  // namespace std

class Auto : public QObject {
    Q_OBJECT

   public:
    Auto();
    ~Auto() override;

    // 禁用复制和移动
    Auto(Auto const&)            = delete;
    Auto& operator=(Auto const&) = delete;
    Auto(Auto&&)                 = delete;
    Auto& operator=(Auto&&)      = delete;

    // 设置和获取参数
    void setUseLearnedParams(bool use);
    [[nodiscard]] bool getUseLearnedParams() const;
    [[nodiscard]] QVector<double> getStrategyParams() const;

    // 主要功能
    int findBestMove(QVector<QVector<int>> const& board);

    // 初始化位棋盘表格 - 公开方法供其他类调用
    void initTables();

    // 缓存相关
    void clearExpectimaxCache();  // 清除缓存

   private:
    // 策略参数
    QVector<double> strategyParams;
    QVector<double> defaultParams;  // 默认参数
    bool useLearnedParams;

    QMutex mutex;

    // 缓存相关
    struct BoardState {
        QVector<QVector<int>> board;
        int depth;
        bool isMaxPlayer;

        bool operator==(BoardState const& other) const {
            return board == other.board && depth == other.depth && isMaxPlayer == other.isMaxPlayer;
        }
    };

    struct BoardStateHash {
        std::size_t operator()(BoardState const& state) const {
            std::size_t hash = 0;
            for (auto const& row : state.board) {
                for (int value : row) {
                    hash = hash * 31 + value;
                }
            }
            hash = hash * 31 + state.depth;
            hash = hash * 31 + (state.isMaxPlayer ? 1 : 0);
            return hash;
        }
    };

    std::unordered_map<BoardState, int, BoardStateHash> expectimaxCache;

    // 使用位操作优化的棋盘表示
    typedef uint64_t BitBoard;

    struct BitBoardState {
        BitBoard board;
        int depth;
        bool isMaxPlayer;

        bool operator==(BitBoardState const& other) const {
            return board == other.board && depth == other.depth && isMaxPlayer == other.isMaxPlayer;
        }
    };

    struct BitBoardStateHash {
        std::size_t operator()(BitBoardState const& state) const {
            return state.board ^ (static_cast<uint64_t>(state.depth) << 32) ^ (state.isMaxPlayer ? 0x1ULL << 63 : 0);
        }
    };

    // 位棋盘缓存
    std::unordered_map<BitBoardState, int, BitBoardStateHash> bitboardCache;

    // 评估函数
    int evaluateBoard(QVector<QVector<int>> const& boardState);
    int evaluateBoardAdvanced(QVector<QVector<int>> const& boardState);
    int evaluateWithParams(QVector<QVector<int>> const& boardState, QVector<double> const& params);
    int evaluateAdvancedPattern(QVector<QVector<int>> const& boardState);
    static double calculateMergeScore(QVector<QVector<int>> const& boardState);

    // 位操作相关函数
    BitBoard convertToBitBoard(QVector<QVector<int>> const& boardState);
    QVector<QVector<int>> convertFromBitBoard(BitBoard board);
    int evaluateBitBoard(BitBoard board);
    int countEmptyTiles(BitBoard board);
    bool simulateMoveBitBoard(BitBoard& board, int direction, int& score);
    int expectimaxBitBoard(BitBoard board, int depth, bool isMaxPlayer);
    int getBestMoveBitBoard(QVector<QVector<int>> const& boardState);
    bool isGameOver(QVector<QVector<int>> const& boardState);
    bool isGameOverBitBoard(BitBoard board);

    // 模拟和搜索
    bool simulateMove(QVector<QVector<int>>& boardState, int direction, int& score);
    int expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer);
};

#endif  // AUTO_H
