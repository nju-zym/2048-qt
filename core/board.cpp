#include "board.h"

#include <QRandomGenerator>
#include <algorithm>

Board::Board() : board(4, QVector<int>(4, 0)) {
    // 构造函数，初始化为4x4的全零矩阵
}

Board::~Board() {
    // 析构函数
}

void Board::init() {
    // 初始化棋盘，重置所有状态为0
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            board[i][j] = 0;
        }
    }
}

QPair<QPair<int, int>, int> Board::generateNewTile() {
    // 生成新方块，返回生成的位置和值
    QVector<QPair<int, int>> emptyTiles = getEmptyTiles();

    // 如果没有空位，返回无效值
    if (emptyTiles.isEmpty()) {
        return qMakePair(qMakePair(-1, -1), 0);
    }

    // 随机选择一个空位
    int randomIndex = QRandomGenerator::global()->bounded(emptyTiles.size());
    int row         = emptyTiles[randomIndex].first;
    int col         = emptyTiles[randomIndex].second;

    // 随机生成2或4，90%概率生成2，10%概率生成4
    int newValue = (QRandomGenerator::global()->bounded(10) < 9) ? 2 : 4;

    // 设置新方块的值
    board[row][col] = newValue;

    // 返回位置和值
    return qMakePair(qMakePair(row, col), newValue);
}

QPair<bool, int> Board::move(int direction) {
    // 保存移动前的棋盘状态，用于比较是否发生变化
    QVector<QVector<int>> previousBoard = board;
    int scoreGained                     = 0;

    // 根据方向执行移动
    switch (direction) {
        case 0:  // 上
            scoreGained = moveUp().second;
            break;
        case 1:  // 右
            scoreGained = moveRight().second;
            break;
        case 2:  // 下
            scoreGained = moveDown().second;
            break;
        case 3:  // 左
            scoreGained = moveLeft().second;
            break;
    }

    // 检查棋盘是否发生变化
    bool changed = false;
    for (int i = 0; i < 4 && !changed; ++i) {
        for (int j = 0; j < 4 && !changed; ++j) {
            if (board[i][j] != previousBoard[i][j]) {
                changed = true;
            }
        }
    }

    // 返回是否变化和得分
    return qMakePair(changed, scoreGained);
}

QVector<QVector<int>> const& Board::getBoard() const {
    // 返回当前棋盘状态
    return board;
}

void Board::setBoard(QVector<QVector<int>> const& newBoard) {
    // 设置棋盘状态
    if (newBoard.size() == 4 && newBoard[0].size() == 4) {
        board = newBoard;
    }
}

QVector<QPair<int, int>> Board::getEmptyTiles() const {
    QVector<QPair<int, int>> emptyTiles;

    // 遍历棋盘找出所有空位
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] == 0) {
                emptyTiles.append(qMakePair(i, j));
            }
        }
    }

    return emptyTiles;
}

bool Board::isGameOver() const {
    // 如果还有空位，游戏未结束
    if (!getEmptyTiles().isEmpty()) {
        return false;
    }

    // 如果水平相邻有相同数字，游戏未结束
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == board[i][j + 1] && board[i][j] != 0) {
                return false;
            }
        }
    }

    // 如果垂直相邻有相同数字，游戏未结束
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (board[i][j] == board[i + 1][j] && board[i][j] != 0) {
                return false;
            }
        }
    }

    // 否则游戏结束
    return true;
}

bool Board::isGameWon() const {
    // 如果棋盘上有2048，游戏胜利
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] == 2048) {
                return true;
            }
        }
    }

    return false;
}

int Board::getTileValue(int row, int col) const {
    // 获取指定位置的值，确保索引有效
    if (row >= 0 && row < 4 && col >= 0 && col < 4) {
        return board[row][col];
    }
    return 0;
}

bool Board::isTileEmpty(int row, int col) const {
    // 检查指定位置是否为空
    return getTileValue(row, col) == 0;
}

