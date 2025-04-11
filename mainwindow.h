#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ai/BitBoardInitializer.h"
#include "ai/ExpectimaxAI.h"
#include "ai/HybridAI.h"
#include "ai/ParallelExpectimaxAI.h"
#include "ui/GameView.h"
#include "utils/GameController.h"

#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QProgressDialog>
#include <QVector>

namespace Ui {
class MainWindow;
}  // namespace Ui

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // 键盘事件处理
    void keyPressEvent(QKeyEvent* event) override;

    // 更新界面元素
    void updateScore(int newScore);
    void updateStatus(QString const& message);

   private slots:
    void on_newGameButton_clicked();
    void on_undoButton_clicked();
    void on_settingsButton_clicked();
    void on_aiButton_clicked();
    void onBitBoardInitializationCompleted();

   private:
    Ui::MainWindow* ui;
    GameView* gameView;
    GameController* gameController;
    BitBoardInitializer* bitBoardInitializer;
    QProgressDialog* progressDialog;
};

#endif  // MAINWINDOW_H