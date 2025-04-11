#include "BitBoard.h"

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent>

// 使用内存映射文件持久化查找表
// 这种方法可以避免每次启动程序时都重新计算查找表

// 查找表文件名
static QString const LOOKUP_TABLE_FILE = "bitboard_lookup_tables.dat";

// 获取查找表文件路径
static QString getLookupTableFilePath() {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return cacheDir + "/" + LOOKUP_TABLE_FILE;
}

// 从文件加载查找表
static bool loadTablesFromFile(std::array<uint16_t, 65536>& leftTable,
                               std::array<uint16_t, 65536>& rightTable,
                               std::array<uint16_t, 65536>& scoreTable) {
    QString filePath = getLookupTableFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开查找表文件:" << filePath;
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);

    // 读取文件头和版本号
    quint32 magic   = 0;
    quint32 version = 0;
    in >> magic >> version;

    if (magic != 0x20'48'BE'EF || version != 1) {
        qWarning() << "查找表文件格式不正确";
        file.close();
        return false;
    }

    // 读取表数据
    for (int i = 0; i < 65536; ++i) {
        quint16 left  = 0;
        quint16 right = 0;
        quint16 score = 0;
        in >> left >> right >> score;
        leftTable[i]  = left;
        rightTable[i] = right;
        scoreTable[i] = score;
    }

    file.close();
    return true;
}

// 将查找表保存到文件
static bool saveTableToFile(std::array<uint16_t, 65536> const& leftTable,
                            std::array<uint16_t, 65536> const& rightTable,
                            std::array<uint16_t, 65536> const& scoreTable) {
    QString filePath = getLookupTableFilePath();
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法创建查找表文件:" << filePath;
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);

    // 写入文件头和版本号
    out << static_cast<quint32>(0x20'48'BE'EF) << static_cast<quint32>(1);

    // 写入表数据
    for (int i = 0; i < 65536; ++i) {
        out << static_cast<quint16>(leftTable[i]) << static_cast<quint16>(rightTable[i])
            << static_cast<quint16>(scoreTable[i]);
    }

    file.close();
    return true;
}

// 优化版的initializeTables方法，使用持久化查找表
void BitBoard::initializeTables() {
    QElapsedTimer timer;
    timer.start();

    qDebug() << "开始初始化BitBoard表...";

    // 尝试从文件加载查找表
    if (loadTablesFromFile(moveTable[LEFT], moveTable[RIGHT], scoreTable)) {
        qDebug() << "从文件加载BitBoard表成功，耗时" << timer.elapsed() << "毫秒";
        tablesInitialized = true;
        return;
    }

    qDebug() << "从文件加载BitBoard表失败，开始计算...";

    // 使用线程池并行计算
    QThreadPool threadPool;
    int maxThreads = QThread::idealThreadCount() * 2;
    threadPool.setMaxThreadCount(maxThreads);

    qDebug() << "使用" << maxThreads << "个线程进行初始化";

    // 将65536个可能的行状态分成多个批次
    int const batchSize  = 1024;
    int const numBatches = 65536 / batchSize;

    // 使用QtConcurrent::map并行处理每个批次
    QVector<int> batchIndices;
    for (int i = 0; i < numBatches; ++i) {
        batchIndices.append(i);
    }

    // 并行处理所有批次
    QtConcurrent::blockingMap(batchIndices, [&](int batch) {
        int startRow = batch * batchSize;
        int endRow   = (batch + 1) * batchSize;

        // 处理一个批次的行
        for (int row = startRow; row < endRow; ++row) {
            // 计算向左移动的结果
            uint16_t leftResult  = moveRow(row, LEFT);
            moveTable[LEFT][row] = leftResult;
            scoreTable[row]      = calculateScore(row, leftResult);

            // 计算向右移动的结果
            uint16_t rightResult  = moveRow(row, RIGHT);
            moveTable[RIGHT][row] = rightResult;
        }
    });

    // 将查找表保存到文件
    saveTableToFile(moveTable[LEFT], moveTable[RIGHT], scoreTable);

    tablesInitialized = true;

    qDebug() << "BitBoard表初始化完成，耗时" << timer.elapsed() << "毫秒";
}
