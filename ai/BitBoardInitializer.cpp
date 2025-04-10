#include "BitBoardInitializer.h"

#include "BitBoard.h"

#include <QDebug>
#include <QDir>
#include <QFile>

// 初始化静态成员
BitBoardInitializer* BitBoardInitializer::instance = nullptr;

BitBoardInitializer* BitBoardInitializer::getInstance(QObject* parent) {
    if (!instance) {
        instance = new BitBoardInitializer(parent);
    }
    return instance;
}

BitBoardInitializer::BitBoardInitializer(QObject* parent) : QObject(parent) {
    // 将对象移动到工作线程
    moveToThread(&workerThread);

    // 连接信号和槽
    connect(&workerThread, &QThread::started, this, &BitBoardInitializer::doInitialization);
}

BitBoardInitializer::~BitBoardInitializer() {
    // 停止工作线程
    workerThread.quit();
    workerThread.wait();
}

void BitBoardInitializer::initialize() {
    // 如果表已经初始化，直接发出完成信号
    if (BitBoard::areTablesInitialized()) {
        emit initializationCompleted();
        return;
    }

    // 否则，启动工作线程
    if (!workerThread.isRunning()) {
        workerThread.start(QThread::HighPriority);
    }
}

void BitBoardInitializer::doInitialization() {
    qDebug() << "Starting BitBoard tables initialization...";

    // 执行初始化
    BitBoard::initializeTablesAsync();

    qDebug() << "BitBoard tables initialization completed.";

    // 发出完成信号
    emit initializationCompleted();

    // 退出工作线程
    workerThread.quit();
}

void BitBoardInitializer::generateTables() {
    // 调用生成查找表的函数
    generateAndSaveTables();

    // 将生成的文件复制到项目目录
    QFile generatedFile("BitBoardTables.cpp");
    if (generatedFile.exists()) {
        // 确保目标目录存在
        QDir dir;
        if (!dir.exists("ai")) {
            dir.mkdir("ai");
        }

        // 复制文件
        if (generatedFile.copy("ai/BitBoardTables.cpp")) {
            qDebug() << "查找表已复制到项目目录: ai/BitBoardTables.cpp";
        } else {
            qDebug() << "无法复制查找表到项目目录:" << generatedFile.errorString();
        }
    } else {
        qDebug() << "生成的查找表文件不存在";
    }
}
