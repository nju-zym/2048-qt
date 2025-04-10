#ifndef TILE_H
#define TILE_H

#include "../bitboard.h"

/**
 * @brief 方块值评估函数命名空间
 *
 * 方块值评估函数用于评估棋盘上方块的价值。
 * 在2048游戏中，高值方块对得分贡献更大，该评估函数鼓励创造和保留高值方块。
 */
namespace Tile {

/**
 * @brief 计算棋盘上的加权方块得分
 *
 * 根据方块值给予非线性加权，促进合成更高的方块。
 * 使用指数函数提高高值方块的重要性，并对达到2048及以上的方块给予额外奖励。
 *
 * @param board BitBoard棋盘
 * @return 加权方块得分，越高表示棋盘上高值方块越多
 */
float calculateWeightedTileScore(BitBoard board);

/**
 * @brief 获取棋盘上的最大方块值（作为2的幂）
 * @param board BitBoard棋盘
 * @return 最大方块的幂值
 */
int getMaxTileValue(BitBoard board);

}  // namespace Tile

#endif  // TILE_H
