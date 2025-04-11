#include "AIConfigDialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QThread>
#include <QVBoxLayout>

AIConfigDialog::AIConfigDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("AI Configuration");
    setMinimumWidth(400);

    // 创建控件
    aiTypeComboBox = new QComboBox(this);
    aiTypeComboBox->addItem("Expectimax AI (单线程)");
    aiTypeComboBox->addItem("并行Expectimax AI (多线程)");
    aiTypeComboBox->addItem("蒙特卡洛树搜索 (MCTS)");
    aiTypeComboBox->addItem("混合策略 (Expectimax + MCTS)");

    depthSpinBox = new QSpinBox(this);
    depthSpinBox->setRange(1, 10);
    depthSpinBox->setValue(3);
    depthSpinBox->setToolTip("搜索深度越大，AI越强，但计算时间也越长");

    threadCountSpinBox = new QSpinBox(this);
    threadCountSpinBox->setRange(1, QThread::idealThreadCount() * 4);
    threadCountSpinBox->setValue(QThread::idealThreadCount());
    threadCountSpinBox->setToolTip("线程数量越多，计算速度越快，但会消耗更多系统资源");

    useAlphaBetaCheckBox = new QCheckBox("使用Alpha-Beta剪枝", this);
    useAlphaBetaCheckBox->setChecked(true);
    useAlphaBetaCheckBox->setToolTip("Alpha-Beta剪枝可以减少搜索空间，提高计算速度");

    useCacheCheckBox = new QCheckBox("使用缓存", this);
    useCacheCheckBox->setChecked(true);
    useCacheCheckBox->setToolTip("缓存可以避免重复计算，提高计算速度");

    cacheSizeSpinBox = new QSpinBox(this);
    cacheSizeSpinBox->setRange(1000, 10000000);
    cacheSizeSpinBox->setValue(1000000);
    cacheSizeSpinBox->setSingleStep(100000);
    cacheSizeSpinBox->setToolTip("缓存大小越大，可以缓存更多的计算结果，但会消耗更多内存");

    useEnhancedEvalCheckBox = new QCheckBox("使用增强评估函数", this);
    useEnhancedEvalCheckBox->setChecked(true);
    useEnhancedEvalCheckBox->setToolTip("增强评估函数可以提高AI的决策质量，但会增加计算量");

    useDynamicDepthCheckBox = new QCheckBox("使用动态深度调整", this);
    useDynamicDepthCheckBox->setChecked(true);
    useDynamicDepthCheckBox->setToolTip("动态深度调整可以根据棋盘状态自动调整搜索深度，提高计算效率");

    minDepthSpinBox = new QSpinBox(this);
    minDepthSpinBox->setRange(1, 5);
    minDepthSpinBox->setValue(2);
    minDepthSpinBox->setToolTip("最小搜索深度，当棋盘状态简单时使用");

    maxDepthSpinBox = new QSpinBox(this);
    maxDepthSpinBox->setRange(2, 10);
    maxDepthSpinBox->setValue(5);
    maxDepthSpinBox->setToolTip("最大搜索深度，当棋盘状态复杂时使用");

    // MCTS相关控件
    mctsSimulationLimitSpinBox = new QSpinBox(this);
    mctsSimulationLimitSpinBox->setRange(1000, 1000000);
    mctsSimulationLimitSpinBox->setValue(10000);
    mctsSimulationLimitSpinBox->setSingleStep(1000);
    mctsSimulationLimitSpinBox->setToolTip("模拟次数越多，AI越强，但计算时间也越长");

    mctsTimeLimitSpinBox = new QSpinBox(this);
    mctsTimeLimitSpinBox->setRange(10, 10000);
    mctsTimeLimitSpinBox->setValue(100);
    mctsTimeLimitSpinBox->setSingleStep(10);
    mctsTimeLimitSpinBox->setToolTip("搜索时间限制（毫秒），越长越精确，但响应越慢");

    // 混合策略相关控件
    mctsWeightSpinBox = new QSpinBox(this);
    mctsWeightSpinBox->setRange(0, 100);
    mctsWeightSpinBox->setValue(50);
    mctsWeightSpinBox->setToolTip("MCTS算法的权重（0-100）");

    expectimaxWeightSpinBox = new QSpinBox(this);
    expectimaxWeightSpinBox->setRange(0, 100);
    expectimaxWeightSpinBox->setValue(50);
    expectimaxWeightSpinBox->setToolTip("Expectimax算法的权重（0-100）");

    okButton     = new QPushButton("确定", this);
    cancelButton = new QPushButton("取消", this);

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow("AI类型:", aiTypeComboBox);

    // Expectimax相关控件
    QWidget* expectimaxWidget     = new QWidget(this);
    QFormLayout* expectimaxLayout = new QFormLayout(expectimaxWidget);
    expectimaxLayout->setContentsMargins(0, 0, 0, 0);
    expectimaxLayout->addRow("搜索深度:", depthSpinBox);
    expectimaxLayout->addRow("线程数量:", threadCountSpinBox);
    expectimaxLayout->addRow(useAlphaBetaCheckBox);
    expectimaxLayout->addRow(useCacheCheckBox);
    expectimaxLayout->addRow("缓存大小:", cacheSizeSpinBox);
    expectimaxLayout->addRow(useEnhancedEvalCheckBox);
    expectimaxLayout->addRow(useDynamicDepthCheckBox);
    expectimaxLayout->addRow("最小搜索深度:", minDepthSpinBox);
    expectimaxLayout->addRow("最大搜索深度:", maxDepthSpinBox);

    // MCTS相关控件
    QWidget* mctsWidget     = new QWidget(this);
    QFormLayout* mctsLayout = new QFormLayout(mctsWidget);
    mctsLayout->setContentsMargins(0, 0, 0, 0);
    mctsLayout->addRow("线程数量:", threadCountSpinBox);
    mctsLayout->addRow("模拟次数限制:", mctsSimulationLimitSpinBox);
    mctsLayout->addRow("搜索时间限制(毫秒):", mctsTimeLimitSpinBox);
    mctsLayout->addRow(useCacheCheckBox);
    mctsLayout->addRow("缓存大小:", cacheSizeSpinBox);

    // 添加说明标签
    QLabel* mctsInfoLabel = new QLabel("纯粹MCTS模式，不使用Expectimax算法", this);
    mctsInfoLabel->setStyleSheet("color: blue; font-style: italic;");
    mctsLayout->addRow(mctsInfoLabel);

    // 混合策略相关控件
    QWidget* hybridWidget     = new QWidget(this);
    QFormLayout* hybridLayout = new QFormLayout(hybridWidget);
    hybridLayout->setContentsMargins(0, 0, 0, 0);
    hybridLayout->addRow("线程数量:", threadCountSpinBox);
    hybridLayout->addRow("MCTS权重:", mctsWeightSpinBox);
    hybridLayout->addRow("Expectimax权重:", expectimaxWeightSpinBox);
    hybridLayout->addRow("搜索时间限制(毫秒):", mctsTimeLimitSpinBox);
    hybridLayout->addRow("Expectimax搜索深度:", depthSpinBox);
    hybridLayout->addRow(useCacheCheckBox);
    hybridLayout->addRow("缓存大小:", cacheSizeSpinBox);

    // 创建堆叠小部件
    QStackedWidget* stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(expectimaxWidget);  // 索引 0: 单线程Expectimax
    stackedWidget->addWidget(expectimaxWidget);  // 索引 1: 多线程Expectimax
    stackedWidget->addWidget(mctsWidget);        // 索引 2: MCTS
    stackedWidget->addWidget(hybridWidget);      // 索引 3: 混合策略

    // 添加到主表单
    formLayout->addRow("", stackedWidget);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);

    // 连接信号和槽
    connect(
        aiTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIConfigDialog::onAITypeChanged);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // 初始化显示状态
    onAITypeChanged(aiTypeComboBox->currentIndex());
}

