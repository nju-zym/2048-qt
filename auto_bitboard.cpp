#include "auto.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QRandomGenerator>
#include <QTextStream>
#include <QTime>
#include <cmath>
#include <iostream>

// 预计算的移动表
static uint16_t row_left_table[65536];
static uint16_t row_right_table[65536];
static uint64_t col_up_table[65536];
static uint64_t col_down_table[65536];

// 预计算的评分表
static float heur_score_table[65536];
static float score_table[65536];

// 常量定义
static const uint64_t ROW_MASK = 0xFFFFULL;
static const uint64_t COL_MASK = 0x000F000F000F000FULL;

// 转置棋盘
static inline BitBoard transpose(BitBoard x) {
    BitBoard a1 = x & 0xF0F00F0FF0F00F0FULL;
    BitBoard a2 = x & 0x0000F0F00000F0F0ULL;
    BitBoard a3 = x & 0x0F0F00000F0F0000ULL;
    BitBoard a = a1 | (a2 << 12) | (a3 >> 12);
    BitBoard b1 = a & 0xFF00FF0000FF00FFULL;
    BitBoard b2 = a & 0x00FF00FF00000000ULL;
    BitBoard b3 = a & 0x00000000FF00FF00ULL;
    return b1 | (b2 >> 24) | (b3 << 24);
}

// 反转一行
static inline uint16_t reverse_row(uint16_t row) {
    return (row >> 12) | ((row >> 4) & 0x00F0) | ((row << 4) & 0x0F00) | (row << 12);
}

// 将列解包为位棋盘
static inline BitBoard unpack_col(uint16_t row) {
    BitBoard tmp = row;
    return (tmp | (tmp << 12ULL) | (tmp << 24ULL) | (tmp << 36ULL)) & COL_MASK;
}

// 初始化预计算表
void Auto::initTables() {
    // 初始化移动和评分表
    for (unsigned row = 0; row < 65536; ++row) {
        unsigned line[4] = {
            (row >> 0) & 0xf,
            (row >> 4) & 0xf,
            (row >> 8) & 0xf,
            (row >> 12) & 0xf
        };

        // 计算分数
        float score = 0.0f;
        for (int i = 0; i < 4; ++i) {
            int rank = line[i];
            if (rank >= 2) {
                // 分数是方块值和所有中间合并方块的总和
                score += (rank - 1) * (1 << rank);
            }
        }
        score_table[row] = score;

        // 启发式评分
        float sum = 0;
        int empty = 0;
        int merges = 0;
        int prev = 0;
        int counter = 0;

        for (int i = 0; i < 4; ++i) {
            int rank = line[i];
            sum += pow(rank, 3.5);
            if (rank == 0) {
                empty++;
            } else {
                if (prev == rank) {
                    counter++;
                } else if (counter > 0) {
                    merges += 1 + counter;
                    counter = 0;
                }
                prev = rank;
            }
        }
        if (counter > 0) {
            merges += 1 + counter;
        }

        float monotonicity_left = 0;
        float monotonicity_right = 0;
        for (int i = 1; i < 4; ++i) {
            if (line[i-1] > line[i]) {
                monotonicity_left += pow(line[i-1], 4) - pow(line[i], 4);
            } else {
                monotonicity_right += pow(line[i], 4) - pow(line[i-1], 4);
            }
        }

        heur_score_table[row] = 200000.0f +
                               270.0f * empty +
                               700.0f * merges -
                               47.0f * std::min(monotonicity_left, monotonicity_right) -
                               11.0f * sum;

        // 执行向左移动
        for (int i = 0; i < 3; ++i) {
            int j;
            for (j = i + 1; j < 4; ++j) {
                if (line[j] != 0) break;
            }
            if (j == 4) break; // 右侧没有更多方块

            if (line[i] == 0) {
                line[i] = line[j];
                line[j] = 0;
                i--; // 重试此条目
            } else if (line[i] == line[j]) {
                if(line[i] != 0xf) {
                    // 假设 32768 + 32768 = 32768（表示限制）
                    line[i]++;
                }
                line[j] = 0;
            }
        }

        uint16_t result = (line[0] << 0) |
                         (line[1] << 4) |
                         (line[2] << 8) |
                         (line[3] << 12);
        uint16_t rev_result = reverse_row(result);
        unsigned rev_row = reverse_row(row);

        row_left_table[row] = row ^ result;
        row_right_table[rev_row] = rev_row ^ rev_result;
        col_up_table[row] = unpack_col(row) ^ unpack_col(result);
        col_down_table[rev_row] = unpack_col(rev_row) ^ unpack_col(rev_result);
    }
}

