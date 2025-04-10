#include "AIConfigDialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QThread>
#include <QVBoxLayout>

AIConfigDialog::AIConfigDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("AI Configuration");
    setMinimumWidth(400);

    // 创建控件
    aiTypeComboBox = new QComboBox(this);
    aiTypeComboBox->addItem("Expectimax AI (单线程)");
    aiTypeComboBox->addItem("并行Expectimax AI (多线程)");

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

    okButton     = new QPushButton("确定", this);
    cancelButton = new QPushButton("取消", this);

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow("AI类型:", aiTypeComboBox);
    formLayout->addRow("搜索深度:", depthSpinBox);
    formLayout->addRow("线程数量:", threadCountSpinBox);
    formLayout->addRow(useAlphaBetaCheckBox);
    formLayout->addRow(useCacheCheckBox);
    formLayout->addRow("缓存大小:", cacheSizeSpinBox);
    formLayout->addRow(useEnhancedEvalCheckBox);
    formLayout->addRow(useDynamicDepthCheckBox);
    formLayout->addRow("最小搜索深度:", minDepthSpinBox);
    formLayout->addRow("最大搜索深度:", maxDepthSpinBox);

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

    // 初始化UI状态
    onAITypeChanged(aiTypeComboBox->currentIndex());
}

void AIConfigDialog::onAITypeChanged(int index) {
    // 如果选择单线程AI，禁用线程数量设置
    threadCountSpinBox->setEnabled(index == 1);
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
