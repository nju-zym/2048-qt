#ifndef TILEVIEW_H
#define TILEVIEW_H

#include <QLabel>
#include <QString>

class TileView {
   public:
    TileView(QWidget* parent = nullptr);
    ~TileView();

    // 设置并显示方块的值
    void setValue(int value);
    int getValue() const;

    // 获取和设置标签
    QLabel* getLabel() const;
    void setLabel(QLabel* label);

    // 应用合适的样式
    void updateAppearance();

    // 方块位置是否为空
    bool isEmpty() const;

   private:
    int value;      // 方块的值
    QLabel* label;  // 方块对应的标签
};

#endif  // TILEVIEW_H