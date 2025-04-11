#ifndef BITBOARD_TABLES_H
#define BITBOARD_TABLES_H

#include <cstdint>

// 位棋盘类型定义
typedef uint64_t BitBoard;

// 常量定义
extern const uint64_t ROW_MASK;
extern const uint64_t COL_MASK;

// 预计算的移动表
extern uint16_t row_left_table[65536];
extern uint16_t row_right_table[65536];
extern uint64_t col_up_table[65536];
extern uint64_t col_down_table[65536];

// 预计算的评分表
extern float heur_score_table[65536];
extern float score_table[65536];

// 转置棋盘
inline BitBoard transpose(BitBoard x) {
    BitBoard a1 = x & 0xF0F00F0FF0F00F0FULL;
    BitBoard a2 = x & 0x0000F0F00000F0F0ULL;
    BitBoard a3 = x & 0x0F0F00000F0F0000ULL;
    BitBoard a = a1 | (a2 << 12) | (a3 >> 12);
    BitBoard b1 = a & 0xFF00FF0000FF00FFULL;
    BitBoard b2 = a & 0x00FF00FF00000000ULL;
    BitBoard b3 = a & 0x00000000FF00FF00ULL;
    return b1 | (b2 >> 24) | (b3 << 24);
}

// 反转一行
inline uint16_t reverse_row(uint16_t row) {
    return (row >> 12) | ((row >> 4) & 0x00F0) | ((row << 4) & 0x0F00) | (row << 12);
}

// 将列解包为位棋盘
inline BitBoard unpack_col(uint16_t row) {
    BitBoard tmp = row;
    return (tmp | (tmp << 12ULL) | (tmp << 24ULL) | (tmp << 36ULL)) & COL_MASK;
}

// 执行向上移动
inline BitBoard execute_move_0(BitBoard board) {
    BitBoard ret = board;
    BitBoard t = transpose(board);
    ret ^= col_up_table[(t >> 0) & ROW_MASK] << 0;
    ret ^= col_up_table[(t >> 16) & ROW_MASK] << 4;
    ret ^= col_up_table[(t >> 32) & ROW_MASK] << 8;
    ret ^= col_up_table[(t >> 48) & ROW_MASK] << 12;
    return ret;
}

// 执行向下移动
inline BitBoard execute_move_1(BitBoard board) {
    BitBoard ret = board;
    BitBoard t = transpose(board);
    ret ^= col_down_table[(t >> 0) & ROW_MASK] << 0;
    ret ^= col_down_table[(t >> 16) & ROW_MASK] << 4;
    ret ^= col_down_table[(t >> 32) & ROW_MASK] << 8;
    ret ^= col_down_table[(t >> 48) & ROW_MASK] << 12;
    return ret;
}

// 执行向左移动
inline BitBoard execute_move_2(BitBoard board) {
    BitBoard ret = board;
    ret ^= BitBoard(row_left_table[(board >> 0) & ROW_MASK]) << 0;
    ret ^= BitBoard(row_left_table[(board >> 16) & ROW_MASK]) << 16;
    ret ^= BitBoard(row_left_table[(board >> 32) & ROW_MASK]) << 32;
    ret ^= BitBoard(row_left_table[(board >> 48) & ROW_MASK]) << 48;
    return ret;
}

// 执行向右移动
inline BitBoard execute_move_3(BitBoard board) {
    BitBoard ret = board;
    ret ^= BitBoard(row_right_table[(board >> 0) & ROW_MASK]) << 0;
    ret ^= BitBoard(row_right_table[(board >> 16) & ROW_MASK]) << 16;
    ret ^= BitBoard(row_right_table[(board >> 32) & ROW_MASK]) << 32;
    ret ^= BitBoard(row_right_table[(board >> 48) & ROW_MASK]) << 48;
    return ret;
}

// 初始化表格（仅用于生成预计算表）
void generate_tables();

#endif // BITBOARD_TABLES_H
