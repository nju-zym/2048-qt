#include "monotonicity.h"

#include <algorithm>
#include <cmath>

// Forward declaration from bitboard.h
extern int getBitBoardValue(BitBoard board, int row, int col);

namespace Monotonicity {

float calculateMonotonicity(BitBoard board) {
    // 计算棋盘的单调性，即数值排列的有序性
    float monotonicity = 0.0f;

    // 计算四个方向的单调性，取最好的结果
    monotonicity = std::max({calculateLeftRightMonotonicity(board),
                             calculateRightLeftMonotonicity(board),
                             calculateTopDownMonotonicity(board),
                             calculateBottomUpMonotonicity(board)});

    return monotonicity;
}

float calculateLeftRightMonotonicity(BitBoard board) {
    // 从左到右的单调性，要求每行从左到右单调增加
    float monotonicityScore = 0.0f;

    for (int row = 0; row < 4; ++row) {
        int prevValue = 0;
        bool valid    = true;

        for (int col = 0; col < 4; ++col) {
            int currValue = getBitBoardValue(board, row, col);

            // 跳过空格子
            if (currValue == 0) {
                continue;
            }

            // 如果当前值小于前一个非零值，不符合单调增加
            if (prevValue > 0 && currValue < prevValue) {
                valid = false;
                break;
            }

            prevValue = currValue;
        }

        if (valid) {
            // 单调行增加得分
            monotonicityScore += 1.0;
        }
    }

    return monotonicityScore;
}

float calculateRightLeftMonotonicity(BitBoard board) {
    // 从右到左的单调性，要求每行从右到左单调增加
    float monotonicityScore = 0.0f;

    for (int row = 0; row < 4; ++row) {
        int prevValue = 0;
        bool valid    = true;

        for (int col = 3; col >= 0; --col) {
            int currValue = getBitBoardValue(board, row, col);

            // 跳过空格子
            if (currValue == 0) {
                continue;
            }

            // 如果当前值小于前一个非零值，不符合单调增加
            if (prevValue > 0 && currValue < prevValue) {
                valid = false;
                break;
            }

            prevValue = currValue;
        }

        if (valid) {
            // 单调行增加得分
            monotonicityScore += 1.0;
        }
    }

    return monotonicityScore;
}

float calculateTopDownMonotonicity(BitBoard board) {
    // 从上到下的单调性，要求每列从上到下单调增加
    float monotonicityScore = 0.0f;

    for (int col = 0; col < 4; ++col) {
        int prevValue = 0;
        bool valid    = true;

        for (int row = 0; row < 4; ++row) {
            int currValue = getBitBoardValue(board, row, col);

            // 跳过空格子
            if (currValue == 0) {
                continue;
            }

            // 如果当前值小于前一个非零值，不符合单调增加
            if (prevValue > 0 && currValue < prevValue) {
                valid = false;
                break;
            }

            prevValue = currValue;
        }

        if (valid) {
            // 单调列增加得分
            monotonicityScore += 1.0;
        }
    }

    return monotonicityScore;
}

float calculateBottomUpMonotonicity(BitBoard board) {
    // 从下到上的单调性，要求每列从下到上单调增加
    float monotonicityScore = 0.0f;

    for (int col = 0; col < 4; ++col) {
        int prevValue = 0;
        bool valid    = true;

        for (int row = 3; row >= 0; --row) {
            int currValue = getBitBoardValue(board, row, col);

            // 跳过空格子
            if (currValue == 0) {
                continue;
            }

            // 如果当前值小于前一个非零值，不符合单调增加
            if (prevValue > 0 && currValue < prevValue) {
                valid = false;
                break;
            }

            prevValue = currValue;
        }

        if (valid) {
            // 单调列增加得分
            monotonicityScore += 1.0;
        }
    }

    return monotonicityScore;
}

float calculateWeightedMonotonicity(BitBoard board) {
    // 加权单调性评估，考虑方块的值
    float weightedScore = 0.0f;

    // 水平方向单调性（左->右）
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row, col + 1);

            if (value1 > 0 && value2 > 0) {
                // 如果单调增加，给予正分；如果单调减少，给予负分
                if (value1 <= value2) {
                    // 值越大，单调性越重要
                    weightedScore += static_cast<float>(std::log2(value2));
                } else {
                    weightedScore -= static_cast<float>(std::log2(value1));
                }
            }
        }
    }

    // 垂直方向单调性（上->下）
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int value1 = getBitBoardValue(board, row, col);
            int value2 = getBitBoardValue(board, row + 1, col);

            if (value1 > 0 && value2 > 0) {
                if (value1 <= value2) {
                    weightedScore += static_cast<float>(std::log2(value2));
                } else {
                    weightedScore -= static_cast<float>(std::log2(value1));
                }
            }
        }
    }

    return weightedScore;
}

}  // namespace Monotonicity