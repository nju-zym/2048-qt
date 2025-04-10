#ifndef EXPECTIMAX_H
#define EXPECTIMAX_H

#include "bitboard.h"

#include <vector>

/**
 * @brief Expectimax策略相关常量和函数
 */
namespace Expectimax {

/**
 * @brief 执行Expectimax算法确定最佳移动
 * @param board 当前棋盘状态
 * @param depth 搜索深度
 * @param maxDepth 最大搜索深度
 * @param score 返回评估分数
 * @return 最佳移动方向(0:上, 1:右, 2:下, 3:左)，无效时返回-1
 */
int getBestMove(BitBoard board, int depth, int maxDepth, float& score);

/**
 * @brief 执行Expectimax算法
 * @param board 当前棋盘状态
 * @param depth 当前深度
 * @param maxDepth 最大搜索深度
 * @param maximizingPlayer 是否为最大化玩家回合
 * @param score 返回评估分数
 * @return 如果是最大化玩家回合，返回最佳移动方向；否则返回-1
 */
int expectimax(BitBoard board, int depth, int maxDepth, bool maximizingPlayer, float& score);

/**
 * @brief 获取所有可能的新方块添加位置和对应概率
 * @param board 当前棋盘状态
 * @return 可能的新棋盘状态和对应概率
 */
std::vector<std::pair<BitBoard, float>> getPossibleNewTiles(BitBoard board);

/**
 * @brief 检查特定方向的移动是否可行
 * @param board 当前棋盘状态
 * @param direction 移动方向
 * @return 是否可行
 */
bool canMove(BitBoard board, int direction);

/**
 * @brief 执行棋盘移动
 * @param board 当前棋盘状态
 * @param direction 移动方向
 * @return 移动后的棋盘状态
 */
BitBoard makeMove(BitBoard board, int direction);

}  // namespace Expectimax

#endif  // EXPECTIMAX_H