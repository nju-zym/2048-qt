#include "auto.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

// 常量定义
static uint64_t const ROW_MASK = 0xFF'FFULL;

// 转置棋盘
inline static BitBoard transpose(BitBoard x) {
    BitBoard a1 = x & 0xF0'F0'0F'0F'F0'F0'0F'0FULL;
    BitBoard a2 = x & 0x00'00'F0'F0'00'00'F0'F0ULL;
    BitBoard a3 = x & 0x0F'0F'00'00'0F'0F'00'00ULL;
    BitBoard a  = a1 | (a2 << 12) | (a3 >> 12);
    BitBoard b1 = a & 0xFF'00'FF'00'00'FF'00'FFULL;
    BitBoard b2 = a & 0x00'FF'00'FF'00'00'00'00ULL;
    BitBoard b3 = a & 0x00'00'00'00'FF'00'FF'00ULL;
    return b1 | (b2 >> 24) | (b3 << 24);
}

// 执行向上移动
inline static BitBoard execute_move_0(BitBoard board) {
    BitBoard ret = board;
    BitBoard t   = transpose(board);

    // 使用预编译的表格
    for (int i = 0; i < 4; ++i) {
        uint16_t row      = (t >> (i * 16)) & ROW_MASK;
        uint16_t new_row  = BitBoardTables::LEFT_MOVE_TABLE[row];
        BitBoard diff     = static_cast<BitBoard>(new_row ^ row) << (i * 16);
        ret              ^= transpose(diff);
    }

    return ret;
}

// 执行向下移动
inline static BitBoard execute_move_1(BitBoard board) {
    BitBoard ret = board;
    BitBoard t   = transpose(board);

    // 使用预编译的表格
    for (int i = 0; i < 4; ++i) {
        uint16_t row         = (t >> (i * 16)) & ROW_MASK;
        uint16_t rev_row     = (row & 0xF) << 12 | (row & 0xF0) << 4 | (row & 0xF'00) >> 4 | (row & 0xF0'00) >> 12;
        uint16_t new_rev_row = BitBoardTables::LEFT_MOVE_TABLE[rev_row];
        uint16_t new_row     = (new_rev_row & 0xF) << 12 | (new_rev_row & 0xF0) << 4 | (new_rev_row & 0xF'00) >> 4
                           | (new_rev_row & 0xF0'00) >> 12;
        BitBoard diff  = static_cast<BitBoard>(new_row ^ row) << (i * 16);
        ret           ^= transpose(diff);
    }

    return ret;
}

// 执行向左移动
inline static BitBoard execute_move_2(BitBoard board) {
    BitBoard ret = board;

    // 使用预编译的表格
    for (int i = 0; i < 4; ++i) {
        uint16_t row      = (board >> (i * 16)) & ROW_MASK;
        uint16_t new_row  = BitBoardTables::LEFT_MOVE_TABLE[row];
        ret              ^= static_cast<BitBoard>(new_row ^ row) << (i * 16);
    }

    return ret;
}

// 执行向右移动
inline static BitBoard execute_move_3(BitBoard board) {
    BitBoard ret = board;

    // 使用预编译的表格
    for (int i = 0; i < 4; ++i) {
        uint16_t row         = (board >> (i * 16)) & ROW_MASK;
        uint16_t rev_row     = (row & 0xF) << 12 | (row & 0xF0) << 4 | (row & 0xF'00) >> 4 | (row & 0xF0'00) >> 12;
        uint16_t new_rev_row = BitBoardTables::LEFT_MOVE_TABLE[rev_row];
        uint16_t new_row     = (new_rev_row & 0xF) << 12 | (new_rev_row & 0xF0) << 4 | (new_rev_row & 0xF'00) >> 4
                           | (new_rev_row & 0xF0'00) >> 12;
        ret ^= static_cast<BitBoard>(new_row ^ row) << (i * 16);
    }

    return ret;
}

// 计算空格数 - 高效位操作版本
int Auto::countEmptyTiles(BitBoard board) {
    // 创建一个掩码，将每个单元的最低位设为1
    // 如果该单元为0，则整个单元的所有位都为0
    BitBoard mask  = board;
    mask          |= (mask >> 1);
    mask          |= (mask >> 2);
    mask          |= (mask >> 3);
    // 现在每个4位单元中，如果原始值为0，则所有位都是0，否则最低位是1

    // 只保留每个单元的最低位
    mask &= 0x11'11'11'11'11'11'11'11ULL;

    // 计算有多少个非零单元
    // 使用位计数技术
    mask = (mask & 0x55'55'55'55'55'55'55'55ULL) + ((mask >> 1) & 0x55'55'55'55'55'55'55'55ULL);
    mask = (mask & 0x33'33'33'33'33'33'33'33ULL) + ((mask >> 2) & 0x33'33'33'33'33'33'33'33ULL);
    mask = (mask & 0x0f'0f'0f'0f'0f'0f'0f'0fULL) + ((mask >> 4) & 0x0f'0f'0f'0f'0f'0f'0f'0fULL);
    mask = (mask & 0x00'ff'00'ff'00'ff'00'ffULL) + ((mask >> 8) & 0x00'ff'00'ff'00'ff'00'ffULL);
    mask = (mask & 0x00'00'ff'ff'00'00'ff'ffULL) + ((mask >> 16) & 0x00'00'ff'ff'00'00'ff'ffULL);
    mask = (mask & 0x00'00'00'00'ff'ff'ff'ffULL) + ((mask >> 32) & 0x00'00'00'00'ff'ff'ff'ffULL);

    // 返回空格数（总单元数 - 非空单元数）
    return 16 - static_cast<int>(mask);
}

// 检查游戏是否结束 - 安全版本
bool Auto::isGameOverBitBoard(BitBoard board) {
    // 如果有空格，游戏未结束
    if (countEmptyTiles(board) > 0) {
        return false;
    }

    // 检查水平方向是否可以合并
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            // 使用int而非uint64_t来避免不必要的转换
            int pos1 = (i * 4 + j) * 4;
            int pos2 = (i * 4 + j + 1) * 4;

            // 确保位移位数在合理范围内
            if (pos1 >= 0 && pos1 < 64 && pos2 >= 0 && pos2 < 64) {
                // 使用无符号类型进行位移操作
                uint64_t val1 = (board >> static_cast<uint64_t>(pos1)) & 0xFULL;
                uint64_t val2 = (board >> static_cast<uint64_t>(pos2)) & 0xFULL;
                if (val1 == val2 && val1 != 0) {
                    return false;  // 可以合并，游戏未结束
                }
            }
        }
    }

    // 检查垂直方向是否可以合并
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            // 使用int而非uint64_t来避免不必要的转换
            int pos1 = (i * 4 + j) * 4;
            int pos2 = ((i + 1) * 4 + j) * 4;

            // 确保位移位数在合理范围内
            if (pos1 >= 0 && pos1 < 64 && pos2 >= 0 && pos2 < 64) {
                // 使用无符号类型进行位移操作
                uint64_t val1 = (board >> static_cast<uint64_t>(pos1)) & 0xFULL;
                uint64_t val2 = (board >> static_cast<uint64_t>(pos2)) & 0xFULL;
                if (val1 == val2 && val1 != 0) {
                    return false;  // 可以合并，游戏未结束
                }
            }
        }
    }

    // 没有空格且没有可合并的方块，游戏结束
    return true;
}

