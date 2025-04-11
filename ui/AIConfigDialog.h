#ifndef AICONFIGDIALOG_H
#define AICONFIGDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

/**
 * @brief AI配置对话框
 *
 * 允许用户配置AI的参数
 */
class AIConfigDialog : public QDialog {
    Q_OBJECT

   public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit AIConfigDialog(QWidget* parent = nullptr);

    /**
     * @brief 获取AI类型
     * @return AI类型（0=单线程Expectimax, 1=多线程Expectimax, 2=MCTS, 3=混合策略）
     */
    int getAIType() const;

    /**
     * @brief 获取搜索深度
     * @return 搜索深度
     */
    int getDepth() const;

    /**
     * @brief 获取线程数量
     * @return 线程数量
     */
    int getThreadCount() const;

    /**
     * @brief 获取是否使用Alpha-Beta剪枝
     * @return 是否使用Alpha-Beta剪枝
     */
    bool getUseAlphaBeta() const;

    /**
     * @brief 获取是否使用缓存
     * @return 是否使用缓存
     */
    bool getUseCache() const;

    /**
     * @brief 获取缓存大小
     * @return 缓存大小
     */
    int getCacheSize() const;

    /**
     * @brief 获取是否使用增强评估函数
     * @return 是否使用增强评估函数
     */
    bool getUseEnhancedEval() const;

    /**
     * @brief 获取是否使用动态深度调整
     * @return 是否使用动态深度调整
     */
    bool getUseDynamicDepth() const;

    /**
     * @brief 获取最小搜索深度
     * @return 最小搜索深度
     */
    int getMinDepth() const;

    /**
     * @brief 获取最大搜索深度
     * @return 最大搜索深度
     */
    int getMaxDepth() const;

    /**
     * @brief 获取MCTS模拟次数限制
     * @return MCTS模拟次数限制
     */
    int getMCTSSimulationLimit() const;

    /**
     * @brief 获取MCTS时间限制
     * @return MCTS时间限制（毫秒）
     */
    int getMCTSTimeLimit() const;

    /**
     * @brief 获取MCTS权重
     * @return MCTS权重（0-100）
     */
    int getMCTSWeight() const;

    /**
     * @brief 获取Expectimax权重
     * @return Expectimax权重（0-100）
     */
    int getExpectimaxWeight() const;

   private slots:
    /**
     * @brief 当AI类型改变时调用
     * @param index 新的索引
     */
    void onAITypeChanged(int index);

   private:
    QComboBox* aiTypeComboBox;
    QSpinBox* depthSpinBox;
    QSpinBox* threadCountSpinBox;
    QCheckBox* useAlphaBetaCheckBox;
    QCheckBox* useCacheCheckBox;
    QSpinBox* cacheSizeSpinBox;
    QCheckBox* useEnhancedEvalCheckBox;
    QCheckBox* useDynamicDepthCheckBox;
    QSpinBox* minDepthSpinBox;
    QSpinBox* maxDepthSpinBox;
    QSpinBox* mctsSimulationLimitSpinBox;
    QSpinBox* mctsTimeLimitSpinBox;
    QSpinBox* mctsWeightSpinBox;
    QSpinBox* expectimaxWeightSpinBox;
    QPushButton* okButton;
    QPushButton* cancelButton;
};

#endif  // AICONFIGDIALOG_H
