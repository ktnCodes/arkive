#ifndef ARTICLEMODEL_H
#define ARTICLEMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

class ArticleModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY articleChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY articleChanged)
    Q_PROPERTY(QString rawContent READ rawContent NOTIFY articleChanged)
    Q_PROPERTY(QString htmlContent READ htmlContent NOTIFY articleChanged)
    Q_PROPERTY(QStringList backlinks READ backlinks NOTIFY articleChanged)
    Q_PROPERTY(QStringList sectionHeadings READ sectionHeadings NOTIFY articleChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY articleChanged)

public:
    explicit ArticleModel(QObject *parent = nullptr);

    QString title() const;
    QString filePath() const;
    QString rawContent() const;
    QString htmlContent() const;
    QStringList backlinks() const;
    QStringList sectionHeadings() const;
    bool loaded() const;

    // Load an article from file
    Q_INVOKABLE void loadFromFile(const QString &path);

    // Clear the current article
    Q_INVOKABLE void clear();

signals:
    void articleChanged();

private:
    QString m_title;
    QString m_filePath;
    QString m_rawContent;
    QString m_htmlContent;
    QStringList m_backlinks;
    QStringList m_sectionHeadings;
    bool m_loaded = false;
};

#endif // ARTICLEMODEL_H
