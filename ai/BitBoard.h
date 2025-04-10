#ifndef BITBOARD_H
#define BITBOARD_H

#include "../core/GameBoard.h"

#include <QMutex>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief 位棋盘表示类，使用64位整数表示4x4的2048游戏棋盘
 *
 * 每个方块使用4位表示，整个棋盘使用64位整数表示
 * 这种表示方法可以在单个机器寄存器中传递整个棋盘，提高计算效率
 */
class BitBoard {
   public:
    // 移动方向枚举
    enum Direction { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };

    // 位置结构体
    struct Position {
        int row;
        int col;

        Position(int r, int c) : row(r), col(c) {}
        bool operator==(Position const& other) const {
            return row == other.row && col == other.col;
        }
    };

    /**
     * @brief 默认构造函数，创建空棋盘
     */
    BitBoard();

    /**
     * @brief 从GameBoard构造BitBoard
     * @param board GameBoard对象
     */
    explicit BitBoard(GameBoard const& board);

    /**
     * @brief 从64位整数构造BitBoard
     * @param board 64位整数表示的棋盘
     */
    explicit BitBoard(uint64_t board);

    /**
     * @brief 获取指定位置的方块值
     * @param row 行索引 (0-3)
     * @param col 列索引 (0-3)
     * @return 方块值 (0, 2, 4, 8, ...)
     */
    int getTile(int row, int col) const;

    /**
     * @brief 设置指定位置的方块值
     * @param row 行索引 (0-3)
     * @param col 列索引 (0-3)
     * @param value 方块值 (0, 2, 4, 8, ...)
     */
    void setTile(int row, int col, int value);

    /**
     * @brief 获取空位置列表
     * @return 空位置的列表
     */
    std::vector<Position> getEmptyPositions() const;

    /**
     * @brief 获取空位置数量
     * @return 空位置数量
     */
    int countEmptyTiles() const;

    /**
     * @brief 在指定位置放置新方块
     * @param pos 位置
     * @param value 方块值 (通常是2或4)
     * @return 放置新方块后的棋盘
     */
    BitBoard placeNewTile(Position const& pos, int value) const;

    /**
     * @brief 执行移动操作
     * @param direction 移动方向
     * @return 移动后的棋盘
     */
    BitBoard move(Direction direction) const;

    /**
     * @brief 检查游戏是否结束
     * @return 如果没有可用移动，返回true
     */
    bool isGameOver() const;

    /**
     * @brief 获取棋盘上的最大方块值
     * @return 最大方块值
     */
    int getMaxTile() const;

    /**
     * @brief 获取棋盘的64位整数表示
     * @return 64位整数表示的棋盘
     */
    uint64_t getBoard() const {
        return board;
    }

    /**
     * @brief 比较两个棋盘是否相等
     * @param other 另一个棋盘
     * @return 如果相等返回true
     */
    bool operator==(BitBoard const& other) const {
        return board == other.board;
    }

    /**
     * @brief 比较两个棋盘是否不相等
     * @param other 另一个棋盘
     * @return 如果不相等返回true
     */
    bool operator!=(BitBoard const& other) const {
        return board != other.board;
    }

    /**
     * @brief 将棋盘转换为字符串表示
     * @return 棋盘的字符串表示
     */
    std::string toString() const;

    /**
     * @brief 异步初始化表
     */
    static void initializeTablesAsync();

    /**
     * @brief 检查表是否已初始化
     * @return 如果表已初始化，返回true
     */
    static bool areTablesInitialized();

   private:
    uint64_t board;  // 64位整数表示的棋盘

    // 预计算的移动表
    static std::array<uint16_t, 65536> moveTable[4];
    static std::array<uint16_t, 65536> scoreTable;

    // 初始化移动表和分数表
    static void initializeTables();
    static bool tablesInitialized;
    static QMutex tablesMutex;

    // 辅助函数
    static uint16_t moveRow(uint16_t row, Direction direction);
    static uint16_t calculateScore(uint16_t before, uint16_t after);
};

#endif  // BITBOARD_H
