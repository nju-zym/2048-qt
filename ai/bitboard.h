#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>

/**
 * BitBoard是一个用于高效表示2048游戏棋盘的数据结构
 * 使用64位整数表示16个方格，每个方格用4位表示（0-15）
 *
 * 数值编码：0代表空白，1代表2，2代表4，3代表8...
 * 即每个方格的值 = 2^(编码值)
 */
typedef uint64_t BitBoard;

/**
 * @brief 从BitBoard获取指定位置的值
 * @param board BitBoard棋盘
 * @param row 行（0-3）
 * @param col 列（0-3）
 * @return 该位置的值编码（0-15）
 */
inline int getBitBoardValue(BitBoard board, int row, int col) {
    int shift = 4 * (4 * row + col);
    return (board >> shift) & 0xF;
}

/**
 * @brief 设置BitBoard指定位置的值
 * @param board BitBoard棋盘
 * @param row 行（0-3）
 * @param col 列（0-3）
 * @param value 要设置的值编码（0-15）
 * @return 设置后的BitBoard
 */
inline BitBoard setBitBoardValue(BitBoard board, int row, int col, int value) {
    int shift = 4 * (4 * row + col);
    // 清除该位置的原有值
    board &= ~(static_cast<BitBoard>(0xF) << shift);
    // 设置新值
    board |= static_cast<BitBoard>(value & 0xF) << shift;
    return board;
}

/**
 * @brief 统计BitBoard中的空格数量
 * @param board BitBoard棋盘
 * @return 空格数量
 */
int countEmptyTiles(BitBoard board);

/**
 * @brief 打印BitBoard的人类可读表示（用于调试）
 * @param board BitBoard棋盘
 */
void printBitBoard(BitBoard board);

#endif  // BITBOARD_H