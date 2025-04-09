#include "auto.h"

#include "trainingworker.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <algorithm>

// TrainingTask的run方法实现
void TrainingTask::run() {
    // 模拟多次游戏并计算平均分数
    int totalScore = 0;
    Auto autoPlayer;

    try {
        // 初始化位棋盘表格 - 确保在开始模拟前初始化
        autoPlayer.initTables();

        // 复制回调函数，避免捕获this
        auto progressCallbackCopy = progressCallback;
        auto finalCallbackCopy    = finalCallback;
        int simulationsCopy       = simulations;

        for (int i = 0; i < simulations; ++i) {
            int gameScore  = autoPlayer.simulateFullGame(params);
            totalScore    += gameScore;

            // 如果提供了进度回调，则在每次模拟后调用
            if (progressCallbackCopy) {
                // 计算当前平均分数
                int currentAvgScore = totalScore / (i + 1);
                // 使用QMetaObject::invokeMethod确保回调在主线程中执行
                QMetaObject::invokeMethod(
                    QApplication::instance(),
                    [progressCallbackCopy, i, currentAvgScore, simulationsCopy]() {
                        if (progressCallbackCopy) {
                            progressCallbackCopy(i + 1, simulationsCopy, currentAvgScore);
                        }
                    },
                    Qt::QueuedConnection);
            }
        }

        int avgScore = totalScore / simulations;

        // 调用最终回调函数返回分数 - 确保在主线程中执行
        QMetaObject::invokeMethod(
            QApplication::instance(),
            [finalCallbackCopy, avgScore]() {
                if (finalCallbackCopy) {
                    finalCallbackCopy(avgScore);
                }
            },
            Qt::QueuedConnection);
    } catch (std::exception const& e) {
        // 异常处理
        qDebug() << "Exception in TrainingTask::run(): " << e.what();

        // 复制回调函数，避免捕获this
        auto finalCallbackCopy = finalCallback;

        // 确保即使发生异常也会调用回调
        QMetaObject::invokeMethod(
            QApplication::instance(),
            [finalCallbackCopy]() {
                if (finalCallbackCopy) {
                    finalCallbackCopy(0);  // 发生异常时返回0分
                }
            },
            Qt::QueuedConnection);
    } catch (...) {
        qDebug() << "Unknown exception in TrainingTask::run()";

        // 复制回调函数，避免捕获this
        auto finalCallbackCopy = finalCallback;

        // 确保即使发生异常也会调用回调
        QMetaObject::invokeMethod(
            QApplication::instance(),
            [finalCallbackCopy]() {
                if (finalCallbackCopy) {
                    finalCallbackCopy(0);  // 发生异常时返回0分
                }
            },
            Qt::QueuedConnection);
    }
}

// 预计算的移动表
static uint16_t row_left_table[65536];
static uint16_t row_right_table[65536];
static uint64_t col_up_table[65536];
static uint64_t col_down_table[65536];

// 预计算的评分表
static float heur_score_table[65536];
static float score_table[65536];

// 常量定义
static uint64_t const ROW_MASK = 0xFF'FFULL;
static uint64_t const COL_MASK = 0x00'0F'00'0F'00'0F'00'0FULL;

// 转置棋盘
inline static BitBoard transpose(BitBoard x) {
    BitBoard a1 = x & 0xF0'F0'0F'0F'F0'F0'0F'0FULL;
    BitBoard a2 = x & 0x00'00'F0'F0'00'00'F0'F0ULL;
    BitBoard a3 = x & 0x0F'0F'00'00'0F'0F'00'00ULL;
    BitBoard a  = a1 | (a2 << 12) | (a3 >> 12);
    BitBoard b1 = a & 0xFF'00'FF'00'00'FF'00'FFULL;
    BitBoard b2 = a & 0x00'FF'00'FF'00'00'00'00ULL;
    BitBoard b3 = a & 0x00'00'00'00'FF'00'FF'00ULL;
    return b1 | (b2 >> 24) | (b3 << 24);
}

// 反转一行
inline static uint16_t reverse_row(uint16_t row) {
    return (row >> 12) | ((row >> 4) & 0x00'F0) | ((row << 4) & 0x0F'00) | (row << 12);
}

// 将列解包为位棋盘
inline static BitBoard unpack_col(uint16_t row) {
    BitBoard tmp = row;
    return (tmp | (tmp << 12ULL) | (tmp << 24ULL) | (tmp << 36ULL)) & COL_MASK;
}

// 执行向上移动
inline static BitBoard execute_move_0(BitBoard board) {
    BitBoard ret  = board;
    BitBoard t    = transpose(board);
    ret          ^= col_up_table[(t >> 0) & ROW_MASK] << 0;
    ret          ^= col_up_table[(t >> 16) & ROW_MASK] << 4;
    ret          ^= col_up_table[(t >> 32) & ROW_MASK] << 8;
    ret          ^= col_up_table[(t >> 48) & ROW_MASK] << 12;
    return ret;
}

// 执行向下移动
inline static BitBoard execute_move_1(BitBoard board) {
    BitBoard ret  = board;
    BitBoard t    = transpose(board);
    ret          ^= col_down_table[(t >> 0) & ROW_MASK] << 0;
    ret          ^= col_down_table[(t >> 16) & ROW_MASK] << 4;
    ret          ^= col_down_table[(t >> 32) & ROW_MASK] << 8;
    ret          ^= col_down_table[(t >> 48) & ROW_MASK] << 12;
    return ret;
}

// 执行向左移动
inline static BitBoard execute_move_2(BitBoard board) {
    BitBoard ret  = board;
    ret          ^= BitBoard(row_left_table[(board >> 0) & ROW_MASK]) << 0;
    ret          ^= BitBoard(row_left_table[(board >> 16) & ROW_MASK]) << 16;
    ret          ^= BitBoard(row_left_table[(board >> 32) & ROW_MASK]) << 32;
    ret          ^= BitBoard(row_left_table[(board >> 48) & ROW_MASK]) << 48;
    return ret;
}

// 执行向右移动
inline static BitBoard execute_move_3(BitBoard board) {
    BitBoard ret  = board;
    ret          ^= BitBoard(row_right_table[(board >> 0) & ROW_MASK]) << 0;
    ret          ^= BitBoard(row_right_table[(board >> 16) & ROW_MASK]) << 16;
    ret          ^= BitBoard(row_right_table[(board >> 32) & ROW_MASK]) << 32;
    ret          ^= BitBoard(row_right_table[(board >> 48) & ROW_MASK]) << 48;
    return ret;
}

// 计算空格数 - 优化版本
int Auto::countEmptyTiles(BitBoard board) {
    // 使用位操作快速计算空格数
    board |= (board >> 2) & 0x33'33'33'33'33'33'33'33ULL;
    board |= (board >> 1);
    board  = ~board & 0x11'11'11'11'11'11'11'11ULL;

    // 此时每个半字节是：
    // 0 如果原始半字节非零
    // 1 如果原始半字节为零
    // 现在将它们全部相加

    board += board >> 32;
    board += board >> 16;
    board += board >> 8;
    board += board >> 4;  // 如果有16个空位，这可能会溢出到下一个半字节

    return static_cast<int>(board & 0xf);
}

// 检查游戏是否结束
bool Auto::isGameOverBitBoard(BitBoard board) {
    // 如果有空格，游戏未结束
    if (countEmptyTiles(board) > 0) {
        return false;
    }

    // 检查水平方向是否可以合并
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            int pos1 = (i * 4 + j) * 4;
            int pos2 = (i * 4 + j + 1) * 4;
            int val1 = static_cast<int>((board >> pos1) & 0xF);
            int val2 = static_cast<int>((board >> pos2) & 0xF);
            if (val1 == val2 && val1 != 0) {
                return false;  // 可以合并，游戏未结束
            }
        }
    }

    // 检查垂直方向是否可以合并
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            int pos1 = (i * 4 + j) * 4;
            int pos2 = ((i + 1) * 4 + j) * 4;
            int val1 = static_cast<int>((board >> pos1) & 0xF);
            int val2 = static_cast<int>((board >> pos2) & 0xF);
            if (val1 == val2 && val1 != 0) {
                return false;  // 可以合并，游戏未结束
            }
        }
    }

    // 没有空格且没有可合并的方块，游戏结束
    return true;
}

