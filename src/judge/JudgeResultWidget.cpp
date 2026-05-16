#include "JudgeResultWidget.h"

#include <QHeaderView>

JudgeResultWidget::JudgeResultWidget(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

JudgeResultWidget::~JudgeResultWidget() = default;

void JudgeResultWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // 汇总标签
    m_summaryLabel = new QLabel("就绪", this);
    m_summaryLabel->setWordWrap(true);
    mainLayout->addWidget(m_summaryLabel);

    // 分割器: 表格 | 详情
    m_splitter = new QSplitter(Qt::Vertical, this);

    // ── 结果表格 ──
    m_resultTable = new QTableWidget(this);
    m_resultTable->setColumnCount(5);
    m_resultTable->setHorizontalHeaderLabels(
        {"#", "状态", "用时", "输出预览", "ID"});
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::Stretch);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setColumnHidden(4, true); // 隐藏 caseId 列（用于内部）
    connect(m_resultTable, &QTableWidget::cellClicked, this,
            &JudgeResultWidget::onCaseClicked);

    m_splitter->addWidget(m_resultTable);

    // ── 详情视图 ──
    m_detailView = new QTextEdit(this);
    m_detailView->setReadOnly(true);
    m_detailView->setPlaceholderText("选择左侧测试用例以查看详细输出…");
    m_splitter->addWidget(m_detailView);

    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(m_splitter, 1);

    // ── 清空按钮 ──
    m_clearBtn = new QPushButton("清空", this);
    connect(m_clearBtn, &QPushButton::clicked, this,
            &JudgeResultWidget::onClear);
    mainLayout->addWidget(m_clearBtn);
}

void JudgeResultWidget::beginJudging(int totalCases)
{
    clear();
    m_totalCases = totalCases;
    m_resultTable->setRowCount(totalCases);

    // 预填充占位行
    for (int i = 0; i < totalCases; ++i)
    {
        m_resultTable->setItem(i, 0,
                               new QTableWidgetItem(QString::number(i + 1)));
        m_resultTable->setItem(i, 1, new QTableWidgetItem("⏳ 等待中…"));
        m_resultTable->setItem(i, 2, new QTableWidgetItem("—"));
        m_resultTable->setItem(i, 3, new QTableWidgetItem("—"));
    }

    m_summaryLabel->setText(QString("评测中… %1 个用例").arg(totalCases));
}

void JudgeResultWidget::updateCaseResult(const JudgeResult &result)
{
    // 按 caseId 查找对应行
    int targetRow = -1;
    for (int i = 0; i < m_resultTable->rowCount(); ++i)
    {
        auto *item = m_resultTable->item(i, 4);
        if (item && item->text() == result.caseId)
        {
            targetRow = i;
            break;
        }
    }

    // 若未找到，追加新行
    if (targetRow < 0)
    {
        targetRow = m_resultTable->rowCount();
        m_resultTable->insertRow(targetRow);
    }

    // 行号
    m_resultTable->setItem(
        targetRow, 0, new QTableWidgetItem(QString::number(targetRow + 1)));
    // 状态（带图标）
    QString statusText = statusIcon(result.status) + " " + result.status;
    auto *statusItem = new QTableWidgetItem(statusText);
    if (result.status == "Accepted")
        statusItem->setForeground(Qt::darkGreen);
    else if (result.status != "Waiting")
        statusItem->setForeground(Qt::red);
    m_resultTable->setItem(targetRow, 1, statusItem);

    // 用时
    m_resultTable->setItem(
        targetRow, 2,
        new QTableWidgetItem(QString::number(result.time, 'f', 3) + "s"));
    // 输出预览（截断至前 80 字符）
    QString preview = result.actualOutput.left(80).replace('\n', "↵");
    m_resultTable->setItem(targetRow, 3, new QTableWidgetItem(preview));
    // caseId（隐藏）
    m_resultTable->setItem(targetRow, 4, new QTableWidgetItem(result.caseId));

    m_allResults.append(result);
    if (result.status == "Accepted")
        ++m_passedCount;

    m_summaryLabel->setText(QString("已判定: %1 / %2   ✔ %3")
                                .arg(m_allResults.size())
                                .arg(m_totalCases)
                                .arg(m_passedCount));
}