// 构造函数：初始化策略参数
Auto::Auto()
    : strategyParams(5, 1.0),  // 初始化为5个参数，默认值为1.0
      defaultParams(5, 1.0),   // 初始化默认参数
      useLearnedParams(false) {
    // 初始化默认参数为经过测试的有效值
    defaultParams[0] = 2.7;  // 空格数量权重
    defaultParams[1] = 1.8;  // 蛇形模式权重
    defaultParams[2] = 3.5;  // 平滑度权重
    defaultParams[3] = 2.0;  // 单调性权重
    defaultParams[4] = 1.5;  // 合并可能性权重

    // 初始化随机数生成器
    srand(time(nullptr));

    // 初始化位棋盘的预计算表
    initTables();

    // 使用默认参数
    qDebug() << "Using default parameters";
}

// 析构函数
Auto::~Auto() {
    qDebug() << "Auto object destroyed successfully";
}

// 设置是否使用学习到的参数
void Auto::setUseLearnedParams(bool use) {
    useLearnedParams = use;
}

// 获取是否使用学习到的参数
bool Auto::getUseLearnedParams() const {
    return useLearnedParams;
}

// 获取策略参数
QVector<double> Auto::getStrategyParams() const {
    return strategyParams;
}

// findBestMove: 找出最佳移动方向
int Auto::findBestMove(QVector<QVector<int>> const& board) {
    // 检查棋盘上的最大值
    int maxValue = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            maxValue = std::max(maxValue, board[i][j]);
        }
    }

    // 初始化预计算表（如果还没有初始化）
    static bool tablesInitialized = false;
    if (!tablesInitialized) {
        initTables();
        tablesInitialized = true;
    }

    // 始终使用位棋盘实现的最佳移动函数，以保持算法一致性
    return getBestMoveBitBoard(board);
}

// evaluateBoard: 评估棋盘状态
int Auto::evaluateBoard(QVector<QVector<int>> const& boardState) {
    int score = 0;

    // 策略：高分数方块尽量放在角落，并保持递减排列

    // 1. 计算空格数量，空格越多越好
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    score += emptyCount * 10;  // 每个空格给10分

    // 2. 奖励角落有高分数方块
    // 右下角最高分
    if (boardState[3][3] > 0) {
        score += boardState[3][3] * 2;
    }

    // 3. 奖励相邻方块数值递减排列
    // 检查行
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            // 如果相邻方块有递减关系，加分
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0 && boardState[i][j] >= boardState[i][j + 1]) {
                score += std::min(boardState[i][j], boardState[i][j + 1]);
            }
        }
    }

    // 检查列
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            // 如果相邻方块有递减关系，加分
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0 && boardState[i][j] >= boardState[i + 1][j]) {
                score += std::min(boardState[i][j], boardState[i + 1][j]);
            }
        }
    }

    return score;
}

// 检查游戏是否结束
bool Auto::isGameOver(QVector<QVector<int>> const& boardState) {
    // 检查是否有空格
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                return false;  // 有空格，游戏未结束
            }
        }
    }

    // 检查是否有相邻相同的方块
    // 检查行
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] == boardState[i][j + 1]) {
                return false;  // 有相邻相同的方块，游戏未结束
            }
        }
    }

    // 检查列
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] == boardState[i + 1][j]) {
                return false;  // 有相邻相同的方块，游戏未结束
            }
        }
    }

    return true;  // 没有空格且没有相邻相同的方块，游戏结束
}

// 计算合并分数 - 评估棋盘上可能的合并操作
double Auto::calculateMergeScore(QVector<QVector<int>> const& boardState) {
    double mergeScore = 0;

    // 检查行上的合并可能性
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i][j + 1]) {
                // 对于高值方块，给予更高的奖励
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                mergeScore       += tileValue * (logValue / 10.0) * 2.0;
            }
        }
    }

    // 检查列上的合并可能性
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i + 1][j]) {
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                mergeScore       += tileValue * (logValue / 10.0) * 2.0;
            }
        }
    }

    // 检查相邻的递增序列（如 2-4-8）
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 2; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0 && boardState[i][j + 2] > 0) {
                if (boardState[i][j + 1] == 2 * boardState[i][j] && boardState[i][j + 2] == 2 * boardState[i][j + 1]) {
                    // 奖励递增序列
                    mergeScore += boardState[i][j + 2] * 0.5;
                }
            }
        }
    }

    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 2; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0 && boardState[i + 2][j] > 0) {
                if (boardState[i + 1][j] == 2 * boardState[i][j] && boardState[i + 2][j] == 2 * boardState[i + 1][j]) {
                    // 奖励递增序列
                    mergeScore += boardState[i + 2][j] * 0.5;
                }
            }
        }
    }

    return mergeScore;
}

