#include "GameBoard.h"

#include <QRandomGenerator>
#include <cmath>

GameBoard::GameBoard(int size) : size(size), score(0) {
    reset();
}

void GameBoard::reset() {
    // 初始化棋盘，全部设为0
    board.clear();
    board.resize(size);
    for (int i = 0; i < size; ++i) {
        board[i].resize(size, 0);
    }
    score = 0;
    lastMoves.clear();
}

bool GameBoard::moveTiles(int direction) {
    QVector<QVector<int>> previousBoard = board;
    int scoreGained                     = 0;
    lastMoves.clear();

    if (direction == 0) {  // 向上移动
        for (int col = 0; col < size; col++) {
            int writePos = 0;
            // 第一步：将非零方块上移，消除中间空隙
            for (int row = 0; row < size; row++) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }
            // 第二步：合并相邻且相同的方块
            for (int row = 0; row < size - 1; row++) {
                if (board[row][col] != 0 && board[row][col] == board[row + 1][col]) {
                    TileMove move = {row + 1, col, row, col, true, board[row + 1][col]};
                    lastMoves.append(move);

                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row + 1][col]  = 0;
                }
            }
            // 第三步：再次上移以消除合并后产生的空隙
            writePos = 0;
            for (int row = 0; row < size; row++) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        if (previousBoard[row][col] != 0) {  // 如果这个方块在原来的位置有值
                            TileMove move = {row, col, writePos, col, false, previousBoard[row][col]};
                            lastMoves.append(move);
                        }
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }
        }
    } else if (direction == 1) {  // 向右移动
        for (int row = 0; row < size; row++) {
            int writePos = size - 1;
            // 将非零方块集中到右侧
            for (int col = size - 1; col >= 0; col--) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }
            // 合并右侧相邻相同的方块
            for (int col = size - 1; col > 0; col--) {
                if (board[row][col] != 0 && board[row][col] == board[row][col - 1]) {
                    TileMove move = {row, col - 1, row, col, true, board[row][col - 1]};
                    lastMoves.append(move);

                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row][col - 1]  = 0;
                }
            }
            // 将合并后产生的空隙再次集中到右侧
            writePos = size - 1;
            for (int col = size - 1; col >= 0; col--) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        if (previousBoard[row][col] != 0) {
                            TileMove move = {row, col, row, writePos, false, previousBoard[row][col]};
                            lastMoves.append(move);
                        }
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }
        }
    } else if (direction == 2) {  // 向下移动
        for (int col = 0; col < size; col++) {
            int writePos = size - 1;
            // 将非零方块下移，填充底部空缺
            for (int row = size - 1; row >= 0; row--) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }
            // 合并下侧相邻相同的方块
            for (int row = size - 1; row > 0; row--) {
                if (board[row][col] != 0 && board[row][col] == board[row - 1][col]) {
                    TileMove move = {row - 1, col, row, col, true, board[row - 1][col]};
                    lastMoves.append(move);

                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row - 1][col]  = 0;
                }
            }
            // 再次下移以消除中间空隙
            writePos = size - 1;
            for (int row = size - 1; row >= 0; row--) {
                if (board[row][col] != 0) {
                    if (row != writePos) {
                        if (previousBoard[row][col] != 0) {
                            TileMove move = {row, col, writePos, col, false, previousBoard[row][col]};
                            lastMoves.append(move);
                        }
                        board[writePos][col] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos--;
                }
            }
        }
    } else if (direction == 3) {  // 向左移动
        for (int row = 0; row < size; row++) {
            int writePos = 0;
            // 将非零方块左移
            for (int col = 0; col < size; col++) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }
            // 合并左侧相邻且相同的方块
            for (int col = 0; col < size - 1; col++) {
                if (board[row][col] != 0 && board[row][col] == board[row][col + 1]) {
                    TileMove move = {row, col + 1, row, col, true, board[row][col + 1]};
                    lastMoves.append(move);

                    board[row][col]     *= 2;
                    scoreGained         += board[row][col];
                    board[row][col + 1]  = 0;
                }
            }
            // 再次左移以填充合并后产生的空隙
            writePos = 0;
            for (int col = 0; col < size; col++) {
                if (board[row][col] != 0) {
                    if (col != writePos) {
                        if (previousBoard[row][col] != 0) {
                            TileMove move = {row, col, row, writePos, false, previousBoard[row][col]};
                            lastMoves.append(move);
                        }
                        board[row][writePos] = board[row][col];
                        board[row][col]      = 0;
                    }
                    writePos++;
                }
            }
        }
    }

    // 判断棋盘是否发生了改变
    bool changed = false;
    for (int i = 0; i < size && !changed; ++i) {
        for (int j = 0; j < size && !changed; ++j) {
            if (board[i][j] != previousBoard[i][j]) {
                changed = true;
            }
        }
    }

    if (changed) {
        score += scoreGained;
    }

    return changed;
}

