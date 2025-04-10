#include "BitBoard.h"

#include "BitBoardTables.h"

#include <QDebug>
#include <QElapsedTimer>
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
    // 如果表已经初始化，直接返回
    if (tablesInitialized) {
        return;
    }

    QElapsedTimer timer;
    timer.start();

    qDebug() << "开始初始化BitBoard表...";

    // 使用延迟加载，只在需要时才初始化
    static bool tablesLoaded = false;

    if (!tablesLoaded) {
        // 直接从预编译的表中复制数据
        memcpy(moveTable[LEFT].data(), BitBoardTables::LEFT_MOVE_TABLE.data(), sizeof(uint16_t) * 65536);
        memcpy(moveTable[RIGHT].data(), BitBoardTables::RIGHT_MOVE_TABLE.data(), sizeof(uint16_t) * 65536);
        memcpy(scoreTable.data(), BitBoardTables::SCORE_TABLE.data(), sizeof(uint16_t) * 65536);

        tablesLoaded = true;
    }

    tablesInitialized = true;

    qDebug() << "BitBoard表初始化完成，耗时" << timer.elapsed() << "毫秒";
}

uint16_t BitBoard::moveRow(uint16_t row, Direction direction) {
    // 直接从预编译的表中获取结果
    if (direction == LEFT) {
        return BitBoardTables::LEFT_MOVE_TABLE[row];
    }
    return BitBoardTables::RIGHT_MOVE_TABLE[row];
}

uint16_t BitBoard::calculateScore(uint16_t before, uint16_t /*after*/) {
    // 直接从预编译的表中获取结果
    return BitBoardTables::SCORE_TABLE[before];
}

size_t BitBoard::hash() const {
    // 使用更高效的哈希函数
    // FNV-1a哈希算法
    constexpr size_t FNV_PRIME        = 1099511628211ULL;
    constexpr size_t FNV_OFFSET_BASIS = 14695981039346656037ULL;

    size_t hash    = FNV_OFFSET_BASIS;
    uint64_t value = board;

    for (int i = 0; i < 8; ++i) {
        hash   ^= (value & 0xFF);
        hash   *= FNV_PRIME;
        value >>= 8;
    }

    return hash;
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
        ss << '\n';
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
