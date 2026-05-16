#include "MainWindow.h"
#include "ActionManager.h"
#include "filemanager/CodeEditor.h"
#include "filemanager/EditorTabWidget.h"
#include "filemanager/FileTreeWidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QDockWidget>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("CPPEditor");
    resize(1280, 800);

    // 创建核心模块
    m_actionManager = new ActionManager(this);

    setupCentralWidget();
    setupMenuBar();
    setupToolBar();
    setupOutputPanel();
    setupStatusBar();

    restoreWindowState();

    // 启动时新建一个空白文件
    m_editorTabs->newFile();
}

MainWindow::~MainWindow() = default;

// ---------- 访问器 ----------
EditorTabWidget *MainWindow::editorTabs() const { return m_editorTabs; }
FileTreeWidget *MainWindow::fileTree() const { return m_fileTree; }
QTabWidget *MainWindow::outputPanel() const { return m_outputPanel; }
QStatusBar *MainWindow::statusBar() const { return QMainWindow::statusBar(); }

// ---------- 关闭事件 ----------
void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowState();

    // 尝试关闭所有文件
    if (m_editorTabs->closeAllFiles())
        event->accept();
    else
        event->ignore();
}

// ---------- 菜单栏 ----------
void MainWindow::setupMenuBar()
{
    QList<QMenu *> menus = m_actionManager->createMenus();
    for (QMenu *menu : menus)
        menuBar()->addMenu(menu);
}

// ---------- 工具栏 ----------
void MainWindow::setupToolBar()
{
    QToolBar *toolbar = addToolBar("主工具栏");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    QList<QAction *> actions = m_actionManager->createToolBarActions();
    for (QAction *action : actions)
        toolbar->addAction(action);
}

// ---------- 中心布局 ----------
void MainWindow::setupCentralWidget()
{
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(2);

    // 文件树
    m_fileTree = new FileTreeWidget(this);
    m_fileTree->setMinimumWidth(180);
    m_fileTree->setMaximumWidth(400);
    m_fileTree->setRootPath(QDir::currentPath());
    m_mainSplitter->addWidget(m_fileTree);

    // 编辑器区域
    m_editorSplitter = new QSplitter(Qt::Horizontal, this);
    m_editorSplitter->setHandleWidth(1);

    m_editorTabs = new EditorTabWidget(this);
    m_editorSplitter->addWidget(m_editorTabs);

    m_mainSplitter->addWidget(m_editorSplitter);

    // 比例 2:8
    m_mainSplitter->setStretchFactor(0, 2);
    m_mainSplitter->setStretchFactor(1, 8);

    setCentralWidget(m_mainSplitter);
}

// ---------- 输出面板 ----------
void MainWindow::setupOutputPanel()
{
    m_outputPanel = new QTabWidget(this);
    m_outputPanel->setMaximumHeight(200);
    m_outputPanel->setTabPosition(QTabWidget::South);

    // 编译输出
    auto *compileOutput = new QTextEdit(this);
    compileOutput->setReadOnly(true);
    compileOutput->setObjectName("CompileOutput");
    m_outputPanel->addTab(compileOutput, "编译输出");

    // 调试输出
    auto *debugOutput = new QTextEdit(this);
    debugOutput->setReadOnly(true);
    debugOutput->setObjectName("DebugOutput");
    m_outputPanel->addTab(debugOutput, "调试输出");

    // 评测结果
    auto *judgeOutput = new QTextEdit(this);
    judgeOutput->setReadOnly(true);
    judgeOutput->setObjectName("JudgeOutput");
    m_outputPanel->addTab(judgeOutput, "评测结果");

    // 将输出面板添加到窗口底部
    QDockWidget *dock = new QDockWidget("输出", this);
    dock->setWidget(m_outputPanel);
    dock->setFeatures(QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

// ---------- 状态栏 ----------
void MainWindow::setupStatusBar()
{
    QStatusBar *sb = QMainWindow::statusBar();

    m_statusCursor = new QLabel("行 1, 列 1");
    m_statusEncoding = new QLabel("UTF-8");
    m_statusLanguage = new QLabel("C++");

    sb->addWidget(m_statusCursor);
    sb->addPermanentWidget(m_statusEncoding);
    sb->addPermanentWidget(m_statusLanguage);

    // 连接编辑器光标变化更新状态栏
    connect(m_editorTabs, &QTabWidget::currentChanged, this,
            [this](int /* idx */)
            {
                CodeEditor *editor = m_editorTabs->currentEditor();
                if (editor)
                {
                    connect(
                        editor, &QPlainTextEdit::cursorPositionChanged, this,
                        [this, editor]()
                        {
                            QTextCursor cursor = editor->textCursor();
                            int line = cursor.blockNumber() + 1;
                            int col = cursor.columnNumber() + 1;
                            m_statusCursor->setText(
                                QString("行 %1, 列 %2").arg(line).arg(col));
                        },
                        Qt::UniqueConnection);
                }
            });
}

// ---------- 窗口状态持久化 ----------
void MainWindow::restoreWindowState()
{
    QSettings settings("CPPEditor", "CPPEditor");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    if (m_mainSplitter)
        m_mainSplitter->restoreState(
            settings.value("mainSplitter").toByteArray());
}

void MainWindow::saveWindowState()
{
    QSettings settings("CPPEditor", "CPPEditor");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    if (m_mainSplitter)
        settings.setValue("mainSplitter", m_mainSplitter->saveState());
}