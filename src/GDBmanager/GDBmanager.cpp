#include "GDBmanager.h"
#include <QApplication>
#include <QMessageBox>
#include <QRegularExpression>

// ==================== DebuggerConsole 实现 ====================
DebuggerConsole::DebuggerConsole(QWidget *parent) : QWidget(parent)
{
    setupUi();
    setWindowTitle("C++ Debugger");
    resize(1000, 650);
}

void DebuggerConsole::setupUi()
{
    // 主水平分割器：左侧面板（控制+表格），右侧控制台
    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);

    // ---- 左侧面板 ----
    auto *leftPanel = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // 工具栏
    m_toolbar = new QToolBar;
    m_toolbar->addAction("▶ 继续", this, &DebuggerConsole::onContinue);
    m_toolbar->addAction("⏸ 暂停", this, &DebuggerConsole::onPause);
    m_toolbar->addSeparator();
    m_toolbar->addAction("↘ 单步跳过", this, &DebuggerConsole::onStepOver);
    m_toolbar->addAction("↓ 单步进入", this, &DebuggerConsole::onStepInto);
    m_toolbar->addAction("↗ 单步跳出", this, &DebuggerConsole::onStepOut);
    m_toolbar->addSeparator();
    m_toolbar->addAction("↺ 重启", this, &DebuggerConsole::onRestart);
    leftLayout->addWidget(m_toolbar);

    // 状态标签
    m_statusLabel = new QLabel("● 调试器未启动");
    m_statusLabel->setStyleSheet("font-weight: bold; color: #888;");
    leftLayout->addWidget(m_statusLabel);

    // 断点视图
    auto *bpLabel = new QLabel("断点");
    bpLabel->setStyleSheet("font-weight: bold;");
    m_breakpointsView = new QTreeWidget;
    m_breakpointsView->setHeaderLabels({"编号", "类型", "位置", "状态"});
    m_breakpointsView->setRootIsDecorated(false);
    m_breakpointsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leftLayout->addWidget(bpLabel);
    leftLayout->addWidget(m_breakpointsView);

    // 断点添加栏
    auto *bpInputLayout = new QHBoxLayout;
    auto *bpInput = new QLineEdit;
    bpInput->setPlaceholderText("添加断点 (如 main 或 file:10)");
    auto *addBpBtn = new QPushButton("+");
    bpInputLayout->addWidget(bpInput);
    bpInputLayout->addWidget(addBpBtn);
    leftLayout->addLayout(bpInputLayout);
    connect(addBpBtn, &QPushButton::clicked, this,
            [this, bpInput]()
            {
                QString loc = bpInput->text().trimmed();
                if (!loc.isEmpty())
                    emit addBreakpointRequested(loc);
                bpInput->clear();
            });

    // 局部变量
    auto *varLabel = new QLabel("局部变量");
    varLabel->setStyleSheet("font-weight: bold;");
    m_localsView = new QTreeWidget;
    m_localsView->setHeaderLabels({"名称", "值", "类型"});
    m_localsView->setRootIsDecorated(false);
    m_localsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leftLayout->addWidget(varLabel);
    leftLayout->addWidget(m_localsView);

    // 调用栈
    auto *stackLabel = new QLabel("调用栈");
    stackLabel->setStyleSheet("font-weight: bold;");
    m_stackView = new QTreeWidget;
    m_stackView->setHeaderLabels({"层级", "函数", "文件:行"});
    m_stackView->setRootIsDecorated(false);
    m_stackView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leftLayout->addWidget(stackLabel);
    leftLayout->addWidget(m_stackView);

    leftPanel->setMinimumWidth(300);
    mainSplitter->addWidget(leftPanel);

    // ---- 右侧控制台 ----
    auto *consolePanel = new QWidget;
    auto *consoleLayout = new QVBoxLayout(consolePanel);
    consoleLayout->setContentsMargins(0, 0, 0, 0);
    m_consoleOutput = new QPlainTextEdit;
    m_consoleOutput->setReadOnly(true);
    m_consoleOutput->setFont(QFont("Courier New", 10));
    m_commandInput = new QLineEdit;
    m_commandInput->setPlaceholderText("GDB 命令...");
    consoleLayout->addWidget(m_consoleOutput);
    consoleLayout->addWidget(m_commandInput);
    consolePanel->setMinimumWidth(300);
    mainSplitter->addWidget(consolePanel);

    // 整体布局
    auto *globalLayout = new QHBoxLayout(this);
    globalLayout->addWidget(mainSplitter);

    // 命令行输入信号（保留）
    connect(m_commandInput, &QLineEdit::returnPressed, this,
            [this]()
            {
                QString cmd = m_commandInput->text();
                if (!cmd.trimmed().isEmpty())
                {
                    emit sendCommand(cmd);
                    m_commandInput->clear();
                }
            });
}