// 加载持久化数据
bool Auto::loadPersistentData() {
    // 打印当前工作目录
    qDebug() << "Current working directory:" << QDir::currentPath();

    // 确保数据目录存在
    QString dataDirPath = getDataDirPath();
    QDir dataDir;

    if (!dataDir.exists(dataDirPath)) {
        bool pathCreated = dataDir.mkpath(dataDirPath);
        qDebug() << "Creating data directory at" << dataDirPath << (pathCreated ? "success" : "failed");
    }

    // 先尝试从数据目录加载持久化数据
    QString persistentFilePath = dataDirPath + "/2048_ai_persistent_data.json";
    QFile dataFile(persistentFilePath);
    qDebug() << "Checking for persistent data file at:" << persistentFilePath;
    qDebug() << "File exists:" << dataFile.exists();

    // 如果数据目录中有文件，尝试打开
    if (dataFile.exists() && dataFile.open(QIODevice::ReadOnly)) {
        QByteArray data = dataFile.readAll();
        dataFile.close();
        qDebug() << "Successfully read data from data directory";
        return processJsonData(data);
    }

    // 如果没有找到数据
    qDebug() << "No persistent training data found in any location";
    return false;
}

// 处理JSON数据
bool Auto::processJsonData(QByteArray const& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Invalid persistent training data format.";
        return false;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("parameters") || !obj["parameters"].isArray() || !obj.contains("score")) {
        qDebug() << "Missing required fields in persistent training data.";
        return false;
    }

    QJsonArray paramsArray = obj["parameters"].toArray();
    strategyParams.clear();

    for (int i = 0; i < paramsArray.size(); ++i) {
        strategyParams.append(paramsArray[i].toDouble());
    }

    bestHistoricalScore = obj["score"].toInt();
    useLearnedParams    = true;

    qDebug() << "Loaded persistent training data with score:" << bestHistoricalScore;
    return true;
}

// 构造函数：初始化策略参数
Auto::Auto()
    : strategyParams(5, 1.0),  // 初始化为5个参数，默认值为1.0
      defaultParams(5, 1.0),   // 初始化默认参数
      useLearnedParams(false),
      bestHistoricalScore(0),  // 初始化历史最佳分数为0
      trainingActive(false) {
    // 初始化默认参数为经过测试的有效值
    defaultParams[0] = 2.7;  // 空格数量权重
    defaultParams[1] = 1.8;  // 蛇形模式权重
    defaultParams[2] = 3.5;  // 平滑度权重
    defaultParams[3] = 2.0;  // 单调性权重
    defaultParams[4] = 1.5;  // 合并可能性权重

    // 初始化随机数生成器
    srand(time(nullptr));

    // 初始化位棋盘的预计算表
    initTables();

    // 尝试加载持久化的训练数据
    if (!loadPersistentData()) {
        qDebug() << "No persistent training data found, using default parameters";
    } else {
        qDebug() << "Loaded persistent training data with score:" << bestHistoricalScore;
        useLearnedParams = true;
    }
}

// 获取数据目录路径
QString Auto::getDataDirPath() const {
    return "/Users/yimingzhong/Code/2048-qt/build/Debug/2048-qt.app/Contents/MacOS/data";
}

// 重置历史最佳分数
void Auto::resetBestHistoricalScore() {
    bestHistoricalScore = 0;
    useLearnedParams    = false;
    strategyParams      = defaultParams;

    // 确保数据目录存在
    QDir dataDir;
    QString dataDirPath = getDataDirPath();
    if (!dataDir.exists(dataDirPath)) {
        bool created = dataDir.mkpath(dataDirPath);
        qDebug() << "Creating data directory at" << dataDirPath << (created ? "success" : "failed");
    }

    // 删除持久化数据文件 - 使用绝对路径
    QString persistentFilePath = dataDirPath + "/2048_ai_persistent_data.json";
    QFile file(persistentFilePath);
    if (file.exists()) {
        bool removed = file.remove();
        qDebug() << "Removed persistent data file from" << persistentFilePath << (removed ? "success" : "failed");
        if (!removed) {
            qDebug() << "Error removing file:" << file.errorString();
        }
    }

    // 尝试删除根目录的文件
    QString rootFilePath = QDir::currentPath() + "/2048_ai_persistent_data.json";
    QFile rootFile(rootFilePath);
    if (rootFile.exists()) {
        bool removed = rootFile.remove();
        qDebug() << "Removed persistent data file from" << rootFilePath << (removed ? "success" : "failed");
        if (!removed) {
            qDebug() << "Error removing file:" << rootFile.errorString();
        }
    }

    // 删除所有参数文件
    QDir paramDir(dataDirPath);
    if (paramDir.exists()) {
        QStringList filters;
        filters << "2048_ai_params_*.json" << "params_*.json";
        QStringList files = paramDir.entryList(filters, QDir::Files);

        for (QString const& fileName : files) {
            QString fullPath = dataDirPath + "/" + fileName;
            QFile paramFile(fullPath);
            bool removed = paramFile.remove();
            qDebug() << "Removed parameter file:" << fullPath << (removed ? "success" : "failed");
            if (!removed) {
                qDebug() << "Error removing file:" << paramFile.errorString();
            }
        }
    }

    qDebug() << "Reset best historical score and removed all persistent data files";
}

// 析构函数
Auto::~Auto() {
    // 确保训练线程已经结束
    if (trainingActive.load()) {
        qDebug() << "Stopping training in Auto destructor";
        trainingActive.store(false);

        // 等待线程池完成所有任务
        QThreadPool::globalInstance()->waitForDone(5000);  // 添加超时时间防止死锁
        qDebug() << "All training tasks completed or timed out";
    }

    // 清除缓存
    expectimaxCache.clear();
    bitboardCache.clear();

    // 释放任何其他资源
    QThreadPool::globalInstance()->clear();

    qDebug() << "Auto object destroyed successfully";
}

// 设置是否使用学习到的参数
void Auto::setUseLearnedParams(bool use) {
    useLearnedParams = use;
}

// 获取是否使用学习到的参数
bool Auto::getUseLearnedParams() const {
    return useLearnedParams;
}

// 获取策略参数
QVector<double> Auto::getStrategyParams() const {
    return strategyParams;
}

// findBestMove: 找出最佳移动方向
int Auto::findBestMove(QVector<QVector<int>> const& board) {
    // 检查棋盘上的最大值
    int maxValue = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] > maxValue) {
                maxValue = board[i][j];
            }
        }
    }

    // 如果棋盘上有高级方块，尝试使用位棋盘实现以提高性能
    // 但是如果失败，则回退到标准方法
    if (maxValue >= 2048) {
        // 清除缓存以确保最新的计算结果
        clearExpectimaxCache();

        // 初始化预计算表（如果还没有初始化）
        static bool tablesInitialized = false;
        if (!tablesInitialized) {
            initTables();
            tablesInitialized = true;
        }

        // 尝试使用位棋盘实现的最佳移动函数
        try {
            int bestMove = getBestMoveBitBoard(board);

            // 如果位棋盘方法没有找到有效移动，回退到标准方法
            if (bestMove != -1) {
                qDebug() << "Using BitBoard method with best move:" << bestMove;
                return bestMove;
            }
            qDebug() << "BitBoard method failed to find a valid move, falling back to standard method";
        } catch (std::exception const& e) {
            qDebug() << "Exception in BitBoard method:" << e.what() << ", falling back to standard method";
        } catch (...) {
            qDebug() << "Unknown exception in BitBoard method, falling back to standard method";
        }
        // 如果到达这里，说明位棋盘方法失败，继续使用标准方法
    }

    int bestScore     = -1;
    int bestDirection = -1;

    // 尝试每个方向，计算移动后的棋盘评分
    for (int direction = 0; direction < 4; ++direction) {
        // 创建棋盘副本
        QVector<QVector<int>> boardCopy = board;

        // 模拟移动
        int moveScore = 0;
        bool moved    = simulateMove(boardCopy, direction, moveScore);

        // 如果这个方向可以移动，计算移动后的棋盘评分
        if (moved) {
            int score = 0;

            // 无论是否使用学习参数，都使用相同的评估方法，只是参数不同
            // 先进行基础评估
            score = evaluateWithParams(boardCopy, useLearnedParams ? strategyParams : defaultParams) + moveScore;

            // 使用expectimax算法进行深度为3的搜索
            int simulationScore  = expectimax(boardCopy, 3, false);
            score               += simulationScore;

            if (score > bestScore) {
                bestScore     = score;
                bestDirection = direction;
            }
        }
    }

    // 如果没有有效移动，随机选择一个方向
    if (bestDirection == -1) {
        bestDirection = rand() % 4;  // 随机选择一个方向
    }

    return bestDirection;
}