void GameBoard::generateNewTile(bool* animated) {
    QVector<QPair<int, int>> emptyTiles = getEmptyTiles();

    if (emptyTiles.isEmpty()) {
        return;
    }

    int randomIndex = QRandomGenerator::global()->bounded(emptyTiles.size());
    int row         = emptyTiles[randomIndex].first;
    int col         = emptyTiles[randomIndex].second;

    // 90%的概率生成数字2，10%生成数字4
    int newValue = (QRandomGenerator::global()->bounded(10) < 9) ? 2 : 4;

    board[row][col] = newValue;

    if (animated != nullptr) {
        *animated = true;  // 指示新生成的方块需要动画效果
    }
}

bool GameBoard::isGameOver() const {
    // 首先检查是否存在空格
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (board[i][j] == 0) {
                return false;
            }
        }
    }

    // 检查水平方向相邻数字是否相同
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size - 1; ++j) {
            if (board[i][j] == board[i][j + 1]) {
                return false;
            }
        }
    }

    // 检查垂直方向相邻数字是否相同
    for (int j = 0; j < size; ++j) {
        for (int i = 0; i < size - 1; ++i) {
            if (board[i][j] == board[i + 1][j]) {
                return false;
            }
        }
    }

    return true;  // 没有空格且无法合并，游戏结束
}

bool GameBoard::isGameWon() const {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (board[i][j] == 2048) {
                return true;
            }
        }
    }
    return false;
}

bool GameBoard::isTileEmpty(int row, int col) const {
    if (row < 0 || row >= size || col < 0 || col >= size) {
        return false;
    }
    return board[row][col] == 0;
}

QVector<QPair<int, int>> GameBoard::getEmptyTiles() const {
    QVector<QPair<int, int>> emptyTiles;

    for (int row = 0; row < size; ++row) {
        for (int col = 0; col < size; ++col) {
            if (board[row][col] == 0) {
                emptyTiles.append(qMakePair(row, col));
            }
        }
    }

    return emptyTiles;
}

int GameBoard::getTileValue(int row, int col) const {
    if (row < 0 || row >= size || col < 0 || col >= size) {
        return 0;
    }
    return board[row][col];
}

void GameBoard::setTileValue(int row, int col, int value) {
    if (row < 0 || row >= size || col < 0 || col >= size) {
        return;
    }
    board[row][col] = value;
}

int GameBoard::getSize() const {
    return size;
}

int GameBoard::getScore() const {
    return score;
}

void GameBoard::setScore(int newScore) {
    score = newScore;
}

QVector<QVector<int>> GameBoard::getBoardState() const {
    return board;
}

void GameBoard::setBoardState(QVector<QVector<int>> const& state) {
    if (state.size() == size && state[0].size() == size) {
        board = state;
    }
}

QVector<GameBoard::TileMove> const& GameBoard::getLastMoves() const {
    return lastMoves;
}