// 高级模式评估 - 专门处理超过2048的复杂情况
int Auto::evaluateAdvancedPattern(QVector<QVector<int>> const& boardState) {
    int score    = 0;
    int maxValue = 0;
    int maxRow = 0, maxCol = 0;

    // 找出最大值及其位置
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            maxValue = std::max(maxValue, boardState[i][j]);
            if (boardState[i][j] == maxValue) {
                maxRow = i;
                maxCol = j;
            }
        }
    }

    // 如果最大值小于2048，使用普通评估函数
    if (maxValue < 2048) {
        return evaluateBoardAdvanced(boardState);
    }

    // 对于超过2048的情况，使用更复杂的评估策略

    // 1. 蛇形模式权重矩阵 - 为了处理高级棋盘状态
    // 定义蛇形路径的权重矩阵，从左上角开始蛇形形式排列
    int const snakePattern[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};

    // 2. 计算蛇形模式得分
    int patternScore = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > 0) {
                // 对于高值方块，给予更高的权重
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                patternScore     += tileValue * snakePattern[i][j] * (logValue / 11.0);  // 11 = log2(2048)
            }
        }
    }
    score += patternScore * 2;  // 增加模式权重

    // 3. 空格数量权重 - 对于高级棋盘更重要
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    // 空格数量的权重与棋盘填充程度相关，棋盘越满空格越重要
    score += emptyCount * (16 - emptyCount) * 50;  // 大幅增加空格权重

    // 4. 平滑度权重 - 相邻方块数值差异越小越好
    double smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                // 使用对数差异来减少大数值之间的差异惩罚
                double logVal1  = log2(boardState[i][j]);
                double logVal2  = log2(boardState[i][j + 1]);
                smoothness     -= std::abs(logVal1 - logVal2) * 4.0;
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                double logVal1  = log2(boardState[i][j]);
                double logVal2  = log2(boardState[i + 1][j]);
                smoothness     -= std::abs(logVal1 - logVal2) * 4.0;
            }
        }
    }
    score += static_cast<int>(smoothness * 40);  // 增加平滑度权重

    // 5. 单调性权重 - 方块数值沿某个方向单调递增或递减
    double monotonicity = 0;

    // 检查行的单调性 - 使用对数值
    for (int i = 0; i < 4; ++i) {
        double current     = 0;
        double left_score  = 0;
        double right_score = 0;

        for (int j = 0; j < 4; ++j) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                if (current > 0) {
                    if (current > value) {
                        left_score += (current - value) * 2;  // 增加左向单调性权重
                    } else {
                        right_score += (value - current) * 2;  // 增加右向单调性权重
                    }
                }
                current = value;
            }
        }
        monotonicity += std::min(left_score, right_score);
    }

    // 检查列的单调性 - 使用对数值
    for (int j = 0; j < 4; ++j) {
        double current    = 0;
        double up_score   = 0;
        double down_score = 0;

        for (int i = 0; i < 4; ++i) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                if (current > 0) {
                    if (current > value) {
                        up_score += (current - value) * 2;  // 增加上向单调性权重
                    } else {
                        down_score += (value - current) * 2;  // 增加下向单调性权重
                    }
                }
                current = value;
            }
        }
        monotonicity += std::min(up_score, down_score);
    }

    score -= static_cast<int>(monotonicity * 60);  // 单调性是负分，越小越好

    // 6. 合并可能性 - 奖励相邻相同值
    double mergeScore  = calculateMergeScore(boardState);
    score             += static_cast<int>(mergeScore * 20);

    // 7. 游戏结束检测 - 如果游戏即将结束，给予大量惩罚
    if (isGameOver(boardState)) {
        score -= 500000;  // 大幅惩罚游戏结束状态
    }

    // 8. 角落奖励 - 如果最大值在角落，给予额外奖励
    if ((maxRow == 0 || maxRow == 3) && (maxCol == 0 || maxCol == 3)) {
        score += maxValue * 2;  // 奖励最大值在角落
    }

    return score;
}

