#include <iostream>
#include <fstream>
#include <array>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <iomanip>

// 移动方向枚举
enum Direction {
    LEFT = 0,
    RIGHT = 1
};

// 计算一行的移动结果
uint16_t moveRow(uint16_t row, Direction direction) {
    uint8_t tiles[4] = {
        static_cast<uint8_t>((row >> 0) & 0xF),
        static_cast<uint8_t>((row >> 4) & 0xF),
        static_cast<uint8_t>((row >> 8) & 0xF),
        static_cast<uint8_t>((row >> 12) & 0xF)
    };

    // 如果是向右移动，反转数组
    if (direction == RIGHT) {
        std::swap(tiles[0], tiles[3]);
        std::swap(tiles[1], tiles[2]);
    }

    // 移除零并合并相同的数字
    int idx = 0;
    for (int i = 0; i < 4; ++i) {
        if (tiles[i] != 0) {
            tiles[idx++] = tiles[i];
        }
    }

    // 清空剩余位置
    while (idx < 4) {
        tiles[idx++] = 0;
    }

    // 合并相同的数字
    for (int i = 0; i < 3; ++i) {
        if (tiles[i] != 0 && tiles[i] == tiles[i + 1]) {
            tiles[i]++;  // 增加对数值（例如，1->2表示2->4）
            tiles[i + 1] = 0;
        }
    }

    // 再次移除零
    idx = 0;
    for (int i = 0; i < 4; ++i) {
        if (tiles[i] != 0) {
            tiles[idx++] = tiles[i];
        }
    }

    // 清空剩余位置
    while (idx < 4) {
        tiles[idx++] = 0;
    }

    // 如果是向右移动，再次反转数组
    if (direction == RIGHT) {
        std::swap(tiles[0], tiles[3]);
        std::swap(tiles[1], tiles[2]);
    }

    // 将4个4位数字合并回16位行
    return static_cast<uint16_t>(tiles[0]) | (static_cast<uint16_t>(tiles[1]) << 4)
           | (static_cast<uint16_t>(tiles[2]) << 8) | (static_cast<uint16_t>(tiles[3]) << 12);
}

// 计算移动产生的分数
uint16_t calculateScore(uint16_t before, uint16_t after) {
    uint16_t score = 0;

    // 提取before和after中的每个4位数字
    uint8_t beforeTiles[4] = {
        static_cast<uint8_t>((before >> 0) & 0xF),
        static_cast<uint8_t>((before >> 4) & 0xF),
        static_cast<uint8_t>((before >> 8) & 0xF),
        static_cast<uint8_t>((before >> 12) & 0xF)
    };

    uint8_t afterTiles[4] = {
        static_cast<uint8_t>((after >> 0) & 0xF),
        static_cast<uint8_t>((after >> 4) & 0xF),
        static_cast<uint8_t>((after >> 8) & 0xF),
        static_cast<uint8_t>((after >> 12) & 0xF)
    };

    // 计算合并产生的分数
    for (int i = 0; i < 4; ++i) {
        if (afterTiles[i] > 0 && afterTiles[i] > beforeTiles[i]) {
            // 如果after中的数字大于before中的数字，说明发生了合并
            // 分数是合并后的数字的实际值
            score += (1 << afterTiles[i]);
        }
    }

    return score;
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::cout << "开始生成查找表..." << std::endl;
    
    // 创建查找表
    std::array<uint16_t, 65536> leftTable;
    std::array<uint16_t, 65536> rightTable;
    std::array<uint16_t, 65536> scoreTable;
    
    // 计算所有可能的行状态的移动结果
    for (uint32_t row = 0; row < 65536; ++row) {
        // 计算向左移动的结果
        uint16_t leftResult = moveRow(static_cast<uint16_t>(row), LEFT);
        leftTable[row] = leftResult;
        scoreTable[row] = calculateScore(static_cast<uint16_t>(row), leftResult);
        
        // 计算向右移动的结果
        uint16_t rightResult = moveRow(static_cast<uint16_t>(row), RIGHT);
        rightTable[row] = rightResult;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    
    std::cout << "查找表生成完成，耗时: " << elapsed.count() << " 秒" << std::endl;
    
    // 打开输出文件
    std::ofstream out("../ai/BitBoardTables.cpp");
    
    if (!out) {
        std::cerr << "无法创建输出文件: ../ai/BitBoardTables.cpp" << std::endl;
        return 1;
    }
    
    // 写入文件头
    out << "// 自动生成的BitBoard查找表\n";
    out << "// 请勿手动修改此文件\n\n";
    out << "#include \"BitBoardTables.h\"\n\n";
    out << "namespace BitBoardTables {\n\n";
    
    // 写入左移动表
    out << "// 向左移动表\n";
    out << "const std::array<uint16_t, 65536> LEFT_MOVE_TABLE = {\n";
    for (size_t i = 0; i < leftTable.size(); ++i) {
        if (i % 8 == 0) {
            out << "    ";
        }
        out << "0x" << std::hex << std::setw(4) << std::setfill('0') << leftTable[i];
        if (i != leftTable.size() - 1) {
            out << ", ";
        }
        if (i % 8 == 7 || i == leftTable.size() - 1) {
            out << "\n";
        }
    }
    out << "};\n\n";
    
    // 写入右移动表
    out << "// 向右移动表\n";
    out << "const std::array<uint16_t, 65536> RIGHT_MOVE_TABLE = {\n";
    for (size_t i = 0; i < rightTable.size(); ++i) {
        if (i % 8 == 0) {
            out << "    ";
        }
        out << "0x" << std::hex << std::setw(4) << std::setfill('0') << rightTable[i];
        if (i != rightTable.size() - 1) {
            out << ", ";
        }
        if (i % 8 == 7 || i == rightTable.size() - 1) {
            out << "\n";
        }
    }
    out << "};\n\n";
    
    // 写入分数表
    out << "// 分数表\n";
    out << "const std::array<uint16_t, 65536> SCORE_TABLE = {\n";
    for (size_t i = 0; i < scoreTable.size(); ++i) {
        if (i % 8 == 0) {
            out << "    ";
        }
        out << "0x" << std::hex << std::setw(4) << std::setfill('0') << scoreTable[i];
        if (i != scoreTable.size() - 1) {
            out << ", ";
        }
        if (i % 8 == 7 || i == scoreTable.size() - 1) {
            out << "\n";
        }
    }
    out << "};\n\n";
    
    // 写入文件尾
    out << "} // namespace BitBoardTables\n";
    
    out.close();
    
    std::cout << "查找表已保存到文件: ../ai/BitBoardTables.cpp" << std::endl;
    
    return 0;
}
