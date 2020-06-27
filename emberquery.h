#ifndef EMBERQUERY_H
#define EMBERQUERY_H

#include <QCoreApplication>

class EmberQuery
{
public:
    EmberQuery();
    void setBrief(bool b) { briefOut = b; }
    void setJson(bool j) { jsonOut = j; briefOut = j; }
    void setQuiet(bool q) { quietOut = q; }
    void setWriteString(bool w, QString wS) { write = w; writeVal = wS;}
    bool isQuiet() { return quietOut; }

private:
    bool briefOut = false;
    bool jsonOut = false;
    bool quietOut = false;
    bool write = false;
    QString writeVal;
};

#endif // EMBERQUERY_H
