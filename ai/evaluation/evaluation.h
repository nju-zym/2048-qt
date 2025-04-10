#ifndef EVALUATION_H
#define EVALUATION_H

#include "../bitboard.h"

/**
 * @brief 棋盘评估函数命名空间，包含各种评估启发式方法
 */
namespace Evaluation {

/**
 * @brief 综合评估函数，结合多种启发式策略
 * @param board BitBoard棋盘
 * @param emptyWeight 空位权重
 * @param monoWeight 单调性权重
 * @param smoothWeight 平滑性权重
 * @param cornerWeight 角落策略权重
 * @param snakeWeight 蛇形路径权重
 * @param mergeWeight 合并机会权重
 * @param tileWeight 方块值权重
 * @param edgeWeight 边缘策略权重
 * @return 评估得分
 */
float evaluateBoard(BitBoard board,
                    float emptyWeight  = 2.7f,
                    float monoWeight   = 1.0f,
                    float smoothWeight = 0.1f,
                    float cornerWeight = 2.0f,
                    float snakeWeight  = 4.0f,
                    float mergeWeight  = 1.0f,
                    float tileWeight   = 1.5f,
                    float edgeWeight   = 2.5f);

/**
 * @brief 计算棋盘上的空位得分
 * @param board BitBoard棋盘
 * @return 空位得分
 */
float calculateEmptyTilesScore(BitBoard board);

/**
 * @brief 计算角落潜力得分
 * @param board BitBoard棋盘
 * @return 角落潜力得分
 */
float calculateCornerPotential(BitBoard board);

/**
 * @brief 计算合并潜力得分
 * @param board BitBoard棋盘
 * @return 合并潜力得分
 */
float calculateMergePotential(BitBoard board);

/**
 * @brief 计算方块值得分
 * @param board BitBoard棋盘
 * @return 方块值得分
 */
float calculateTileValuesScore(BitBoard board);

}  // namespace Evaluation

#endif  // EVALUATION_H