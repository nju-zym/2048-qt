#ifndef BITBOARD_TABLES_H
#define BITBOARD_TABLES_H

#include <array>
#include <cstdint>

// 预编译的移动表和分数表
// 这些表是通过单独的程序生成的，然后直接编译到程序中
// 完全避免了运行时计算

namespace BitBoardTables {

// 向左移动表
extern const std::array<uint16_t, 65536> LEFT_MOVE_TABLE;

// 向右移动表
extern const std::array<uint16_t, 65536> RIGHT_MOVE_TABLE;

// 分数表
extern const std::array<uint16_t, 65536> SCORE_TABLE;

} // namespace BitBoardTables

#endif // BITBOARD_TABLES_H
