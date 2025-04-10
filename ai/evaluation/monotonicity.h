#ifndef MONOTONICITY_H
#define MONOTONICITY_H

#include "../bitboard.h"

/**
 * @brief 单调性评估函数命名空间
 *
 * 单调性是指数值在某个方向上保持递增或递减的程度。
 * 在2048游戏中，保持棋盘的单调性有助于维持良好的棋盘排列，
 * 使得高值方块集中在一个区域，增加合并机会。
 */
namespace Monotonicity {

/**
 * @brief 计算棋盘的单调性得分
 *
 * 评估棋盘上方块值在水平和垂直方向上的单调递增/递减的程度。
 * 良好的单调性意味着数值按一定方向有序排列，有助于维持棋盘秩序。
 *
 * @param board BitBoard棋盘
 * @return 单调性得分，越高表示单调性越好
 */
float calculateMonotonicity(BitBoard board);

/**
 * @brief 计算从左到右的单调性
 * @param board BitBoard棋盘
 * @return 从左到右的单调性得分
 */
float calculateLeftRightMonotonicity(BitBoard board);

/**
 * @brief 计算从右到左的单调性
 * @param board BitBoard棋盘
 * @return 从右到左的单调性得分
 */
float calculateRightLeftMonotonicity(BitBoard board);

/**
 * @brief 计算从上到下的单调性
 * @param board BitBoard棋盘
 * @return 从上到下的单调性得分
 */
float calculateTopDownMonotonicity(BitBoard board);

/**
 * @brief 计算从下到上的单调性
 * @param board BitBoard棋盘
 * @return 从下到上的单调性得分
 */
float calculateBottomUpMonotonicity(BitBoard board);

/**
 * @brief 计算加权单调性
 * @param board BitBoard棋盘
 * @return 加权单调性得分
 */
float calculateWeightedMonotonicity(BitBoard board);

}  // namespace Monotonicity

#endif  // MONOTONICITY_H