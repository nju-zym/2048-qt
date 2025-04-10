#include "tile.h"

#include <algorithm>
#include <cmath>

namespace Tile {

// 从BitBoard中获取指定位置的值
inline int getBitBoardValue(BitBoard board, int row, int col) {
    int shift = 4 * (4 * row + col);
    return (board >> shift) & 0xF;
}

float calculateWeightedTileScore(BitBoard board) {
    float score = 0.0F;
    int maxTileValue = 0;

    // 计算每个方块的贡献并找到最大值
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = getBitBoardValue(board, i, j);
            if (value > 0) {
                // 使用指数函数提高高值方块的重要性
                score += static_cast<float>(std::pow(value, 3.5));  // 3.5次方给予高值方块更大的权重
                maxTileValue = std::max(maxTileValue, value);
            }
        }
    }

    // 额外奖励最大方块值
    // 当达到11（2048）及以上时给予更大的奖励
    if (maxTileValue >= 11) {  // 2048 = 2^11
        score += static_cast<float>(std::pow(2, maxTileValue) * 10);
    }

    return score;
}

int getMaxTileValue(BitBoard board) {
    int maxValue = 0;

    for (int i = 0; i < 16; ++i) {
        int shift = 4 * i;
        int value = (board >> shift) & 0xF;
        maxValue = std::max(maxValue, value);
    }

    return maxValue;
}

}  // namespace Tile
