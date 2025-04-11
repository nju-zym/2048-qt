#include "bitboard_tables.h"

// 常量定义
const uint64_t ROW_MASK = 0xFFFFULL;
const uint64_t COL_MASK = 0x000F000F000F000FULL;

// 预计算的移动表 - 这里只包含一些示例值，实际使用时需要生成完整的表
uint16_t row_left_table[65536] = {0};
uint16_t row_right_table[65536] = {0};
uint64_t col_up_table[65536] = {0};
uint64_t col_down_table[65536] = {0};

// 预计算的评分表 - 这里只包含一些示例值，实际使用时需要生成完整的表
float heur_score_table[65536] = {0};
float score_table[65536] = {0};