void AIConfigDialog::onAITypeChanged(int index) {
    // 获取堆叠小部件
    QStackedWidget* stackedWidget = findChild<QStackedWidget*>();
    if (!stackedWidget) {
        return;
    }

    // 切换到对应的页面
    stackedWidget->setCurrentIndex(index);

    // 根据AI类型设置控件状态
    if (index == 0) {
        // 单线程Expectimax
        threadCountSpinBox->setEnabled(false);
    } else {
        // 多线程Expectimax、MCTS、混合策略
        threadCountSpinBox->setEnabled(true);
    }
}

int AIConfigDialog::getAIType() const {
    return aiTypeComboBox->currentIndex();
}

int AIConfigDialog::getDepth() const {
    return depthSpinBox->value();
}

int AIConfigDialog::getThreadCount() const {
    return threadCountSpinBox->value();
}

bool AIConfigDialog::getUseAlphaBeta() const {
    return useAlphaBetaCheckBox->isChecked();
}

bool AIConfigDialog::getUseCache() const {
    return useCacheCheckBox->isChecked();
}

int AIConfigDialog::getCacheSize() const {
    return cacheSizeSpinBox->value();
}

bool AIConfigDialog::getUseEnhancedEval() const {
    return useEnhancedEvalCheckBox->isChecked();
}

bool AIConfigDialog::getUseDynamicDepth() const {
    return useDynamicDepthCheckBox->isChecked();
}

int AIConfigDialog::getMinDepth() const {
    return minDepthSpinBox->value();
}

int AIConfigDialog::getMaxDepth() const {
    return maxDepthSpinBox->value();
}

int AIConfigDialog::getMCTSSimulationLimit() const {
    return mctsSimulationLimitSpinBox->value();
}

int AIConfigDialog::getMCTSTimeLimit() const {
    return mctsTimeLimitSpinBox->value();
}

int AIConfigDialog::getMCTSWeight() const {
    return mctsWeightSpinBox->value();
}

int AIConfigDialog::getExpectimaxWeight() const {
    return expectimaxWeightSpinBox->value();
}
