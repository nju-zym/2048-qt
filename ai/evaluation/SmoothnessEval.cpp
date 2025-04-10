#include "SmoothnessEval.h"
#include <cmath>

float SmoothnessEval::evaluate(const BitBoard& board) {
    float smoothness = 0.0f;
    
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = board.getTile(row, col);
            if (value == 0) continue;
            
            float logValue = std::log2(value);
            
            // 检查右侧相邻方块
            if (col < 3) {
                int rightValue = board.getTile(row, col + 1);
                if (rightValue != 0) {
                    float rightLogValue = std::log2(rightValue);
                    smoothness -= std::abs(logValue - rightLogValue);
                }
            }
            
            // 检查下方相邻方块
            if (row < 3) {
                int downValue = board.getTile(row + 1, col);
                if (downValue != 0) {
                    float downLogValue = std::log2(downValue);
                    smoothness -= std::abs(logValue - downLogValue);
                }
            }
        }
    }
    
    return smoothness;
}
