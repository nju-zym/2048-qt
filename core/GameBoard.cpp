#include "GameBoard.h"

// 检查游戏是否结束
bool GameBoard::isGameOver(QVector<QVector<int>> const& boardState) {
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

// 模拟移动
bool GameBoard::simulateMove(QVector<QVector<int>>& boardState, int direction, int& score) {
    bool moved = false;
    score      = 0;

    if (direction == UP) {  // 向上
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
    } else if (direction == RIGHT) {  // 向右
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
    } else if (direction == DOWN) {  // 向下
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
    } else if (direction == LEFT) {  // 向左
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

// 计算空格数量
int GameBoard::countEmptyCells(QVector<QVector<int>> const& boardState) {
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    return emptyCount;
}

// 获取最大的瓦片值
int GameBoard::getMaxTile(QVector<QVector<int>> const& boardState) {
    int maxValue = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
            }
        }
    }
    return maxValue;
}