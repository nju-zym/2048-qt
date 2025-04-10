# 2048-Qt

这是一个使用Qt框架实现的2048游戏，具有美观的界面和流畅的动画效果。

## 项目结构

项目采用模块化设计，将代码按照功能和职责进行分类：

```
2048-qt/
├── CMakeLists.txt          # 项目构建文件
├── main.cpp                # 程序入口
├── mainwindow.cpp          # 主窗口实现
├── mainwindow.h            # 主窗口头文件
├── mainwindow.ui           # 主窗口UI设计文件
├── core/                   # 核心游戏逻辑模块
│   ├── GameBoard.cpp       # 游戏棋盘实现
│   ├── GameBoard.h         # 游戏棋盘头文件
│   ├── GameState.cpp       # 游戏状态管理实现
│   └── GameState.h         # 游戏状态管理头文件
├── utils/                  # 工具类模块
│   ├── AnimationManager.cpp # 动画管理实现
│   ├── AnimationManager.h   # 动画管理头文件
│   ├── StyleManager.cpp     # 样式管理实现
│   └── StyleManager.h       # 样式管理头文件
├── ui/                     # 界面相关模块（待扩展）
└── ai/                     # AI模块（待扩展）
    └── evaluation/         # 评估函数目录
```

## 模块功能说明

### 核心模块 (core/)

- **GameBoard**: 管理游戏棋盘状态和基本操作，如方块的移动、合并和生成新方块
- **GameState**: 管理游戏状态、历史记录和最高分数，支持撤销操作

### 工具模块 (utils/)

- **StyleManager**: 处理方块的样式和颜色，根据方块数值提供相应的样式表
- **AnimationManager**: 管理所有动画效果，包括方块移动、合并、新方块生成和分数增加的动画

### 界面模块 (ui/)

- 预留用于存放更多UI相关的组件和设计

### AI模块 (ai/)

- 预留用于实现AI玩家和算法
- evaluation/ 目录用于存放评估函数相关代码

## 主要功能

- 基本的2048游戏玩法
- 流畅的方块移动和合并动画
- 分数显示和最高分记录
- 撤销操作支持
- 游戏状态提示（游戏胜利、游戏结束）

## 编译与运行

### 依赖项

- Qt 5.x 或 Qt 6.x
- CMake 3.16 或更高版本
- C++17 兼容的编译器

### 编译步骤

1. 克隆仓库：
```
git clone [仓库地址]
cd 2048-qt
```

2. 创建构建目录：
```
mkdir build
cd build
```

3. 配置和编译：
```
cmake ..
cmake --build .
```

4. 运行游戏：
```
./2048-qt
```

## 未来计划

- [ ] 添加设置对话框
- [ ] 实现主题切换功能
- [ ] 添加本地存储功能，保存最高分和游戏设置
- [ ] 实现AI模式
- [ ] 添加更多动画和视觉效果

## 如何贡献

欢迎提交问题报告和合并请求！请确保遵循项目的代码风格和贡献指南。

## 许可证

[添加适当的许可证信息]