#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QString>

/**
 * @brief 单个测试用例
 */
struct TestCase
{
    QString id;
    QString input;
    QString expectedOutput;
    double timeLimit = 1.0; // 秒
};

/**
 * @brief 单个评测结果
 */
struct JudgeResult
{
    QString caseId;
    QString status; // "Accepted", "Wrong Answer", "TimeLimitExceeded",
                    // "RuntimeError", "CompilationError"
    QString actualOutput;
    QString expectedOutput;
    QString error;
    double time = 0.0; // 实际运行时间（秒）
    int exitCode = 0;
};

/**
 * @brief 代码评测引擎
 *
 * 编译并运行用户代码，比对标准输入/输出以判定结果。
 * 支持多个测试用例，返回每个用例的详细结果。
 */
class JudgeEngine : public QObject
{
    Q_OBJECT

  public:
    explicit JudgeEngine(QObject *parent = nullptr);
    ~JudgeEngine() override;

    /**
     * @brief 设置编译器路径
     */
    void setCompilerPath(const QString &path);

    /**
     * @brief 设置编译参数（如 -O2, -std=c++20 等）
     */
    void setCompileArgs(const QStringList &args);

    /**
     * @brief 设置超时时间（秒）
     */
    void setTimeLimit(double seconds);

  public slots:
    /**
     * @brief 编译并评测
     * @param sourcePath 源代码文件路径
     * @param testCases 测试用例列表 (JSON)
     * @return QJsonObject { "success": bool, "results": [...JudgeResult],
     * "compileError": QString }
     */
    QJsonObject judge(const QString &sourcePath, const QJsonArray &testCases);

  signals:
    /**
     * @brief 编译完成
     * @param success 是否成功
     * @param outputPath 可执行文件路径
     */
    void compilationFinished(bool success, const QString &outputPath);

    /**
     * @brief 单用例评测完成
     * @param result 评测结果
     */
    void caseFinished(const JudgeResult &result);

    /**
     * @brief 全部评测完成
     * @param allResults 所有结果
     */
    void judgingFinished(const QList<JudgeResult> &allResults);

  private:
    /**
     * @brief 编译源代码
     * @return QJsonObject { "success": bool, "executablePath": QString,
     * "error": QString }
     */
    QJsonObject compile(const QString &sourcePath);

    /**
     * @brief 运行单个测试用例
     * @param executablePath 可执行文件路径
     * @param testCase 测试用例
     * @return JudgeResult
     */
    JudgeResult runTestCase(const QString &executablePath,
                            const TestCase &testCase);

    /**
     * @brief 解析 JSON 测试用例数组
     */
    QList<TestCase> parseTestCases(const QJsonArray &jsonArray) const;

    /**
     * @brief 标准化输出（去除尾部空白，统一换行）
     */
    QString normalizeOutput(const QString &output) const;

    // ────────── 成员 ──────────
    QString m_compilerPath;    // g++ 路径
    QStringList m_compileArgs; // 编译参数
    double m_timeLimit = 2.0;  // 默认 2 秒
};