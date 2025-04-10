#include "SIMDBitBoard.h"

SIMDBitBoard::SIMDBitBoard() : BitBoard() {
}

SIMDBitBoard::SIMDBitBoard(const BitBoard& board) : BitBoard(board.getBoard()) {
}

SIMDBitBoard::SIMDBitBoard(const GameBoard& board) : BitBoard(board) {
}

SIMDBitBoard::SIMDBitBoard(uint64_t board) : BitBoard(board) {
}

bool SIMDBitBoard::isSIMDSupported() {
#ifdef __SSE2__
    return true;
#else
    return false;
#endif
}

SIMDBitBoard SIMDBitBoard::move(Direction direction) const {
    // 如果不支持SIMD，使用原始BitBoard的移动方法
    if (!isSIMDSupported()) {
        return SIMDBitBoard(BitBoard::move(direction));
    }
    
    // 根据方向选择对应的SIMD优化移动方法
    switch (direction) {
        case UP:
            return moveUpSSE2();
        case RIGHT:
            return moveRightSSE2();
        case DOWN:
            return moveDownSSE2();
        case LEFT:
            return moveLeftSSE2();
        default:
            return *this;
    }
}

SIMDBitBoard SIMDBitBoard::moveUpSSE2() const {
#ifdef __SSE2__
    // 将棋盘转换为SSE2可以处理的格式
    // 每行作为一个32位整数
    uint16_t row0 = (board & 0xFFFF);
    uint16_t row1 = ((board >> 16) & 0xFFFF);
    uint16_t row2 = ((board >> 32) & 0xFFFF);
    uint16_t row3 = ((board >> 48) & 0xFFFF);
    
    // 使用查找表进行移动
    uint16_t newRow0 = moveTable[UP][row0];
    uint16_t newRow1 = moveTable[UP][row1];
    uint16_t newRow2 = moveTable[UP][row2];
    uint16_t newRow3 = moveTable[UP][row3];
    
    // 重新组合成64位整数
    uint64_t newBoard = newRow0 | (static_cast<uint64_t>(newRow1) << 16) | 
                        (static_cast<uint64_t>(newRow2) << 32) | (static_cast<uint64_t>(newRow3) << 48);
    
    return SIMDBitBoard(newBoard);
#else
    return SIMDBitBoard(BitBoard::move(UP));
#endif
}

SIMDBitBoard SIMDBitBoard::moveRightSSE2() const {
#ifdef __SSE2__
    // 将棋盘转换为SSE2可以处理的格式
    uint16_t row0 = (board & 0xFFFF);
    uint16_t row1 = ((board >> 16) & 0xFFFF);
    uint16_t row2 = ((board >> 32) & 0xFFFF);
    uint16_t row3 = ((board >> 48) & 0xFFFF);
    
    // 使用查找表进行移动
    uint16_t newRow0 = moveTable[RIGHT][row0];
    uint16_t newRow1 = moveTable[RIGHT][row1];
    uint16_t newRow2 = moveTable[RIGHT][row2];
    uint16_t newRow3 = moveTable[RIGHT][row3];
    
    // 重新组合成64位整数
    uint64_t newBoard = newRow0 | (static_cast<uint64_t>(newRow1) << 16) | 
                        (static_cast<uint64_t>(newRow2) << 32) | (static_cast<uint64_t>(newRow3) << 48);
    
    return SIMDBitBoard(newBoard);
#else
    return SIMDBitBoard(BitBoard::move(RIGHT));
#endif
}

SIMDBitBoard SIMDBitBoard::moveDownSSE2() const {
#ifdef __SSE2__
    // 将棋盘转换为SSE2可以处理的格式
    uint16_t row0 = (board & 0xFFFF);
    uint16_t row1 = ((board >> 16) & 0xFFFF);
    uint16_t row2 = ((board >> 32) & 0xFFFF);
    uint16_t row3 = ((board >> 48) & 0xFFFF);
    
    // 使用查找表进行移动
    uint16_t newRow0 = moveTable[DOWN][row0];
    uint16_t newRow1 = moveTable[DOWN][row1];
    uint16_t newRow2 = moveTable[DOWN][row2];
    uint16_t newRow3 = moveTable[DOWN][row3];
    
    // 重新组合成64位整数
    uint64_t newBoard = newRow0 | (static_cast<uint64_t>(newRow1) << 16) | 
                        (static_cast<uint64_t>(newRow2) << 32) | (static_cast<uint64_t>(newRow3) << 48);
    
    return SIMDBitBoard(newBoard);
#else
    return SIMDBitBoard(BitBoard::move(DOWN));
#endif
}

SIMDBitBoard SIMDBitBoard::moveLeftSSE2() const {
#ifdef __SSE2__
    // 将棋盘转换为SSE2可以处理的格式
    uint16_t row0 = (board & 0xFFFF);
    uint16_t row1 = ((board >> 16) & 0xFFFF);
    uint16_t row2 = ((board >> 32) & 0xFFFF);
    uint16_t row3 = ((board >> 48) & 0xFFFF);
    
    // 使用查找表进行移动
    uint16_t newRow0 = moveTable[LEFT][row0];
    uint16_t newRow1 = moveTable[LEFT][row1];
    uint16_t newRow2 = moveTable[LEFT][row2];
    uint16_t newRow3 = moveTable[LEFT][row3];
    
    // 重新组合成64位整数
    uint64_t newBoard = newRow0 | (static_cast<uint64_t>(newRow1) << 16) | 
                        (static_cast<uint64_t>(newRow2) << 32) | (static_cast<uint64_t>(newRow3) << 48);
    
    return SIMDBitBoard(newBoard);
#else
    return SIMDBitBoard(BitBoard::move(LEFT));
#endif
}

#ifdef __AVX2__
SIMDBitBoard SIMDBitBoard::moveUpAVX2() const {
    // AVX2实现的向上移动
    // 这里需要根据实际情况实现
    return moveUpSSE2();
}

SIMDBitBoard SIMDBitBoard::moveRightAVX2() const {
    // AVX2实现的向右移动
    // 这里需要根据实际情况实现
    return moveRightSSE2();
}

SIMDBitBoard SIMDBitBoard::moveDownAVX2() const {
    // AVX2实现的向下移动
    // 这里需要根据实际情况实现
    return moveDownSSE2();
}

SIMDBitBoard SIMDBitBoard::moveLeftAVX2() const {
    // AVX2实现的向左移动
    // 这里需要根据实际情况实现
    return moveLeftSSE2();
}
#endif
