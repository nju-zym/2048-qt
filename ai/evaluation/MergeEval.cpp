#include "MergeEval.h"

#include <cmath>

float MergeEval::evaluate(BitBoard const& board) {
    float mergeScore = 0.0F;

    // 获取棋盘上的最大值
    uint32_t maxTile = 0;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            uint32_t tileValue = board.getTile(row, col);
            if (tileValue > maxTile) {
                maxTile = tileValue;
            }
        }
    }

    // 检查水平方向的合并可能性
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int value     = board.getTile(row, col);
            int nextValue = board.getTile(row, col + 1);

            if (value > 0 && value == nextValue) {
                // 相邻相同值方块，增加合并分数
                // 值越大的方块合并，得分越高
                float logValue = static_cast<float>(std::log2(value));

                // 如果是大数字，给予更高的奖励
                if (value >= maxTile / 2) {
                    mergeScore += logValue * 2.5F;  // 大数字合并给予更高奖励
                } else {
                    mergeScore += logValue * 1.5F;  // 普通合并
                }
            }
        }
    }

    // 检查垂直方向的合并可能性
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 3; ++row) {
            int value     = board.getTile(row, col);
            int nextValue = board.getTile(row + 1, col);

            if (value > 0 && value == nextValue) {
                // 相邻相同值方块，增加合并分数
                float logValue = static_cast<float>(std::log2(value));

                // 如果是大数字，给予更高的奖励
                if (value >= maxTile / 2) {
                    mergeScore += logValue * 2.5F;  // 大数字合并给予更高奖励
                } else {
                    mergeScore += logValue * 1.5F;  // 普通合并
                }
            }
        }
    }

    return mergeScore;
}