// 执行向上移动
static inline BitBoard execute_move_0(BitBoard board) {
    BitBoard ret = board;
    BitBoard t = transpose(board);
    ret ^= col_up_table[(t >> 0) & ROW_MASK] << 0;
    ret ^= col_up_table[(t >> 16) & ROW_MASK] << 4;
    ret ^= col_up_table[(t >> 32) & ROW_MASK] << 8;
    ret ^= col_up_table[(t >> 48) & ROW_MASK] << 12;
    return ret;
}

// 执行向下移动
static inline BitBoard execute_move_1(BitBoard board) {
    BitBoard ret = board;
    BitBoard t = transpose(board);
    ret ^= col_down_table[(t >> 0) & ROW_MASK] << 0;
    ret ^= col_down_table[(t >> 16) & ROW_MASK] << 4;
    ret ^= col_down_table[(t >> 32) & ROW_MASK] << 8;
    ret ^= col_down_table[(t >> 48) & ROW_MASK] << 12;
    return ret;
}

// 执行向左移动
static inline BitBoard execute_move_2(BitBoard board) {
    BitBoard ret = board;
    ret ^= BitBoard(row_left_table[(board >> 0) & ROW_MASK]) << 0;
    ret ^= BitBoard(row_left_table[(board >> 16) & ROW_MASK]) << 16;
    ret ^= BitBoard(row_left_table[(board >> 32) & ROW_MASK]) << 32;
    ret ^= BitBoard(row_left_table[(board >> 48) & ROW_MASK]) << 48;
    return ret;
}

// 执行向右移动
static inline BitBoard execute_move_3(BitBoard board) {
    BitBoard ret = board;
    ret ^= BitBoard(row_right_table[(board >> 0) & ROW_MASK]) << 0;
    ret ^= BitBoard(row_right_table[(board >> 16) & ROW_MASK]) << 16;
    ret ^= BitBoard(row_right_table[(board >> 32) & ROW_MASK]) << 32;
    ret ^= BitBoard(row_right_table[(board >> 48) & ROW_MASK]) << 48;
    return ret;
}

// 执行移动
bool Auto::simulateMoveBitBoard(BitBoard& board, int direction, int& score) {
    BitBoard newboard;
    
    switch(direction) {
        case 0: // 上
            newboard = execute_move_0(board);
            break;
        case 1: // 下
            newboard = execute_move_1(board);
            break;
        case 2: // 左
            newboard = execute_move_2(board);
            break;
        case 3: // 右
            newboard = execute_move_3(board);
            break;
        default:
            return false;
    }
    
    if (newboard == board) {
        return false;
    }
    
    // 计算分数
    score = 0;
    BitBoard diff = board ^ newboard;
    BitBoard temp = diff;
    while (temp) {
        int pos = __builtin_ctzll(temp) / 4 * 4;
        int val = (newboard >> pos) & 0xf;
        if (val > 1) { // 如果发生了合并
            score += (1 << val);
        }
        temp &= ~(0xfULL << pos);
    }
    
    board = newboard;
    return true;
}

// 计算空白格子数
int Auto::countEmptyTiles(BitBoard board) {
    board |= (board >> 2) & 0x3333333333333333ULL;
    board |= (board >> 1);
    board = ~board & 0x1111111111111111ULL;
    
    // 此时每个半字节是：
    // 0 如果原始半字节非零
    // 1 如果原始半字节为零
    // 现在将它们全部相加
    
    board += board >> 32;
    board += board >> 16;
    board += board >> 8;
    board += board >> 4; // 如果有16个空位，这可能会溢出到下一个半字节
    
    return board & 0xf;
}

