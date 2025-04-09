// OpenCL kernel for 2048 game board evaluation

// Helper function to get tile value from BitBoard
int getTileValue(ulong board, int row, int col) {
    int bitValue = (board >> ((row * 4 + col) * 4)) & 0xF;
    if (bitValue == 0) {
        return 0;
    }
    return 1 << bitValue;
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

// Calculate monotonicity score
float calculateMonotonicity(ulong board) {
    float monotonicityScore = 0.0f;

    // Horizontal monotonicity
    for (int i = 0; i < 4; ++i) {
        // Left to right monotonicity
        int leftToRight = 0;
        // Right to left monotonicity
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

        // Take the smaller penalty
        monotonicityScore -= min(leftToRight, rightToLeft);
    }

    // Vertical monotonicity
    for (int j = 0; j < 4; ++j) {
        // Top to bottom monotonicity
        int topToBottom = 0;
        // Bottom to top monotonicity
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

        // Take the smaller penalty
        monotonicityScore -= min(topToBottom, bottomToTop);
    }

    return monotonicityScore;
}

// Calculate smoothness score
float calculateSmoothness(ulong board) {
    float smoothnessScore = 0.0f;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = getTileValue(board, i, j);

            if (value != 0) {
                // Check right tile
                if (j < 3) {
                    int rightValue = getTileValue(board, i, j + 1);
                    if (rightValue != 0) {
                        smoothnessScore -= fabs(log2((float)value) - log2((float)rightValue));
                    }
                }

                // Check below tile
                if (i < 3) {
                    int belowValue = getTileValue(board, i + 1, j);
                    if (belowValue != 0) {
                        smoothnessScore -= fabs(log2((float)value) - log2((float)belowValue));
                    }
                }
            }
        }
    }

    return smoothnessScore;
}

// Calculate corner score
float calculateCornerScore(ulong board) {
    float cornerScore = 0.0f;

    // Check the four corners
    int topLeft = getTileValue(board, 0, 0);
    int topRight = getTileValue(board, 0, 3);
    int bottomLeft = getTileValue(board, 3, 0);
    int bottomRight = getTileValue(board, 3, 3);

    // Find the maximum value
    int maxCorner = max(max(topLeft, topRight), max(bottomLeft, bottomRight));

    // Reward if the max value is in a corner
    if (maxCorner > 0) {
        cornerScore += log2((float)maxCorner);
    }

    return cornerScore;
}

// Calculate merge score
float calculateMergeScore(ulong board) {
    float mergeScore = 0.0f;

    // Check horizontal merges
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (getTileValue(board, i, j) != 0 && getTileValue(board, i, j) == getTileValue(board, i, j + 1)) {
                mergeScore += log2((float)getTileValue(board, i, j));
            }
        }
    }

    // Check vertical merges
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (getTileValue(board, i, j) != 0 && getTileValue(board, i, j) == getTileValue(board, i + 1, j)) {
                mergeScore += log2((float)getTileValue(board, i, j));
            }
        }
    }

    return mergeScore;
}

// Calculate pattern score (snake pattern)
float calculatePatternScore(ulong board) {
    float patternScore = 0.0f;

    // Define snake pattern weights
    const float weights[4][4] = {
        {15.0f, 14.0f, 13.0f, 12.0f},
        {8.0f, 9.0f, 10.0f, 11.0f},
        {7.0f, 6.0f, 5.0f, 4.0f},
        {0.0f, 1.0f, 2.0f, 3.0f}
    };

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int value = getTileValue(board, i, j);
            if (value > 0) {
                patternScore += weights[i][j] * log2((float)value);
            }
        }
    }

    return patternScore;
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

// Main kernel function for board evaluation
__kernel void evaluateBoard(
    __global const ulong* boards,
    __global const float* weights,
    __global float* scores,
    const int boardCount
) {
    int idx = get_global_id(0);
    if (idx >= boardCount) return;

    ulong board = boards[idx];
    float score = 0.0f;

    // Empty tiles heuristic
    score += weights[0] * (float)getEmptyTileCount(board);

    // Monotonicity heuristic
    score += weights[1] * calculateMonotonicity(board);

    // Smoothness heuristic
    score += weights[2] * calculateSmoothness(board);

    // Corner heuristic
    score += weights[3] * calculateCornerScore(board);

    // Merge heuristic
    score += weights[4] * calculateMergeScore(board);

    // Pattern heuristic for high tiles
    int maxTile = getMaxTile(board);
    if (maxTile >= 2048) {
        float patternWeight = log2((float)maxTile) / 11.0f;  // 2048's log2 is 11
        score += patternWeight * calculatePatternScore(board);
    }

    scores[idx] = score;
}
