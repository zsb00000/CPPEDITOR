#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>

class ActionManager;
class FileTreeWidget;
class EditorTabWidget;
class QssManager;

/**
 * @brief 主窗口 —— 组合所有子模块
 *
 * 布局：
 *   ┌─────────────────────────────────────────┐ 菜单栏 + 工具栏
 *   │ [文件树] │ [标签页编辑器区]     │ [侧栏] │
 *   ├──────────┴───────────────────────┴────────┤
 *   │ 输出面板（编译 / 调试 / 评测 / 搜索）      │
 *   ├──────────────────────────────────────────┤
 *   │ 状态栏（光标位置 / 编码 / 语言）           │
 *   └──────────────────────────────────────────┘
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // 获取核心子部件（供 ActionManager 连接）
    EditorTabWidget *editorTabs() const;
    FileTreeWidget *fileTree() const;
    QTabWidget *outputPanel() const;
    QStatusBar *statusBar() const;

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupOutputPanel();
    void setupStatusBar();
    void restoreWindowState();
    void saveWindowState();

    // ---------- 核心部件 ----------
    ActionManager *m_actionManager = nullptr;

    // 中心区域
    QSplitter *m_mainSplitter = nullptr;   // 水平：文件树 + 编辑器
    QSplitter *m_editorSplitter = nullptr; // 水平：标签页 + (可选侧栏)
    FileTreeWidget *m_fileTree = nullptr;
    EditorTabWidget *m_editorTabs = nullptr;

    // 输出面板
    QTabWidget *m_outputPanel = nullptr;

    // 状态栏
    QLabel *m_statusCursor = nullptr;
    QLabel *m_statusEncoding = nullptr;
    QLabel *m_statusLanguage = nullptr;
};