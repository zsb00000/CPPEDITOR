#pragma once

#include <QTreeView>

class QFileSystemModel;

/**
 * @brief 文件树 —— 显示项目文件结构
 *
 * Phase 2 将完整实现。
 */
class FileTreeWidget : public QTreeView
{
    Q_OBJECT

  public:
    explicit FileTreeWidget(QWidget *parent = nullptr);

    void setRootPath(const QString &path);
    QString rootPath() const;

  signals:
    void fileDoubleClicked(const QString &filePath);

  private:
    QFileSystemModel *m_model = nullptr;

  private slots:
    void onDoubleClicked(const QModelIndex &index);
    void onCustomContextMenu(const QPoint &pos);
};