// 检查游戏是否结束
static bool isGameOverBitBoard(BitBoard board) {
    // 如果有空格，游戏未结束
    if ((board & 0xf) == 0 ||
        (board & 0xf0) == 0 ||
        (board & 0xf00) == 0 ||
        (board & 0xf000) == 0 ||
        (board & 0xf0000) == 0 ||
        (board & 0xf00000) == 0 ||
        (board & 0xf000000) == 0 ||
        (board & 0xf0000000) == 0 ||
        (board & 0xf00000000) == 0 ||
        (board & 0xf000000000) == 0 ||
        (board & 0xf0000000000) == 0 ||
        (board & 0xf00000000000) == 0 ||
        (board & 0xf000000000000) == 0 ||
        (board & 0xf0000000000000) == 0 ||
        (board & 0xf00000000000000) == 0 ||
        (board & 0xf000000000000000) == 0) {
        return false;
    }
    
    // 检查是否可以合并
    BitBoard row_mask = 0xFFFFULL;
    for (int i = 0; i < 4; ++i) {
        uint16_t row = (board >> (i * 16)) & row_mask;
        for (int j = 0; j < 3; ++j) {
            if (((row >> (j * 4)) & 0xf) == ((row >> ((j + 1) * 4)) & 0xf)) {
                return false;
            }
        }
    }
    
    // 检查列
    BitBoard t = transpose(board);
    for (int i = 0; i < 4; ++i) {
        uint16_t row = (t >> (i * 16)) & row_mask;
        for (int j = 0; j < 3; ++j) {
            if (((row >> (j * 4)) & 0xf) == ((row >> ((j + 1) * 4)) & 0xf)) {
                return false;
            }
        }
    }
    
    return true;
}

// 评估位棋盘
int Auto::evaluateBitBoard(BitBoard board) {
    // 使用预计算的表格进行评估
    float score = 0.0f;
    
    // 行评估
    score += heur_score_table[(board >> 0) & ROW_MASK];
    score += heur_score_table[(board >> 16) & ROW_MASK];
    score += heur_score_table[(board >> 32) & ROW_MASK];
    score += heur_score_table[(board >> 48) & ROW_MASK];
    
    // 列评估（转置后）
    BitBoard t = transpose(board);
    score += heur_score_table[(t >> 0) & ROW_MASK];
    score += heur_score_table[(t >> 16) & ROW_MASK];
    score += heur_score_table[(t >> 32) & ROW_MASK];
    score += heur_score_table[(t >> 48) & ROW_MASK];
    
    return static_cast<int>(score);
}

// 将标准棋盘转换为位棋盘
BitBoard Auto::convertToBitBoard(QVector<QVector<int>> const& boardState) {
    BitBoard board = 0;
    
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int val = boardState[i][j];
            int pos = 0;
            
            // 计算位置的对数值
            if (val > 0) {
                pos = static_cast<int>(log2(val));
            }
            
            // 将值放入位棋盘
            board |= (BitBoard)(pos) << ((i * 4 + j) * 4);
        }
    }
    
    return board;
}

// 将位棋盘转换回标准棋盘
QVector<QVector<int>> Auto::convertFromBitBoard(BitBoard board) {
    QVector<QVector<int>> result(4, QVector<int>(4, 0));
    
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int pos = (i * 4 + j) * 4;
            int val = (board >> pos) & 0xf;
            
            // 将对数值转换回实际值
            if (val > 0) {
                result[i][j] = 1 << val;
            } else {
                result[i][j] = 0;
            }
        }
    }
    
    return result;
}

