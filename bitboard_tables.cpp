#include "bitboard_tables.h"

#include <cmath>
#include <cstdlib>

// 常量定义
uint64_t const ROW_MASK = 0xFF'FFULL;
uint64_t const COL_MASK = 0x00'0F'00'0F'00'0F'00'0FULL;

// 预计算的移动表
uint16_t row_left_table[65536];
uint16_t row_right_table[65536];
uint64_t col_up_table[65536];
uint64_t col_down_table[65536];

// 预计算的评分表
float heur_score_table[65536];
float score_table[65536];

// 生成预计算表
void generate_tables() {
    // 初始化移动和评分表
    for (unsigned row = 0; row < 65536; ++row) {
        unsigned line[4] = {(row >> 0) & 0xf, (row >> 4) & 0xf, (row >> 8) & 0xf, (row >> 12) & 0xf};

        // 计算分数
        float score = 0.0f;
        for (int i = 0; i < 4; ++i) {
            int rank = line[i];
            if (rank >= 2) {
                // 分数是方块值和所有中间合并方块的总和
                score += (rank - 1) * (1 << rank);
            }
        }
        score_table[row] = score;

        // 启发式评分
        float sum   = 0;
        int empty   = 0;
        int merges  = 0;
        int prev    = 0;
        int counter = 0;

        for (int i = 0; i < 4; ++i) {
            int rank  = line[i];
            sum      += pow(rank, 3.5);
            if (rank == 0) {
                empty++;
            } else {
                if (prev == rank) {
                    counter++;
                } else if (counter > 0) {
                    merges  += 1 + counter;
                    counter  = 0;
                }
                prev = rank;
            }
        }
        if (counter > 0) {
            merges += 1 + counter;
        }

        // 计算单调性
        int mono = 0;
        for (int i = 1; i < 4; ++i) {
            if (line[i - 1] > line[i]) {
                mono++;
            } else if (line[i - 1] < line[i]) {
                mono--;
            }
        }
        mono = std::abs(mono);

        // 计算平滑度
        int smoothness = 0;
        for (int i = 0; i < 3; ++i) {
            if (line[i] > 0 && line[i + 1] > 0) {
                smoothness -= std::abs(static_cast<int>(line[i]) - static_cast<int>(line[i + 1]));
            }
        }

        // 综合评分
        float heur_score = empty * 30.0f +      // 空格奖励
                           merges * 10.0f +     // 合并奖励
                           mono * 15.0f +       // 单调性奖励
                           smoothness * 5.0f +  // 平滑度奖励
                           sum;                 // 方块值总和

        heur_score_table[row] = heur_score;

        // 模拟左移
        uint16_t result      = 0;
        bool merged          = false;
        uint16_t new_line[4] = {0, 0, 0, 0};
        int count            = 0;

        // 先将所有非零元素移动到最左边
        for (int i = 0; i < 4; ++i) {
            if (line[i] != 0) {
                new_line[count++] = line[i];
            }
        }

        // 合并相邻相同的元素
        for (int i = 0; i < 3; ++i) {
            if (new_line[i] != 0 && new_line[i] == new_line[i + 1]) {
                new_line[i]++;
                new_line[i + 1] = 0;
                merged          = true;
            }
        }

        // 再次压缩
        if (merged) {
            count                 = 0;
            uint16_t temp_line[4] = {0, 0, 0, 0};
            for (int i = 0; i < 4; ++i) {
                if (new_line[i] != 0) {
                    temp_line[count++] = new_line[i];
                }
            }
            for (int i = 0; i < 4; ++i) {
                new_line[i] = temp_line[i];
            }
        }

        // 将结果打包回16位整数
        result = (new_line[0] << 0) | (new_line[1] << 4) | (new_line[2] << 8) | (new_line[3] << 12);

        uint16_t rev_result = reverse_row(result);
        unsigned rev_row    = reverse_row(row);

        row_left_table[row]      = row ^ result;
        row_right_table[rev_row] = rev_row ^ rev_result;
        col_up_table[row]        = unpack_col(row) ^ unpack_col(result);
        col_down_table[rev_row]  = unpack_col(rev_row) ^ unpack_col(rev_result);
    }
}
