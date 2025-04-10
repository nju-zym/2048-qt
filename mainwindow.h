#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui/GameView.h"
#include "utils/GameController.h"

#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
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

   private:
    Ui::MainWindow* ui;
    GameView* gameView;
    GameController* gameController;
};

#endif  // MAINWINDOW_H