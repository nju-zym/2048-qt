#ifndef AUTO_H
#define AUTO_H

#include "AISearch.h"

#include <QMutex>
#include <QObject>
#include <QRandomGenerator>
#include <QVector>

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

    // 主要功能 - 现在是对AISearch的简单包装
    int findBestMove(QVector<QVector<int>> const& board);

    // 清除缓存
    void clearExpectimaxCache();

   private:
    // 策略状态信息
    bool useLearnedParams;
    int bestHistoricalScore;

    // 互斥锁
    QMutex mutex;

    // AI搜索引擎
    AISearch aiSearch;

    // 随机数生成器
    QRandomGenerator randomGenerator;
};

#endif  // AUTO_H
