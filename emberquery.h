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
    void setQuiet(bool q) { quietOut = q; }
    void setWriteString(bool w, QString wS) { write = w; writeVal = wS;}
    void setTimeOut(int t) { timeOut = t;}
    bool isQuiet() { return quietOut; }
    bool setAddress(QString address);

public slots:
    void start();
    void finishedEmber(QStringList output);
    void errorEmber(int retval, QString errorMsg);

private:
    void quit();

    bool briefOut = false;
    bool jsonOut = false;
    bool quietOut = false;
    bool write = false;
    int timeOut = DEF_TIMEOUT;
    QString writeVal;
    QUrl m_url;

    libember_slim_wrapper *m_libember;

signals:
    void quitApp();
    void error(int retval);
};

#endif // EMBERQUERY_H