void DebuggerConsole::appendConsoleOutput(const QString &text)
{
    m_consoleOutput->appendPlainText(text);
}

void DebuggerConsole::clearConsole() { m_consoleOutput->clear(); }

void DebuggerConsole::updateBreakpoints(const QList<Breakpoint> &bps)
{
    m_breakpointsView->clear();
    for (const auto &bp : bps)
    {
        auto *item = new QTreeWidgetItem(m_breakpointsView);
        item->setText(0, QString::number(bp.number));
        item->setText(1, bp.type);
        item->setText(2, bp.func.isEmpty() ? bp.address : bp.func);
        item->setText(3, bp.enabled == "y" ? "启用" : "禁用");
    }
}

void DebuggerConsole::updateLocals(const QList<Variable> &vars)
{
    m_localsView->clear();
    for (const auto &v : vars)
    {
        auto *item = new QTreeWidgetItem(m_localsView);
        item->setText(0, v.name);
        item->setText(1, v.value);
        item->setText(2, v.type);
    }
}

void DebuggerConsole::updateStack(const QList<StackFrame> &frames)
{
    m_stackView->clear();
    for (const auto &f : frames)
    {
        auto *item = new QTreeWidgetItem(m_stackView);
        item->setText(0, QString::number(f.level));
        item->setText(1, f.func.isEmpty() ? f.address : f.func);
        item->setText(2, f.file.isEmpty()
                             ? ""
                             : QString("%1:%2").arg(f.file).arg(f.line));
    }
}

void DebuggerConsole::setProgramStatus(bool running)
{
    if (running)
    {
        m_statusLabel->setText("▶ 程序运行中");
        m_statusLabel->setStyleSheet("font-weight: bold; color: #0a0;");
    }
    else
    {
        m_statusLabel->setText("⏸ 程序已停止");
        m_statusLabel->setStyleSheet("font-weight: bold; color: #c00;");
    }
}

void DebuggerConsole::setCurrentLocation(const QString &file, int line)
{
    // 可连接主编辑器跳转到特定行，此处简单输出
    appendConsoleOutput(QString("当前停止在 %1:%2").arg(file).arg(line));
}

void DebuggerConsole::onContinue() { emit continueRequested(); }
void DebuggerConsole::onPause() { emit pauseRequested(); }
void DebuggerConsole::onStepOver() { emit stepOverRequested(); }
void DebuggerConsole::onStepInto() { emit stepIntoRequested(); }
void DebuggerConsole::onStepOut() { emit stepOutRequested(); }
void DebuggerConsole::onRestart() { emit restartRequested(); }
void DebuggerConsole::onAddBreakpoint() { /* 由内嵌按钮处理 */ }

// ==================== DebuggerManager 实现 ====================
DebuggerManager::DebuggerManager(QObject *parent) : QObject(parent) {}

DebuggerManager::~DebuggerManager()
{
    if (m_gdbProcess && m_gdbProcess->state() != QProcess::NotRunning)
    {
        m_gdbProcess->terminate();
        m_gdbProcess->waitForFinished(2000);
    }
    delete m_ui;
}

void DebuggerManager::initial(const QString &gdbPath)
{
    m_gdbPath = gdbPath;
    if (!QFileInfo::exists(m_gdbPath))
    {
        QMessageBox::warning(nullptr, "警告", "GDB 路径不存在: " + m_gdbPath);
    }
}

void DebuggerManager::setup()
{
    emit requestCompileWithDebug();
    startUi();
    startGdb();
}

void DebuggerManager::startUi()
{
    if (m_ui)
    {
        m_ui->show();
        return;
    }

    m_ui = new DebuggerConsole;

    // 图形按钮信号  管理器槽
    connect(m_ui, &DebuggerConsole::continueRequested, this,
            &DebuggerManager::handleContinue);
    connect(m_ui, &DebuggerConsole::pauseRequested, this,
            &DebuggerManager::handlePause);
    connect(m_ui, &DebuggerConsole::stepOverRequested, this,
            &DebuggerManager::handleStepOver);
    connect(m_ui, &DebuggerConsole::stepIntoRequested, this,
            &DebuggerManager::handleStepInto);
    connect(m_ui, &DebuggerConsole::stepOutRequested, this,
            &DebuggerManager::handleStepOut);
    connect(m_ui, &DebuggerConsole::restartRequested, this,
            &DebuggerManager::handleRestart);
    connect(m_ui, &DebuggerConsole::addBreakpointRequested, this,
            &DebuggerManager::handleAddBreakpoint);
    // 命令行输入（保留）
    connect(m_ui, &DebuggerConsole::sendCommand, this, &DebuggerManager::trans);

    // 管理器信号  UI 更新
    connect(this, &DebuggerManager::breakpointsUpdated, m_ui,
            &DebuggerConsole::updateBreakpoints);
    connect(this, &DebuggerManager::localsUpdated, m_ui,
            &DebuggerConsole::updateLocals);
    connect(this, &DebuggerManager::stackUpdated, m_ui,
            &DebuggerConsole::updateStack);
    connect(this, &DebuggerManager::programStopped, m_ui,
            [this](const QString &) { m_ui->setProgramStatus(false); });
    connect(this, &DebuggerManager::programRunning, m_ui,
            [this]() { m_ui->setProgramStatus(true); });
    connect(this, &DebuggerManager::consoleOutput, m_ui,
            &DebuggerConsole::appendConsoleOutput);

    m_ui->show();
}

