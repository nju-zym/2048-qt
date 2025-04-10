#ifndef SIMDBITBOARD_H
#define SIMDBITBOARD_H

#include "BitBoard.h"

#ifdef __SSE2__
#include <emmintrin.h>  // SSE2
#endif

#ifdef __AVX2__
#include <immintrin.h>  // AVX2
#endif

/**
 * @brief 使用SIMD指令集优化的BitBoard类
 * 
 * 这个类扩展了BitBoard，使用SIMD指令集来加速计算
 */
class SIMDBitBoard : public BitBoard {
public:
    /**
     * @brief 默认构造函数
     */
    SIMDBitBoard();
    
    /**
     * @brief 从BitBoard构造
     * @param board BitBoard对象
     */
    explicit SIMDBitBoard(const BitBoard& board);
    
    /**
     * @brief 从GameBoard构造
     * @param board GameBoard对象
     */
    explicit SIMDBitBoard(const GameBoard& board);
    
    /**
     * @brief 从64位整数构造
     * @param board 64位整数表示的棋盘
     */
    explicit SIMDBitBoard(uint64_t board);
    
    /**
     * @brief 使用SIMD指令集优化的移动操作
     * @param direction 移动方向
     * @return 移动后的棋盘
     */
    SIMDBitBoard move(Direction direction) const;
    
    /**
     * @brief 检查是否支持SIMD指令集
     * @return 是否支持SIMD
     */
    static bool isSIMDSupported();
    
private:
    /**
     * @brief 使用SSE2指令集优化的向上移动
     * @return 移动后的棋盘
     */
    SIMDBitBoard moveUpSSE2() const;
    
    /**
     * @brief 使用SSE2指令集优化的向右移动
     * @return 移动后的棋盘
     */
    SIMDBitBoard moveRightSSE2() const;
    
    /**
     * @brief 使用SSE2指令集优化的向下移动
     * @return 移动后的棋盘
     */
    SIMDBitBoard moveDownSSE2() const;
    
    /**
     * @brief 使用SSE2指令集优化的向左移动
     * @return 移动后的棋盘
     */
    SIMDBitBoard moveLeftSSE2() const;
    
#ifdef __AVX2__
    /**
     * @brief 使用AVX2指令集优化的向上移动
     * @return 移动后的棋盘
     */
    SIMDBitBoard moveUpAVX2() const;
    
    /**
     * @brief 使用AVX2指令集优化的向右移动
     * @return 移动后的棋盘
     */
    SIMDBitBoard moveRightAVX2() const;
    
    /**
     * @brief 使用AVX2指令集优化的向下移动
     * @return 移动后的棋盘
     */
    SIMDBitBoard moveDownAVX2() const;
    
    /**
     * @brief 使用AVX2指令集优化的向左移动
     * @return 移动后的棋盘
     */
    SIMDBitBoard moveLeftAVX2() const;
#endif
};

#endif // SIMDBITBOARD_H
