# VCS - Vision Camera Studio

视觉相机工作站 - 基于 Qt/C++ 的工业相机采集与设备管理客户端。

## 功能特性

- 插件化架构，支持动态加载不同品牌相机
- 海康威视相机支持（MVS SDK）
- 虚拟相机用于开发测试
- 实时图像采集与显示
- 图像缩放、平移、自适应窗口
- 相机参数读写与配置
- JSON 配置导入导出
- 多线程采集，界面不卡顿

## 技术栈

- C++17
- Qt 6.11 (Widgets)
- OpenCV 4.10
- 海康 MVS SDK
- qmake 构建系统

## 目录结构

```
VCSCameraStudio/
├── depends/                    # 第三方依赖库
│   └── HikCamera/              # 海康 MVS SDK
├── src/
│   └── VCSGUI/                 # 主项目源码
│       ├── CameraInterface/    # 相机抽象接口
│       ├── CameraPlugin/       # 插件管理器与接口定义
│       ├── Plugins/            # 具体相机插件
│       ├── ControlWidget/      # 相机控制面板
│       ├── ParamWidget/        # 参数管理面板
│       ├── ViewWidget/         # 图像显示面板
│       ├── ParseUiJson/        # JSON 配置解析
│       ├── Utils/              # 工具类
│       ├── AppStyle/           # 样式表
│       └── Tests/              # 插件冒烟测试
└── README.md
```

## 构建

### 环境要求

- Qt 6.11+ (MSVC 2022 64bit)
- OpenCV 4.10
- 海康 MVS SDK 运行时（运行海康相机时需要）

### 编译步骤

1. 打开 Qt Creator
2. 打开 src/VCSGUI/VCSGUI.pro
3. 配置构建套件（MSVC 2022 64bit）
4. 执行 qmake
5. 构建项目

## License

MIT