// evaluateBoardAdvanced: 高级评估棋盘状态
int Auto::evaluateBoardAdvanced(QVector<QVector<int>> const& boardState) {
    int score = 0;

    // 1. 空格数量权重 - 空格越多越好
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    // 空格数量的权重与棋盘填充程度相关，棋盘越满空格越重要
    score += emptyCount * (16 - emptyCount) * 20;  // 增加空格权重

    // 2. 角落策略 - 使用权重矩阵
    // 定义权重矩阵，优先将大数放在角落，特别是左上角
    int const weightMatrix[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};

    // 计算权重得分
    int weightScore = 0;
    int maxValue    = 0;
    int maxRow = 0, maxCol = 0;

    // 找出最大值及其位置
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            maxValue = std::max(maxValue, boardState[i][j]);
            if (boardState[i][j] == maxValue) {
                maxRow = i;
                maxCol = j;
            }
            // 根据权重矩阵计算得分
            if (boardState[i][j] > 0) {
                weightScore += boardState[i][j] * weightMatrix[i][j];
            }
        }
    }

    // 如果最大值在左上角，给予额外奖励
    if (maxRow == 0 && maxCol == 0) {
        weightScore += maxValue * 4;
    }
    // 如果最大值在任意角落，也给予奖励
    else if ((maxRow == 0 || maxRow == 3) && (maxCol == 0 || maxCol == 3)) {
        weightScore += maxValue * 2;
    }

    score += weightScore;

    // 3. 平滑度权重 - 相邻方块数值差异越小越好
    int smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                // 使用对数差异来减少大数值之间的差异惩罚
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i][j + 1] > 0 ? log2(boardState[i][j + 1]) : 0;
                smoothness     -= abs(logVal1 - logVal2) * 2.0;
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                // 使用对数差异
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i + 1][j] > 0 ? log2(boardState[i + 1][j]) : 0;
                smoothness     -= abs(logVal1 - logVal2) * 2.0;
            }
        }
    }
    score += smoothness * 30;  // 增加平滑度权重

    // 4. 单调性权重 - 方块数值沿某个方向单调递增或递减
    double monotonicity = 0;

    // 检查行的单调性 - 使用对数值
    for (int i = 0; i < 4; ++i) {
        double current     = 0;
        double left_score  = 0;
        double right_score = 0;

        for (int j = 0; j < 4; ++j) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                // 计算左向单调性
                if (current > value) {
                    left_score += current - value;
                } else {
                    right_score += value - current;
                }
                current = value;
            }
        }
        monotonicity += std::min(left_score, right_score);
    }

    // 检查列的单调性 - 使用对数值
    for (int j = 0; j < 4; ++j) {
        double current    = 0;
        double up_score   = 0;
        double down_score = 0;

        for (int i = 0; i < 4; ++i) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                // 计算上向单调性
                if (current > value) {
                    up_score += current - value;
                } else {
                    down_score += value - current;
                }
                current = value;
            }
        }
        monotonicity += std::min(up_score, down_score);
    }

    score -= monotonicity * 50;  // 单调性是负分，越小越好

    // 5. 合并可能性 - 奖励相邻相同值
    int mergeScore = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i][j + 1]) {
                mergeScore += boardState[i][j];
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i + 1][j]) {
                mergeScore += boardState[i][j];
            }
        }
    }
    score += mergeScore * 10;

    // 6. 大数值聚集 - 奖励大数值聚集在一起
    int clusterScore = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            int cellValue = boardState[i][j];
            if (cellValue > 0) {
                // 检查右边和下边的方块
                if (boardState[i][j + 1] > 0) {
                    clusterScore += std::min(cellValue, boardState[i][j + 1]);
                }
                if (boardState[i + 1][j] > 0) {
                    clusterScore += std::min(cellValue, boardState[i + 1][j]);
                }
            }
        }
    }
    score += clusterScore;

    return score;
}

// simulateMove: 模拟移动
bool Auto::simulateMove(QVector<QVector<int>>& boardState, int direction, int& score) {
    bool moved = false;
    score      = 0;

    if (direction == 0) {  // 向上
        for (int col = 0; col < 4; ++col) {
            for (int row = 1; row < 4; ++row) {
                if (boardState[row][col] != 0) {
                    int r = row;
                    while (r > 0 && boardState[r - 1][col] == 0) {
                        boardState[r - 1][col] = boardState[r][col];
                        boardState[r][col]     = 0;
                        r--;
                        moved = true;
                    }
                    if (r > 0 && boardState[r - 1][col] == boardState[r][col]) {
                        boardState[r - 1][col] *= 2;
                        score                  += boardState[r - 1][col];
                        boardState[r][col]      = 0;
                        moved                   = true;
                    }
                }
            }
        }
    } else if (direction == 1) {  // 向右
        for (int row = 0; row < 4; ++row) {
            for (int col = 2; col >= 0; --col) {
                if (boardState[row][col] != 0) {
                    int c = col;
                    while (c < 3 && boardState[row][c + 1] == 0) {
                        boardState[row][c + 1] = boardState[row][c];
                        boardState[row][c]     = 0;
                        c++;
                        moved = true;
                    }
                    if (c < 3 && boardState[row][c + 1] == boardState[row][c]) {
                        boardState[row][c + 1] *= 2;
                        score                  += boardState[row][c + 1];
                        boardState[row][c]      = 0;
                        moved                   = true;
                    }
                }
            }
        }
    } else if (direction == 2) {  // 向下
        for (int col = 0; col < 4; ++col) {
            for (int row = 2; row >= 0; --row) {
                if (boardState[row][col] != 0) {
                    int r = row;
                    while (r < 3 && boardState[r + 1][col] == 0) {
                        boardState[r + 1][col] = boardState[r][col];
                        boardState[r][col]     = 0;
                        r++;
                        moved = true;
                    }
                    if (r < 3 && boardState[r + 1][col] == boardState[r][col]) {
                        boardState[r + 1][col] *= 2;
                        score                  += boardState[r + 1][col];
                        boardState[r][col]      = 0;
                        moved                   = true;
                    }
                }
            }
        }
    } else if (direction == 3) {  // 向左
        for (int row = 0; row < 4; ++row) {
            for (int col = 1; col < 4; ++col) {
                if (boardState[row][col] != 0) {
                    int c = col;
                    while (c > 0 && boardState[row][c - 1] == 0) {
                        boardState[row][c - 1] = boardState[row][c];
                        boardState[row][c]     = 0;
                        c--;
                        moved = true;
                    }
                    if (c > 0 && boardState[row][c - 1] == boardState[row][c]) {
                        boardState[row][c - 1] *= 2;
                        score                  += boardState[row][c - 1];
                        boardState[row][c]      = 0;
                        moved                   = true;
                    }
                }
            }
        }
    }

    return moved;
}