// evaluateBoard: 评估棋盘状态
int Auto::evaluateBoard(QVector<QVector<int>> const& boardState) {
    int score = 0;

    // 策略：高分数方块尽量放在角落，并保持递减排列

    // 1. 计算空格数量，空格越多越好
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    score += emptyCount * 10;  // 每个空格给10分

    // 2. 奖励角落有高分数方块
    // 右下角最高分
    if (boardState[3][3] > 0) {
        score += boardState[3][3] * 2;
    }

    // 3. 奖励相邻方块数值递减排列
    // 检查行
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            // 如果相邻方块有递减关系，加分
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0 && boardState[i][j] >= boardState[i][j + 1]) {
                score += std::min(boardState[i][j], boardState[i][j + 1]);
            }
        }
    }

    // 检查列
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            // 如果相邻方块有递减关系，加分
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0 && boardState[i][j] >= boardState[i + 1][j]) {
                score += std::min(boardState[i][j], boardState[i + 1][j]);
            }
        }
    }

    return score;
}

// 检查游戏是否结束
bool Auto::isGameOver(QVector<QVector<int>> const& boardState) {
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

// 计算合并分数 - 评估棋盘上可能的合并操作
double Auto::calculateMergeScore(QVector<QVector<int>> const& boardState) {
    double mergeScore = 0;

    // 检查行上的合并可能性
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i][j + 1]) {
                // 对于高值方块，给予更高的奖励
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                mergeScore       += tileValue * (logValue / 10.0) * 2.0;
            }
        }
    }

    // 检查列上的合并可能性
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i + 1][j]) {
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                mergeScore       += tileValue * (logValue / 10.0) * 2.0;
            }
        }
    }

    // 检查相邻的递增序列（如 2-4-8）
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 2; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0 && boardState[i][j + 2] > 0) {
                if (boardState[i][j + 1] == 2 * boardState[i][j] && boardState[i][j + 2] == 2 * boardState[i][j + 1]) {
                    // 奖励递增序列
                    mergeScore += boardState[i][j + 2] * 0.5;
                }
            }
        }
    }

    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 2; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0 && boardState[i + 2][j] > 0) {
                if (boardState[i + 1][j] == 2 * boardState[i][j] && boardState[i + 2][j] == 2 * boardState[i + 1][j]) {
                    // 奖励递增序列
                    mergeScore += boardState[i + 2][j] * 0.5;
                }
            }
        }
    }

    return mergeScore;
}

// 高级模式评估 - 专门处理超过2048的复杂情况
int Auto::evaluateAdvancedPattern(QVector<QVector<int>> const& boardState) {
    int score    = 0;
    int maxValue = 0;
    int maxRow = 0, maxCol = 0;

    // 找出最大值及其位置
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
                maxRow   = i;
                maxCol   = j;
            }
        }
    }

    // 如果最大值小于2048，使用普通评估函数
    if (maxValue < 2048) {
        return evaluateBoardAdvanced(boardState);
    }

    // 对于超过2048的情况，使用更复杂的评估策略

    // 1. 蛇形模式权重矩阵 - 为了处理高级棋盘状态
    // 定义蛇形路径的权重矩阵，从左上角开始蛇形形式排列
    int const snakePattern[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};

    // 2. 计算蛇形模式得分
    int patternScore = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > 0) {
                // 对于高值方块，给予更高的权重
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                patternScore     += tileValue * snakePattern[i][j] * (logValue / 11.0);  // 11 = log2(2048)
            }
        }
    }
    score += patternScore * 2;  // 增加模式权重

    // 3. 空格数量权重 - 对于高级棋盘更重要
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    // 空格数量的权重与棋盘填充程度相关，棋盘越满空格越重要
    score += emptyCount * (16 - emptyCount) * 50;  // 大幅增加空格权重

    // 4. 平滑度权重 - 相邻方块数值差异越小越好
    double smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                // 使用对数差异来减少大数值之间的差异惩罚
                double logVal1  = log2(boardState[i][j]);
                double logVal2  = log2(boardState[i][j + 1]);
                smoothness     -= std::abs(logVal1 - logVal2) * 4.0;
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                double logVal1  = log2(boardState[i][j]);
                double logVal2  = log2(boardState[i + 1][j]);
                smoothness     -= std::abs(logVal1 - logVal2) * 4.0;
            }
        }
    }
    score += static_cast<int>(smoothness * 40);  // 增加平滑度权重

    // 5. 单调性权重 - 方块数值沿某个方向单调递增或递减
    double monotonicity = 0;

    // 检查行的单调性 - 使用对数值
    for (int i = 0; i < 4; ++i) {
        double current     = 0;
        double left_score  = 0;
        double right_score = 0;

        for (int j = 0; j < 4; ++j) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                if (current > 0) {
                    if (current > value) {
                        left_score += (current - value) * 2;  // 增加左向单调性权重
                    } else {
                        right_score += (value - current) * 2;  // 增加右向单调性权重
                    }
                }
                current = value;
            }
        }
        monotonicity += std::min(left_score, right_score);
    }

    // 检查列的单调性 - 使用对数值
    for (int j = 0; j < 4; ++j) {
        double current    = 0;
        double up_score   = 0;
        double down_score = 0;

        for (int i = 0; i < 4; ++i) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                if (current > 0) {
                    if (current > value) {
                        up_score += (current - value) * 2;  // 增加上向单调性权重
                    } else {
                        down_score += (value - current) * 2;  // 增加下向单调性权重
                    }
                }
                current = value;
            }
        }
        monotonicity += std::min(up_score, down_score);
    }

    score -= static_cast<int>(monotonicity * 60);  // 单调性是负分，越小越好

    // 6. 合并可能性 - 奖励相邻相同值
    double mergeScore  = calculateMergeScore(boardState);
    score             += static_cast<int>(mergeScore * 20);

    // 7. 游戏结束检测 - 如果游戏即将结束，给予大量惩罚
    if (isGameOver(boardState)) {
        score -= 500000;  // 大幅惩罚游戏结束状态
    }

    return score;
}

