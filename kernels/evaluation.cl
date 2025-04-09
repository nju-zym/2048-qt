// BitBoard representation for 2048 game
// Each tile is represented by 4 bits, so a 4x4 board fits in a 64-bit integer

// Constants
#define BOARD_SIZE 4
#define EMPTY_WEIGHT 0
#define MONOTONICITY_WEIGHT 1
#define SMOOTHNESS_WEIGHT 2
#define CORNER_WEIGHT 3
#define MERGE_WEIGHT 4

// Get the value of a tile at a specific position
int getTileValue(ulong board, int row, int col) {
    int shift = ((row * 4 + col) * 4);
    int value = (int)((board >> shift) & 0xF);
    
    if (value == 0) {
        return 0;
    }
    
    return 1 << value;
}

// Count empty tiles on the board
int countEmptyTiles(ulong board) {
    int count = 0;
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (getTileValue(board, i, j) == 0) {
                count++;
            }
        }
    }
    
    return count;
}

// Calculate monotonicity score
float calculateMonotonicity(ulong board) {
    int leftToRight = 0;
    int rightToLeft = 0;
    int topToBottom = 0;
    int bottomToTop = 0;
    
    // Check horizontal monotonicity
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE - 1; j++) {
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
    }
    
    // Check vertical monotonicity
    for (int j = 0; j < BOARD_SIZE; j++) {
        for (int i = 0; i < BOARD_SIZE - 1; i++) {
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
    }
    
    // Return the minimum of the two directions
    int horizontal = min(leftToRight, rightToLeft);
    int vertical = min(topToBottom, bottomToTop);
    
    return -1.0f * (horizontal + vertical);
}

// Calculate smoothness score
float calculateSmoothness(ulong board) {
    float smoothness = 0.0f;
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            int value = getTileValue(board, i, j);
            
            if (value != 0) {
                float valueLog = log2((float)value);
                
                // Check right neighbor
                if (j < BOARD_SIZE - 1) {
                    int rightValue = getTileValue(board, i, j + 1);
                    if (rightValue != 0) {
                        float rightValueLog = log2((float)rightValue);
                        smoothness -= fabs(valueLog - rightValueLog);
                    }
                }
                
                // Check bottom neighbor
                if (i < BOARD_SIZE - 1) {
                    int bottomValue = getTileValue(board, i + 1, j);
                    if (bottomValue != 0) {
                        float bottomValueLog = log2((float)bottomValue);
                        smoothness -= fabs(valueLog - bottomValueLog);
                    }
                }
            }
        }
    }
    
    return smoothness;
}

// Calculate corner score
float calculateCornerScore(ulong board) {
    int topLeft = getTileValue(board, 0, 0);
    int topRight = getTileValue(board, 0, 3);
    int bottomLeft = getTileValue(board, 3, 0);
    int bottomRight = getTileValue(board, 3, 3);
    
    // Find the maximum value in the corners
    int maxCorner = max(max(topLeft, topRight), max(bottomLeft, bottomRight));
    
    if (maxCorner == 0) {
        return 0.0f;
    }
    
    // Calculate score based on the maximum corner value
    float cornerScore = 0.0f;
    
    // Bonus for having the max tile in a corner
    if (maxCorner == topLeft) {
        cornerScore += log2((float)maxCorner) * 2.0f;
        
        // Additional bonus for having decreasing values in the right direction
        for (int j = 1; j < BOARD_SIZE; j++) {
            int current = getTileValue(board, 0, j);
            int prev = getTileValue(board, 0, j - 1);
            if (current != 0 && prev != 0 && current < prev) {
                cornerScore += log2((float)current);
            }
        }
        
        // Additional bonus for having decreasing values in the down direction
        for (int i = 1; i < BOARD_SIZE; i++) {
            int current = getTileValue(board, i, 0);
            int prev = getTileValue(board, i - 1, 0);
            if (current != 0 && prev != 0 && current < prev) {
                cornerScore += log2((float)current);
            }
        }
    } else if (maxCorner == topRight) {
        cornerScore += log2((float)maxCorner) * 2.0f;
        
        // Additional bonus for having decreasing values in the left direction
        for (int j = 2; j >= 0; j--) {
            int current = getTileValue(board, 0, j);
            int prev = getTileValue(board, 0, j + 1);
            if (current != 0 && prev != 0 && current < prev) {
                cornerScore += log2((float)current);
            }
        }
        
        // Additional bonus for having decreasing values in the down direction
        for (int i = 1; i < BOARD_SIZE; i++) {
            int current = getTileValue(board, i, 3);
            int prev = getTileValue(board, i - 1, 3);
            if (current != 0 && prev != 0 && current < prev) {
                cornerScore += log2((float)current);
            }
        }
    } else if (maxCorner == bottomLeft) {
        cornerScore += log2((float)maxCorner) * 2.0f;
        
        // Additional bonus for having decreasing values in the right direction
        for (int j = 1; j < BOARD_SIZE; j++) {
            int current = getTileValue(board, 3, j);
            int prev = getTileValue(board, 3, j - 1);
            if (current != 0 && prev != 0 && current < prev) {
                cornerScore += log2((float)current);
            }
        }
        
        // Additional bonus for having decreasing values in the up direction
        for (int i = 2; i >= 0; i--) {
            int current = getTileValue(board, i, 0);
            int prev = getTileValue(board, i + 1, 0);
            if (current != 0 && prev != 0 && current < prev) {
                cornerScore += log2((float)current);
            }
        }
    } else if (maxCorner == bottomRight) {
        cornerScore += log2((float)maxCorner) * 2.0f;
        
        // Additional bonus for having decreasing values in the left direction
        for (int j = 2; j >= 0; j--) {
            int current = getTileValue(board, 3, j);
            int prev = getTileValue(board, 3, j + 1);
            if (current != 0 && prev != 0 && current < prev) {
                cornerScore += log2((float)current);
            }
        }
        
        // Additional bonus for having decreasing values in the up direction
        for (int i = 2; i >= 0; i--) {
            int current = getTileValue(board, i, 3);
            int prev = getTileValue(board, i + 1, 3);
            if (current != 0 && prev != 0 && current < prev) {
                cornerScore += log2((float)current);
            }
        }
    }
    
    return cornerScore;
}

