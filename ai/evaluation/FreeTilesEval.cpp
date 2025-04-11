#include "FreeTilesEval.h"

#include <cmath>

float FreeTilesEval::evaluate(BitBoard const& board) {
    int emptyTiles = board.countEmptyTiles();

    // 使用指数函数增强空位数量的重要性
    // 当空位很少时，每个空位的价值更高
    if (emptyTiles == 0) {
        return -10.0F;  // 没有空位是一个非常糟糕的情况，给予强烈的负面评价
    } else if (emptyTiles <= 3) {
        // 空位很少时，使用指数函数增强其重要性
        return static_cast<float>(emptyTiles * emptyTiles) * 1.5F;
    } else {
        // 空位足够多时，使用对数函数增强其重要性
        return static_cast<float>(std::log2(emptyTiles)) * 3.0F;
    }
}
