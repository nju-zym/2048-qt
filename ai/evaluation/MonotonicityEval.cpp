#include "MonotonicityEval.h"
#include <cmath>
#include <algorithm>

float MonotonicityEval::evaluate(const BitBoard& board) {
    float score = 0.0f;
    
    // 计算行的单调性
    for (int row = 0; row < 4; ++row) {
        int a = board.getTile(row, 0) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(row, 0)));
        int b = board.getTile(row, 1) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(row, 1)));
        int c = board.getTile(row, 2) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(row, 2)));
        int d = board.getTile(row, 3) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(row, 3)));
        
        score += calculateMonotonicity(a, b, c, d);
    }
    
    // 计算列的单调性
    for (int col = 0; col < 4; ++col) {
        int a = board.getTile(0, col) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(0, col)));
        int b = board.getTile(1, col) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(1, col)));
        int c = board.getTile(2, col) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(2, col)));
        int d = board.getTile(3, col) == 0 ? 0 : static_cast<int>(std::log2(board.getTile(3, col)));
        
        score += calculateMonotonicity(a, b, c, d);
    }
    
    return score;
}

float MonotonicityEval::calculateMonotonicity(int a, int b, int c, int d) {
    // 计算递增和递减的单调性
    float increasing = 0.0f;
    float decreasing = 0.0f;
    
    // 检查递增单调性
    if (a > b) increasing += (a - b);
    if (b > c) increasing += (b - c);
    if (c > d) increasing += (c - d);
    
    // 检查递减单调性
    if (a < b) decreasing += (b - a);
    if (b < c) decreasing += (c - b);
    if (c < d) decreasing += (d - c);
    
    // 返回较小的值（更好的单调性）
    return -std::min(increasing, decreasing);
}