// evaluateBoardAdvanced: 高级评估棋盘状态
int Auto::evaluateBoardAdvanced(QVector<QVector<int>> const& boardState) {
    int score = 0;

    // 1. 空格数量权重 - 空格越多越好
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    // 空格数量的权重与棋盘填充程度相关，棋盘越满空格越重要
    score += emptyCount * (16 - emptyCount) * 20;  // 增加空格权重

    // 2. 角落策略 - 使用权重矩阵
    // 定义权重矩阵，优先将大数放在角落，特别是左上角
    int const weightMatrix[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};

    // 计算权重得分
    int weightScore = 0;
    int maxValue    = 0;
    int maxRow = 0, maxCol = 0;

    // 找出最大值及其位置
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
                maxRow   = i;
                maxCol   = j;
            }
            // 根据权重矩阵计算得分
            if (boardState[i][j] > 0) {
                weightScore += boardState[i][j] * weightMatrix[i][j];
            }
        }
    }

    // 如果最大值在左上角，给予额外奖励
    if (maxRow == 0 && maxCol == 0) {
        weightScore += maxValue * 4;
    }
    // 如果最大值在任意角落，也给予奖励
    else if ((maxRow == 0 || maxRow == 3) && (maxCol == 0 || maxCol == 3)) {
        weightScore += maxValue * 2;
    }

    score += weightScore;

    // 3. 平滑度权重 - 相邻方块数值差异越小越好
    int smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                // 使用对数差异来减少大数值之间的差异惩罚
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i][j + 1] > 0 ? log2(boardState[i][j + 1]) : 0;
                smoothness     -= abs(logVal1 - logVal2) * 2.0;
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                // 使用对数差异
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i + 1][j] > 0 ? log2(boardState[i + 1][j]) : 0;
                smoothness     -= abs(logVal1 - logVal2) * 2.0;
            }
        }
    }
    score += smoothness * 30;  // 增加平滑度权重

    // 4. 单调性权重 - 方块数值沿某个方向单调递增或递减
    double monotonicity = 0;

    // 检查行的单调性 - 使用对数值
    for (int i = 0; i < 4; ++i) {
        double current     = 0;
        double left_score  = 0;
        double right_score = 0;

        for (int j = 0; j < 4; ++j) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                // 计算左向单调性
                if (current > value) {
                    left_score += current - value;
                } else {
                    right_score += value - current;
                }
                current = value;
            }
        }
        monotonicity += std::min(left_score, right_score);
    }

    // 检查列的单调性 - 使用对数值
    for (int j = 0; j < 4; ++j) {
        double current    = 0;
        double up_score   = 0;
        double down_score = 0;

        for (int i = 0; i < 4; ++i) {
            double value = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            if (value > 0) {
                // 计算上向单调性
                if (current > value) {
                    up_score += current - value;
                } else {
                    down_score += value - current;
                }
                current = value;
            }
        }
        monotonicity += std::min(up_score, down_score);
    }

    score -= monotonicity * 50;  // 单调性是负分，越小越好

    // 5. 合并可能性 - 奖励相邻相同值
    int mergeScore = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i][j + 1]) {
                mergeScore += boardState[i][j];
            }
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 3; ++i) {
            if (boardState[i][j] > 0 && boardState[i][j] == boardState[i + 1][j]) {
                mergeScore += boardState[i][j];
            }
        }
    }
    score += mergeScore * 10;

    // 6. 大数值聚集 - 奖励大数值聚集在一起
    int clusterScore = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            int cellValue = boardState[i][j];
            if (cellValue > 0) {
                // 检查右边和下边的方块
                if (boardState[i][j + 1] > 0) {
                    clusterScore += std::min(cellValue, boardState[i][j + 1]);
                }
                if (boardState[i + 1][j] > 0) {
                    clusterScore += std::min(cellValue, boardState[i + 1][j]);
                }
            }
        }
    }
    score += clusterScore;

    return score;
}

// simulateMove: 模拟移动
bool Auto::simulateMove(QVector<QVector<int>>& boardState, int direction, int& score) {
    bool moved = false;
    score      = 0;

    if (direction == 0) {  // 向上
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
    } else if (direction == 1) {  // 向右
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
    } else if (direction == 2) {  // 向下
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
    } else if (direction == 3) {  // 向左
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

// monteCarloSimulation: 蒙特卡洛模拟
int Auto::monteCarloSimulation(QVector<QVector<int>> const& boardState, int depth) {
    if (depth <= 0) {
        return evaluateBoardAdvanced(boardState);
    }

    int bestScore = -1;

    // 尝试每个方向
    for (int direction = 0; direction < 4; ++direction) {
        QVector<QVector<int>> boardCopy = boardState;
        int moveScore                   = 0;

        // 模拟移动
        bool moved = simulateMove(boardCopy, direction, moveScore);

        if (moved) {
            // 计算当前移动的分数
            int currentScore = moveScore;

            // 找出空格
            QVector<QPair<int, int>> emptyTiles;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (boardCopy[i][j] == 0) {
                        emptyTiles.append(qMakePair(i, j));
                    }
                }
            }

            if (!emptyTiles.isEmpty()) {
                // 对于每个空格，模拟生成新方块
                int totalFutureScore = 0;
                int simCount         = 0;

                // 为了提高效率，只随机选择一部分空格进行模拟
                int maxSimulations = std::min(3, static_cast<int>(emptyTiles.size()));
                for (int sim = 0; sim < maxSimulations; sim++) {
                    // 随机选择一个空位置
                    int randomIndex = rand() % emptyTiles.size();
                    int row         = emptyTiles[randomIndex].first;
                    int col         = emptyTiles[randomIndex].second;

                    // 模拟生成2和4两种情况
                    for (int newTile : {2, 4}) {
                        QVector<QVector<int>> newBoardCopy = boardCopy;
                        newBoardCopy[row][col]             = newTile;

                        // 递归模拟下一步最佳移动
                        int futureScore = expectimax(newBoardCopy, depth - 1, false);

                        // 加权平均：2出现90%的概率，4出现10%的概率
                        double probability  = (newTile == 2) ? 0.9 : 0.1;
                        totalFutureScore   += static_cast<int>(futureScore * probability);
                        simCount++;
                    }
                }

                // 计算平均未来分数
                if (simCount > 0) {
                    currentScore += totalFutureScore / simCount;
                }
            }

            // 更新最佳分数
            if (currentScore > bestScore) {
                bestScore = currentScore;
            }
        }
    }

    return bestScore > 0 ? bestScore : 0;
}

// 清除expectimax缓存
void Auto::clearExpectimaxCache() {
    expectimaxCache.clear();
    bitboardCache.clear();
}

// 初始化位棋盘的预计算表
void Auto::initTables() {
    // 使用静态标志确保表只初始化一次
    static std::atomic<bool> tablesInitialized(false);
    static QMutex initMutex;

    // 如果表已经初始化，则直接返回
    if (tablesInitialized.load()) {
        return;
    }

    // 使用互斥锁确保只有一个线程初始化表
    QMutexLocker locker(&initMutex);

    // 再次检查，防止多个线程同时通过第一次检查
    if (tablesInitialized.load()) {
        return;
    }

    // 初始化移动表
    for (int row = 0; row < 65536; ++row) {
        // 左移表
        uint16_t line[4] = {static_cast<uint16_t>((row >> 0) & 0xF),
                            static_cast<uint16_t>((row >> 4) & 0xF),
                            static_cast<uint16_t>((row >> 8) & 0xF),
                            static_cast<uint16_t>((row >> 12) & 0xF)};

        // 模拟左移
        uint16_t result = 0;
        int score       = 0;
        bool merged     = false;

        // 先将所有非零元素移动到最左边
        for (int i = 0; i < 4; ++i) {
            if (line[i] == 0) {
                continue;
            }

            // 找到最左边的空位置
            int j = i;
            while (j > 0 && line[j - 1] == 0) {
                line[j - 1] = line[j];
                line[j]     = 0;
                j--;
                merged = true;
            }

            // 如果可以合并
            if (j > 0 && line[j - 1] == line[j] && line[j] != 0) {
                line[j - 1]++;  // 对数增加1，相当于值翻倍
                line[j]  = 0;
                score   += (1 << line[j - 1]);  // 计算分数
                merged   = true;
            }
        }

        // 将结果打包回16位整数
        result = static_cast<uint16_t>(line[0] | (line[1] << 4) | (line[2] << 8) | (line[3] << 12));

        // 存储到移动表中
        row_left_table[row] = result;

        // 右移可以通过左移的逆操作实现
        uint16_t rev_row     = reverse_row(row);
        uint16_t rev_result  = reverse_row(row_left_table[rev_row]);
        row_right_table[row] = rev_result;

        // 上移和下移可以通过转置实现
        BitBoard row_col   = unpack_col(static_cast<uint16_t>(row));
        BitBoard row_left  = unpack_col(row_left_table[row]);
        BitBoard row_right = unpack_col(row_right_table[row]);

        col_up_table[row]   = transpose(row_left);
        col_down_table[row] = transpose(row_right);

        // 计算评分表
        float heur_score = 0.0f;
        float tile_score = 0.0f;

        // 计算每个位置的分数
        for (int i = 0; i < 4; ++i) {
            int tile_value = line[i];
            if (tile_value > 0) {
                // 使用对数值作为分数
                tile_score += (1 << tile_value);

                // 奖励递减排列
                if (i > 0 && line[i - 1] > line[i]) {
                    heur_score += (line[i - 1] - line[i]) * 0.5f;
                }
            }
        }

        heur_score_table[row] = heur_score;
        score_table[row]      = tile_score;
    }

    // 标记表已初始化
    tablesInitialized.store(true);

    // 在主线程中输出调试信息
    QMetaObject::invokeMethod(
        QApplication::instance(), []() { qDebug() << "BitBoard tables initialized"; }, Qt::QueuedConnection);
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
    BitBoard t  = transpose(board);
    score      += heur_score_table[(t >> 0) & ROW_MASK];
    score      += heur_score_table[(t >> 16) & ROW_MASK];
    score      += heur_score_table[(t >> 32) & ROW_MASK];
    score      += heur_score_table[(t >> 48) & ROW_MASK];

    // 空格数量奖励
    int emptyTiles  = countEmptyTiles(board);
    score          += emptyTiles * 30.0f;

    return static_cast<int>(score);
}

