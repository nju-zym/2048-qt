#include "FreeTilesEval.h"
#include <cmath>

float FreeTilesEval::evaluate(const BitBoard& board) {
    int emptyTiles = board.countEmptyTiles();
    
    // 使用对数函数增强空位数量的重要性
    // 当空位很少时，每个空位的价值更高
    if (emptyTiles == 0) {
        return 0.0f;
    } else {
        return std::log2(emptyTiles) * 2.0f;
    }
}
