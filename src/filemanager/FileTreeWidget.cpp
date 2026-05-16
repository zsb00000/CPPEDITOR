#include "FileTreeWidget.h"

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QMenu>

FileTreeWidget::FileTreeWidget(QWidget *parent) : QTreeView(parent)
{
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setNameFilters({"*.cpp", "*.h", "*.hpp", "*.c", "*.cc", "*.cxx",
                             "*.txt", "*.md", "*.qss", "*.json", "*.xml",
                             "*.yaml", "*.yml", "*.pro", "*.cmake",
                             "CMakeLists.txt", "*.py"});
    m_model->setNameFilterDisables(false);

    setModel(m_model);
    setRootIndex(m_model->index(QDir::currentPath()));

    // 隐藏大小、类型等列，只显示文件名
    setColumnHidden(1, true); // 大小
    setColumnHidden(2, true); // 类型
    setColumnHidden(3, true); // 修改日期

    setHeaderHidden(true);
    setAnimated(true);
    setIndentation(16);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QTreeView::doubleClicked, this,
            &FileTreeWidget::onDoubleClicked);
    connect(this, &QTreeView::customContextMenuRequested, this,
            &FileTreeWidget::onCustomContextMenu);
}

void FileTreeWidget::setRootPath(const QString &path)
{
    if (path.isEmpty())
        return;
    setRootIndex(m_model->index(path));
}

QString FileTreeWidget::rootPath() const
{
    return m_model->filePath(rootIndex());
}

void FileTreeWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!m_model->isDir(index))
    {
        QString filePath = m_model->filePath(index);
        emit fileDoubleClicked(filePath);
    }
}

void FileTreeWidget::onCustomContextMenu(const QPoint &pos)
{
    QModelIndex index = indexAt(pos);
    if (!index.isValid())
        return;

    QMenu contextMenu(this);

    if (m_model->isDir(index))
    {
        QAction *openFolderAction = contextMenu.addAction("在资源管理器中打开");
        connect(openFolderAction, &QAction::triggered, this,
                [this, index]()
                {
                    QString path = m_model->filePath(index);
                    // TODO: 跨平台打开文件夹
                });
    }
    else
    {
        QAction *openAction = contextMenu.addAction("打开");
        connect(openAction, &QAction::triggered, this, [this, index]()
                { emit fileDoubleClicked(m_model->filePath(index)); });

        QAction *deleteAction = contextMenu.addAction("删除");
        connect(deleteAction, &QAction::triggered, this,
                [this, index]()
                {
                    QString path = m_model->filePath(index);
                    m_model->remove(index);
                });
    }

    contextMenu.exec(viewport()->mapToGlobal(pos));
}