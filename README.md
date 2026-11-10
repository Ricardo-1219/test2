# 小牛文本编辑器

Qt 实验二完整工程。界面使用 Qt Designer 生成的 `mainwindow.ui`，功能代码按模块拆分，支持文件读写、编辑、格式设置、查找替换、状态栏信息、未保存提示和行号显示。

## 开发与验证环境

- Windows 64 位
- Qt 6.11.2 MinGW 64-bit
- 代码使用 Qt 5.12 已支持的 Widgets、`QPlainTextEdit`、`QSaveFile` 和 `QTextDocument` 接口编写

## 主要文件

- `mainwindow.ui`：主窗口界面、菜单、工具栏和 Action
- `mainwindow.cpp`：主窗口业务逻辑与自动测试
- `codeeditor.cpp`：带行号区域的 `QPlainTextEdit` 派生类
- `finddialog.cpp`：查找对话框
- `replacedialog.cpp`：替换对话框
- `aboutdialog.cpp`：关于对话框

## 构建

```powershell
mkdir build
cd build
C:\Qt\6.11.2\mingw_64\bin\qmake.exe ..\MyNotepad\MyNotepad.pro "CONFIG+=release"
C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe -j2
```

生成程序位于 `build\release\MyNotepad.exe`。也可以直接用 Qt Creator 打开 `MyNotepad.pro` 后构建运行。

## 自动测试

```powershell
MyNotepad.exe --self-test "D:\Program Files\codex\outputs\实验二简单文本编辑器\运行结果"
```

自动测试会生成主窗口、查找、替换和关于对话框截图，以及 `自动测试报告.txt`。
