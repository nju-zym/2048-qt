#ifndef RISK_EVAL_H
#define RISK_EVAL_H

#include "../BitBoard.h"

/**
 * @brief 风险评估类
 * 
 * 评估棋盘上的风险情况，例如被小数字包围的大数字
 */
class RiskEval {
public:
    /**
     * @brief 评估棋盘
     * @param board 棋盘
     * @return 评估分数
     */
    static float evaluate(const BitBoard& board);
    
private:
    /**
     * @brief 计算被困住的方块的风险
     * @param board 棋盘
     * @return 风险分数
     */
    static float calculateTrappedTilesRisk(const BitBoard& board);
    
    /**
     * @brief 计算棋盘的填充风险
     * @param board 棋盘
     * @return 风险分数
     */
    static float calculateFillingRisk(const BitBoard& board);
};

#endif // RISK_EVAL_H
