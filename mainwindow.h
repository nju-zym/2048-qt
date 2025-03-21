#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QPair>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

   protected:
    void keyPressEvent(QKeyEvent* event) override;

   private slots:
    void on_newGameButton_clicked();
    void on_undoButton_clicked();
    void on_settingsButton_clicked();

   private:
    Ui::MainWindow* ui;

    // 游戏数据
    QVector<QVector<int>> board;
    QVector<QVector<QLabel*>> tileLabels;
    int score;
    int bestScore;
    QVector<QPair<QVector<QVector<int>>, int>> history;  // 用于撤销操作，存储棋盘状态和分数

    // 初始化函数
    void setupBoard();
    void initializeTiles();
    void startNewGame();

    // 游戏逻辑
    bool moveTiles(int direction);  // 0=up, 1=right, 2=down, 3=left
    void generateNewTile();
    bool isMoveAvailable() const;
    bool isGameOver() const;
    bool isGameWon() const;
    bool isTileEmpty(int row, int col) const;        // 检查格子是否为空
    QVector<QPair<int, int>> getEmptyTiles() const;  // 获取所有空格子

    // UI更新
    void updateTileAppearance(int row, int col);
    void updateScore(int newScore);
    void updateStatus(QString const& message);
    QString getTileStyleSheet(int value) const;
    void showGameOverMessage();
    void showWinMessage();

    // 动画
    void animateTileMovement(QLabel* label, QPoint const& from, QPoint const& to);
    void animateTileMerge(QLabel* label);
};

#endif  // MAINWINDOW_H