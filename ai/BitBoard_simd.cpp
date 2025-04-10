#include "BitBoard.h"

// 使用SIMD指令优化的moveRow方法
// 注意：这需要支持SSE4.1指令集的CPU
#ifdef __SSE4_1__
#include <immintrin.h>

uint16_t BitBoard::moveRow(uint16_t row, Direction direction) {
    // 提取4个4位数字
    uint8_t tiles[4] = {
        static_cast<uint8_t>((row >> 0) & 0xF),
        static_cast<uint8_t>((row >> 4) & 0xF),
        static_cast<uint8_t>((row >> 8) & 0xF),
        static_cast<uint8_t>((row >> 12) & 0xF)
    };
    
    // 使用SSE加载4个字节
    __m128i v = _mm_loadu_si32(tiles);
    
    // 如果是向右移动，反转向量
    if (direction == RIGHT) {
        v = _mm_shuffle_epi8(v, _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 0));
    }
    
    // 移除零并合并相同的数字
    // 这部分比较复杂，需要多个SIMD操作
    // 这里简化为标量代码
    
    // 提取回标量值
    _mm_storeu_si32(tiles, v);
    
    // 移除零并合并相同的数字（标量代码）
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
            tiles[i]++;  // 增加对数值
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
    
    // 如果是向右移动，再次反转
    if (direction == RIGHT) {
        std::swap(tiles[0], tiles[3]);
        std::swap(tiles[1], tiles[2]);
    }
    
    // 将4个4位数字合并回16位行
    return static_cast<uint16_t>(tiles[0]) | (static_cast<uint16_t>(tiles[1]) << 4)
           | (static_cast<uint16_t>(tiles[2]) << 8) | (static_cast<uint16_t>(tiles[3]) << 12);
}

#endif // __SSE4_1__
