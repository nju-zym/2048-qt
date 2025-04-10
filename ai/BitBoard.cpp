#include "BitBoard.h"

#include <cmath>
#include <iomanip>
#include <sstream>

// 静态成员初始化
std::array<uint16_t, 65536> BitBoard::moveTable[4];
std::array<uint16_t, 65536> BitBoard::scoreTable;
bool BitBoard::tablesInitialized = false;
QMutex BitBoard::tablesMutex;

BitBoard::BitBoard() : board(0) {
    // 不再在构造函数中初始化表
    // 表的初始化将在后台线程中进行
}

BitBoard::BitBoard(GameBoard const& gameBoard) : board(0) {
    // 不再在构造函数中初始化表
    // 表的初始化将在后台线程中进行

    // 从GameBoard转换为BitBoard
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = gameBoard.getTileValue(row, col);
            if (value > 0) {
                // 将2的幂转换为对数值（例如，2->1, 4->2, 8->3, ...）
                int logValue = static_cast<int>(std::log2(value));
                setTile(row, col, logValue);
            }
        }
    }
}

BitBoard::BitBoard(uint64_t board) : board(board) {
    // 不再在构造函数中初始化表
    // 表的初始化将在后台线程中进行
}

int BitBoard::getTile(int row, int col) const {
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        return 0;
    }

    int shift = (row * 16) + (col * 4);
    int value = (board >> shift) & 0xF;

    // 将对数值转换回实际值（例如，1->2, 2->4, 3->8, ...）
    return value == 0 ? 0 : (1 << value);
}

void BitBoard::setTile(int row, int col, int value) {
    if (row < 0 || row >= 4 || col < 0 || col >= 4) {
        return;
    }

    int shift = (row * 16) + (col * 4);

    // 清除该位置的旧值
    board &= ~(0xFULL << shift);

    // 设置新值
    if (value > 0) {
        // 将实际值转换为对数值（例如，2->1, 4->2, 8->3, ...）
        int logValue  = static_cast<int>(std::log2(value));
        board        |= (static_cast<uint64_t>(logValue) << shift);
    }
}

std::vector<BitBoard::Position> BitBoard::getEmptyPositions() const {
    std::vector<Position> emptyPositions;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int shift = (row * 16) + (col * 4);
            if (((board >> shift) & 0xF) == 0) {
                emptyPositions.emplace_back(row, col);
            }
        }
    }

    return emptyPositions;
}

int BitBoard::countEmptyTiles() const {
    int count = 0;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int shift = (row * 16) + (col * 4);
            if (((board >> shift) & 0xF) == 0) {
                count++;
            }
        }
    }

    return count;
}

BitBoard BitBoard::placeNewTile(Position const& pos, int value) const {
    BitBoard newBoard(*this);

    // 将实际值转换为对数值（例如，2->1, 4->2, 8->3, ...）
    int logValue = static_cast<int>(std::log2(value));

    int shift       = (pos.row * 16) + (pos.col * 4);
    newBoard.board &= ~(0xFULL << shift);                          // 清除该位置的旧值
    newBoard.board |= (static_cast<uint64_t>(logValue) << shift);  // 设置新值

    return newBoard;
}

void BitBoard::initializeTables() {
    // 初始化移动表和分数表
    for (uint16_t row = 0; row < 65536; ++row) {
        // 将16位行转换为4个4位数字
        uint8_t tiles[4] = {static_cast<uint8_t>((row >> 0) & 0xF),
                            static_cast<uint8_t>((row >> 4) & 0xF),
                            static_cast<uint8_t>((row >> 8) & 0xF),
                            static_cast<uint8_t>((row >> 12) & 0xF)};

        // 计算每个方向的移动结果
        uint16_t result;

        // 向左移动
        result               = moveRow(row, LEFT);
        moveTable[LEFT][row] = result;
        scoreTable[row]      = calculateScore(row, result);

        // 向右移动
        result                = moveRow(row, RIGHT);
        moveTable[RIGHT][row] = result;

        // 向上和向下的移动在实际使用时通过转置矩阵实现
        // 这里只需要计算左右移动的表
    }

    tablesInitialized = true;
}

