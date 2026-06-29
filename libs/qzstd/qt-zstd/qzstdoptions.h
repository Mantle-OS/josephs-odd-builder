#ifndef QZSTDOPTIONS_H
#define QZSTDOPTIONS_H

#include <QObject>
#include <QDataStream>

class QZstdOptions : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString input READ input WRITE setInput NOTIFY inputChanged FINAL)
    Q_PROPERTY(QString output READ output WRITE setOutput NOTIFY outputChanged FINAL)

    Q_PROPERTY(int current READ current WRITE setCurrent NOTIFY currentChanged FINAL)
    Q_PROPERTY(int total READ total WRITE setTotal NOTIFY totalChanged FINAL)

    Q_PROPERTY(int compressionLevel READ compressionLevel WRITE setCompressionLevel NOTIFY compressionLevelChanged)

    Q_PROPERTY(QString errorString READ errorString WRITE setErrorString NOTIFY errorStringChanged FINAL)
public:
    explicit QZstdOptions(QObject *parent = nullptr);
    ~QZstdOptions() override = default;

    static QString magicDirString()  {return  QStringLiteral("JOBZCRYPDIR1"); }
    static QString magicFileString() {return  QStringLiteral("JOBZCRYPFILE1"); }
    // Maybe needed later ?
    static QString magicLinkString() {return  QStringLiteral("JOBZCRYPLINK1"); }
    static QDataStream::Version headerVersion(){ return QDataStream::Qt_6_0; }

    QString input() const;
    void setInput(const QString &newInput);

    QString output() const;
    void setOutput(const QString &newOutput);

    int current() const;
    void setCurrent(int newCurrent);

    int total() const;
    void setTotal(int newTotal);

    int compressionLevel() const;
    void setCompressionLevel(int newCompressionLevel);

    QString errorString() const;
    void setErrorString(const QString &newErrorString);


Q_SIGNALS:
    void inputChanged();
    void outputChanged();
    void currentChanged(int curr);
    void totalChanged(int total);
    void compressionLevelChanged();
    void finished();
    void errorStringChanged(QString);

private:
    QString m_input;
    QString m_output;
    int m_current;
    int m_total;
    int m_compressionLevel;
    QString m_errorString;
};

#endif // QZSTDOPTIONS_H
