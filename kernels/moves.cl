// BitBoard representation for 2048 game
// Each tile is represented by 4 bits, so a 4x4 board fits in a 64-bit integer

// Constants
#define BOARD_SIZE 4
#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

// Get the value of a tile at a specific position
int getTileValue(ulong board, int row, int col) {
    int shift = ((row * 4 + col) * 4);
    int value = (int)((board >> shift) & 0xF);
    return value;
}

// Set the value of a tile at a specific position
void setTileValue(ulong* board, int row, int col, int value) {
    int shift = ((row * 4 + col) * 4);
    // Clear the current value
    *board &= ~((ulong)0xF << shift);
    // Set the new value
    *board |= ((ulong)value << shift);
}

// Move a line (row or column) and merge tiles
bool moveLine(int line[4]) {
    bool moved = false;
    
    // Move non-zero elements to the front
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (line[i] == 0 && line[j] != 0) {
                line[i] = line[j];
                line[j] = 0;
                moved = true;
                j = i; // Recheck current position
            }
        }
    }
    
    // Merge identical elements
    for (int i = 0; i < 3; i++) {
        if (line[i] != 0 && line[i] == line[i + 1]) {
            line[i] += 1; // Increment by 1 since we store log2 values
            line[i + 1] = 0;
            moved = true;
        }
    }
    
    // Move non-zero elements to the front again
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (line[i] == 0 && line[j] != 0) {
                line[i] = line[j];
                line[j] = 0;
                moved = true;
                j = i; // Recheck current position
            }
        }
    }
    
    return moved;
}

// Make a move in the specified direction
ulong makeMove(ulong board, int direction) {
    ulong newBoard = board;
    bool moved = false;
    
    switch (direction) {
        case UP: // Up
            for (int j = 0; j < 4; j++) {
                // Extract column
                int col[4] = {
                    getTileValue(newBoard, 0, j),
                    getTileValue(newBoard, 1, j),
                    getTileValue(newBoard, 2, j),
                    getTileValue(newBoard, 3, j)
                };
                
                // Move and merge
                bool colMoved = moveLine(col);
                
                // Update board
                if (colMoved) {
                    moved = true;
                    for (int i = 0; i < 4; i++) {
                        setTileValue(&newBoard, i, j, col[i]);
                    }
                }
            }
            break;
            
        case RIGHT: // Right
            for (int i = 0; i < 4; i++) {
                // Extract row
                int row[4] = {
                    getTileValue(newBoard, i, 0),
                    getTileValue(newBoard, i, 1),
                    getTileValue(newBoard, i, 2),
                    getTileValue(newBoard, i, 3)
                };
                
                // Reverse row to use same move logic
                int temp;
                temp = row[0]; row[0] = row[3]; row[3] = temp;
                temp = row[1]; row[1] = row[2]; row[2] = temp;
                
                // Move and merge
                bool rowMoved = moveLine(row);
                
                // Reverse back
                temp = row[0]; row[0] = row[3]; row[3] = temp;
                temp = row[1]; row[1] = row[2]; row[2] = temp;
                
                // Update board
                if (rowMoved) {
                    moved = true;
                    for (int j = 0; j < 4; j++) {
                        setTileValue(&newBoard, i, j, row[j]);
                    }
                }
            }
            break;
            
        case DOWN: // Down
            for (int j = 0; j < 4; j++) {
                // Extract column
                int col[4] = {
                    getTileValue(newBoard, 0, j),
                    getTileValue(newBoard, 1, j),
                    getTileValue(newBoard, 2, j),
                    getTileValue(newBoard, 3, j)
                };
                
                // Reverse column to use same move logic
                int temp;
                temp = col[0]; col[0] = col[3]; col[3] = temp;
                temp = col[1]; col[1] = col[2]; col[2] = temp;
                
                // Move and merge
                bool colMoved = moveLine(col);
                
                // Reverse back
                temp = col[0]; col[0] = col[3]; col[3] = temp;
                temp = col[1]; col[1] = col[2]; col[2] = temp;
                
                // Update board
                if (colMoved) {
                    moved = true;
                    for (int i = 0; i < 4; i++) {
                        setTileValue(&newBoard, i, j, col[i]);
                    }
                }
            }
            break;
            
        case LEFT: // Left
            for (int i = 0; i < 4; i++) {
                // Extract row
                int row[4] = {
                    getTileValue(newBoard, i, 0),
                    getTileValue(newBoard, i, 1),
                    getTileValue(newBoard, i, 2),
                    getTileValue(newBoard, i, 3)
                };
                
                // Move and merge
                bool rowMoved = moveLine(row);
                
                // Update board
                if (rowMoved) {
                    moved = true;
                    for (int j = 0; j < 4; j++) {
                        setTileValue(&newBoard, i, j, row[j]);
                    }
                }
            }
            break;
    }
    
    return moved ? newBoard : board;
}

// Kernel to make moves for multiple boards in parallel
__kernel void makeMoves(
    __global const ulong* boards,
    __global ulong* newBoards,
    __global char* moveResults,
    const int direction,
    const int boardCount
) {
    int idx = get_global_id(0);
    
    if (idx < boardCount) {
        ulong board = boards[idx];
        ulong newBoard = makeMove(board, direction);
        
        newBoards[idx] = newBoard;
        moveResults[idx] = (newBoard != board) ? 1 : 0;
    }
}