uint16_t BitBoard::moveRow(uint16_t row, Direction direction) {
    uint8_t tiles[4] = {static_cast<uint8_t>((row >> 0) & 0xF),
                        static_cast<uint8_t>((row >> 4) & 0xF),
                        static_cast<uint8_t>((row >> 8) & 0xF),
                        static_cast<uint8_t>((row >> 12) & 0xF)};

    // 如果是向右移动，反转数组
    if (direction == RIGHT) {
        std::swap(tiles[0], tiles[3]);
        std::swap(tiles[1], tiles[2]);
    }

    // 移除零并合并相同的数字
    int idx = 0;
    for (int i = 0; i < 4; ++i) {
        if (tiles[i] != 0) {
            tiles[idx++] = tiles[i];
        }
    }

    // 清空剩余位置
    while (idx < 4) {
        tiles[idx++] = 0;
    }

    // 合并相同的数字
    for (int i = 0; i < 3; ++i) {
        if (tiles[i] != 0 && tiles[i] == tiles[i + 1]) {
            tiles[i]++;  // 增加对数值（例如，1->2表示2->4）
            tiles[i + 1] = 0;
        }
    }

    // 再次移除零
    idx = 0;
    for (int i = 0; i < 4; ++i) {
        if (tiles[i] != 0) {
            tiles[idx++] = tiles[i];
        }
    }

    // 清空剩余位置
    while (idx < 4) {
        tiles[idx++] = 0;
    }

    // 如果是向右移动，再次反转数组
    if (direction == RIGHT) {
        std::swap(tiles[0], tiles[3]);
        std::swap(tiles[1], tiles[2]);
    }

    // 将4个4位数字合并回16位行
    return static_cast<uint16_t>(tiles[0]) | (static_cast<uint16_t>(tiles[1]) << 4)
           | (static_cast<uint16_t>(tiles[2]) << 8) | (static_cast<uint16_t>(tiles[3]) << 12);
}

uint16_t BitBoard::calculateScore(uint16_t before, uint16_t after) {
    uint16_t score = 0;

    // 提取before和after中的每个4位数字
    uint8_t beforeTiles[4] = {static_cast<uint8_t>((before >> 0) & 0xF),
                              static_cast<uint8_t>((before >> 4) & 0xF),
                              static_cast<uint8_t>((before >> 8) & 0xF),
                              static_cast<uint8_t>((before >> 12) & 0xF)};

    uint8_t afterTiles[4] = {static_cast<uint8_t>((after >> 0) & 0xF),
                             static_cast<uint8_t>((after >> 4) & 0xF),
                             static_cast<uint8_t>((after >> 8) & 0xF),
                             static_cast<uint8_t>((after >> 12) & 0xF)};

    // 计算合并产生的分数
    for (int i = 0; i < 4; ++i) {
        if (afterTiles[i] > 0 && afterTiles[i] > beforeTiles[i]) {
            // 如果after中的数字大于before中的数字，说明发生了合并
            // 分数是合并后的数字的实际值
            score += (1 << afterTiles[i]);
        }
    }

    return score;
}

BitBoard BitBoard::move(Direction direction) const {
    uint64_t result       = 0;
    uint64_t currentBoard = board;

    if (direction == UP || direction == DOWN) {
        // 转置棋盘
        uint64_t transposed = 0;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                int fromShift   = (row * 16) + (col * 4);
                int toShift     = (col * 16) + (row * 4);
                uint64_t value  = (currentBoard >> fromShift) & 0xF;
                transposed     |= (value << toShift);
            }
        }
        currentBoard = transposed;

        // 使用LEFT或RIGHT的移动表
        Direction actualDirection = (direction == UP) ? LEFT : RIGHT;

        // 对每一行应用移动
        for (int row = 0; row < 4; ++row) {
            int shift          = row * 16;
            uint16_t rowValue  = (currentBoard >> shift) & 0xFF'FF;
            uint16_t newRow    = moveTable[actualDirection][rowValue];
            result            |= (static_cast<uint64_t>(newRow) << shift);
        }

        // 再次转置回来
        uint64_t finalResult = 0;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                int fromShift   = (row * 16) + (col * 4);
                int toShift     = (col * 16) + (row * 4);
                uint64_t value  = (result >> fromShift) & 0xF;
                finalResult    |= (value << toShift);
            }
        }
        result = finalResult;
    } else {
        // 对每一行应用移动
        for (int row = 0; row < 4; ++row) {
            int shift          = row * 16;
            uint16_t rowValue  = (currentBoard >> shift) & 0xFF'FF;
            uint16_t newRow    = moveTable[direction][rowValue];
            result            |= (static_cast<uint64_t>(newRow) << shift);
        }
    }

    return BitBoard(result);
}

bool BitBoard::isGameOver() const {
    // 如果有空位，游戏没有结束
    if (countEmptyTiles() > 0) {
        return false;
    }

    // 检查是否有可能的移动
    for (int direction = 0; direction < 4; ++direction) {
        if (move(static_cast<Direction>(direction)) != *this) {
            return false;
        }
    }

    // 没有空位且没有可能的移动，游戏结束
    return true;
}

int BitBoard::getMaxTile() const {
    int maxTile = 0;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getTile(row, col);
            maxTile   = std::max(maxTile, value);
        }
    }

    return maxTile;
}

std::string BitBoard::toString() const {
    std::stringstream ss;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int value = getTile(row, col);
            ss << std::setw(5) << value;
        }
        ss << std::endl;
    }

    return ss.str();
}

void BitBoard::initializeTablesAsync() {
    QMutexLocker locker(&tablesMutex);
    if (!tablesInitialized) {
        initializeTables();
        tablesInitialized = true;
    }
}

bool BitBoard::areTablesInitialized() {
    QMutexLocker locker(&tablesMutex);
    return tablesInitialized;
}
