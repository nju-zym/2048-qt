#include "BitBoard.h"
#include <array>
#include <cstring>

// 使用预计算的查找表优化moveRow方法
// 这种方法适用于所有平台，不需要SIMD支持

// 静态查找表，在程序启动时初始化
static std::array<uint16_t, 65536> moveLeftTable;
static std::array<uint16_t, 65536> moveRightTable;
static bool tablesInitialized = false;

// 初始化查找表
static void initializeLookupTables() {
    if (tablesInitialized) {
        return;
    }
    
    // 计算所有可能的行状态的移动结果
    for (uint16_t row = 0; row < 65536; ++row) {
        // 提取4个4位数字
        uint8_t tiles[4] = {
            static_cast<uint8_t>((row >> 0) & 0xF),
            static_cast<uint8_t>((row >> 4) & 0xF),
            static_cast<uint8_t>((row >> 8) & 0xF),
            static_cast<uint8_t>((row >> 12) & 0xF)
        };
        
        // 计算向左移动的结果
        uint8_t leftResult[4] = {0, 0, 0, 0};
        int idx = 0;
        
        // 移除零
        for (int i = 0; i < 4; ++i) {
            if (tiles[i] != 0) {
                leftResult[idx++] = tiles[i];
            }
        }
        
        // 合并相同的数字
        for (int i = 0; i < 3; ++i) {
            if (leftResult[i] != 0 && leftResult[i] == leftResult[i + 1]) {
                leftResult[i]++;
                leftResult[i + 1] = 0;
            }
        }
        
        // 再次移除零
        uint8_t finalLeft[4] = {0, 0, 0, 0};
        idx = 0;
        for (int i = 0; i < 4; ++i) {
            if (leftResult[i] != 0) {
                finalLeft[idx++] = leftResult[i];
            }
        }
        
        // 计算向右移动的结果
        uint8_t rightResult[4] = {0, 0, 0, 0};
        idx = 3;
        
        // 移除零（从右到左）
        for (int i = 3; i >= 0; --i) {
            if (tiles[i] != 0) {
                rightResult[idx--] = tiles[i];
            }
        }
        
        // 合并相同的数字（从右到左）
        for (int i = 3; i > 0; --i) {
            if (rightResult[i] != 0 && rightResult[i] == rightResult[i - 1]) {
                rightResult[i]++;
                rightResult[i - 1] = 0;
            }
        }
        
        // 再次移除零（从右到左）
        uint8_t finalRight[4] = {0, 0, 0, 0};
        idx = 3;
        for (int i = 3; i >= 0; --i) {
            if (rightResult[i] != 0) {
                finalRight[idx--] = rightResult[i];
            }
        }
        
        // 将结果存入查找表
        moveLeftTable[row] = static_cast<uint16_t>(finalLeft[0]) | (static_cast<uint16_t>(finalLeft[1]) << 4)
                           | (static_cast<uint16_t>(finalLeft[2]) << 8) | (static_cast<uint16_t>(finalLeft[3]) << 12);
        
        moveRightTable[row] = static_cast<uint16_t>(finalRight[0]) | (static_cast<uint16_t>(finalRight[1]) << 4)
                            | (static_cast<uint16_t>(finalRight[2]) << 8) | (static_cast<uint16_t>(finalRight[3]) << 12);
    }
    
    tablesInitialized = true;
}

// 优化版的moveRow方法，使用查找表
uint16_t BitBoard::moveRow(uint16_t row, Direction direction) {
    // 确保查找表已初始化
    if (!tablesInitialized) {
        initializeLookupTables();
    }
    
    // 直接从查找表中获取结果
    if (direction == LEFT) {
        return moveLeftTable[row];
    } else {
        return moveRightTable[row];
    }
}
