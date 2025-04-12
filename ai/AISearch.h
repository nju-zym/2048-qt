#ifndef AISEARCH_H
#define AISEARCH_H

#include "AICache.h"
#include "BoardEvaluator.h"
#include "GameBoard.h"

#include <QMutex>
#include <QPair>
#include <QVector>
#include <memory>  // 添加智能指针支持

class AISearch {
   public:
    // 构造函数
    AISearch();

    // 禁用复制构造和赋值，确保资源唯一性
    AISearch(AISearch const&)            = delete;
    AISearch& operator=(AISearch const&) = delete;

    // 期望最大值搜索
    int expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer);

    // 查找最佳移动
    int findBestMove(QVector<QVector<int>> const& board);

    // 清除缓存
    void clearCache();

   private:
    // 棋盘评估器 - 使用智能指针
    std::unique_ptr<BoardEvaluator> evaluator;

    // 缓存系统 - 使用智能指针
    std::shared_ptr<AICache> expectimaxCache;

    // 互斥锁，保护缓存操作的线程安全
    QMutex cacheMutex;
};

#endif  // AISEARCH_H