#include "bitboard.h"

#include <iostream>

int countEmptyTiles(BitBoard board) {
    int count = 0;
    for (int i = 0; i < 16; ++i) {
        int shift = 4 * i;
        int value = (board >> shift) & 0xF;
        if (value == 0) {
            count++;
        }
    }
    return count;
}

void printBitBoard(BitBoard board) {
    std::cout << "+------+------+------+------+" << std::endl;
    for (int i = 0; i < 4; ++i) {
        std::cout << "|";
        for (int j = 0; j < 4; ++j) {
            int value = getBitBoardValue(board, i, j);
            if (value == 0) {
                std::cout << "      |";
            } else {
                std::cout << " " << (1 << value) << " |";
            }
        }
        std::cout << std::endl << "+------+------+------+------+" << std::endl;
    }
}