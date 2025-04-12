#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include <QVector>

class GameBoard {
   public:
    // 移动方向常量
    static int const UP    = 0;
    static int const RIGHT = 1;
    static int const DOWN  = 2;
    static int const LEFT  = 3;

    // 游戏状态判断
    static bool isGameOver(QVector<QVector<int>> const& boardState);

    // 模拟移动
    static bool simulateMove(QVector<QVector<int>>& boardState, int direction, int& score);

    // 辅助函数
    static int countEmptyCells(QVector<QVector<int>> const& boardState);
    static int getMaxTile(QVector<QVector<int>> const& boardState);
};

#endif  // GAMEBOARD_H