// 模拟移动位棋盘 - 使用直接位操作以提高性能
bool Auto::simulateMoveBitBoard(BitBoard& board, int direction, int& score) {
    BitBoard oldBoard = board;
    score             = 0;

    // 使用直接位操作执行移动
    switch (direction) {
        case 0:  // 上
            board = execute_move_0(board);
            break;
        case 1:  // 下
            board = execute_move_1(board);
            break;
        case 2:  // 左
            board = execute_move_2(board);
            break;
        case 3:  // 右
            board = execute_move_3(board);
            break;
        default:
            qDebug() << "Invalid direction:" << direction;
            return false;
    }

    // 如果棋盘没有变化，说明这个方向不能移动
    if (board == oldBoard) {
        qDebug() << "BitBoard move in direction" << direction << "did not change the board";
        return false;
    }

    // 计算分数 - 分数来自于合并的方块
    BitBoard diff = oldBoard ^ board;
    BitBoard temp = diff;
    while (temp != 0U) {
        int pos = __builtin_ctzll(temp) / 4 * 4;
        int val = (board >> pos) & 0xf;
        if (val > 1) {  // 如果发生了合并
            score += (1 << val);
        }
        temp &= ~(0xfULL << pos);
    }

    return true;
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
    static int const MAX_ABSOLUTE_DEPTH = 5;  // 降低最大深度以提高性能
    depth                               = std::min(depth, MAX_ABSOLUTE_DEPTH);

    // 如果到达最大深度，返回评估分数
    if (depth <= 0) {
        // 直接使用位棋盘评估函数
        int score            = evaluateBitBoard(board);
        bitboardCache[state] = score;  // 缓存结果
        return score;
    }

    // 检测游戏是否结束
    if (isGameOverBitBoard(board)) {
        int score            = -500000;  // 游戏结束给予大量惩罚
        bitboardCache[state] = score;
        return score;
    }

    // 快速检测空格数
    int emptyCount = countEmptyTiles(board);

    // 获取最大值
    int maxValue = 0;
    for (int i = 0; i < 16; i++) {
        int shift = i * 4;
        int value = (board >> shift) & 0xF;
        if (value > maxValue) {
            maxValue = value;
        }
    }
    // 将对数值转换为实际值
    maxValue = maxValue > 0 ? (1 << maxValue) : 0;

    // 对于高级棋盘，减少搜索深度以提高性能
    int extraDepth = 0;
    if (maxValue >= 2048) {  // 2048 = 2^11
        // 对于高级棋盘，根据空格数量动态调整搜索深度
        if (emptyCount <= 4) {
            depth = std::min(depth, 3);  // 空格很少时限制深度
        } else {
            extraDepth = 1;  // 空格较多时增加深度
        }
    }

    int result = 0;
    if (isMaxPlayer) {
        // MAX节点：选择最佳移动
        int bestScore = -1;

        // 尝试所有可能的移动
        for (int direction = 0; direction < 4; ++direction) {
            BitBoard boardCopy = board;
            int moveScore      = 0;

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
            int score            = evaluateBitBoard(board);
            bitboardCache[state] = score;
            return score;
        }

        // 优化：对于高级棋盘，只模拟一个空格以提高性能
        int tilesToSimulate = 1;
        if (maxValue < 2048 && emptyCount > 1) {
            tilesToSimulate = std::min(2, emptyCount);
        }

        double totalScore = 0.0;

        // 找到所有空格位置
        QVector<QPair<int, int>> emptyPositions;
        // 直接从位棋盘获取空格位置
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                int pos = (i * 4 + j) * 4;
                int val = (board >> pos) & 0xF;
                if (val == 0) {
                    emptyPositions.append(qMakePair(i, j));
                    if (emptyPositions.size() >= tilesToSimulate) {
                        break;
                    }
                }
            }
            if (emptyPositions.size() >= tilesToSimulate) {
                break;
            }
        }

        // 模拟在空格中放置新方块
        for (int i = 0; i < tilesToSimulate; ++i) {
            int row = emptyPositions[i].first;
            int col = emptyPositions[i].second;

            // 直接在位棋盘上模拟生成2和4
            BitBoard bitBoardWith2 = board;
            int pos2               = (row * 4 + col) * 4;
            // 清零该位置然后设置为2（1对应于位棋盘中的2）
            bitBoardWith2 &= ~(0xfULL << pos2);
            bitBoardWith2 |= 1ULL << pos2;

            // 优化：对于高级棋盘，只考虑生成2的情况
            if (maxValue >= 4096) {  // 4096 = 2^12
                totalScore += expectimaxBitBoard(bitBoardWith2, depth - 1, true);
            } else {
                // 创建生成4的位棋盘
                BitBoard bitBoardWith4 = board;
                // 清零该位置然后设置为4（2对应于位棋盘中的4）
                bitBoardWith4 &= ~(0xfULL << pos2);
                bitBoardWith4 |= 2ULL << pos2;

                // 90%概率生成2，10%概率生成4
                totalScore += 0.9 * expectimaxBitBoard(bitBoardWith2, depth - 1, true);
                totalScore += 0.1 * expectimaxBitBoard(bitBoardWith4, depth - 1, true);
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

    int bestMove       = -1;
    int bestScore      = -1;
    int validMoveCount = 0;

    // 尝试所有可能的移动
    for (int move = 0; move < 4; ++move) {
        BitBoard boardCopy = board;
        int moveScore      = 0;

        bool moved = simulateMoveBitBoard(boardCopy, move, moveScore);

        if (moved) {
            validMoveCount++;
            // 计算此移动的分数
            int score = moveScore + expectimaxBitBoard(boardCopy, 3, false);
            qDebug() << "BitBoard method - Direction:" << move << "Score:" << score;

            if (score > bestScore) {
                bestScore = score;
                bestMove  = move;
            }
        } else {
            qDebug() << "BitBoard method - Direction:" << move << "is not valid";
        }
    }

    // 如果没有有效移动，返回-1表示失败，让标准方法处理
    if (bestMove == -1) {
        qDebug() << "BitBoard method - No valid moves found, returning -1 to fall back to standard method";
        return -1;
    }

    // 验证最佳移动是否真的有效
    QVector<QVector<int>> testBoard = boardState;
    int testScore                   = 0;
    bool validMove                  = simulateMove(testBoard, bestMove, testScore);

    if (!validMove) {
        qDebug() << "BitBoard method selected move" << bestMove
                 << "but it's not valid when tested with standard method";

        // 尝试标准方法找到有效移动
        for (int move = 0; move < 4; ++move) {
            QVector<QVector<int>> moveTestBoard = boardState;
            int moveTestScore                   = 0;
            if (simulateMove(moveTestBoard, move, moveTestScore)) {
                qDebug() << "Found valid move" << move << "using standard method";
                return move;
            }
        }

        return -1;  // 如果标准方法也找不到有效移动，返回-1
    }

    qDebug() << "BitBoard method - Best move:" << bestMove << "with score:" << bestScore << "(" << validMoveCount
             << " valid moves)";
    qDebug() << "Using BitBoard method with best move:" << bestMove;

    return bestMove;
}

// expectimax: 期望最大算法 - 高效版本
int Auto::expectimax(QVector<QVector<int>> const& boardState, int depth, bool isMaxPlayer) {
    // 检查缓存
    BoardState state{boardState, depth, isMaxPlayer};
    auto cacheIt = expectimaxCache.find(state);
    if (cacheIt != expectimaxCache.end()) {
        return cacheIt->second;
    }

    // 绝对深度限制 - 防止过深递归
    static int const MAX_ABSOLUTE_DEPTH = 5;  // 降低最大深度以提高性能
    if (depth > MAX_ABSOLUTE_DEPTH) {
        depth = MAX_ABSOLUTE_DEPTH;
    }

    // 如果到达最大深度，返回评估分数
    if (depth <= 0) {
        int score              = evaluateAdvancedPattern(boardState);
        expectimaxCache[state] = score;  // 缓存结果
        return score;
    }

    // 检测游戏是否结束
    if (isGameOver(boardState)) {
        int score              = -500000;  // 游戏结束给予大量惩罚
        expectimaxCache[state] = score;
        return score;
    }

    // 快速检测最大值和空格数
    int maxValue   = 0;
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
            }
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }

    // 对于高级棋盘，减少搜索深度以提高性能
    int extraDepth = 0;
    if (maxValue >= 2048) {
        // 对于高级棋盘，根据空格数量动态调整搜索深度
        if (emptyCount <= 4) {
            depth = std::min(depth, 3);  // 空格很少时限制深度
        } else {
            extraDepth = 1;  // 空格较多时增加深度
        }
    }

    int result = 0;
    if (isMaxPlayer) {
        // MAX节点：选择最佳移动
        int bestScore = -1;

        // 尝试所有可能的移动
        for (int direction = 0; direction < 4; ++direction) {
            QVector<QVector<int>> boardCopy = boardState;
            int moveScore                   = 0;

            bool moved = simulateMove(boardCopy, direction, moveScore);

            if (moved) {
                // 递归计算期望分数
                int score = moveScore + expectimax(boardCopy, depth - 1 + extraDepth, false);
                bestScore = std::max(bestScore, score);
            }
        }

        result = bestScore > 0 ? bestScore : 0;
    } else {
        // CHANCE节点：随机生成新方块
        // 如果没有空格，返回评估分数
        if (emptyCount == 0) {
            int score              = evaluateBoardAdvanced(boardState);
            expectimaxCache[state] = score;
            return score;
        }

        // 优化：对于高级棋盘，只模拟一个空格以提高性能
        int tilesToSimulate = 1;
        if (maxValue < 2048 && emptyCount > 1) {
            tilesToSimulate = std::min(2, emptyCount);
        }

        double totalScore = 0.0;

        // 优化空格选择策略
        QVector<QPair<int, int>> emptyPositions;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (boardState[i][j] == 0) {
                    emptyPositions.append(qMakePair(i, j));
                    if (emptyPositions.size() >= tilesToSimulate) {
                        break;
                    }
                }
            }
            if (emptyPositions.size() >= tilesToSimulate) {
                break;
            }
        }

        for (int i = 0; i < tilesToSimulate; ++i) {
            int row = emptyPositions[i].first;
            int col = emptyPositions[i].second;

            // 模拟生成2和4两种情况
            QVector<QVector<int>> boardWith2 = boardState;
            boardWith2[row][col]             = 2;

            // 优化：对于高级棋盘，只考虑生成2的情况
            if (maxValue >= 4096) {
                totalScore += expectimax(boardWith2, depth - 1, true);
            } else {
                QVector<QVector<int>> boardWith4 = boardState;
                boardWith4[row][col]             = 4;

                totalScore += 0.9 * expectimax(boardWith2, depth - 1, true);  // 90%概率生成2
                totalScore += 0.1 * expectimax(boardWith4, depth - 1, true);  // 10%概率生成4
            }
        }

        result = static_cast<int>(totalScore / tilesToSimulate);
    }

    // 缓存结果
    expectimaxCache[state] = result;

    // 限制缓存大小以防止内存溢出
    if (expectimaxCache.size() > 10000) {
        // 当缓存过大时清除
        expectimaxCache.clear();
    }

    return result;
}

