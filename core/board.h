#ifndef BOARD_H
#define BOARD_H

#include <QPair>
#include <QVector>

/**
 * @brief 游戏棋盘类，处理2048游戏的核心逻辑
 */
class Board {
   public:
    Board();
    ~Board();

    /**
     * @brief 初始化棋盘，重置所有状态
     */
    void init();

    /**
     * @brief 在随机空位生成新的数字方块
     * @return 生成的位置和值，如果无法生成则返回(-1, -1, 0)
     */
    QPair<QPair<int, int>, int> generateNewTile();

    /**
     * @brief 向指定方向移动方块
     * @param direction 移动方向(0:上, 1:右, 2:下, 3:左)
     * @return 移动是否改变了棋盘状态以及获得的分数
     */
    QPair<bool, int> move(int direction);

    /**
     * @brief 获取棋盘当前状态
     * @return 4x4的整数矩阵，表示每个位置的数值
     */
    QVector<QVector<int>> const& getBoard() const;

    /**
     * @brief 设置棋盘状态(用于测试或恢复)
     * @param newBoard 要设置的新棋盘状态
     */
    void setBoard(QVector<QVector<int>> const& newBoard);

    /**
     * @brief 获取所有空位置
     * @return 所有空位置的坐标列表
     */
    QVector<QPair<int, int>> getEmptyTiles() const;

    /**
     * @brief 检查游戏是否结束(无法再移动)
     * @return 游戏是否结束
     */
    bool isGameOver() const;

    /**
     * @brief 检查游戏是否胜利(是否有2048)
     * @return 是否有2048方块
     */
    bool isGameWon() const;

    /**
     * @brief 获取特定位置的数值
     * @param row 行索引
     * @param col 列索引
     * @return 该位置的数值
     */
    int getTileValue(int row, int col) const;

   private:
    QVector<QVector<int>> board;  // 4x4的整数矩阵，表示棋盘状态

    /**
     * @brief 向上移动方块
     * @return 移动是否改变了棋盘状态以及获得的分数
     */
    QPair<bool, int> moveUp();

    /**
     * @brief 向右移动方块
     * @return 移动是否改变了棋盘状态以及获得的分数
     */
    QPair<bool, int> moveRight();

    /**
     * @brief 向下移动方块
     * @return 移动是否改变了棋盘状态以及获得的分数
     */
    QPair<bool, int> moveDown();

    /**
     * @brief 向左移动方块
     * @return 移动是否改变了棋盘状态以及获得的分数
     */
    QPair<bool, int> moveLeft();

    /**
     * @brief 检查指定位置是否为空(值为0)
     */
    bool isTileEmpty(int row, int col) const;
};

#endif  // BOARD_H