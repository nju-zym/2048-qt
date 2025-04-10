#ifndef BITBOARD_INITIALIZER_H
#define BITBOARD_INITIALIZER_H

#include <QObject>
#include <QThread>

class BitBoardInitializer : public QObject {
    Q_OBJECT
    
public:
    explicit BitBoardInitializer(QObject* parent = nullptr);
    ~BitBoardInitializer();
    
    void initialize();
    
signals:
    void initializationCompleted();
    
private slots:
    void doInitialization();
    
private:
    QThread workerThread;
};

#endif // BITBOARD_INITIALIZER_H