// 初始化位棋盘的预计算表
void Auto::initTables() {
    // 使用静态标志确保表只初始化一次
    static std::atomic<bool> tablesInitialized(false);
    static QMutex initMutex;

    // 如果表已经初始化，则直接返回
    if (tablesInitialized.load()) {
        return;
    }

    // 使用互斥锁确保只有一个线程初始化表
    QMutexLocker locker(&initMutex);

    // 再次检查，防止多个线程同时通过第一次检查
    if (tablesInitialized.load()) {
        return;
    }

    // 表已经在BitBoardTables.cpp中预先计算好了，不需要在运行时计算
    // 直接使用预编译的表

    // 标记表已初始化
    tablesInitialized.store(true);

    // 在主线程中输出调试信息
    QMetaObject::invokeMethod(
        QApplication::instance(),
        []() { qDebug() << "BitBoard tables loaded from precompiled data"; },
        Qt::QueuedConnection);
}

// 清除expectimax缓存的空方法（为了保持兼容性）
void Auto::clearExpectimaxCache() {
    // 缓存机制已经完全移除，这个方法现在什么也不做
    // 仅为了保持与现有代码的兼容性而保留
}

// 计算移动的启发式评分 - 用于移动排序
int Auto::calculateMoveHeuristic(BitBoard board, int direction) {
    BitBoard newBoard = board;

    // 模拟移动
    switch (direction) {
        case 0:  // 上
            newBoard = execute_move_0(board);
            break;
        case 1:  // 下
            newBoard = execute_move_1(board);
            break;
        case 2:  // 左
            newBoard = execute_move_2(board);
            break;
        case 3:  // 右
            newBoard = execute_move_3(board);
            break;
        default:
            break;
    }

    // 如果移动无效，返回最低分
    if (newBoard == board) {
        return -1000000;
    }

    // 1. 计算移动后的空格数
    int emptyTiles = countEmptyTiles(newBoard);
    int moveScore  = emptyTiles * 10;

    // 2. 评估移动后的棋盘状态
    moveScore += evaluateBitBoard(newBoard);

    // 3. 如果是向上或向左移动，给予小幅奖励
    // 这有助于保持大数字在角落
    if (direction == 0 || direction == 2) {
        moveScore += 5;
    }

    return moveScore;
}

// 将标准棋盘转换为位棋盘 - 安全版本
BitBoard Auto::convertToBitBoard(QVector<QVector<int>> const& boardState) {
    BitBoard board = 0ULL;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int val      = boardState[i][j];
            uint64_t pos = 0;

            // 计算位置的对数值
            if (val > 0) {
                pos = static_cast<uint64_t>(log2(static_cast<double>(val)));
            }

            // 安全检查
            if (pos > 15) {
                pos = 15;  // 限制最大值为15，避免溢出
            }

            // 将值放入位棋盘
            uint64_t shift  = static_cast<uint64_t>((i * 4 + j) * 4);
            board          |= (pos << shift);
        }
    }

    return board;
}

// 将位棋盘转换回标准棋盘 - 安全版本
QVector<QVector<int>> Auto::convertFromBitBoard(BitBoard board) {
    QVector<QVector<int>> result(4, QVector<int>(4, 0));

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            uint64_t pos = static_cast<uint64_t>((i * 4 + j) * 4);
            uint64_t val = (board >> pos) & 0xfULL;

            // 将对数值转换回实际值
            if (val > 0) {
                // 防止溢出
                if (val < 31) {  // 限制最大值，避免移位过多
                    result[i][j] = 1 << static_cast<int>(val);
                } else {
                    result[i][j] = 2147483647;  // INT_MAX
                }
            } else {
                result[i][j] = 0;
            }
        }
    }

    return result;
}

