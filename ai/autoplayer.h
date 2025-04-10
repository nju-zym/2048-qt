#ifndef AUTOPLAYER_H
#define AUTOPLAYER_H

#include "bitboard.h"
#include "evaluation/evaluation.h"
#include "expectimax.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVector>
#include <QWaitCondition>
#include <atomic>
#include <random>
#include <unordered_map>
#include <vector>

// 为BitBoard类型定义哈希函数
inline uint qHash(BitBoard const& key, uint seed = 0) {
    // 使用Qt的哈希函数计算BitBoard的哈希值
    // 将BitBoard拆分为两个32位整数进行哈希
    quint64 value = static_cast<quint64>(key);
    uint high     = static_cast<uint>(value >> 32);
    uint low      = static_cast<uint>(value & 0xFF'FF'FF'FF);
    return (high ^ low) ^ seed;
}

// Worker thread for AI calculations
class AIWorker : public QThread {
    Q_OBJECT

   public:
    explicit AIWorker(QObject* parent = nullptr);
    ~AIWorker() override;

    void requestMove(QVector<QVector<int>> const& board);
    void stopCalculation();

   signals:
    void moveCalculated(int direction);

   protected:
    void run() override;

   private:
    QMutex mutex;
    QWaitCondition condition;
    QVector<QVector<int>> currentBoard;
    bool abort;
    bool restart;

    // 缓存机制
    struct CacheEntry {
        float score;
        int bestMove;
    };
    QHash<BitBoard, CacheEntry> transpositionTable;  // 缓存计算结果

    // AI calculation methods
    int calculateBestMove(QVector<QVector<int>> const& board);
    BitBoard convertToBitBoard(QVector<QVector<int>> const& board);
    int expectimax(BitBoard board, int depth, bool maximizingPlayer, float& score);
    float evaluateBoard(BitBoard board);

    // Helper methods for bitboard operations
    bool canMove(BitBoard board, int direction);
    BitBoard makeMove(BitBoard board, int direction);
    BitBoard processTiles(BitBoard row);
    BitBoard reverseRow(BitBoard row);
};

class AutoPlayer : public QObject {
    Q_OBJECT

   public:
    explicit AutoPlayer(QObject* parent = nullptr);
    ~AutoPlayer() override;

    // Start/stop auto-play
    void startAutoPlay();
    void stopAutoPlay();
    bool isAutoPlaying() const;

    // Get the next best move using Expectimax algorithm
    // direction: 0=up, 1=right, 2=down, 3=left
    int getBestMove(QVector<QVector<int>> const& board);

    // Training functions
    void startTraining(int iterations);
    void stopTraining();
    bool isTraining() const;

    // Load/save learned parameters
    bool loadParameters();
    bool saveParameters();

   signals:
    void moveDecided(int direction);
    void trainingProgress(int current, int total);
    void trainingComplete();

   private slots:
    void handleMoveCalculated(int direction);

   private:
    // Auto-play state
    bool autoPlaying;
    bool training;

    // Worker thread for AI calculations
    AIWorker* worker;

    // Last calculated move
    int lastCalculatedMove;

    // Helper methods for auto-play
    BitBoard convertToBitBoard(QVector<QVector<int>> const& board);
    QVector<QVector<int>> convertFromBitBoard(BitBoard board);
    int expectimax(BitBoard board, int depth, bool maximizingPlayer, float& score);
    float evaluateBoard(BitBoard board);
    float countEmptyTiles(BitBoard board);
    float calculateMonotonicity(BitBoard board);
    float calculateSmoothness(BitBoard board);
    float cornerStrategy(BitBoard board);
    BitBoard makeMove(BitBoard board, int direction);
    BitBoard processTiles(BitBoard row);
    BitBoard reverseRow(BitBoard row);
    bool canMove(BitBoard board, int direction);
    std::vector<std::pair<BitBoard, float>> getPossibleNewTiles(BitBoard board);

    // 重载的expectimax函数，带有权重参数
    int expectimax(BitBoard board,
                   int depth,
                   bool maximizingPlayer,
                   float& score,
                   float emptyWeight,
                   float monoWeight,
                   float smoothWeight,
                   float cornerWeight,
                   float snakeWeight,
                   float mergeWeight,
                   float tileWeight);

    // 训练相关方法
    float simulateGame(
        float emptyW, float monoW, float smoothW, float cornerW, float snakeW, float mergeW, float tileW);
    BitBoard addRandomTile(BitBoard board);
    int getMaxTileValue(BitBoard board);

    // 高级评估函数
    float snakePatternStrategy(BitBoard board);
    float mergeOpportunityScore(BitBoard board);
    float weightedTileScore(BitBoard board);

    // 使用缓存提高效率
    using BoardCacheKey = BitBoard;
    std::unordered_map<BoardCacheKey, float> evaluationCache;
    std::unordered_map<BoardCacheKey, int> expectimaxCache;

    // Parameters for evaluation
    float emptyWeight;
    float monoWeight;
    float smoothWeight;
    float cornerWeight;
    float snakeWeight;
    float mergeWeight;
    float tileWeight;

    // Random number generator
    std::mt19937 rng;
};

#endif  // AUTOPLAYER_H
