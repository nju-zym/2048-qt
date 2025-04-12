#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "game/GameController.h"
#include "ui/UIManager.h"

#include <QKeyEvent>
#include <QMainWindow>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void keyPressEvent(QKeyEvent* event) override;

   private slots:
    // 按钮事件
    void on_newGameButton_clicked();
    void on_undoButton_clicked();
    void on_settingsButton_clicked();
    void on_autoPlayButton_clicked();

    // 游戏事件处理
    void handleScoreUpdate(int newScore);
    void handleTileAdded(int row, int col, int value, bool animate);
    void handleTileMoved(int fromRow, int fromCol, int toRow, int toCol, int value, bool merged);
    void handleGameOver();
    void handleGameWon();
    void handleStatusUpdate(QString const& message);
    void handleBoardUpdate();

   private:
    Ui::MainWindow* ui;

    // 游戏控制器
    GameController* gameController;

    // UI管理器
    UIManager* uiManager;

    // 辅助函数
    void showGameOverMessage();
    void showWinMessage();
};

#endif  // MAINWINDOW_H