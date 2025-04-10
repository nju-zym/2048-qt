#ifndef SNAKE_H
#define SNAKE_H

#include "../bitboard.h"

/**
 * @brief 蛇形策略评估函数命名空间
 *
 * 蛇形策略是一种特殊的棋盘排列策略，鼓励方块按照蛇形路径排列，
 * 通常按照从大到小的顺序，从一个角落开始，沿着蛇形路径排列方块。
 * 这种策略有助于保持棋盘的有序结构，提高合并效率。
 */
namespace Snake {

/**
 * @brief 计算棋盘的蛇形策略得分
 *
 * 评估棋盘上方块值按照蛇形路径排列的程度。
 * 典型的蛇形路径从右上角或左上角开始，沿着之字形路径向下排列。
 *
 * @param board BitBoard棋盘
 * @return 蛇形策略得分，越高表示排列越接近理想蛇形
 */
float calculateSnakePattern(BitBoard board);

/**
 * @brief 计算从右上角开始的蛇形模式得分
 * @param board BitBoard棋盘
 * @return 右上角蛇形模式得分
 */
float calculateTopRightSnake(BitBoard board);

/**
 * @brief 计算从左上角开始的蛇形模式得分
 * @param board BitBoard棋盘
 * @return 左上角蛇形模式得分
 */
float calculateTopLeftSnake(BitBoard board);

/**
 * @brief 计算从右下角开始的蛇形模式得分
 * @param board BitBoard棋盘
 * @return 右下角蛇形模式得分
 */
float calculateBottomRightSnake(BitBoard board);

/**
 * @brief 计算从左下角开始的蛇形模式得分
 * @param board BitBoard棋盘
 * @return 左下角蛇形模式得分
 */
float calculateBottomLeftSnake(BitBoard board);

/**
 * @brief 计算角落策略得分
 * @param board BitBoard棋盘
 * @return 角落策略得分
 */
float calculateCornerScore(BitBoard board);

}  // namespace Snake

#endif  // SNAKE_H