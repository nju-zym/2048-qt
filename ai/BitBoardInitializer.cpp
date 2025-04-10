#include "BitBoardInitializer.h"
#include "BitBoard.h"
#include <QDebug>

BitBoardInitializer::BitBoardInitializer(QObject* parent) : QObject(parent) {
    // 将对象移动到工作线程
    moveToThread(&workerThread);
    
    // 连接信号和槽
    connect(&workerThread, &QThread::started, this, &BitBoardInitializer::doInitialization);
    
    // 启动工作线程
    workerThread.start();
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
        workerThread.start();
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