// 评估位棋盘 - 高效版本
int Auto::evaluateBitBoard(BitBoard board) {
    // 使用预编译的表格进行评估
    int score = 0;

    // 直接使用位操作提取行数据并访问评分表
    // 使用内联定义避免函数调用开销
    score += BitBoardTables::SCORE_TABLE[static_cast<uint16_t>((board >> 0ULL) & 0xFF'FFULL)];
    score += BitBoardTables::SCORE_TABLE[static_cast<uint16_t>((board >> 16ULL) & 0xFF'FFULL)];
    score += BitBoardTables::SCORE_TABLE[static_cast<uint16_t>((board >> 32ULL) & 0xFF'FFULL)];
    score += BitBoardTables::SCORE_TABLE[static_cast<uint16_t>((board >> 48ULL) & 0xFF'FFULL)];

    // 空格数量奖励 - 使用位计数技术直接计算
    // 创建一个掩码，将每个单元的最低位设为1
    BitBoard mask  = board;
    mask          |= (mask >> 1);
    mask          |= (mask >> 2);
    mask          |= (mask >> 3);

    // 只保留每个单元的最低位
    mask &= 0x11'11'11'11'11'11'11'11ULL;

    // 计算有多少个非零单元
    mask = (mask & 0x55'55'55'55'55'55'55'55ULL) + ((mask >> 1) & 0x55'55'55'55'55'55'55'55ULL);
    mask = (mask & 0x33'33'33'33'33'33'33'33ULL) + ((mask >> 2) & 0x33'33'33'33'33'33'33'33ULL);
    mask = (mask & 0x0f'0f'0f'0f'0f'0f'0f'0fULL) + ((mask >> 4) & 0x0f'0f'0f'0f'0f'0f'0f'0fULL);
    mask = (mask & 0x00'ff'00'ff'00'ff'00'ffULL) + ((mask >> 8) & 0x00'ff'00'ff'00'ff'00'ffULL);
    mask = (mask & 0x00'00'ff'ff'00'00'ff'ffULL) + ((mask >> 16) & 0x00'00'ff'ff'00'00'ff'ffULL);
    mask = (mask & 0x00'00'00'00'ff'ff'ff'ffULL) + ((mask >> 32) & 0x00'00'00'00'ff'ff'ff'ffULL);

    // 空格数 = 总单元数 - 非空单元数
    int emptyTiles = 16 - static_cast<int>(mask);

    // 空格奖励
    score += emptyTiles * 10;

    return score;
}

// 模拟移动位棋盘 - 使用直接位操作以提高性能 - 安全版本
bool Auto::simulateMoveBitBoard(BitBoard& board, int direction, int& score) {
    // 保存原始棋盘状态以进行比较
    BitBoard oldBoard = board;
    score             = 0;

    // 验证方向参数
    if (direction < 0 || direction > 3) {
        return false;
    }

    // 使用直接位操作执行移动
    switch (direction) {
        case 0:  // 上
            board = execute_move_0(board);
            break;
        case 1:  // 下
            board = execute_move_1(board);
            break;
        case 2:  // 左
            board = execute_move_2(board);
            break;
        case 3:  // 右
            board = execute_move_3(board);
            break;
        default:
            return false;
    }

    // 如果棋盘没有变化，说明这个方向不能移动
    if (board == oldBoard) {
        return false;
    }

    // 简化的分数计算 - 使用安全的方式计算分数
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            // 使用int而非uint64_t来避免不必要的转换
            int pos = (i * 4 + j) * 4;
            // 确保位移位数在合理范围内
            if (pos >= 0 && pos < 64) {
                uint64_t oldVal = (oldBoard >> pos) & 0xFULL;
                uint64_t newVal = (board >> pos) & 0xFULL;

                // 如果新值大于旧值，说明发生了合并
                if (newVal > oldVal && newVal > 1) {
                    // 安全地计算分数，防止移位过多
                    if (newVal < 31) {  // 防止移位过多导致溢出
                        score += (1 << static_cast<int>(newVal));
                    } else {
                        score += 2147483647;  // INT_MAX
                    }
                }
            }
        }
    }

    return true;
}

// 使用位棋盘的expectimax算法 - 带启发式剪枝版本
int Auto::expectimaxBitBoard(BitBoard board, int depth, bool isMaxPlayer, int alpha, int beta) {
    // 防止栈溢出的静态计数器
    static thread_local int recursionDepth = 0;

    // 递归深度计数
    recursionDepth++;

    // 如果递归过深，直接返回评估分数
    if (recursionDepth > 20) {  // 绝对最大递归深度
        recursionDepth--;
        return evaluateBitBoard(board);
    }

    // 绝对深度限制 - 防止过深递归
    static int const MAX_ABSOLUTE_DEPTH = 3;  // 降低最大深度以提高性能和防止栈溢出
    depth                               = std::min(depth, MAX_ABSOLUTE_DEPTH);

    // 如果到达最大深度，返回评估分数
    if (depth <= 0) {
        // 直接使用位棋盘评估函数
        int score = evaluateBitBoard(board);
        recursionDepth--;
        return score;
    }

    // 检测游戏是否结束
    if (isGameOverBitBoard(board)) {
        int score = -500000;  // 游戏结束给予大量惩罚
        recursionDepth--;
        return score;
    }

    // 快速检测空格数
    int emptyCount = countEmptyTiles(board);

    // 获取最大值 - 安全版本
    int maxValue = 0;
    for (int i = 0; i < 16; i++) {
        uint64_t shift = static_cast<uint64_t>(i * 4);
        uint64_t value = (board >> shift) & 0xFULL;
        maxValue       = std::max(maxValue, static_cast<int>(value));
    }

    // 将对数值转换为实际值
    if (maxValue > 0 && maxValue < 31) {  // 防止溢出
        maxValue = 1 << maxValue;
    } else if (maxValue >= 31) {
        maxValue = 2147483647;  // INT_MAX
    } else {
        maxValue = 0;
    }

    // 根据棋盘复杂度动态调整搜索深度 - 但不要过深
    int extraDepth = 0;

    // 简化的动态深度调整，防止搜索过深
    if (emptyCount <= 4) {
        extraDepth = 1;  // 当空格很少时增加搜索深度
    }

    int result = 0;
    if (isMaxPlayer) {
        // MAX节点：选择最佳移动
        int bestScore = alpha;  // 使用alpha作为初始最佳分数

        // 对移动进行排序，优先探索更有希望的移动
        std::array<std::pair<int, int>, 4> moveScores;
        for (int direction = 0; direction < 4; ++direction) {
            moveScores[direction] = {direction, calculateMoveHeuristic(board, direction)};
        }

        // 按启发式评分从高到低排序
        std::sort(moveScores.begin(), moveScores.end(), [](std::pair<int, int> const& a, std::pair<int, int> const& b) {
            return a.second > b.second;
        });

        // 尝试所有可能的移动（按排序后的顺序）
        for (auto const& [direction, heuristic] : moveScores) {
            // 如果启发式评分很低，说明这个移动无效，跳过
            if (heuristic == -1000000) {
                continue;
            }

            BitBoard boardCopy = board;
            int moveScore      = 0;

            bool moved = simulateMoveBitBoard(boardCopy, direction, moveScore);

            if (moved) {
                // 递归计算期望分数
                int score = moveScore + expectimaxBitBoard(boardCopy, depth - 1 + extraDepth, false, bestScore, beta);

                // 更新最佳分数
                if (score > bestScore) {
                    bestScore = score;

                    // Beta剪枝：如果当前分数已经超过beta，则可以提前终止
                    // 这在Expectimax中不是严格的Alpha-Beta剪枝，但可以减少一些不必要的搜索
                    if (bestScore >= beta) {
                        break;
                    }
                }
            }
        }

        result = bestScore > 0 ? bestScore : 0;
    } else {
        // CHANCE节点：随机生成新方块
        // 如果没有空格，返回评估分数
        if (emptyCount == 0) {
            int score = evaluateBitBoard(board);
            recursionDepth--;
            return score;
        }

        // 减少模拟的空格数量，提高性能
        // 当空格过多时，只模拟一部分空格
        int tilesToSimulate = std::min(2, emptyCount);

        // 如果空格过多，可以进一步减少模拟数量
        if (emptyCount > 6 && depth > 1) {
            tilesToSimulate = 1;  // 在深度较大且空格较多时只模拟一个空格
        }

        double totalScore = 0.0;

        // 找到所有空格位置
        QVector<QPair<int, int>> emptyPositions;
        // 直接从位棋盘获取空格位置
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                uint64_t pos = static_cast<uint64_t>((i * 4 + j) * 4);
                uint64_t val = (board >> pos) & 0xFULL;
                if (val == 0) {
                    emptyPositions.append(qMakePair(i, j));
                    if (emptyPositions.size() >= tilesToSimulate) {
                        break;
                    }
                }
            }
            if (emptyPositions.size() >= tilesToSimulate) {
                break;
            }
        }

        // 模拟在空格中放置新方块
        for (int i = 0; i < tilesToSimulate; ++i) {
            int row = emptyPositions[i].first;
            int col = emptyPositions[i].second;

            // 直接在位棋盘上模拟生成2
            BitBoard bitBoardWith2 = board;
            int pos2               = (row * 4 + col) * 4;
            // 确保位移位数在合理范围内
            if (pos2 >= 0 && pos2 < 64) {
                // 清零该位置然后设置为2（1对应于位棋盘中的2）
                bitBoardWith2 &= ~(0xfULL << pos2);
                bitBoardWith2 |= 1ULL << pos2;

                // 简化计算，只考虑生成2的情况，提高性能
                // 对于概率节点，使用当前的alpha和beta值
                totalScore += expectimaxBitBoard(bitBoardWith2, depth - 1 + extraDepth, true, alpha, beta);
            }
        }

        result = static_cast<int>(totalScore / tilesToSimulate);
    }

    // 递归计数器减少
    recursionDepth--;
    return result;
}

// 带剪枝的Expectimax算法的入口点
int Auto::expectimaxBitBoardWithPruning(BitBoard board, int depth, bool isMaxPlayer) {
    return expectimaxBitBoard(board, depth, isMaxPlayer, -1000000, 1000000);
}

// 使用位棋盘优化的getBestMove函数
int Auto::getBestMoveBitBoard(QVector<QVector<int>> const& boardState) {
    // 转换为位棋盘
    BitBoard board = convertToBitBoard(boardState);

    int bestMove  = -1;
    int bestScore = -1;

    // 对移动进行排序，优先探索更有希望的移动
    std::array<std::pair<int, int>, 4> moveScores;
    for (int direction = 0; direction < 4; ++direction) {
        moveScores[direction] = {direction, calculateMoveHeuristic(board, direction)};
    }

    // 按启发式评分从高到低排序
    std::sort(moveScores.begin(), moveScores.end(), [](std::pair<int, int> const& a, std::pair<int, int> const& b) {
        return a.second > b.second;
    });

    // 存储所有有效移动
    QVector<int> validMoves;

    // 尝试所有可能的移动（按排序后的顺序）
    for (auto const& [direction, heuristic] : moveScores) {
        // 如果启发式评分很低，说明这个移动无效，跳过
        if (heuristic == -1000000) {
            continue;
        }

        BitBoard boardCopy = board;
        int moveScore      = 0;

        bool moved = simulateMoveBitBoard(boardCopy, direction, moveScore);

        if (moved) {
            // 记录有效移动
            validMoves.append(direction);

            // 计算此移动的分数 - 使用带剪枝的算法
            int score = moveScore + expectimaxBitBoardWithPruning(boardCopy, 3, false);

            if (score > bestScore) {
                bestScore = score;
                bestMove  = direction;
            }
        }
    }

    // 如果没有有效移动，随机选择一个方向
    if (bestMove == -1) {
        if (!validMoves.isEmpty()) {
            // 如果有有效移动，随机选择一个
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, validMoves.size() - 1);
            bestMove = validMoves[dis(gen)];
        } else {
            // 如果没有有效移动，随机选择一个方向
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 3);
            bestMove = dis(gen);
        }
    }

    return bestMove;
}

// expectimax: 期望最大算法 - 高效版本
int Auto::expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer) {
    // 绝对深度限制 - 防止过深递归
    static int const MAX_ABSOLUTE_DEPTH = 5;  // 降低最大深度以提高性能
    depth                               = std::min(depth, MAX_ABSOLUTE_DEPTH);

    // 如果到达最大深度，返回评估分数
    if (depth <= 0) {
        int score = evaluateAdvancedPattern(boardState);
        return score;
    }

    // 检测游戏是否结束
    if (isGameOver(boardState)) {
        int score = -500000;  // 游戏结束给予大量惩罚
        return score;
    }

    // 快速检测最大值和空格数
    int maxValue   = 0;
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            maxValue = std::max(maxValue, boardState[i][j]);
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }

    // 根据棋盘状态动态调整搜索深度
    int extraDepth = 0;

    // 根据空格数量调整搜索深度
    if (emptyCount <= 4) {
        // 空格很少，需要更深的搜索
        extraDepth = 2;
    } else if (emptyCount <= 8) {
        // 空格适中，需要中等深度的搜索
        extraDepth = 1;
    }

    // 最大值越大，表示棋盘状态越复杂，需要更深的搜索
    if (maxValue >= 2048) {  // 2048 = 2^11 或更大
        extraDepth += 1;

        // 对于更高级的棋盘，再增加搜索深度
        if (maxValue >= 4096) {  // 4096 = 2^12
            extraDepth += 1;
        }
    }

    int result = 0;
    if (isMaxPlayer) {
        // MAX节点：选择最佳移动
        int bestScore = -1;

        // 尝试所有可能的移动
        for (int direction = 0; direction < 4; ++direction) {
            QVector<QVector<int>> boardCopy = boardState;
            int moveScore                   = 0;

            bool moved = simulateMove(boardCopy, direction, moveScore);

            if (moved) {
                // 递归计算期望分数
                int score = moveScore + expectimax(boardCopy, depth - 1 + extraDepth, false);
                bestScore = std::max(bestScore, score);
            }
        }

        result = bestScore > 0 ? bestScore : 0;
    } else {
        // CHANCE节点：随机生成新方块
        // 如果没有空格，返回评估分数
        if (emptyCount == 0) {
            int score = evaluateBoardAdvanced(boardState);
            return score;
        }

        // 始终模拟多个空格，保持算法一致性
        int tilesToSimulate = std::min(3, emptyCount);

        double totalScore = 0.0;

        // 优化空格选择策略
        QVector<QPair<int, int>> emptyPositions;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (boardState[i][j] == 0) {
                    emptyPositions.append(qMakePair(i, j));
                    if (emptyPositions.size() >= tilesToSimulate) {
                        break;
                    }
                }
            }
            if (emptyPositions.size() >= tilesToSimulate) {
                break;
            }
        }

        for (int i = 0; i < tilesToSimulate; ++i) {
            int row = emptyPositions[i].first;
            int col = emptyPositions[i].second;

            // 模拟生成2和4两种情况
            QVector<QVector<int>> boardWith2 = boardState;
            boardWith2[row][col]             = 2;

            // 始终考虑生成2和4的情况，保持算法一致性
            QVector<QVector<int>> boardWith4 = boardState;
            boardWith4[row][col]             = 4;

            // 90%概率生成2，10%概率生成4
            totalScore += 0.9 * expectimax(boardWith2, depth - 1 + extraDepth, true);
            totalScore += 0.1 * expectimax(boardWith4, depth - 1 + extraDepth, true);
        }

        result = static_cast<int>(totalScore / tilesToSimulate);
    }

    return result;
}

