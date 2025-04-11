#include "bitboard_tables.h"
#include <fstream>
#include <iostream>
#include <string>

int main() {
    // 生成预计算表
    generate_tables();
    
    // 将表保存到文件
    std::ofstream outFile("bitboard_tables_data.cpp");
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for writing" << std::endl;
        return 1;
    }
    
    // 写入文件头
    outFile << "#include \"bitboard_tables.h\"\n\n";
    outFile << "// 常量定义\n";
    outFile << "const uint64_t ROW_MASK = 0xFFFFULL;\n";
    outFile << "const uint64_t COL_MASK = 0x000F000F000F000FULL;\n\n";
    
    // 写入移动表
    outFile << "// 预计算的移动表\n";
    
    // row_left_table
    outFile << "uint16_t row_left_table[65536] = {\n    ";
    for (int i = 0; i < 65536; ++i) {
        outFile << "0x" << std::hex << row_left_table[i];
        if (i < 65535) {
            outFile << ", ";
            if ((i + 1) % 8 == 0) {
                outFile << "\n    ";
            }
        }
    }
    outFile << "\n};\n\n";
    
    // row_right_table
    outFile << "uint16_t row_right_table[65536] = {\n    ";
    for (int i = 0; i < 65536; ++i) {
        outFile << "0x" << std::hex << row_right_table[i];
        if (i < 65535) {
            outFile << ", ";
            if ((i + 1) % 8 == 0) {
                outFile << "\n    ";
            }
        }
    }
    outFile << "\n};\n\n";
    
    // col_up_table
    outFile << "uint64_t col_up_table[65536] = {\n    ";
    for (int i = 0; i < 65536; ++i) {
        outFile << "0x" << std::hex << col_up_table[i] << "ULL";
        if (i < 65535) {
            outFile << ", ";
            if ((i + 1) % 4 == 0) {
                outFile << "\n    ";
            }
        }
    }
    outFile << "\n};\n\n";
    
    // col_down_table
    outFile << "uint64_t col_down_table[65536] = {\n    ";
    for (int i = 0; i < 65536; ++i) {
        outFile << "0x" << std::hex << col_down_table[i] << "ULL";
        if (i < 65535) {
            outFile << ", ";
            if ((i + 1) % 4 == 0) {
                outFile << "\n    ";
            }
        }
    }
    outFile << "\n};\n\n";
    
    // 写入评分表
    outFile << "// 预计算的评分表\n";
    
    // heur_score_table
    outFile << "float heur_score_table[65536] = {\n    ";
    for (int i = 0; i < 65536; ++i) {
        outFile << std::fixed << heur_score_table[i] << "f";
        if (i < 65535) {
            outFile << ", ";
            if ((i + 1) % 8 == 0) {
                outFile << "\n    ";
            }
        }
    }
    outFile << "\n};\n\n";
    
    // score_table
    outFile << "float score_table[65536] = {\n    ";
    for (int i = 0; i < 65536; ++i) {
        outFile << std::fixed << score_table[i] << "f";
        if (i < 65535) {
            outFile << ", ";
            if ((i + 1) % 8 == 0) {
                outFile << "\n    ";
            }
        }
    }
    outFile << "\n};\n";
    
    outFile.close();
    
    std::cout << "Tables generated and saved to bitboard_tables_data.cpp" << std::endl;
    return 0;
}