// evaluateWithParams: 使用给定参数评估棋盘
int Auto::evaluateWithParams(QVector<QVector<int>> const& boardState, QVector<double> const& params) {
    int score = 0;

    // 确保参数数量足够
    if (params.size() < 5) {
        return evaluateBoardAdvanced(boardState);  // 如果参数不足，使用默认评估函数
    }

    // 1. 空格数量权重 - 空格越多越好
    int emptyCount = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] == 0) {
                emptyCount++;
            }
        }
    }
    score += static_cast<int>(emptyCount * params[0] * 10);

    // 2. 蛇形模式权重 - 使用改进的蛇形模式
    int const snakePattern[4][4] = {{16, 15, 14, 13}, {9, 10, 11, 12}, {8, 7, 6, 5}, {1, 2, 3, 4}};
    int snakeScore               = 0;

    // 找出最大值及其位置
    int maxValue = 0;
    int maxRow = 0, maxCol = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > maxValue) {
                maxValue = boardState[i][j];
                maxRow   = i;
                maxCol   = j;
            }
        }
    }

    // 根据最大值的位置调整蛇形模式
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > 0) {
                // 对于高值方块，给予更高的权重
                double tileValue  = boardState[i][j];
                double logValue   = log2(tileValue);
                snakeScore       += static_cast<int>(tileValue * snakePattern[i][j] * (logValue / 11.0));
            }
        }
    }
    score += static_cast<int>(snakeScore * params[1] / 10);

    // 3. 平滑度权重 - 使用对数差异来减少大数值之间的差异惩罚
    int smoothness = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (boardState[i][j] > 0 && boardState[i][j + 1] > 0) {
                // 使用对数差异
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i][j + 1] > 0 ? log2(boardState[i][j + 1]) : 0;
                smoothness     -= static_cast<int>(std::abs(logVal1 - logVal2) * 2.0);
            }
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (boardState[i][j] > 0 && boardState[i + 1][j] > 0) {
                // 使用对数差异
                double logVal1  = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
                double logVal2  = boardState[i + 1][j] > 0 ? log2(boardState[i + 1][j]) : 0;
                smoothness     -= static_cast<int>(std::abs(logVal1 - logVal2) * 2.0);
            }
        }
    }
    score += static_cast<int>(smoothness * params[2]);

    // 4. 单调性权重 - 改进的单调性计算
    int monotonicity = 0;

    // 计算行的单调性
    for (int i = 0; i < 4; ++i) {
        // 计算左右方向的单调性
        int left_right = 0;
        int right_left = 0;

        for (int j = 0; j < 3; ++j) {
            double curr = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            double next = boardState[i][j + 1] > 0 ? log2(boardState[i][j + 1]) : 0;

            if (curr > next) {
                left_right += static_cast<int>((curr - next) * 2);
            } else {
                right_left += static_cast<int>((next - curr) * 2);
            }
        }

        monotonicity += std::min(left_right, right_left);
    }

    // 计算列的单调性
    for (int j = 0; j < 4; ++j) {
        // 计算上下方向的单调性
        int up_down = 0;
        int down_up = 0;

        for (int i = 0; i < 3; ++i) {
            double curr = boardState[i][j] > 0 ? log2(boardState[i][j]) : 0;
            double next = boardState[i + 1][j] > 0 ? log2(boardState[i + 1][j]) : 0;

            if (curr > next) {
                up_down += static_cast<int>((curr - next) * 2);
            } else {
                down_up += static_cast<int>((next - curr) * 2);
            }
        }

        monotonicity += std::min(up_down, down_up);
    }

    score -= static_cast<int>(monotonicity * params[3]);

    // 5. 合并可能性权重 - 奖励相邻相同值
    double mergeScore  = calculateMergeScore(boardState);
    score             += static_cast<int>(mergeScore * params[4]);

    // 6. 角落奖励 - 如果最大值在角落，给予额外奖励
    if ((maxRow == 0 || maxRow == 3) && (maxCol == 0 || maxCol == 3)) {
        score += static_cast<int>(maxValue * 2);
    }

    return score;
}