// Calculate merge potential score
float calculateMergePotential(ulong board) {
    float mergeScore = 0.0f;
    
    // Check horizontal merge possibilities
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE - 1; j++) {
            int current = getTileValue(board, i, j);
            int next = getTileValue(board, i, j + 1);
            
            if (current != 0 && current == next) {
                mergeScore += log2((float)current);
            }
        }
    }
    
    // Check vertical merge possibilities
    for (int j = 0; j < BOARD_SIZE; j++) {
        for (int i = 0; i < BOARD_SIZE - 1; i++) {
            int current = getTileValue(board, i, j);
            int next = getTileValue(board, i + 1, j);
            
            if (current != 0 && current == next) {
                mergeScore += log2((float)current);
            }
        }
    }
    
    return mergeScore;
}

// Kernel to evaluate multiple boards in parallel
__kernel void evaluateBoards(
    __global const ulong* boards,
    __global const float* weights,
    __global float* scores,
    const int boardCount
) {
    int idx = get_global_id(0);
    
    if (idx < boardCount) {
        ulong board = boards[idx];
        
        // Calculate heuristics
        float emptyScore = (float)countEmptyTiles(board) * weights[EMPTY_WEIGHT];
        float monoScore = calculateMonotonicity(board) * weights[MONOTONICITY_WEIGHT];
        float smoothScore = calculateSmoothness(board) * weights[SMOOTHNESS_WEIGHT];
        float cornerScore = calculateCornerScore(board) * weights[CORNER_WEIGHT];
        float mergeScore = calculateMergePotential(board) * weights[MERGE_WEIGHT];
        
        // Combine scores
        scores[idx] = emptyScore + monoScore + smoothScore + cornerScore + mergeScore;
        
        // Add pattern heuristic for high-value boards
        int maxTile = 0;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                int value = getTileValue(board, i, j);
                maxTile = max(maxTile, value);
            }
        }
        
        if (maxTile >= 2048) {
            // Snake pattern weights
            const float patternWeights[4][4] = {
                {15.0f, 14.0f, 13.0f, 12.0f},
                {8.0f, 9.0f, 10.0f, 11.0f},
                {7.0f, 6.0f, 5.0f, 4.0f},
                {0.0f, 1.0f, 2.0f, 3.0f}
            };
            
            float patternScore = 0.0f;
            for (int i = 0; i < BOARD_SIZE; i++) {
                for (int j = 0; j < BOARD_SIZE; j++) {
                    int value = getTileValue(board, i, j);
                    if (value > 0) {
                        patternScore += patternWeights[i][j] * log2((float)value);
                    }
                }
            }
            
            // Weight the pattern score based on the max tile
            float patternWeight = log2((float)maxTile) / 11.0f;  // 2048's log2 is 11
            scores[idx] += patternWeight * patternScore;
        }
    }
}
