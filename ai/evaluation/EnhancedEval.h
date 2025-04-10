#ifndef ENHANCED_EVAL_H
#define ENHANCED_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 增强版评估函数类
 * 
 * 结合多种评估策略，提供更准确的棋盘状态评估
 */
class EnhancedEval {
public:
    /**
     * @brief 评估棋盘状态
     * @param board 棋盘状态
     * @return 评估分数
     */
    static float evaluate(const BitBoard& board);
    
private:
    // 评估函数权重
    static constexpr float MONOTONICITY_WEIGHT = 1.0F;
    static constexpr float SMOOTHNESS_WEIGHT = 0.1F;
    static constexpr float FREE_TILES_WEIGHT = 2.7F;
    static constexpr float MERGE_WEIGHT = 1.0F;
    static constexpr float TILE_PLACEMENT_WEIGHT = 1.0F;
    static constexpr float CORNER_MAX_WEIGHT = 2.0F;
    static constexpr float SNAKE_PATTERN_WEIGHT = 1.5F;
    
    /**
     * @brief 评估最大值是否在角落
     * @param board 棋盘状态
     * @return 评估分数
     */
    static float evaluateCornerMax(const BitBoard& board);
    
    /**
     * @brief 评估蛇形模式
     * @param board 棋盘状态
     * @return 评估分数
     */
    static float evaluateSnakePattern(const BitBoard& board);
};

#endif // ENHANCED_EVAL_H
