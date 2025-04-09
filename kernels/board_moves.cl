// OpenCL kernel for 2048 game board moves

// Helper function to get tile value from BitBoard
int getTileValue(ulong board, int row, int col) {
    int bitValue = (board >> ((row * 4 + col) * 4)) & 0xF;
    if (bitValue == 0) {
        return 0;
    }
    return 1 << bitValue;
}

// Helper function to set tile value in BitBoard
void setTileValue(ulong* board, int row, int col, int value) {
    // Clear the existing value
    *board &= ~((ulong)0xF << ((row * 4 + col) * 4));
    
    // Calculate bit value
    int bitValue = 0;
    if (value > 0) {
        bitValue = (int)log2((float)value);
    }
    
    // Set the new value
    *board |= (ulong)bitValue << ((row * 4 + col) * 4);
}

// Helper function to move a single line
bool moveLine(int line[4]) {
    bool moved = false;
    
    // First, remove zeros and compact the line
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            if (line[i] == 0 && line[j] != 0) {
                line[i] = line[j];
                line[j] = 0;
                moved = true;
                break;
            }
        }
    }
    
    // Then, merge adjacent identical tiles
    for (int i = 0; i < 3; ++i) {
        if (line[i] != 0 && line[i] == line[i + 1]) {
            line[i] *= 2;
            line[i + 1] = 0;
            moved = true;
        }
    }
    
    // Finally, compact again
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            if (line[i] == 0 && line[j] != 0) {
                line[i] = line[j];
                line[j] = 0;
                moved = true;
                break;
            }
        }
    }
    
    return moved;
}

// Make a move in the specified direction
// direction: 0=up, 1=right, 2=down, 3=left
__kernel void makeMove(
    __global const ulong* inputBoards,
    __global ulong* outputBoards,
    __global int* moveResults,  // 1 if moved, 0 if not moved
    const int direction,
    const int boardCount
) {
    int idx = get_global_id(0);
    if (idx >= boardCount) return;
    
    ulong board = inputBoards[idx];
    ulong newBoard = board;
    bool moved = false;
    
    if (direction == 0) {  // Up
        for (int col = 0; col < 4; ++col) {
            int line[4];
            for (int row = 0; row < 4; ++row) {
                line[row] = getTileValue(board, row, col);
            }
            
            if (moveLine(line)) {
                moved = true;
                for (int row = 0; row < 4; ++row) {
                    setTileValue(&newBoard, row, col, line[row]);
                }
            }
        }
    }
    else if (direction == 1) {  // Right
        for (int row = 0; row < 4; ++row) {
            int line[4];
            for (int col = 0; col < 4; ++col) {
                line[3 - col] = getTileValue(board, row, col);
            }
            
            if (moveLine(line)) {
                moved = true;
                for (int col = 0; col < 4; ++col) {
                    setTileValue(&newBoard, row, col, line[3 - col]);
                }
            }
        }
    }
    else if (direction == 2) {  // Down
        for (int col = 0; col < 4; ++col) {
            int line[4];
            for (int row = 0; row < 4; ++row) {
                line[3 - row] = getTileValue(board, row, col);
            }
            
            if (moveLine(line)) {
                moved = true;
                for (int row = 0; row < 4; ++row) {
                    setTileValue(&newBoard, row, col, line[3 - row]);
                }
            }
        }
    }
    else if (direction == 3) {  // Left
        for (int row = 0; row < 4; ++row) {
            int line[4];
            for (int col = 0; col < 4; ++col) {
                line[col] = getTileValue(board, row, col);
            }
            
            if (moveLine(line)) {
                moved = true;
                for (int col = 0; col < 4; ++col) {
                    setTileValue(&newBoard, row, col, line[col]);
                }
            }
        }
    }
    
    outputBoards[idx] = newBoard;
    moveResults[idx] = moved ? 1 : 0;
}
