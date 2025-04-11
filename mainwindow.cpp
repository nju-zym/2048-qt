#include "mainwindow.h"

#include "ui/AIConfigDialog.h"
#include "ui_mainwindow.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);  // 确保窗口能响应键盘事件

    // 创建游戏视图
    gameView = new GameView(this, ui->boardLayout);
    gameView->initializeTiles();

    // 创建游戏控制器
    gameController = new GameController(this);
    gameController->setGameView(gameView);

    // 初始化成员变量
    bitBoardInitializer = nullptr;
    progressDialog      = nullptr;

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

void MainWindow::onBitBoardInitializationCompleted() {
    // 隐藏进度对话框
    if (progressDialog) {
        progressDialog->hide();
    }

    // 启用AI按钮
    QPushButton* aiButton = findChild<QPushButton*>("aiButton");
    if (aiButton) {
        aiButton->setEnabled(true);
    }
}

void MainWindow::on_aiButton_clicked() {
    // 获取aiButton按钮
    QPushButton* aiButton = findChild<QPushButton*>("aiButton");
    if (!aiButton) {
        QMessageBox::warning(this, "Error", "AI button not found!");
        return;
    }

    // 如果AI已经在运行，停止AI
    if (gameController->isAIRunning()) {
        gameController->stopAI();
        aiButton->setText("Start AI");
    } else {
        // 检查BitBoard表是否已初始化
        if (!BitBoard::areTablesInitialized()) {
            // 初始化BitBoardInitializer
            if (!bitBoardInitializer) {
                bitBoardInitializer = BitBoardInitializer::getInstance(this);
                connect(bitBoardInitializer,
                        &BitBoardInitializer::initializationCompleted,
                        this,
                        &MainWindow::onBitBoardInitializationCompleted);
            }

            // 创建进度对话框
            if (!progressDialog) {
                progressDialog = new QProgressDialog("Initializing AI...", "Cancel", 0, 0, this);
                progressDialog->setWindowModality(Qt::WindowModal);
                progressDialog->setMinimumDuration(0);
                progressDialog->setCancelButton(nullptr);
                progressDialog->setAutoClose(true);
                progressDialog->setAutoReset(true);
            }

            // 显示进度对话框
            progressDialog->show();

            // 开始初始化
            bitBoardInitializer->initialize();

            // 禁用AI按钮，直到初始化完成
            aiButton->setEnabled(false);

            // 返回，等待初始化完成
            return;
        }

        // 显示AI配置对话框
        AIConfigDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            AIInterface* ai = nullptr;

            int depth = dialog.getDepth();

            switch (dialog.getAIType()) {
                case 0: {
                    // 创建Expectimax AI实例
                    ai = new ExpectimaxAI(depth, this);
                    break;
                }
                case 1: {
                    // 创建并行Expectimax AI实例
                    ParallelExpectimaxAI* parallelAI = new ParallelExpectimaxAI(depth, dialog.getThreadCount(), this);

                    // 设置高级配置
                    parallelAI->setUseAlphaBeta(dialog.getUseAlphaBeta());
                    parallelAI->setUseCache(dialog.getUseCache());
                    parallelAI->setCacheSize(dialog.getCacheSize());
                    parallelAI->setUseEnhancedEval(dialog.getUseEnhancedEval());

                    // 设置动态深度调整
                    parallelAI->setUseDynamicDepth(dialog.getUseDynamicDepth());
                    parallelAI->setMinDepth(dialog.getMinDepth());
                    parallelAI->setMaxDepth(dialog.getMaxDepth());

                    ai = parallelAI;
                    break;
                }
                case 2: {
                    // 创建MCTS AI实例
                    HybridAI* mctsAI = new HybridAI(this);

                    // 设置MCTS权重为100%
                    mctsAI->setMCTSWeight(1.0f);
                    mctsAI->setExpectimaxWeight(0.0f);

                    // 设置线程数和时间限制
                    mctsAI->setThreadCount(dialog.getThreadCount());
                    mctsAI->setTimeLimit(dialog.getMCTSTimeLimit());

                    // 设置缓存
                    mctsAI->setUseCache(dialog.getUseCache());
                    mctsAI->setCacheSize(dialog.getCacheSize());

                    ai = mctsAI;
                    break;
                }
                case 3: {
                    // 创建混合策略AI实例
                    HybridAI* hybridAI = new HybridAI(this);

                    // 设置MCTS和Expectimax权重
                    float mctsWeight       = dialog.getMCTSWeight() / 100.0f;
                    float expectimaxWeight = dialog.getExpectimaxWeight() / 100.0f;
                    hybridAI->setMCTSWeight(mctsWeight);
                    hybridAI->setExpectimaxWeight(expectimaxWeight);

                    // 设置线程数和时间限制
                    hybridAI->setThreadCount(dialog.getThreadCount());
                    hybridAI->setTimeLimit(dialog.getMCTSTimeLimit());

                    // 设置缓存
                    hybridAI->setUseCache(dialog.getUseCache());
                    hybridAI->setCacheSize(dialog.getCacheSize());

                    ai = hybridAI;
                    break;
                }
                default:
                    // 默认使用Expectimax AI
                    ai = new ExpectimaxAI(depth, this);
                    break;
            }

            if (ai) {
                // 启动AI
                gameController->startAI(ai);
                aiButton->setText("Stop AI");
            }
        }
    }
}