// simulateFullGame: 模拟完整游戏
int Auto::simulateFullGame(QVector<double> const& params) {
    int score   = 0;
    int maxTile = 0;
    simulateFullGameDetailed(params, score, maxTile);
    return score;
}

// simulateFullGameDetailed: 模拟完整游戏并返回详细信息
void Auto::simulateFullGameDetailed(QVector<double> const& params, int& score, int& maxTile) {
    // 初始化模拟棋盘
    QVector<QVector<int>> simBoard(4, QVector<int>(4, 0));
    score   = 0;
    maxTile = 0;

    // 随机生成两个初始方块
    QVector<QPair<int, int>> emptyTiles;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            emptyTiles.append(qMakePair(i, j));
        }
    }

    // 随机选择两个位置生成初始方块
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int i = 0; i < 2; ++i) {
        std::uniform_int_distribution<> posDis(0, static_cast<int>(emptyTiles.size()) - 1);
        std::uniform_int_distribution<> valueDis(0, 9);  // 0-8 生成2，9 生成4

        int randomIndex    = posDis(gen);
        int row            = emptyTiles[randomIndex].first;
        int col            = emptyTiles[randomIndex].second;
        simBoard[row][col] = (valueDis(gen) < 9) ? 2 : 4;  // 90%的概率生成2，10%生成4
        emptyTiles.removeAt(randomIndex);
    }

    // 模拟游戏过程，直到游戏结束
    bool gameOver = false;
    int moveCount = 0;
    int maxMoves  = 2000;  // 增加最大移动次数，以便能达到2048及以上

    while (!gameOver && moveCount < maxMoves) {
        // 更新最大砖块值
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (simBoard[i][j] > maxTile) {
                    maxTile = simBoard[i][j];
                }
            }
        }

        // 找出最佳移动方向
        int bestDirection = -1;
        int bestScore     = -1;

        // 尝试每个方向
        for (int direction = 0; direction < 4; ++direction) {
            QVector<QVector<int>> boardCopy = simBoard;
            int moveScore                   = 0;

            // 模拟移动
            bool moved = simulateMove(boardCopy, direction, moveScore);

            if (moved) {
                // 对于高级棋盘，使用更复杂的评估
                int evalScore = 0;
                if (maxTile >= 1024) {
                    // 对于高级棋盘，使用更复杂的评估和更深的搜索
                    evalScore = evaluateAdvancedPattern(boardCopy);
                    // 深度为2的蒙特卡洛模拟
                    evalScore += monteCarloSimulation(boardCopy, 2);
                } else {
                    // 对于低级棋盘，使用参数化评估
                    evalScore = evaluateWithParams(boardCopy, params);
                }

                int score = evalScore + moveScore;

                if (score > bestScore) {
                    bestScore     = score;
                    bestDirection = direction;
                }
            }
        }

        // 如果没有可用的移动，游戏结束
        if (bestDirection == -1) {
            gameOver = true;
            continue;
        }

        // 执行最佳移动
        int moveScore = 0;
        simulateMove(simBoard, bestDirection, moveScore);
        score += moveScore;
        moveCount++;

        // 随机生成新方块
        emptyTiles.clear();
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (simBoard[i][j] == 0) {
                    emptyTiles.append(qMakePair(i, j));
                }
            }
        }

        if (emptyTiles.isEmpty()) {
            gameOver = true;
            continue;
        }

        std::uniform_int_distribution<> posDis(0, static_cast<int>(emptyTiles.size()) - 1);
        std::uniform_int_distribution<> valueDis(0, 9);  // 0-8 生成2，9 生成4

        int randomIndex    = posDis(gen);
        int row            = emptyTiles[randomIndex].first;
        int col            = emptyTiles[randomIndex].second;
        simBoard[row][col] = (valueDis(gen) < 9) ? 2 : 4;  // 90%的概率生成2，10%生成4
    }

    // 最后再次检查最大砖块值
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (simBoard[i][j] > maxTile) {
                maxTile = simBoard[i][j];
            }
        }
    }
}

// 评估参数性能 - 更全面地评估参数的效果
int Auto::evaluateParameters(QVector<double> const& params, int simulations) {
    int totalScore        = 0;
    int maxTile           = 0;
    int successCount      = 0;  // 计算达到2048的次数
    int highTileCount     = 0;  // 计算达到4096的次数
    int veryHighTileCount = 0;  // 计算达到8192的次数

    // 保存所有游戏的分数和最大砖块，以计算标准差
    QVector<int> allScores;
    QVector<int> allMaxTiles;

    try {
        // 运行多次模拟游戏并计算平均分数
        for (int i = 0; i < simulations; ++i) {
            int gameScore = 0;
            int gameTile  = 0;

            // 模拟完整游戏并返回分数和最大砖块
            simulateFullGameDetailed(params, gameScore, gameTile);

            totalScore += gameScore;
            maxTile     = std::max(maxTile, gameTile);

            allScores.append(gameScore);
            allMaxTiles.append(gameTile);

            // 计算不同级别的成功率
            if (gameTile >= 2048) {
                successCount++;
                if (gameTile >= 4096) {
                    highTileCount++;
                    if (gameTile >= 8192) {
                        veryHighTileCount++;
                    }
                }
            }
        }

        // 计算平均分数和标准差
        int avgScore = totalScore / simulations;

        // 计算分数的标准差
        double scoreVariance = 0;
        for (int score : allScores) {
            scoreVariance += (score - avgScore) * (score - avgScore);
        }
        double scoreStdDev = std::sqrt(scoreVariance / simulations);

        // 计算成功率
        int successRate      = (successCount * 100) / simulations;
        int highTileRate     = (highTileCount * 100) / simulations;
        int veryHighTileRate = (veryHighTileCount * 100) / simulations;

        // 输出详细的评估结果
        qDebug() << "\nEvaluation results:";
        qDebug() << "Average score:" << avgScore << "points (std dev:" << scoreStdDev << ")";
        qDebug() << "Success rates: 2048:" << successRate << "%, 4096:" << highTileRate
                 << "%, 8192:" << veryHighTileRate << "%";
        qDebug() << "Max tile achieved:" << maxTile;

        // 改进的加权分数公式
        // 1. 平均分数权重
        double scoreWeight = 1.0;

        // 2. 成功率奖励 - 非线性增长
        double successBonus = successRate * 100 + highTileRate * 500 + veryHighTileRate * 2000;

        // 3. 最大砖块奖励 - 指数增长
        double maxTileBonus = 0;
        if (maxTile >= 2048) {
            // 对于高级砖块，给予指数级奖励
            int logTile  = static_cast<int>(log2(maxTile));
            maxTileBonus = std::pow(2, logTile - 10) * 2000;  // 2048 = 2^11, 所以 11-10=1
        }

        // 4. 稳定性奖励 - 如果标准差小，说明策略更稳定
        double stabilityBonus = 0;
        if (scoreStdDev < avgScore * 0.3) {  // 如果标准差小于平均值的30%
            stabilityBonus = 5000;
        } else if (scoreStdDev < avgScore * 0.5) {  // 如果标准差小于平均值的50%
            stabilityBonus = 2000;
        }

        // 计算最终分数
        int finalScore = static_cast<int>(avgScore * scoreWeight + successBonus + maxTileBonus + stabilityBonus);

        qDebug() << "Final weighted score:" << finalScore;
        qDebug() << "  - Base score:" << static_cast<int>(avgScore * scoreWeight);
        qDebug() << "  - Success bonus:" << static_cast<int>(successBonus);
        qDebug() << "  - Max tile bonus:" << static_cast<int>(maxTileBonus);
        qDebug() << "  - Stability bonus:" << static_cast<int>(stabilityBonus);

        return finalScore;
    } catch (std::exception const& e) {
        qDebug() << "Exception in evaluateParameters:" << e.what();
        return 0;  // 发生异常时返回0分
    } catch (...) {
        qDebug() << "Unknown exception in evaluateParameters";
        return 0;  // 发生未知异常时返回0分
    }
}