QPair<bool, int> Board::moveUp() {
    bool changed    = false;
    int scoreGained = 0;

    for (int col = 0; col < 4; col++) {
        int writePos = 0;

        // 第一步：将非零方块上移，消除中间空隙
        for (int row = 0; row < 4; row++) {
            if (board[row][col] != 0) {
                if (row != writePos) {
                    board[writePos][col] = board[row][col];
                    board[row][col]      = 0;
                    changed              = true;
                }
                writePos++;
            }
        }

        // 第二步：合并相邻且相同的方块
        for (int row = 0; row < 3; row++) {
            if (board[row][col] != 0 && board[row][col] == board[row + 1][col]) {
                board[row][col]     *= 2;
                scoreGained         += board[row][col];
                board[row + 1][col]  = 0;
                changed              = true;
            }
        }

        // 第三步：再次上移以消除合并后产生的空隙
        writePos = 0;
        for (int row = 0; row < 4; row++) {
            if (board[row][col] != 0) {
                if (row != writePos) {
                    board[writePos][col] = board[row][col];
                    board[row][col]      = 0;
                    changed              = true;
                }
                writePos++;
            }
        }
    }

    return qMakePair(changed, scoreGained);
}

QPair<bool, int> Board::moveRight() {
    bool changed    = false;
    int scoreGained = 0;

    for (int row = 0; row < 4; row++) {
        int writePos = 3;

        // 将非零方块向右移，消除中间空隙
        for (int col = 3; col >= 0; col--) {
            if (board[row][col] != 0) {
                if (col != writePos) {
                    board[row][writePos] = board[row][col];
                    board[row][col]      = 0;
                    changed              = true;
                }
                writePos--;
            }
        }

        // 合并相邻且相同的方块
        for (int col = 3; col > 0; col--) {
            if (board[row][col] != 0 && board[row][col] == board[row][col - 1]) {
                board[row][col]     *= 2;
                scoreGained         += board[row][col];
                board[row][col - 1]  = 0;
                changed              = true;
            }
        }

        // 再次向右移动以消除合并后产生的空隙
        writePos = 3;
        for (int col = 3; col >= 0; col--) {
            if (board[row][col] != 0) {
                if (col != writePos) {
                    board[row][writePos] = board[row][col];
                    board[row][col]      = 0;
                    changed              = true;
                }
                writePos--;
            }
        }
    }

    return qMakePair(changed, scoreGained);
}

QPair<bool, int> Board::moveDown() {
    bool changed    = false;
    int scoreGained = 0;

    for (int col = 0; col < 4; col++) {
        int writePos = 3;

        // 将非零方块向下移，消除中间空隙
        for (int row = 3; row >= 0; row--) {
            if (board[row][col] != 0) {
                if (row != writePos) {
                    board[writePos][col] = board[row][col];
                    board[row][col]      = 0;
                    changed              = true;
                }
                writePos--;
            }
        }

        // 合并相邻且相同的方块
        for (int row = 3; row > 0; row--) {
            if (board[row][col] != 0 && board[row][col] == board[row - 1][col]) {
                board[row][col]     *= 2;
                scoreGained         += board[row][col];
                board[row - 1][col]  = 0;
                changed              = true;
            }
        }

        // 再次向下移动以消除合并后产生的空隙
        writePos = 3;
        for (int row = 3; row >= 0; row--) {
            if (board[row][col] != 0) {
                if (row != writePos) {
                    board[writePos][col] = board[row][col];
                    board[row][col]      = 0;
                    changed              = true;
                }
                writePos--;
            }
        }
    }

    return qMakePair(changed, scoreGained);
}

QPair<bool, int> Board::moveLeft() {
    bool changed    = false;
    int scoreGained = 0;

    for (int row = 0; row < 4; row++) {
        int writePos = 0;

        // 将非零方块向左移，消除中间空隙
        for (int col = 0; col < 4; col++) {
            if (board[row][col] != 0) {
                if (col != writePos) {
                    board[row][writePos] = board[row][col];
                    board[row][col]      = 0;
                    changed              = true;
                }
                writePos++;
            }
        }

        // 合并相邻且相同的方块
        for (int col = 0; col < 3; col++) {
            if (board[row][col] != 0 && board[row][col] == board[row][col + 1]) {
                board[row][col]     *= 2;
                scoreGained         += board[row][col];
                board[row][col + 1]  = 0;
                changed              = true;
            }
        }

        // 再次向左移动以消除合并后产生的空隙
        writePos = 0;
        for (int col = 0; col < 4; col++) {
            if (board[row][col] != 0) {
                if (col != writePos) {
                    board[row][writePos] = board[row][col];
                    board[row][col]      = 0;
                    changed              = true;
                }
                writePos++;
            }
        }
    }

    return qMakePair(changed, scoreGained);
}