#include "TileView.h"

#include "../utils/StyleManager.h"

#include <QMutexLocker>

TileView::TileView(QWidget* parent) : value(0), label(nullptr) {
    if (parent) {
        label = new QLabel(parent);
        label->setAlignment(Qt::AlignCenter);
        label->setFixedSize(80, 80);
        updateAppearance();
    }
}

TileView::~TileView() {
    // 标签由父窗口管理，不需要在这里删除
}

void TileView::setValue(int value) {
    this->value = value;
    updateAppearance();
}

int TileView::getValue() const {
    return value;
}

QLabel* TileView::getLabel() const {
    return label;
}

void TileView::setLabel(QLabel* label) {
    this->label = label;
    updateAppearance();
}

void TileView::updateAppearance() {
    if (label) {
        QMutexLocker locker(&g_styleSheetMutex);

        // 设置样式 - 确保即使是空白方块也有样式
        QString styleSheet = StyleManager::getTileStyleSheet(value);
        if (!styleSheet.isEmpty()) {
            label->setStyleSheet(styleSheet);
        }

        // 设置文本
        label->setText(value == 0 ? "" : QString::number(value));

        // 确保标签可见
        label->setVisible(true);
    }
}

bool TileView::isEmpty() const {
    return value == 0;
}