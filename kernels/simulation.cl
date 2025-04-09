// OpenCL kernel for 2048 game simulation

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
ulong makeMove(ulong board, int direction, bool* moved) {
    ulong newBoard = board;
    *moved = false;
    
    if (direction == 0) {  // Up
        for (int col = 0; col < 4; ++col) {
            int line[4];
            for (int row = 0; row < 4; ++row) {
                line[row] = getTileValue(board, row, col);
            }
            
            if (moveLine(line)) {
                *moved = true;
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
                *moved = true;
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
                *moved = true;
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
                *moved = true;
                for (int col = 0; col < 4; ++col) {
                    setTileValue(&newBoard, row, col, line[col]);
                }
            }
        }
    }
    
    return newBoard;
}

// Check if game is over
bool isGameOver(ulong board) {
    // Check if there are empty tiles
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (getTileValue(board, i, j) == 0) {
                return false;
            }
        }
    }
    
    // Check if there are possible merges
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (getTileValue(board, i, j) == getTileValue(board, i, j + 1)) {
                return false;
            }
        }
    }
    
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (getTileValue(board, i, j) == getTileValue(board, i + 1, j)) {
                return false;
            }
        }
    }
    
    return true;
}

// Get maximum tile value
int getMaxTile(ulong board) {
    int maxTile = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            maxTile = max(maxTile, getTileValue(board, i, j));
        }
    }
    return maxTile;
}

// Calculate score
int getScore(ulong board) {
    int score = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = getTileValue(board, i, j);
            if (value > 0) {
                score += value * ((int)log2((float)value) - 1);
            }
        }
    }
    return score;
}

// Count empty tiles
int getEmptyTileCount(ulong board) {
    int count = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (getTileValue(board, i, j) == 0) {
                count++;
            }
        }
    }
    return count;
}

// Add a random tile (2 or 4) to the board
ulong addRandomTile(ulong board, uint* seed) {
    // Find all empty positions
    int emptyPositions[16][2];
    int emptyCount = 0;
    
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (getTileValue(board, i, j) == 0) {
                emptyPositions[emptyCount][0] = i;
                emptyPositions[emptyCount][1] = j;
                emptyCount++;
            }
        }
    }
    
    if (emptyCount == 0) {
        return board;
    }
    
    // Simple random number generator
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    int randomIndex = *seed % emptyCount;
    
    // 90% chance for 2, 10% chance for 4
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    int value = (*seed % 10 < 9) ? 2 : 4;
    
    int row = emptyPositions[randomIndex][0];
    int col = emptyPositions[randomIndex][1];
    
    ulong newBoard = board;
    setTileValue(&newBoard, row, col, value);
    
    return newBoard;
}

// Evaluate board using heuristics
float evaluateBoard(ulong board, __global const float* weights) {
    float score = 0.0f;
    
    // Empty tiles heuristic
    score += weights[0] * (float)getEmptyTileCount(board);
    
    // Monotonicity heuristic
    float monotonicityScore = 0.0f;
    
    // Horizontal monotonicity
    for (int i = 0; i < 4; ++i) {
        int leftToRight = 0;
        int rightToLeft = 0;
        
        for (int j = 0; j < 3; ++j) {
            int current = getTileValue(board, i, j);
            int next = getTileValue(board, i, j + 1);
            
            if (current != 0 && next != 0) {
                if (current > next) {
                    leftToRight += (int)log2((float)current / next);
                } else if (next > current) {
                    rightToLeft += (int)log2((float)next / current);
                }
            }
        }
        
        monotonicityScore -= min(leftToRight, rightToLeft);
    }
    
    // Vertical monotonicity
    for (int j = 0; j < 4; ++j) {
        int topToBottom = 0;
        int bottomToTop = 0;
        
        for (int i = 0; i < 3; ++i) {
            int current = getTileValue(board, i, j);
            int next = getTileValue(board, i + 1, j);
            
            if (current != 0 && next != 0) {
                if (current > next) {
                    topToBottom += (int)log2((float)current / next);
                } else if (next > current) {
                    bottomToTop += (int)log2((float)next / current);
                }
            }
        }
        
        monotonicityScore -= min(topToBottom, bottomToTop);
    }
    
    score += weights[1] * monotonicityScore;
    
    // Other heuristics would be added here...
    
    return score;
}

// Find best move using simple evaluation
int findBestMove(ulong board, __global const float* weights) {
    int bestMove = -1;
    float bestScore = -INFINITY;
    
    for (int dir = 0; dir < 4; ++dir) {
        bool moved = false;
        ulong newBoard = makeMove(board, dir, &moved);
        
        if (moved) {
            float moveScore = evaluateBoard(newBoard, weights);
            
            if (moveScore > bestScore) {
                bestScore = moveScore;
                bestMove = dir;
            }
        }
    }
    
    return bestMove;
}

// Simulate a game using the provided weights
__kernel void simulateGame(
    __global const float* weights,
    __global int* scores,
    __global int* maxTiles,
    __global int* movesCounts,
    const uint randomSeed,
    const int maxMoves,
    const int simulationCount
) {
    int idx = get_global_id(0);
    if (idx >= simulationCount) return;
    
    // Initialize random seed for this simulation
    uint seed = randomSeed + idx;
    
    // Initialize board with two random tiles
    ulong board = 0;
    board = addRandomTile(board, &seed);
    board = addRandomTile(board, &seed);
    
    int score = 0;
    int moves = 0;
    int maxTile = 0;
    
    // Run game until it's over or max moves reached
    while (!isGameOver(board) && moves < maxMoves) {
        int bestMove = findBestMove(board, weights);
        
        // If no valid move, end the game
        if (bestMove == -1) {
            break;
        }
        
        // Make the move
        bool moved = false;
        board = makeMove(board, bestMove, &moved);
        
        // If the move was valid, add a random tile
        if (moved) {
            board = addRandomTile(board, &seed);
            score = getScore(board);
            moves++;
            maxTile = max(maxTile, getMaxTile(board));
        } else {
            // If the move wasn't valid, try another direction
            continue;
        }
    }
    
    // Store results
    scores[idx] = score;
    maxTiles[idx] = maxTile;
    movesCounts[idx] = moves;
}
