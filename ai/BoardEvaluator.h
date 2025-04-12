#ifndef BOARDEVALUATOR_H
#define BOARDEVALUATOR_H

#include <QVector>
#include <cmath>

class BoardEvaluator {
   public:
    // 构造函数
    BoardEvaluator();

    // 评估函数
    int evaluateBoard(QVector<QVector<int>> const& boardState);
    int evaluateAdvancedPattern(QVector<QVector<int>> const& boardState);

    // 辅助评估函数
    static double calculateMergeScore(QVector<QVector<int>> const& boardState);

   private:
    // 默认评估参数
    QVector<double> defaultParams;

    // 基本评估函数
    int evaluateBoardAdvanced(QVector<QVector<int>> const& boardState);
};

#endif  // BOARDEVALUATOR_H