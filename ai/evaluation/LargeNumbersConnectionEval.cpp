#include "LargeNumbersConnectionEval.h"
#include <cmath>
#include <algorithm>

float LargeNumbersConnectionEval::evaluate(const BitBoard& board) {
    float score = 0.0f;
    
    // 检查水平方向的连接
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int value1 = board.getTile(row, col);
            int value2 = board.getTile(row, col + 1);
            
            if (value1 > 0 && value2 > 0) {
                score += calculateConnectionScore(value1, value2);
            }
        }
    }
    
    // 检查垂直方向的连接
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int value1 = board.getTile(row, col);
            int value2 = board.getTile(row + 1, col);
            
            if (value1 > 0 && value2 > 0) {
                score += calculateConnectionScore(value1, value2);
            }
        }
    }
    
    return score;
}

float LargeNumbersConnectionEval::calculateConnectionScore(int value1, int value2) {
    // 计算两个数字的对数值
    float log1 = std::log2(value1);
    float log2 = std::log2(value2);
    
    // 取较大的对数值
    float maxLog = std::max(log1, log2);
    
    // 计算对数值的差异
    float diff = std::abs(log1 - log2);
    
    // 如果差异为1，表示两个数字可以合并，给予额外奖励
    if (diff < 0.01f) {  // 使用小的阈值来处理浮点数比较
        return maxLog * 2.0f;  // 可合并的数字给予双倍奖励
    } else if (diff <= 1.0f) {
        // 差异小于等于1，给予奖励，但随着差异增大而减少
        return maxLog * (1.0f - diff);
    } else if (diff <= 2.0f) {
        // 差异在1到2之间，给予较小的奖励
        return maxLog * 0.5f * (2.0f - diff);
    } else {
        // 差异太大，不给予奖励
        return 0.0f;
    }
}
