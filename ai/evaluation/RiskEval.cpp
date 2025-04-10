#include "RiskEval.h"
#include <cmath>
#include <algorithm>

float RiskEval::evaluate(const BitBoard& board) {
    // 组合多个风险评估
    float trappedRisk = calculateTrappedTilesRisk(board);
    float fillingRisk = calculateFillingRisk(board);
    
    // 返回负的风险分数（风险越高，分数越低）
    return -(trappedRisk + fillingRisk);
}

float RiskEval::calculateTrappedTilesRisk(const BitBoard& board) {
    float risk = 0.0f;
    
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = board.getTile(row, col);
            if (value <= 0) continue;
            
            float logValue = std::log2(value);
            
            // 检查周围的方块
            bool isTrapped = true;
            float surroundingSum = 0.0f;
            int surroundingCount = 0;
            
            // 上方方块
            if (row > 0) {
                int upValue = board.getTile(row - 1, col);
                if (upValue == 0) {
                    isTrapped = false;  // 有空位，不被困住
                } else {
                    surroundingSum += std::log2(upValue);
                    surroundingCount++;
                }
            }
            
            // 下方方块
            if (row < 3) {
                int downValue = board.getTile(row + 1, col);
                if (downValue == 0) {
                    isTrapped = false;  // 有空位，不被困住
                } else {
                    surroundingSum += std::log2(downValue);
                    surroundingCount++;
                }
            }
            
            // 左侧方块
            if (col > 0) {
                int leftValue = board.getTile(row, col - 1);
                if (leftValue == 0) {
                    isTrapped = false;  // 有空位，不被困住
                } else {
                    surroundingSum += std::log2(leftValue);
                    surroundingCount++;
                }
            }
            
            // 右侧方块
            if (col < 3) {
                int rightValue = board.getTile(row, col + 1);
                if (rightValue == 0) {
                    isTrapped = false;  // 有空位，不被困住
                } else {
                    surroundingSum += std::log2(rightValue);
                    surroundingCount++;
                }
            }
            
            // 如果被困住且周围的方块值都比当前方块小，则增加风险
            if (isTrapped && surroundingCount > 0) {
                float avgSurrounding = surroundingSum / surroundingCount;
                if (logValue > avgSurrounding) {
                    // 当前方块比周围方块大，且被困住，增加风险
                    risk += (logValue - avgSurrounding) * logValue;
                }
            }
        }
    }
    
    return risk;
}

float RiskEval::calculateFillingRisk(const BitBoard& board) {
    int emptyTiles = board.countEmptyTiles();
    
    // 空位越少，风险越高
    if (emptyTiles <= 2) {
        // 只有0-2个空位，高风险
        return 10.0f;
    } else if (emptyTiles <= 4) {
        // 3-4个空位，中等风险
        return 5.0f;
    } else if (emptyTiles <= 6) {
        // 5-6个空位，低风险
        return 2.0f;
    } else {
        // 7个或更多空位，几乎没有风险
        return 0.0f;
    }
}
