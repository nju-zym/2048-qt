#include "auto.h"

#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <ctime>

// 构造函数
Auto::Auto()
    : useLearnedParams(false),
      bestHistoricalScore(0),
      randomGenerator(static_cast<quint32>(QDateTime::currentSecsSinceEpoch())) {
    // 不再直接修改全局随机数生成器
}

// 析构函数
Auto::~Auto() {
    qDebug() << "Auto object destroyed successfully";
}

// findBestMove: 找出最佳移动方向
int Auto::findBestMove(QVector<QVector<int>> const& board) {
    // 使用AI搜索引擎找出最佳移动
    return aiSearch.findBestMove(board);
}

// 清除expectimax缓存
void Auto::clearExpectimaxCache() {
    aiSearch.clearCache();
}