void DebuggerManager::startGdb()
{
    if (m_gdbPath.isEmpty())
        return;

    if (m_gdbProcess)
    {
        delete m_gdbProcess;
    }

    m_gdbProcess = new QProcess(this);
    m_gdbProcess->setProgram(m_gdbPath);
    m_gdbProcess->setArguments({"--interpreter=mi2", "--quiet"});

    connect(m_gdbProcess, &QProcess::readyReadStandardOutput, this,
            &DebuggerManager::recv);
    connect(m_gdbProcess, &QProcess::readyReadStandardError, this,
            &DebuggerManager::onGdbError);
    connect(m_gdbProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &DebuggerManager::onGdbFinished);

    m_gdbProcess->start();
    if (m_gdbProcess->waitForStarted(3000))
    {
        emit programRunning(); // GDB 启动后通常等待命令
        // 初始命令：开启异步模式
        sendToGdb("-gdb-set mi-async on");
        sendToGdb("-enable-pretty-printing");
    }
    else
    {
        emit consoleOutput("错误：无法启动 GDB！");
    }
}

void DebuggerManager::sendToGdb(const QString &cmd)
{
    if (!m_gdbProcess || m_gdbProcess->state() != QProcess::Running)
    {
        emit consoleOutput("GDB 未运行");
        return;
    }
    m_gdbProcess->write((cmd.trimmed() + "\n").toUtf8());
}

// 公开的 trans 接口，用于外部（如主编辑器）直接发送命令
void DebuggerManager::trans(const QString &command) { sendToGdb(command); }

// ------ 图形按钮动作 ------
void DebuggerManager::handleContinue() { sendToGdb("-exec-continue"); }
void DebuggerManager::handlePause() { sendToGdb("-exec-interrupt"); }
void DebuggerManager::handleStepOver() { sendToGdb("-exec-next"); }
void DebuggerManager::handleStepInto() { sendToGdb("-exec-step"); }
void DebuggerManager::handleStepOut() { sendToGdb("-exec-finish"); }
void DebuggerManager::handleRestart() { sendToGdb("-exec-run"); }
void DebuggerManager::handleAddBreakpoint(const QString &location)
{
    sendToGdb("-break-insert " + location);
}

// ------ GDB 输出解析 (recv) ------
void DebuggerManager::recv()
{
    if (!m_gdbProcess)
        return;

    QByteArray data = m_gdbProcess->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    for (const QString &rawLine : lines)
    {
        // 回显到控制台
        emit consoleOutput(rawLine);
        parseGdbOutput(rawLine);
    }
}