// 保存参数到文件
bool Auto::saveParameters(QString const& filename) {
    // 确保数据目录存在
    QString dataDirPath = getDataDirPath();
    QDir dataDir;

    if (!dataDir.exists(dataDirPath)) {
        bool pathCreated = dataDir.mkpath(dataDirPath);
        qDebug() << "Creating data directory at" << dataDirPath << (pathCreated ? "success" : "failed");

        if (!pathCreated) {
            qDebug() << "Failed to create data directory for parameters";
            return false;
        }
    }

    // 准备文件名
    QString saveFilename;
    if (filename.isEmpty()) {
        // 生成默认文件名，包含时间戳
        QDateTime now = QDateTime::currentDateTime();
        saveFilename  = dataDirPath + "/2048_ai_params_" + now.toString("yyyyMMdd_hhmmss") + ".json";
    } else if (filename.startsWith("/")) {
        // 如果是绝对路径，直接使用
        saveFilename = filename;
    } else {
        // 其他情况，添加到数据目录
        saveFilename = dataDirPath + "/" + filename;
    }

    qDebug() << "Preparing to save parameters to:" << saveFilename;

    // 创建 JSON 对象
    QJsonObject paramsObj;
    QJsonArray paramsArray;

    for (double param : strategyParams) {
        paramsArray.append(param);
    }

    paramsObj["parameters"] = paramsArray;
    paramsObj["score"]      = bestHistoricalScore;  // 存储该参数集的最高分
    paramsObj["timestamp"]  = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(paramsObj);

    // 先尝试保存持久化数据，确保它被保存
    bool persistentSaved = savePersistentData(doc);
    if (!persistentSaved) {
        qDebug() << "Warning: Failed to save persistent data";
    }

    // 然后保存参数文件
    QFile file(saveFilename);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << saveFilename << ": " << file.errorString();

        // 尝试使用绝对路径
        QString absPath = QDir::currentPath() + "/" + saveFilename;
        QFile absFile(absPath);
        qDebug() << "Trying absolute path:" << absPath;

        if (absFile.open(QIODevice::WriteOnly)) {
            absFile.write(doc.toJson());
            absFile.close();
            qDebug() << "Parameters saved to absolute path:" << absPath;
            return true;
        } else {
            qDebug() << "Still failed to save parameters using absolute path:" << absFile.errorString();
            // 即使参数文件保存失败，只要持久化数据保存成功，也返回true
            return persistentSaved;
        }
    }

    file.write(doc.toJson());
    file.close();
    qDebug() << "Parameters saved to:" << saveFilename;
    return true;
}

// 保存持久化数据
bool Auto::savePersistentData(QJsonDocument const& doc) {
    // 确保数据目录存在
    QString dataDirPath = getDataDirPath();
    QDir dataDir;

    if (!dataDir.exists(dataDirPath)) {
        bool pathCreated = dataDir.mkpath(dataDirPath);
        qDebug() << "Creating data directory at" << dataDirPath << (pathCreated ? "success" : "failed");

        if (!pathCreated) {
            qDebug() << "Failed to create data directory for persistent data";
            return false;
        }
    }

    // 保存数据
    QString persistentFilePath = dataDirPath + "/2048_ai_persistent_data.json";
    QFile persistentDataFile(persistentFilePath);

    qDebug() << "Attempting to save persistent data to:" << persistentFilePath;

    if (!persistentDataFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to save persistent data to:" << persistentFilePath << ": "
                 << persistentDataFile.errorString();
        return false;
    }

    persistentDataFile.write(doc.toJson());
    persistentDataFile.close();
    qDebug() << "Successfully saved persistent training data to:" << persistentFilePath;
    return true;
}

// 从文件加载参数
bool Auto::loadParameters(QString const& filename) {
    QString loadFilename = filename;
    if (loadFilename.isEmpty()) {
        // 如果没有指定文件名，尝试加载默认文件
        loadFilename = "data/2048_ai_params_default.json";
    } else if (!loadFilename.startsWith("data/") && !loadFilename.startsWith("/")) {
        // 如果没有指定路径，先尝试从数据目录加载
        QString dataFilename = "data/" + loadFilename;
        QFile dataFile(dataFilename);
        if (dataFile.exists()) {
            loadFilename = dataFilename;
        }
    }

    QFile file(loadFilename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for reading:" << loadFilename;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("parameters") || !obj["parameters"].isArray()) {
        return false;
    }

    QJsonArray paramsArray = obj["parameters"].toArray();
    QVector<double> loadedParams;

    for (auto const& value : paramsArray) {
        if (value.isDouble()) {
            loadedParams.append(value.toDouble());
        }
    }

    // 确保参数数量正确
    if (loadedParams.size() < 5) {
        return false;
    }

    // 加载历史最佳分数
    if (obj.contains("score")) {
        bestHistoricalScore = obj["score"].toInt();
    } else {
        bestHistoricalScore = 0;
    }

    // 更新参数
    strategyParams   = loadedParams;
    useLearnedParams = true;

    return true;
}

// learnParameters: 学习最佳参数
void Auto::learnParameters(int populationSize, int generations, int simulations) {
    // 如果已经在训练中，则返回
    if (trainingActive.load()) {
        return;
    }

    // 清除缓存，确保内存干净
    clearExpectimaxCache();
    bitboardCache.clear();

    // 设置训练状态
    trainingActive.store(true);

    // 创建训练线程，避免阻塞主线程
    QThread* trainingThread = new QThread();
    TrainingWorker* worker  = new TrainingWorker(this, populationSize, generations, simulations);
    worker->moveToThread(trainingThread);

    // 连接信号和槽
    connect(trainingThread, &QThread::started, worker, &TrainingWorker::doTraining);
    connect(worker, &TrainingWorker::finished, trainingThread, &QThread::quit);
    connect(worker, &TrainingWorker::finished, worker, &QObject::deleteLater);
    connect(trainingThread, &QThread::finished, trainingThread, &QObject::deleteLater);

    // 添加错误处理
    connect(trainingThread, &QThread::finished, [this]() {
        // 确保训练状态被重置，即使发生错误
        trainingActive.store(false);
        qDebug() << "Training thread finished and cleaned up";
    });

    // 启动训练线程
    trainingThread->start();
}

// findTopIndices: 找出最高分数的索引
QVector<int> Auto::findTopIndices(QVector<int> const& scores, int count) {
    QVector<int> indices(scores.size());
    for (int i = 0; i < scores.size(); ++i) {
        indices[i] = i;
    }

    // 根据分数排序索引
    std::sort(indices.begin(), indices.end(), [&scores](int a, int b) { return scores[a] > scores[b]; });

    // 返回前 count 个索引
    return indices.mid(0, count);
}

// tournamentSelection: 选择算法
int Auto::tournamentSelection(QVector<int> const& scores) {
    // 使用现代C++随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(scores.size()) - 1);

    // 随机选择3个个体进行比赛
    int a = dis(gen);
    int b = dis(gen);
    int c = dis(gen);

    // 返回分数最高的
    if (scores[a] >= scores[b] && scores[a] >= scores[c]) {
        return a;
    }
    if (scores[b] >= scores[a] && scores[b] >= scores[c]) {
        return b;
    }
    return c;
}

// crossover: 交叉算法
QVector<double> Auto::crossover(QVector<double> const& parent1, QVector<double> const& parent2) {
    QVector<double> child(parent1.size());

    // 使用现代C++随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    // 均匀交叉
    for (int i = 0; i < parent1.size(); ++i) {
        // 50%的概率从父代1继承，50%的概率从父代2继承
        child[i] = (dis(gen) == 0) ? parent1[i] : parent2[i];
    }

    return child;
}

// mutate: 变异算法
void Auto::mutate(QVector<double>& params, double mutationRate) {
    // 使用现代C++随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::uniform_real_distribution<> change_dis(-0.5, 0.5);  // 增大变异幅度到 -50% 到 +50%

    // 每个参数有 mutationRate 的概率发生变异
    for (int i = 0; i < params.size(); ++i) {
        if (dis(gen) < mutationRate) {
            // 变异幅度为当前值的 -50% 到 +50%
            double change  = params[i] * change_dis(gen);
            params[i]     += change;

            // 确保参数不为负且有上限
            params[i] = std::max(0.0, std::min(params[i], 20.0));
        }
    }
}