// evaluateWithParams: 使用给定参数评估棋盘
int Auto::evaluateWithParams(QVector<QVector<int>> const& boardState, QVector<double> const& params) {
    int score = 0;

    // 确保参数数量足够
    if (params.size() < 5) {
        return evaluateBoardAdvanced(boardState);  // 如果参数不足，使用默认评估函数
    }

    // 1. 空格数量权重 - 空格越多越好
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    score += static_cast<int>(emptyCount * params[0] * 10);

    // 2. 蛇形模式权重 - 使用改进的蛇形模式
    int const snakePattern[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};
    int snakeScore               = 0;

    // 找出最大值及其位置
    int maxValue = 0;
    int maxRow = 0, maxCol = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            maxValue = std::max(maxValue, boardState[i][j]);
            if (boardState[i][j] == maxValue) {
                maxRow = i;
                maxCol = j;
            }
        }
    }

    // 根据最大值的位置调整蛇形模式
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > 0) {
                // 对于高值方块，给予更高的权重
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                snakeScore       += static_cast<int>(tileValue * snakePattern[i][j] * (logValue / 11.0));
            }
        }
    }
    score += static_cast<int>(snakeScore * params[1] / 10);

    // 3. 平滑度权重 - 使用对数差异来减少大数值之间的差异惩罚
    int smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                // 使用对数差异
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i][j + 1] > 0 ? log2(boardState[i][j + 1]) : 0;
                smoothness     -= static_cast<int>(std::abs(logVal1 - logVal2) * 2.0);
            }
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                // 使用对数差异
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i + 1][j] > 0 ? log2(boardState[i + 1][j]) : 0;
                smoothness     -= static_cast<int>(std::abs(logVal1 - logVal2) * 2.0);
            }
        }
    }
    score += static_cast<int>(smoothness * params[2]);

    // 4. 单调性权重 - 改进的单调性计算
    int monotonicity = 0;

    // 计算行的单调性
    for (int i = 0; i < 4; ++i) {
        // 计算左右方向的单调性
        int left_right = 0;
        int right_left = 0;

        for (int j = 0; j < 3; ++j) {
            double curr = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            double next = boardState[i][j + 1] > 0 ? log2(boardState[i][j + 1]) : 0;

            if (curr > next) {
                left_right += static_cast<int>((curr - next) * 2);
            } else {
                right_left += static_cast<int>((next - curr) * 2);
            }
        }

        monotonicity += std::min(left_right, right_left);
    }

    // 计算列的单调性
    for (int j = 0; j < 4; ++j) {
        // 计算上下方向的单调性
        int up_down = 0;
        int down_up = 0;

        for (int i = 0; i < 3; ++i) {
            double curr = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            double next = boardState[i + 1][j] > 0 ? log2(boardState[i + 1][j]) : 0;

            if (curr > next) {
                up_down += static_cast<int>((curr - next) * 2);
            } else {
                down_up += static_cast<int>((next - curr) * 2);
            }
        }

        monotonicity += std::min(up_down, down_up);
    }

    score -= static_cast<int>(monotonicity * params[3]);

    // 5. 合并可能性权重 - 奖励相邻相同值
    double mergeScore  = calculateMergeScore(boardState);
    score             += static_cast<int>(mergeScore * params[4]);

    // 6. 角落奖励 - 如果最大值在角落，给予额外奖励
    if ((maxRow == 0 || maxRow == 3) && (maxCol == 0 || maxCol == 3)) {
        score += maxValue * 2;
    }

    return score;
}
