#include "mainwindow.h"

#include "ui_mainwindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);  // 确保窗口能响应键盘事件

    // 创建游戏视图
    gameView = new GameView(this, ui->boardLayout);
    gameView->initializeTiles();

    // 创建游戏控制器
    gameController = new GameController(this);
    gameController->setGameView(gameView);

    // 开始新游戏
    gameController->startNewGame();
}

MainWindow::~MainWindow() {
    delete ui;
    // gameView 和 gameController 由 Qt 的父子关系自动释放
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    gameController->handleKeyPress(event);
}

void MainWindow::updateScore(int newScore) {
    // 更新分数显示
    ui->scoreValue->setText(QString::number(newScore));

    // 更新最高分
    int bestScore = ui->bestValue->text().toInt();
    if (newScore > bestScore) {
        ui->bestValue->setText(QString::number(newScore));
    }
}

void MainWindow::updateStatus(QString const& message) {
    ui->statusLabel->setText(message);
}

void MainWindow::on_newGameButton_clicked() {
    gameController->startNewGame();
}

void MainWindow::on_undoButton_clicked() {
    gameController->undoMove();
}

void MainWindow::on_settingsButton_clicked() {
    QMessageBox::information(this, "Settings", "Settings dialog will be implemented here.");
}