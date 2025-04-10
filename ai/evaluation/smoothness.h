#ifndef SMOOTHNESS_H
#define SMOOTHNESS_H

#include "../bitboard.h"

/**
 * @brief 平滑性评估函数命名空间
 *
 * 平滑性是指相邻方块值的相似程度。
 * 在2048游戏中，保持棋盘的平滑性能够增加相邻方块合并的可能性，
 * 避免小值方块被孤立，导致无法有效利用空间。
 */
namespace Smoothness {

/**
 * @brief 计算棋盘的平滑性得分
 *
 * 评估棋盘上相邻方块值的差异程度。
 * 相邻方块值越接近，平滑性越高，合并可能性越大。
 *
 * @param board BitBoard棋盘
 * @return 平滑性得分，通常为负值，越接近0表示平滑性越好
 */
float calculateSmoothness(BitBoard board);

/**
 * @brief 计算水平方向的平滑性
 * @param board BitBoard棋盘
 * @return 水平平滑性得分
 */
float calculateHorizontalSmoothness(BitBoard board);

/**
 * @brief 计算垂直方向的平滑性
 * @param board BitBoard棋盘
 * @return 垂直平滑性得分
 */
float calculateVerticalSmoothness(BitBoard board);

/**
 * @brief 计算相邻两个方块值的平滑性得分
 * @param value1 第一个方块值
 * @param value2 第二个方块值
 * @return 两个方块之间的平滑性得分
 */
float calculatePairSmoothness(int value1, int value2);

}  // namespace Smoothness

#endif  // SMOOTHNESS_H