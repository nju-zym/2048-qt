#include "mainwindow.h"

#include "ui_mainwindow.h"

#include <QDebug>
#include <QKeyEvent>
#include <QMessageBox>
#include <QTimer>

// 构造函数：初始化UI和游戏组件
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      gameController(new GameController(this)),
      uiManager(new UIManager(this)) {
    ui->setupUi(this);                // 初始化UI界面
    setFocusPolicy(Qt::StrongFocus);  // 设置焦点策略，确保窗口能响应键盘事件

    // 初始化游戏界面
    uiManager->initializeTiles(ui->boardLayout, ui->gameBoard);

    // 连接游戏控制器与UI管理器的信号和槽
    connect(gameController, &GameController::scoreUpdated, this, &MainWindow::handleScoreUpdate);
    connect(gameController, &GameController::tileAdded, this, &MainWindow::handleTileAdded);
    connect(gameController, &GameController::tileMoved, this, &MainWindow::handleTileMoved);
    connect(gameController, &GameController::gameOver, this, &MainWindow::handleGameOver);
    connect(gameController, &GameController::gameWon, this, &MainWindow::handleGameWon);
    connect(gameController, &GameController::statusUpdated, this, &MainWindow::handleStatusUpdate);
    connect(gameController, &GameController::boardUpdated, this, &MainWindow::handleBoardUpdate);

    // 开始新游戏
    gameController->startNewGame();
}

// 析构函数：清理资源
MainWindow::~MainWindow() {
    delete ui;
    // gameController 和 uiManager 由 QObject 父子关系管理，无需手动删除
}

// 键盘事件处理：响应方向键移动
void MainWindow::keyPressEvent(QKeyEvent* event) {
    // 如果动画正在进行，忽略键盘输入
    if (uiManager->isAnimationInProgress()) {
        return;
    }

    bool moved = false;

    // 根据按键的方向值，调用对应的移动函数
    switch (event->key()) {
        case Qt::Key_Up:
            moved = gameController->moveTiles(0);  // 0代表向上移动
            break;
        case Qt::Key_Right:
            moved = gameController->moveTiles(1);  // 1代表向右移动
            break;
        case Qt::Key_Down:
            moved = gameController->moveTiles(2);  // 2代表向下移动
            break;
        case Qt::Key_Left:
            moved = gameController->moveTiles(3);  // 3代表向左移动
            break;
        default:
            // 其他键不处理
            return;
    }

    // 如果棋盘有变化，则生成新的方块并检测游戏结束或胜利条件
    if (moved) {
        // 如果有动画正在进行，等待所有动画完成后再生成新方块
        if (uiManager->getPendingAnimations() > 0) {
            // 使用单次计时器延迟生成新方块，等待所有动画完成
            QTimer::singleShot(200, [this]() {
                gameController->generateNewTile(true);  // 随机在空位置生成一个新的数字，使用动画效果
            });
        } else {
            gameController->generateNewTile(true);  // 随机在空位置生成一个新的数字，使用动画效果
        }
    }
}

// 新游戏按钮点击事件
void MainWindow::on_newGameButton_clicked() {
    gameController->startNewGame();
}

// 撤销按钮点击事件
void MainWindow::on_undoButton_clicked() {
    gameController->undo();
}

// 设置按钮点击事件
void MainWindow::on_settingsButton_clicked() {
    QMessageBox::information(this, tr("Settings"), tr("Settings dialog will be implemented here."));
}

// 自动游戏按钮点击事件
void MainWindow::on_autoPlayButton_clicked() {
    gameController->toggleAutoPlay();

    // 更新按钮文本
    bool isAutoPlaying = gameController->isAutoPlayActive();  // 使用正确的方法判断自动播放状态
    ui->autoPlayButton->setText(isAutoPlaying ? "Stop Auto" : "Auto Play");
}

// 处理分数更新事件
void MainWindow::handleScoreUpdate(int newScore) {
    // 更新当前分数
    ui->scoreValue->setText(QString::number(newScore));

    // 更新最佳分数
    if (newScore > gameController->getBestScore()) {
        ui->bestValue->setText(QString::number(newScore));
    }

    // 添加分数增加的动画效果
    // 注意：这个实现需要知道之前的分数，这里简化处理
}

// 处理添加新方块事件
void MainWindow::handleTileAdded(int row, int col, int value, bool animate) {
    if (animate) {
        uiManager->animateNewTile(row, col, value);
    } else {
        uiManager->updateTileAppearance(row, col, value);
    }
}

// 处理方块移动事件
void MainWindow::handleTileMoved(int fromRow, int fromCol, int toRow, int toCol, int value, bool merged) {
    uiManager->animateTileMovement(fromRow, fromCol, toRow, toCol, value, merged);
}

// 处理游戏结束事件
void MainWindow::handleGameOver() {
    showGameOverMessage();
}

// 处理游戏胜利事件
void MainWindow::handleGameWon() {
    showWinMessage();
}

// 处理状态更新事件
void MainWindow::handleStatusUpdate(QString const& message) {
    ui->statusLabel->setText(message);
}

// 处理棋盘更新事件
void MainWindow::handleBoardUpdate() {
    // 更新棋盘上每个方块的显示
    auto const& board = gameController->getBoard();
    for (int i = 0; i < board.size(); ++i) {
        for (int j = 0; j < board[i].size(); ++j) {
            uiManager->updateTileAppearance(i, j, board[i][j]);
        }
    }
}

// showGameOverMessage: 显示游戏结束对话框
void MainWindow::showGameOverMessage() {
    QMessageBox msgBox(this);
    msgBox.setText(tr("Game Over!"));
    msgBox.setInformativeText(tr("Do you want to start a new game?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        gameController->startNewGame();
    }
}

// showWinMessage: 显示游戏胜利对话框
void MainWindow::showWinMessage() {
    QMessageBox msgBox(this);
    msgBox.setText(tr("You Win!"));
    msgBox.setInformativeText(tr("Do you want to continue playing?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::No) {
        gameController->startNewGame();
    } else {
        handleStatusUpdate(tr("Keep going to get a higher score!"));
    }
}