void DebuggerManager::parseGdbOutput(const QString &line)
{
    // MI 输出格式：
    // 结果记录: ^done, ^error, etc.
    // 异步输出: *stopped, =breakpoint-created, ~"text", &"log"
    // 简化处理，使用正则识别关键记录。

    // --- 停止事件 ---
    if (line.startsWith("*stopped"))
    {
        // *stopped,reason="breakpoint-hit",bkptno="1",...
        QString reason = "unknown";
        QRegularExpression re("reason=\"([^\"]+)\"");
        auto match = re.match(line);
        if (match.hasMatch())
            reason = match.captured(1);
        emitStopped(reason, line);

        // 停止后自动请求变量和栈信息
        sendToGdb("-stack-list-frames");
        sendToGdb("-stack-list-variables --simple-values");
    }
    // --- 运行事件 ---
    else if (line.startsWith("*running"))
    {
        emit programRunning();
    }
    // --- 断点创建 ---
    else if (line.startsWith("=breakpoint-created"))
    {
        // 简单刷新断点列表
        sendToGdb("-break-list");
    }
    // --- 栈帧信息 ---
    else if (line.startsWith("^done,stack=[frame="))
    {
        // -stack-list-frames 的响应
        QList<StackFrame> frames;
        QRegularExpression frameRe("frame=\\{([^}]+)\\}");
        auto it = frameRe.globalMatch(line);
        while (it.hasNext())
        {
            auto fMatch = it.next();
            QString frameStr = fMatch.captured(1);
            StackFrame sf;
            QRegularExpression levelRe("level=\"(\\d+)\"");
            QRegularExpression funcRe("func=\"([^\"]+)\"");
            QRegularExpression fileRe("file=\"([^\"]+)\"");
            QRegularExpression lineRe("line=\"(\\d+)\"");
            auto lvl = levelRe.match(frameStr);
            auto fn = funcRe.match(frameStr);
            auto fl = fileRe.match(frameStr);
            auto ln = lineRe.match(frameStr);
            if (lvl.hasMatch())
                sf.level = lvl.captured(1).toInt();
            if (fn.hasMatch())
                sf.func = fn.captured(1);
            if (fl.hasMatch())
                sf.file = fl.captured(1);
            if (ln.hasMatch())
                sf.line = ln.captured(1).toInt();
            // 如果没有函数名但有from，可以使用from
            if (sf.func.isEmpty())
            {
                QRegularExpression fromRe("from=\"([^\"]+)\"");
                auto frm = fromRe.match(frameStr);
                if (frm.hasMatch())
                    sf.func = frm.captured(1);
            }
            frames.append(sf);
        }
        m_currentStack = frames;
        emit stackUpdated(frames);
        // 也请求局部变量（可能已单独请求，但保证刷新）
    }
    // --- 变量列表响应 ---
    else if (line.contains("^done,variables=["))
    {
        QList<Variable> vars;
        // 匹配 {name="var",value="val",type="int"}
        QRegularExpression varRe("\\{([^}]+)\\}");
        auto it = varRe.globalMatch(line);
        while (it.hasNext())
        {
            auto vMatch = it.next();
            QString varStr = vMatch.captured(1);
            Variable v;
            QRegularExpression nameRe("name=\"([^\"]+)\"");
            QRegularExpression valueRe("value=\"([^\"]*)\""); // 值可能为空
            QRegularExpression typeRe("type=\"([^\"]+)\"");
            auto nm = nameRe.match(varStr);
            auto val = valueRe.match(varStr);
            auto tp = typeRe.match(varStr);
            if (nm.hasMatch())
                v.name = nm.captured(1);
            if (val.hasMatch())
                v.value = val.captured(1);
            if (tp.hasMatch())
                v.type = tp.captured(1);
            if (!v.name.isEmpty())
                vars.append(v);
        }
        m_currentLocals = vars;
        emit localsUpdated(vars);
    }
    // --- 断点列表响应 ---
    else if (line.startsWith("^done,BreakpointTable="))
    {
        QList<Breakpoint> bps;
        QRegularExpression bpRe("bkpt=\\{([^}]+)\\}");
        auto it = bpRe.globalMatch(line);
        while (it.hasNext())
        {
            auto bMatch = it.next();
            QString bpStr = bMatch.captured(1);
            Breakpoint bp;
            QRegularExpression numRe("number=\"(\\d+)\"");
            QRegularExpression typeRe("type=\"([^\"]+)\"");
            QRegularExpression enabledRe("enabled=\"([yn])\"");
            QRegularExpression funcRe("func=\"([^\"]+)\"");
            QRegularExpression fileRe("file=\"([^\"]+)\"");
            QRegularExpression lineRe("line=\"(\\d+)\"");
            auto num = numRe.match(bpStr);
            auto typ = typeRe.match(bpStr);
            auto en = enabledRe.match(bpStr);
            auto fn = funcRe.match(bpStr);
            auto fl = fileRe.match(bpStr);
            auto ln = lineRe.match(bpStr);
            if (num.hasMatch())
                bp.number = num.captured(1).toInt();
            if (typ.hasMatch())
                bp.type = typ.captured(1);
            if (en.hasMatch())
                bp.enabled = en.captured(1);
            if (fn.hasMatch())
                bp.func = fn.captured(1);
            if (fl.hasMatch())
                bp.file = fl.captured(1);
            if (ln.hasMatch())
                bp.line = ln.captured(1).toInt();
            bps.append(bp);
        }
        m_breakpoints = bps;
        emit breakpointsUpdated(bps);
    }
    // 其他输出（~"console stream output"）控制台已经显示，无需额外处理
}

void DebuggerManager::emitStopped(const QString &reason, const QString &details)
{
    Q_UNUSED(details)
    m_isRunning = false;
    emit programStopped(reason);
}

void DebuggerManager::onGdbError()
{
    if (m_gdbProcess)
    {
        QByteArray err = m_gdbProcess->readAllStandardError();
        QString errText = QString::fromUtf8(err);
        if (!errText.isEmpty())
        {
            emit consoleOutput("[stderr] " + errText);
        }
    }
}

void DebuggerManager::onGdbFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(status)
    m_isRunning = false;
    emit consoleOutput("GDB 进程已结束。");
    emit programStopped("exited");
}