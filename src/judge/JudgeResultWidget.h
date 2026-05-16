#pragma once

#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "JudgeEngine.h"

/**
 * @brief 评测结果展示面板 (底部/侧边)
 *
 * 表格列表显示所有用例状态，
 * 下方文本框显示选中用例的详细输出。
 */
class JudgeResultWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit JudgeResultWidget(QWidget *parent = nullptr);
    ~JudgeResultWidget() override;

  public slots:
    /**
     * @brief 设置总用例数，初始化表格
     */
    void beginJudging(int totalCases);

    /**
     * @brief 更新单个用例结果
     */
    void updateCaseResult(const JudgeResult &result);

    /**
     * @brief 评测完成，显示汇总
     */
    void finishJudging(const QList<JudgeResult> &allResults);

    /**
     * @brief 清空所有结果
     */
    void clear();

    /**
     * @brief 显示错误信息（如编译错误）
     */
    void showError(const QString &error);

  signals:
    /**
     * @brief 请求跳转到测试用例对应的预期输出区域（可引申）
     */
    void caseSelected(const QString &caseId);

  private slots:
    void onCaseClicked(int row, int col);
    void onClear();

  private:
    void setupUi();
    void addResultRow(const JudgeResult &result);
    QString statusIcon(const QString &status) const;

    // ────────── 组件 ──────────
    QTableWidget *m_resultTable = nullptr;
    QTextEdit *m_detailView = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QSplitter *m_splitter = nullptr;

    // ────────── 状态 ──────────
    int m_totalCases = 0;
    int m_passedCount = 0;
    QList<JudgeResult> m_allResults;
};