void JudgeResultWidget::finishJudging(const QList<JudgeResult> &allResults)
{
    m_allResults = allResults;
    m_passedCount = 0;

    m_resultTable->setRowCount(0);
    for (const auto &r : allResults)
    {
        addResultRow(r);
        if (r.status == "Accepted")
            ++m_passedCount;
    }

    bool allPassed =
        (m_passedCount == allResults.size() && !allResults.isEmpty());
    QString summary;
    if (allResults.isEmpty())
        summary = "无结果";
    else if (allPassed)
        summary = QString("✔ 全部通过! (%1/%1)").arg(m_passedCount);
    else
        summary =
            QString("✘ %1 / %2 通过").arg(m_passedCount).arg(allResults.size());

    m_summaryLabel->setText(summary);
    if (allPassed)
        m_summaryLabel->setStyleSheet("color: green; font-weight: bold;");
    else
        m_summaryLabel->setStyleSheet("color: red; font-weight: bold;");
}

void JudgeResultWidget::clear()
{
    m_resultTable->setRowCount(0);
    m_detailView->clear();
    m_summaryLabel->setText("就绪");
    m_summaryLabel->setStyleSheet("");
    m_totalCases = 0;
    m_passedCount = 0;
    m_allResults.clear();
}

void JudgeResultWidget::showError(const QString &error)
{
    clear();
    m_summaryLabel->setText("❌ 错误");
    m_summaryLabel->setStyleSheet("color: red; font-weight: bold;");
    m_detailView->setPlainText(error);
}

void JudgeResultWidget::onCaseClicked(int row, int col)
{
    Q_UNUSED(col)
    if (row < 0 || row >= m_allResults.size())
        return;

    const auto &r = m_allResults.at(row);

    QString detail;
    detail += "用例 ID: " + r.caseId + "\n";
    detail += "状态: " + r.status + "\n";
    detail += "用时: " + QString::number(r.time, 'f', 3) + "s\n";
    detail += "退出码: " + QString::number(r.exitCode) + "\n";
    detail += "──────────────────────\n";
    detail += "▼ 预期输出:\n";
    detail += r.expectedOutput + "\n\n";
    detail += "▼ 实际输出:\n";
    detail += r.actualOutput + "\n\n";
    if (!r.error.isEmpty())
    {
        detail += "▼ 错误信息:\n";
        detail += r.error + "\n";
    }

    m_detailView->setPlainText(detail);
    emit caseSelected(r.caseId);
}

void JudgeResultWidget::onClear() { clear(); }

void JudgeResultWidget::addResultRow(const JudgeResult &result)
{
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    m_resultTable->setItem(row, 0,
                           new QTableWidgetItem(QString::number(row + 1)));

    QString statusText = statusIcon(result.status) + " " + result.status;
    auto *statusItem = new QTableWidgetItem(statusText);
    if (result.status == "Accepted")
        statusItem->setForeground(Qt::darkGreen);
    else
        statusItem->setForeground(Qt::red);
    m_resultTable->setItem(row, 1, statusItem);

    m_resultTable->setItem(
        row, 2,
        new QTableWidgetItem(QString::number(result.time, 'f', 3) + "s"));

    QString preview = result.actualOutput.left(80).replace('\n', "↵");
    m_resultTable->setItem(row, 3, new QTableWidgetItem(preview));

    m_resultTable->setItem(row, 4, new QTableWidgetItem(result.caseId));
}

QString JudgeResultWidget::statusIcon(const QString &status) const
{
    if (status == "Accepted")
        return "✔";
    if (status == "Wrong Answer")
        return "✘";
    if (status == "TimeLimitExceeded")
        return "⏰";
    if (status == "RuntimeError")
        return "💥";
    if (status == "CompilationError")
        return "🔧";
    return "❓";
}