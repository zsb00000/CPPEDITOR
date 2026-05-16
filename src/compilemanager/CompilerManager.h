#ifndef COMPILERMANAGER_H
#define COMPILERMANAGER_H

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;

class CompilerManager : public QObject
{
    Q_OBJECT

  public:
    explicit CompilerManager(QObject *parent = nullptr);
    ~CompilerManager() override;

    /** 启动时加载已有编译器配置 */
    void setup();

    /** 配置编译器并保存到 JSON */
    QJsonObject setcompile(const QString &compilerPath,
                           const QString &compilerName = QString(),
                           const QStringList &defaultFlags = {});

    // ────────── 同步 API ──────────
    QJsonObject compiling(const QString &sourceFile,
                          const QString &outputFile = QString(),
                          const QStringList &extraFlags = {}) const;

    QJsonObject running(const QString &executablePath,
                        const QStringList &args = {}, bool pause = true) const;

    // ────────── 异步 API ──────────
    QJsonObject compileAsync(const QString &sourceFile,
                             const QString &outputFile = QString(),
                             const QStringList &extraFlags = {});

    QJsonObject runAsync(const QString &executablePath,
                         const QStringList &args = {});

    QJsonObject compileAndRun(const QString &sourceFile,
                              const QStringList &extraFlags = {});

    QJsonObject stop();

    QString lastOutputPath() const;

    QJsonObject manage();

  signals:
    void compilationStarted(const QString &sourceFile);
    void compilationOutput(const QString &line);
    void compilationFinished(bool success, const QString &outputFile,
                             const QString &error);

    void programStarted(const QString &executablePath);
    void programOutput(const QString &line);
    void programFinished(int exitCode);

  private slots:
    void onCompileFinished(int exitCode, QProcess::ExitStatus status);
    void onCompileReadyRead();
    void onRunFinished(int exitCode, QProcess::ExitStatus status);
    void onRunReadyRead();

  private:
    static QJsonObject makeResponse(const QString &process, const QString &type,
                                    const QString &address,
                                    const QString &message);

    QString configFilePath() const;
    void loadConfig();
    void saveConfig() const;

    void killCompileProcess();
    void killRunProcess();

    QString m_compilerPath;
    QString m_compilerName;
    QStringList m_defaultFlags;

    QProcess *m_compileProcess = nullptr;
    QProcess *m_runProcess = nullptr;
    QString m_lastOutputPath;
    QString m_pendingRunExe;
};

#endif // COMPILERMANAGER_H