#include "CornerStrategyEval.h"
#include <cmath>
#include <algorithm>

float CornerStrategyEval::evaluate(const BitBoard& board) {
    // 组合多个策略评估
    float snakeScore = calculateSnakePattern(board);
    float cornerScore = calculateCornerWeight(board);
    float gradientScore = calculateGradientWeight(board);
    
    // 加权组合
    return snakeScore * 1.5f + cornerScore * 2.0f + gradientScore * 1.0f;
}

float CornerStrategyEval::calculateSnakePattern(const BitBoard& board) {
    float score = 0.0f;
    
    // 检查四个可能的蛇形模式（从四个角落开始）
    
    // 1. 从左上角开始的蛇形
    {
        // 理想的蛇形路径
        const int path[16][2] = {
            {0, 0}, {0, 1}, {0, 2}, {0, 3},
            {1, 3}, {1, 2}, {1, 1}, {1, 0},
            {2, 0}, {2, 1}, {2, 2}, {2, 3},
            {3, 3}, {3, 2}, {3, 1}, {3, 0}
        };
        
        float pathScore = 0.0f;
        int prevValue = 0;
        
        for (int i = 0; i < 16; ++i) {
            int row = path[i][0];
            int col = path[i][1];
            int value = board.getTile(row, col);
            
            if (value == 0) continue;
            
            // 如果当前值大于前一个值，则不符合蛇形模式
            if (prevValue > 0 && value > prevValue) {
                pathScore -= std::log2(value) * 0.5f;
            } else {
                pathScore += std::log2(value);
                prevValue = value;
            }
        }
        
        score = std::max(score, pathScore);
    }
    
    // 2. 从右上角开始的蛇形
    {
        const int path[16][2] = {
            {0, 3}, {0, 2}, {0, 1}, {0, 0},
            {1, 0}, {1, 1}, {1, 2}, {1, 3},
            {2, 3}, {2, 2}, {2, 1}, {2, 0},
            {3, 0}, {3, 1}, {3, 2}, {3, 3}
        };
        
        float pathScore = 0.0f;
        int prevValue = 0;
        
        for (int i = 0; i < 16; ++i) {
            int row = path[i][0];
            int col = path[i][1];
            int value = board.getTile(row, col);
            
            if (value == 0) continue;
            
            if (prevValue > 0 && value > prevValue) {
                pathScore -= std::log2(value) * 0.5f;
            } else {
                pathScore += std::log2(value);
                prevValue = value;
            }
        }
        
        score = std::max(score, pathScore);
    }
    
    // 3. 从左下角开始的蛇形
    {
        const int path[16][2] = {
            {3, 0}, {3, 1}, {3, 2}, {3, 3},
            {2, 3}, {2, 2}, {2, 1}, {2, 0},
            {1, 0}, {1, 1}, {1, 2}, {1, 3},
            {0, 3}, {0, 2}, {0, 1}, {0, 0}
        };
        
        float pathScore = 0.0f;
        int prevValue = 0;
        
        for (int i = 0; i < 16; ++i) {
            int row = path[i][0];
            int col = path[i][1];
            int value = board.getTile(row, col);
            
            if (value == 0) continue;
            
            if (prevValue > 0 && value > prevValue) {
                pathScore -= std::log2(value) * 0.5f;
            } else {
                pathScore += std::log2(value);
                prevValue = value;
            }
        }
        
        score = std::max(score, pathScore);
    }
    
    // 4. 从右下角开始的蛇形
    {
        const int path[16][2] = {
            {3, 3}, {3, 2}, {3, 1}, {3, 0},
            {2, 0}, {2, 1}, {2, 2}, {2, 3},
            {1, 3}, {1, 2}, {1, 1}, {1, 0},
            {0, 0}, {0, 1}, {0, 2}, {0, 3}
        };
        
        float pathScore = 0.0f;
        int prevValue = 0;
        
        for (int i = 0; i < 16; ++i) {
            int row = path[i][0];
            int col = path[i][1];
            int value = board.getTile(row, col);
            
            if (value == 0) continue;
            
            if (prevValue > 0 && value > prevValue) {
                pathScore -= std::log2(value) * 0.5f;
            } else {
                pathScore += std::log2(value);
                prevValue = value;
            }
        }
        
        score = std::max(score, pathScore);
    }
    
    return score;
}