// 使用位棋盘的expectimax算法
int Auto::expectimaxBitBoard(BitBoard board, int depth, bool isMaxPlayer) {
    // 检查缓存
    BitBoardState state{board, depth, isMaxPlayer};
    auto cacheIt = bitboardCache.find(state);
    if (cacheIt != bitboardCache.end()) {
        return cacheIt->second;
    }
    
    // 绝对深度限制 - 防止过深递归
    static int const MAX_ABSOLUTE_DEPTH = 5; // 降低最大深度以提高性能
    if (depth > MAX_ABSOLUTE_DEPTH) {
        depth = MAX_ABSOLUTE_DEPTH;
    }
    
    // 如果到达最大深度，返回评估分数
    if (depth <= 0) {
        int score = evaluateBitBoard(board);
        bitboardCache[state] = score; // 缓存结果
        return score;
    }

    // 检测游戏是否结束
    if (isGameOverBitBoard(board)) {
        int score = -500000;  // 游戏结束给予大量惩罚
        bitboardCache[state] = score;
        return score;
    }

    // 快速检测最大值和空格数
    int maxValue = 0;
    int emptyCount = countEmptyTiles(board);
    
    // 获取棋盘上的最大值
    BitBoard temp = board;
    for (int i = 0; i < 16; ++i) {
        int val = temp & 0xf;
        if (val > maxValue) {
            maxValue = val;
        }
        temp >>= 4;
    }

    // 对于高级棋盘，减少搜索深度以提高性能
    int extraDepth = 0;
    if (maxValue >= 11) { // 2048 = 2^11
        // 对于高级棋盘，根据空格数量动态调整搜索深度
        if (emptyCount <= 4) {
            depth = std::min(depth, 3); // 空格很少时限制深度
        } else {
            extraDepth = 1; // 空格较多时增加深度
        }
    }

    int result = 0;
    if (isMaxPlayer) {
        // MAX节点：选择最佳移动
        int bestScore = -1;

        // 尝试所有可能的移动
        for (int direction = 0; direction < 4; ++direction) {
            BitBoard boardCopy = board;
            int moveScore = 0;

            bool moved = simulateMoveBitBoard(boardCopy, direction, moveScore);

            if (moved) {
                // 递归计算期望分数
                int score = moveScore + expectimaxBitBoard(boardCopy, depth - 1 + extraDepth, false);
                bestScore = std::max(bestScore, score);
            }
        }

        result = bestScore > 0 ? bestScore : 0;
    } else {
        // CHANCE节点：随机生成新方块
        // 如果没有空格，返回评估分数
        if (emptyCount == 0) {
            int score = evaluateBitBoard(board);
            bitboardCache[state] = score;
            return score;
        }

        // 优化：对于高级棋盘，只模拟一个空格以提高性能
        int tilesToSimulate = 1;
        if (maxValue < 11 && emptyCount > 1) {
            tilesToSimulate = std::min(2, emptyCount);
        }
        
        double totalScore = 0.0;
        
        // 找到所有空格位置
        QVector<int> emptyPositions;
        BitBoard tempBoard = board;
        for (int i = 0; i < 16; ++i) {
            if ((tempBoard & 0xf) == 0) {
                emptyPositions.append(i);
                if (emptyPositions.size() >= tilesToSimulate) {
                    break;
                }
            }
            tempBoard >>= 4;
        }
        
        // 模拟在空格中放置新方块
        for (int i = 0; i < tilesToSimulate; ++i) {
            int pos = emptyPositions[i] * 4;
            
            // 模拟生成2和4两种情况
            BitBoard boardWith2 = board | (BitBoard(1) << pos);
            
            // 优化：对于高级棋盘，只考虑生成2的情况
            if (maxValue >= 12) { // 4096 = 2^12
                totalScore += expectimaxBitBoard(boardWith2, depth - 1, true);
            } else {
                BitBoard boardWith4 = board | (BitBoard(2) << pos);
                
                totalScore += 0.9 * expectimaxBitBoard(boardWith2, depth - 1, true);  // 90%概率生成2
                totalScore += 0.1 * expectimaxBitBoard(boardWith4, depth - 1, true);  // 10%概率生成4
            }
        }
        
        result = static_cast<int>(totalScore / tilesToSimulate);
    }
    
    // 缓存结果
    bitboardCache[state] = result;
    
    // 限制缓存大小以防止内存溢出
    if (bitboardCache.size() > 10000) {
        // 当缓存过大时清除
        bitboardCache.clear();
    }
    
    return result;
}

// 使用位棋盘优化的getBestMove函数
int Auto::getBestMoveBitBoard(QVector<QVector<int>> const& boardState) {
    // 转换为位棋盘
    BitBoard board = convertToBitBoard(boardState);
    
    int bestMove = -1;
    int bestScore = -1;
    
    // 尝试所有可能的移动
    for (int move = 0; move < 4; ++move) {
        BitBoard boardCopy = board;
        int moveScore = 0;
        
        bool moved = simulateMoveBitBoard(boardCopy, move, moveScore);
        
        if (moved) {
            // 计算此移动的分数
            int score = moveScore + expectimaxBitBoard(boardCopy, 3, false);
            
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
        }
    }
    
    return bestMove;
}
