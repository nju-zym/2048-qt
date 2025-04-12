#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QFuture>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QVector>
#include <memory>  // 添加智能指针支持

// 前向声明
class Auto;
class QLabel;

class GameController : public QObject {
    Q_OBJECT

   public:
    explicit GameController(QObject* parent = nullptr);
    ~GameController();

    // 游戏状态操作
    void setupBoard();
    void startNewGame();
    bool moveTiles(int direction);
    void generateNewTile(bool animate);

    // 游戏状态检查
    bool isGameOver() const;
    bool isGameWon() const;
    bool isMoveAvailable() const;
    bool isTileEmpty(int row, int col) const;
    QVector<QPair<int, int>> getEmptyTiles() const;

    // 自动操作相关方法
    void toggleAutoPlay();
    void startAiCalculation();
    int findBestMove();

    // 获取当前游戏状态
    QVector<QVector<int>> const& getBoard() const {
        return board;
    }
    int getScore() const {
        return score;
    }
    int getBestScore() const {
        return bestScore;
    }
    bool isAutoPlayActive() const {
        return autoPlayActive;
    }

    // 保存历史状态
    void saveHistory();
    void undo();

   signals:
    void scoreUpdated(int newScore);
    void boardUpdated();
    void gameWon();
    void gameOver();
    void tileAdded(int row, int col, int value, bool animate);
    void tileMoved(int fromRow, int fromCol, int toRow, int toCol, int value, bool merged);
    void statusUpdated(QString const& message);

   public slots:
    void onAiCalculationFinished();
    void onAiCalculationTimeout();
    void autoPlayStep();

   private:
    // 游戏数据
    QVector<QVector<int>> board;
    int score;
    int bestScore;
    // 使用共享指针存储历史记录，避免深拷贝
    using BoardHistory = std::shared_ptr<QPair<QVector<QVector<int>>, int>>;
    QVector<BoardHistory> history;

    // 自动操作相关 - 使用智能指针
    bool autoPlayActive;
    std::unique_ptr<Auto> autoPlayer;        // 使用unique_ptr替代原生指针
    std::shared_ptr<QTimer> autoPlayTimer;   // 使用shared_ptr管理QTimer
    std::shared_ptr<QTimer> aiTimeoutTimer;  // 使用shared_ptr管理QTimer

    // AI计算状态
    QMutex aiMutex;
    bool aiCalculating;
    int aiCalculatedMove;
    QFuture<int> aiFuture;

    // 游戏状态标志
    bool winAlertShown;
};

#endif  // GAMECONTROLLER_H