#pragma once

#include <QJsonArray>
#include <QObject>
#include <QProcess>
#include <QString>

/**
 * @brief 单条 Lint 诊断信息
 */
struct LintDiagnostic
{
    QString filePath;
    int line = 0;
    int column = 0;
    QString severity; // "error", "warning", "note"
    QString code;     // e.g. "unused-variable"
    QString message;
};

/**
 * @brief 动态代码检错模块
 *
 * 封装 clang-tidy / clang 静态分析调用，
 * 提供实时诊断信息用于编辑器中下划线高亮。
 * 也支持 GCC 的诊断输出格式（fallback）。
 */
class LinterManager : public QObject
{
    Q_OBJECT

  public:
    explicit LinterManager(QObject *parent = nullptr);
    ~LinterManager() override;

    /**
     * @brief 设置编译器路径（用于 clang-tidy / clang++）
     */
    void setCompilerPath(const QString &compilerPath);

    /**
     * @brief 设置启用/禁用实时检测
     */
    void setEnabled(bool enabled);

    /**
     * @brief 是否启用
     */
    bool isEnabled() const;

  public slots:
    /**
     * @brief 对代码进行静态分析
     * @param filePath 文件路径
     * @param code 当前代码内容（用于内存分析）
     * @return QJsonArray of LintDiagnostic
     */
    QJsonArray lintFile(const QString &filePath, const QString &code);

  signals:
    /**
     * @brief 诊断结果就绪
     * @param filePath 相关文件
     * @param diagnostics JSON 数组，每个元素为诊断对象
     */
    void diagnosticsReady(const QString &filePath,
                          const QJsonArray &diagnostics);

    /**
     * @brief Lint 错误
     */
    void lintError(const QString &error);

  private:
    /**
     * @brief 使用 clang-tidy 分析
     */
    QJsonArray lintWithClangTidy(const QString &filePath);

    /**
     * @brief 使用编译器 -fsyntax-only 分析（fallback）
     */
    QJsonArray lintWithCompiler(const QString &filePath);

    /**
     * @brief 使用 clang++ -fsyntax-only 快速语法检查
     */
    QJsonArray lintWithClangSyntax(const QString &filePath,
                                   const QString &compiler);

    /**
     * @brief 解析 clang/GCC 的诊断输出
     */
    QList<LintDiagnostic> parseCompilerOutput(const QString &output) const;

    /**
     * @brief 检测 clang-tidy 是否可用
     */
    bool isClangTidyAvailable() const;

    // ────────── 成员 ──────────
    QString m_compilerPath; // 编译器路径（g++ 或 clang++）
    bool m_enabled = true;
};