#ifndef MERGE_H
#define MERGE_H

#include "../bitboard.h"

/**
 * @brief 合并机会评估函数命名空间
 *
 * 合并机会评估函数用于评估棋盘上相邻方块可能合并的机会。
 * 在2048游戏中，相邻相同值的方块可以合并，这是获得高分的关键。
 * 该评估函数鼓励创造更多的合并机会。
 */
namespace Merge {

/**
 * @brief 计算棋盘上的合并机会得分
 *
 * 评估棋盘上相邻方块可能合并的机会。
 * 对于每对相邻且值相同的方块，根据其值给予分数奖励。
 *
 * @param board BitBoard棋盘
 * @return 合并机会得分，越高表示合并机会越多
 */
float calculateMergeScore(BitBoard board);

/**
 * @brief 计算水平方向的合并机会
 * @param board BitBoard棋盘
 * @return 水平方向合并机会得分
 */
float calculateHorizontalMergeScore(BitBoard board);

/**
 * @brief 计算垂直方向的合并机会
 * @param board BitBoard棋盘
 * @return 垂直方向合并机会得分
 */
float calculateVerticalMergeScore(BitBoard board);

}  // namespace Merge

#endif  // MERGE_H
