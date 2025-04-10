#ifndef LARGE_NUMBERS_CONNECTION_EVAL_H
#define LARGE_NUMBERS_CONNECTION_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 大数字连接评估类
 * 
 * 评估棋盘上的大数字是否相邻，鼓励大数字聚集在一起
 */
class LargeNumbersConnectionEval {
public:
    /**
     * @brief 评估棋盘
     * @param board 棋盘
     * @return 评估分数
     */
    static float evaluate(const BitBoard& board);
    
private:
    /**
     * @brief 计算两个数字的连接分数
     * @param value1 第一个数字
     * @param value2 第二个数字
     * @return 连接分数
     */
    static float calculateConnectionScore(int value1, int value2);
};

#endif // LARGE_NUMBERS_CONNECTION_EVAL_H
