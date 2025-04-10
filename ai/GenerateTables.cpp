#include "BitBoard.h"
#include "BitBoardTables.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <QDebug>

// 生成查找表并保存到文件
void generateAndSaveTables() {
    auto start = std::chrono::high_resolution_clock::now();
    
    qDebug() << "开始生成BitBoard查找表...";
    
    // 创建查找表
    std::array<uint16_t, 65536> leftTable;
    std::array<uint16_t, 65536> rightTable;
    std::array<uint16_t, 65536> scoreTable;
    
    // 计算所有可能的行状态的移动结果
    for (uint32_t row = 0; row < 65536; ++row) {
        // 计算向左移动的结果
        uint16_t leftResult = BitBoard::moveRow(static_cast<uint16_t>(row), BitBoard::LEFT);
        leftTable[row] = leftResult;
        scoreTable[row] = BitBoard::calculateScore(static_cast<uint16_t>(row), leftResult);
        
        // 计算向右移动的结果
        uint16_t rightResult = BitBoard::moveRow(static_cast<uint16_t>(row), BitBoard::RIGHT);
        rightTable[row] = rightResult;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    
    qDebug() << "查找表生成完成，耗时:" << elapsed.count() << "秒";
    
    // 打开输出文件
    std::ofstream out("BitBoardTables.cpp");
    
    if (!out) {
        qDebug() << "无法创建输出文件: BitBoardTables.cpp";
        return;
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
    
    qDebug() << "查找表已保存到文件: BitBoardTables.cpp";
}
