#ifndef BITBOARD_INITIALIZER_H
#define BITBOARD_INITIALIZER_H

#include <QObject>
#include <QThread>

// 声明生成查找表的函数
// 定义在 GenerateTables.cpp 中
extern void generateAndSaveTables();

class BitBoardInitializer : public QObject {
    Q_OBJECT

   public:
    // 获取单例
    static BitBoardInitializer* getInstance(QObject* parent = nullptr);

    // 析构函数
    ~BitBoardInitializer();

    // 禁止复制和赋值
    BitBoardInitializer(BitBoardInitializer const&)            = delete;
    BitBoardInitializer& operator=(BitBoardInitializer const&) = delete;
    BitBoardInitializer(BitBoardInitializer&&)                 = delete;
    BitBoardInitializer& operator=(BitBoardInitializer&&)      = delete;

    // 初始化方法
    void initialize();

    // 生成并保存查找表
    void generateTables();

   private:
    // 私有构造函数
    explicit BitBoardInitializer(QObject* parent = nullptr);

    // 单例指针
    static BitBoardInitializer* instance;

   signals:
    void initializationCompleted();

   private slots:
    void doInitialization();

   private:
    QThread workerThread;
};

#endif  // BITBOARD_INITIALIZER_H
