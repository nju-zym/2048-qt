#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include <QPair>
#include <QVector>

class GameBoard {
   public:
    GameBoard(int size = 4);
    ~GameBoard() = default;

    // 棋盘操作
    void reset();                                    // 重置棋盘
    bool moveTiles(int direction);                   // 0=up, 1=right, 2=down, 3=left
    void generateNewTile(bool* animated = nullptr);  // 在随机空位置生成新方块

    // 状态检查
    bool isGameOver() const;                         // 检查游戏是否结束
    bool isGameWon() const;                          // 检查是否有方块达到2048
    bool isTileEmpty(int row, int col) const;        // 检查指定位置是否为空
    QVector<QPair<int, int>> getEmptyTiles() const;  // 获取所有空位置

    // 数据访问
    int getTileValue(int row, int col) const;        // 获取指定位置的值
    void setTileValue(int row, int col, int value);  // 设置指定位置的值
    int getSize() const;                             // 获取棋盘大小
    int getScore() const;                            // 获取当前分数
    void setScore(int newScore);                     // 设置分数

    // 用于撤销操作的状态管理
    QVector<QVector<int>> getBoardState() const;             // 获取当前棋盘状态
    void setBoardState(QVector<QVector<int>> const& state);  // 设置棋盘状态

    // 移动追踪结构体
    struct TileMove {
        int fromRow, fromCol;  // 原始位置
        int toRow, toCol;      // 目标位置
        bool merged;           // 是否发生合并
        int value;             // 移动前的值
    };

    // 获取上次移动的方块移动信息
    QVector<TileMove> const& getLastMoves() const;

   private:
    QVector<QVector<int>> board;  // 棋盘数据
    int size;                     // 棋盘大小
    int score;                    // 当前分数
    QVector<TileMove> lastMoves;  // 上次移动的数据
};

#endif  // GAMEBOARD_H