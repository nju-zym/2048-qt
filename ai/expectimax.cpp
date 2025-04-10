#include "expectimax.h"

#include <algorithm>
#include <limits>

namespace Expectimax {

// BitBoard utility functions
static uint64_t const ROW_MASK   = 0xFF'FFULL;
static uint64_t const COL_MASK   = 0x00'0F'00'0F'00'0F'00'0FULL;
static float const PROBABILITY_2 = 0.9f;
static float const PROBABILITY_4 = 0.1f;

int getBestMove(BitBoard board, int depth, int maxDepth, float& score) {
    return expectimax(board, depth, maxDepth, true, score);
}

int expectimax(BitBoard board, int depth, int maxDepth, bool maximizingPlayer, float& score) {
    // 终止条件
    if (depth >= maxDepth) {
        // 到达搜索深度限制，评估棋盘状态
        // 这里需要实际的棋盘评估函数，暂时返回0
        score = 0;
        return -1;
    }

    if (maximizingPlayer) {
        // 玩家回合 - 选择最佳移动
        float bestScore   = -std::numeric_limits<float>::infinity();
        int bestDirection = -1;

        for (int direction = 0; direction < 4; ++direction) {
            if (canMove(board, direction)) {
                BitBoard newBoard  = makeMove(board, direction);
                float currentScore = 0.0f;
                expectimax(newBoard, depth + 1, maxDepth, false, currentScore);

                if (currentScore > bestScore) {
                    bestScore     = currentScore;
                    bestDirection = direction;
                }
            }
        }

        score = bestScore;
        return bestDirection;
    } else {
        // 计算机回合 - 平均所有可能的方块放置
        auto possibleNewTiles = getPossibleNewTiles(board);
        if (possibleNewTiles.empty()) {
            score = 0;  // 没有空位，评估为0
            return -1;
        }

        float expectedScore = 0.0f;
        for (auto const& [newBoard, probability] : possibleNewTiles) {
            float currentScore = 0.0f;
            expectimax(newBoard, depth + 1, maxDepth, true, currentScore);
            expectedScore += probability * currentScore;
        }

        score = expectedScore;
        return -1;
    }
}

std::vector<std::pair<BitBoard, float>> getPossibleNewTiles(BitBoard board) {
    std::vector<std::pair<BitBoard, float>> result;

    // 找到所有空格
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int shift = 4 * (4 * i + j);
            int value = (board >> shift) & 0xF;

            if (value == 0) {
                // 这个位置为空，可以添加新方块
                BitBoard newBoard2 = board | (static_cast<BitBoard>(1) << shift);  // 添加值为2的方块 (2^1)
                BitBoard newBoard4 = board | (static_cast<BitBoard>(2) << shift);  // 添加值为4的方块 (2^2)

                result.push_back(std::make_pair(newBoard2, PROBABILITY_2));  // 90%几率出现2
                result.push_back(std::make_pair(newBoard4, PROBABILITY_4));  // 10%几率出现4
            }
        }
    }

    return result;
}

bool canMove(BitBoard board, int direction) {
    // 检查是否可以向指定方向移动
    return board != makeMove(board, direction);
}

// 处理一行或一列的方块移动和合并
BitBoard processTiles(BitBoard row) {
    BitBoard result = 0;
    int position    = 0;
    int prev_tile   = 0;

    // 移动所有非零方块并合并相同值
    for (int i = 0; i < 4; ++i) {
        int current = (row >> (4 * i)) & 0xF;
        if (current == 0) {
            continue;
        }

        if (prev_tile == 0) {
            // 第一个方块或合并后
            prev_tile = current;
        } else if (prev_tile == current) {
            // 合并相同值的方块
            result |= static_cast<BitBoard>(prev_tile + 1) << (4 * position);
            position++;
            prev_tile = 0;
        } else {
            // 不同值，无法合并
            result |= static_cast<BitBoard>(prev_tile) << (4 * position);
            position++;
            prev_tile = current;
        }
    }

    // 处理最后一个方块
    if (prev_tile != 0) {
        result |= static_cast<BitBoard>(prev_tile) << (4 * position);
    }

    return result;
}

// 翻转一行或一列
BitBoard reverseRow(BitBoard row) {
    BitBoard result = 0;
    for (int i = 0; i < 4; ++i) {
        int value  = (row >> (4 * i)) & 0xF;
        result    |= static_cast<BitBoard>(value) << (4 * (3 - i));
    }
    return result;
}

BitBoard makeMove(BitBoard board, int direction) {
    BitBoard newBoard = board;
    BitBoard row;

    // 根据方向处理每一行或列
    switch (direction) {
        case 0:  // 上
            for (int col = 0; col < 4; ++col) {
                // 提取列
                row = 0;
                for (int i = 0; i < 4; ++i) {
                    int value  = (board >> (4 * (4 * i + col))) & 0xF;
                    row       |= static_cast<BitBoard>(value) << (4 * i);
                }

                // 处理列
                row = processTiles(row);

                // 放回处理后的列
                for (int i = 0; i < 4; ++i) {
                    // 清除新棋盘上该位置的值
                    newBoard &= ~(static_cast<BitBoard>(0xF) << (4 * (4 * i + col)));
                    // 设置新值
                    int value  = (row >> (4 * i)) & 0xF;
                    newBoard  |= static_cast<BitBoard>(value) << (4 * (4 * i + col));
                }
            }
            break;

        case 1:  // 右
            for (int row_idx = 0; row_idx < 4; ++row_idx) {
                // 提取行
                row = (board >> (16 * row_idx)) & 0xFF'FF;

                // 处理行（因为是向右，先翻转，处理，再翻转回来）
                row = reverseRow(row);
                row = processTiles(row);
                row = reverseRow(row);

                // 放回处理后的行
                newBoard &= ~(static_cast<BitBoard>(0xFF'FF) << (16 * row_idx));
                newBoard |= row << (16 * row_idx);
            }
            break;

        case 2:  // 下
            for (int col = 0; col < 4; ++col) {
                // 提取列
                row = 0;
                for (int i = 0; i < 4; ++i) {
                    int value  = (board >> (4 * (4 * i + col))) & 0xF;
                    row       |= static_cast<BitBoard>(value) << (4 * i);
                }

                // 处理列（因为是向下，先翻转，处理，再翻转回来）
                row = reverseRow(row);
                row = processTiles(row);
                row = reverseRow(row);

                // 放回处理后的列
                for (int i = 0; i < 4; ++i) {
                    // 清除新棋盘上该位置的值
                    newBoard &= ~(static_cast<BitBoard>(0xF) << (4 * (4 * i + col)));
                    // 设置新值
                    int value  = (row >> (4 * i)) & 0xF;
                    newBoard  |= static_cast<BitBoard>(value) << (4 * (4 * i + col));
                }
            }
            break;

        case 3:  // 左
            for (int row_idx = 0; row_idx < 4; ++row_idx) {
                // 提取行
                row = (board >> (16 * row_idx)) & 0xFF'FF;

                // 处理行
                row = processTiles(row);

                // 放回处理后的行
                newBoard &= ~(static_cast<BitBoard>(0xFF'FF) << (16 * row_idx));
                newBoard |= row << (16 * row_idx);
            }
            break;
    }

    return newBoard;
}

}  // namespace Expectimax