float CornerStrategyEval::calculateCornerWeight(const BitBoard& board) {
    float score = 0.0f;
    
    // 检查四个角落
    int cornerValues[4] = {
        board.getTile(0, 0),  // 左上
        board.getTile(0, 3),  // 右上
        board.getTile(3, 0),  // 左下
        board.getTile(3, 3)   // 右下
    };
    
    // 找出最大的角落值
    int maxCornerValue = 0;
    int maxCornerIndex = -1;
    
    for (int i = 0; i < 4; ++i) {
        if (cornerValues[i] > maxCornerValue) {
            maxCornerValue = cornerValues[i];
            maxCornerIndex = i;
        }
    }
    
    // 如果有角落有值，给予奖励
    if (maxCornerValue > 0) {
        // 最大值在角落，给予额外奖励
        score += std::log2(maxCornerValue) * 2.0f;
        
        // 根据最大角落的位置，检查相邻方块是否形成递减序列
        if (maxCornerIndex == 0) {  // 左上角
            int right = board.getTile(0, 1);
            int down = board.getTile(1, 0);
            
            if (right > 0 && right < maxCornerValue) {
                score += std::log2(right);
            }
            
            if (down > 0 && down < maxCornerValue) {
                score += std::log2(down);
            }
        } else if (maxCornerIndex == 1) {  // 右上角
            int left = board.getTile(0, 2);
            int down = board.getTile(1, 3);
            
            if (left > 0 && left < maxCornerValue) {
                score += std::log2(left);
            }
            
            if (down > 0 && down < maxCornerValue) {
                score += std::log2(down);
            }
        } else if (maxCornerIndex == 2) {  // 左下角
            int right = board.getTile(3, 1);
            int up = board.getTile(2, 0);
            
            if (right > 0 && right < maxCornerValue) {
                score += std::log2(right);
            }
            
            if (up > 0 && up < maxCornerValue) {
                score += std::log2(up);
            }
        } else if (maxCornerIndex == 3) {  // 右下角
            int left = board.getTile(3, 2);
            int up = board.getTile(2, 3);
            
            if (left > 0 && left < maxCornerValue) {
                score += std::log2(left);
            }
            
            if (up > 0 && up < maxCornerValue) {
                score += std::log2(up);
            }
        }
    }
    
    return score;
}

float CornerStrategyEval::calculateGradientWeight(const BitBoard& board) {
    // 定义四个角落的梯度权重矩阵
    const float gradientWeights[4][4][4] = {
        // 左上角梯度
        {
            {4.0f, 3.0f, 2.0f, 1.0f},
            {3.0f, 2.0f, 1.0f, 0.0f},
            {2.0f, 1.0f, 0.0f, -1.0f},
            {1.0f, 0.0f, -1.0f, -2.0f}
        },
        // 右上角梯度
        {
            {1.0f, 2.0f, 3.0f, 4.0f},
            {0.0f, 1.0f, 2.0f, 3.0f},
            {-1.0f, 0.0f, 1.0f, 2.0f},
            {-2.0f, -1.0f, 0.0f, 1.0f}
        },
        // 左下角梯度
        {
            {1.0f, 0.0f, -1.0f, -2.0f},
            {2.0f, 1.0f, 0.0f, -1.0f},
            {3.0f, 2.0f, 1.0f, 0.0f},
            {4.0f, 3.0f, 2.0f, 1.0f}
        },
        // 右下角梯度
        {
            {-2.0f, -1.0f, 0.0f, 1.0f},
            {-1.0f, 0.0f, 1.0f, 2.0f},
            {0.0f, 1.0f, 2.0f, 3.0f},
            {1.0f, 2.0f, 3.0f, 4.0f}
        }
    };
    
    // 计算每个角落的梯度分数
    float gradientScores[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    for (int corner = 0; corner < 4; ++corner) {
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                int value = board.getTile(row, col);
                if (value > 0) {
                    gradientScores[corner] += gradientWeights[corner][row][col] * std::log2(value);
                }
            }
        }
    }
    
    // 返回最高的梯度分数
    return *std::max_element(gradientScores, gradientScores + 4);
}
