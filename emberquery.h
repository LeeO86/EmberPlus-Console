#ifndef EMBERQUERY_H
#define EMBERQUERY_H

#include <QObject>
#include <QTextStream>

#include "libember_slim_wrapper.h"

#define DEF_TIMEOUT 1000


class EmberQuery : public QObject
{
    Q_OBJECT

public:
    explicit EmberQuery(QObject *parent = nullptr);
    ~EmberQuery();
    void setBrief(bool b) { briefOut = b; }
    void setJson(bool j) { jsonOut = j; briefOut = j; }
    void setNumberOut(bool n) { if(n) m_flags |= EMBER_FLAGS_NUMBER_OUT; else m_flags &= ~EMBER_FLAGS_NUMBER_OUT; }
    bool setPaths(QStringList paths, QString &errorMsg);
    void setQuiet(bool q) { quietOut = q; }
    void setVerbose(bool v) { if(v) m_flags |= EMBER_FLAGS_VERBOSE_OUT; else m_flags &= ~EMBER_FLAGS_VERBOSE_OUT; }
    void setWriteString(bool w) { if(w) m_flags |= EMBER_FLAGS_WRITE; else m_flags &= ~EMBER_FLAGS_WRITE; }
    void setTimeOut(int t) { timeOut = t*1000;}
    bool isJson() { return jsonOut; }
    bool isQuiet() { return quietOut; }
    bool setAddress(QString address);

public slots:
    void start();
    void run();
    void finishedEmber(QStringList output);
    void errorEmber(int retval, QString errorMsg);

private:
    bool sendNext();
    void quit();

    bool briefOut = false;
    bool jsonOut = false;
    bool quietOut = false;
    byte m_flags = 0;
    int timeOut = DEF_TIMEOUT;
    int sentPath = 0;
    int retValBuffer = 0;
    QList<QStringList> m_pathList;
    QList<QString> m_writeValList;
    QUrl m_url;

    libember_slim_wrapper *m_libember;

signals:
    void quitApp();
    void error(int retval);
};

#endif // EMBERQUERY_H
