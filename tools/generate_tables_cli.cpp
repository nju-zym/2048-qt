#include "../ai/BitBoardInitializer.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("BitBoardTableGenerator");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Generate BitBoard lookup tables");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(app);

    qDebug() << "Generating BitBoard lookup tables...";

    // 生成查找表
    BitBoardInitializer* initializer = BitBoardInitializer::getInstance();
    initializer->generateTables();

    qDebug() << "BitBoard lookup tables generation completed.";

    return 0;
}
