#include "MergeEval.h"
#include <cmath>

float MergeEval::evaluate(const BitBoard& board) {
    float mergeScore = 0.0f;
    
    // 检查水平方向的合并可能性
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int value = board.getTile(row, col);
            int nextValue = board.getTile(row, col + 1);
            
            if (value > 0 && value == nextValue) {
                // 相邻相同值方块，增加合并分数
                // 值越大的方块合并，得分越高
                mergeScore += std::log2(value);
            }
        }
    }
    
    // 检查垂直方向的合并可能性
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int value = board.getTile(row, col);
            int nextValue = board.getTile(row + 1, col);
            
            if (value > 0 && value == nextValue) {
                // 相邻相同值方块，增加合并分数
                mergeScore += std::log2(value);
            }
        }
    }
    
    return mergeScore;
}
