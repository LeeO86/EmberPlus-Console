#include "emberquery.h"
#include <QDebug>

extern QTextStream out;
extern QTextStream err;


EmberQuery::EmberQuery(QObject *parent) : QObject(parent)
{
    m_libember = new libember_slim_wrapper(this);
    connect(m_libember, &libember_slim_wrapper::finishedWalk, this, &EmberQuery::finishedEmber);
    connect(m_libember, &libember_slim_wrapper::error, this, &EmberQuery::errorEmber);
    connect(m_libember, &libember_slim_wrapper::destroyed, this, &EmberQuery::quit);
}

EmberQuery::~EmberQuery()
{
    qDebug() << "Destruct EmberQuery";
    if(m_libember != nullptr)
            delete m_libember;
}

bool EmberQuery::setPaths(QStringList paths, QString &errorMsg)
{
    bool ret = true;
    QStringList path;
    QString value;
    for (QString &p : paths) {
        if (p.isEmpty() || p.startsWith("  "))
                continue;
        if(p.contains("/:")) {
            int delimPos = p.lastIndexOf("/:");
            value = p.right(p.size()-(delimPos + 2));
            if (value.isEmpty() && m_flags & EMBER_FLAGS_WRITE) {
                errorMsg.append(QString("There is no Value to write in %1 but the --write option is set! Please specify a Value to write.").arg(p));
                ret = false;
            } else
                    m_writeValList.append(value);
            p.truncate(delimPos);
            path = p.split(m_flags & EMBER_FLAGS_NUMBER_OUT ? "." : "/");
            if (m_flags & EMBER_FLAGS_NUMBER_OUT) {
                for (QString &num : path) {
                    bool ok;
                    num.toInt(&ok);
                    if(!ok){
                        errorMsg.append(QString("In Path %1 is \"%2\" not a number but the --numbered-path Option is set. Please specify a valid numbered Path.").arg(p).arg(num));
                        ret = false;
                    }
                }
            }
            m_pathList.append(path);
        } else {
            errorMsg.append(QString("There was no Path-Delimiter ( /: ) found in %1").arg(p));
            ret = false;
        }

    }
    if (paths.isEmpty() && m_flags & EMBER_FLAGS_WRITE) {
        errorMsg.append("With no \"'path'/:'value'\" specified, the --write option is not allowed.");
        ret = false;
    }
    qDebug() << paths << m_pathList << m_writeValList;
    return ret;
}

bool EmberQuery::setAddress(QString address)
{
    m_url = QUrl::fromUserInput(address);
    bool ret = m_url.isValid();
    m_url.setScheme("ember");
    return ret;
}

void EmberQuery::start()
{
    if(!(quietOut || briefOut)){
        if (timeOut != DEF_TIMEOUT)
                out << "Timeout is set to: " << timeOut/1000 << "s" << Qt::endl;
        out << "Starting to connect to " << m_url.toString() << "/ ... ";
    }
    connect(m_libember, &libember_slim_wrapper::emberConnected, this, &EmberQuery::run);
    m_libember->connectEmber(m_url, timeOut);
}

void EmberQuery::run()
{
    if(!(quietOut || briefOut)){
        out << " connetcted" << Qt::endl;
        if (m_flags & EMBER_FLAGS_WRITE)
                out << "Writing to Ember+ Tree ... ";
        else
                out << "Walking over Ember+ Tree ... ";
    }
    m_libember->walkTree(m_flags, m_pathList.value(sentPath), m_writeValList.value(sentPath));
}

void EmberQuery::finishedEmber(QMap<QString, Answer> output)
{
    if(!(quietOut || briefOut) && sentPath == 0)
            out << " started \n" << Qt::endl;
    m_outputMap.insert(output);
    if (!sendNext()) {
        quitEmber();
    }
}

void EmberQuery::errorEmber(int retval, QString errorMsg)
{
    retValBuffer = retval;
    if(!quietOut){
        if (!briefOut && sentPath == 0)
            out << "error\n" << Qt::endl;
        err << "Error: " << errorMsg << Qt::endl;
    }
    if (retval == 126) {
        if (sendNext())
            return;
    }
    m_libember->disconnect();
    emit error(retval);
}

bool EmberQuery::sendNext()
{
    sentPath++;
    if(m_pathList.size() > sentPath) {
        m_libember->walkTree(m_flags, m_pathList.value(sentPath), m_writeValList.value(sentPath));
        return true;
    } else {
        return false;
    }
}

void EmberQuery::quitEmber()
{
    qDebug() << "Starting Emberquery::quitEmber()";
    m_libember->disconnect();
    m_libember->deleteLater();
}

void EmberQuery::quit()
{
    qDebug() << "Starting EmberQuery::quit()";
    QStringList inPaths;
    if (!quietOut && sentPath == 0)
            out << m_url.toString() << "/ :" << Qt::endl;
    if (!(quietOut || briefOut) && m_flags & EMBER_FLAGS_WRITE){
        for (auto &pL : m_pathList) {
            if (m_flags & EMBER_FLAGS_NUMBER_OUT)
                    inPaths.append(QString(pL.join(".")).append("/:"));
            else
                    inPaths.append(QString(pL.join("/")).append("/:"));
        }
    }
    QMapIterator<QString, Answer> iter(m_outputMap);
    while (iter.hasNext())
    {
        iter.next();
        if (m_flags & EMBER_FLAGS_NUMBER_OUT){
            out << iter.value().numPath << iter.value().value << Qt::endl;
        } else {
            out << iter.value().identPath << iter.value().value << Qt::endl;
        }
        if (!iter.value().verboseOut.isEmpty()){
            for(auto& vOut : iter.value().verboseOut){
                out << vOut << Qt::endl;
            }
        }
        if (!(quietOut || briefOut) && m_flags & EMBER_FLAGS_WRITE){
            int index = inPaths.lastIndexOf((m_flags & EMBER_FLAGS_NUMBER_OUT) ? iter.value().numPath : iter.value().identPath);
            if (QString::compare(m_writeValList.at(index), iter.value().value)){
                out << "Warning: The written value '" << m_writeValList.at(index) << "' differs from '" << iter.value().value << "'! Probably its already overwritten?" << Qt::endl << Qt::endl;
            }
        }
    }
    if (!(quietOut || briefOut) && !retValBuffer){
        out << "\nEmber+ Action finished.\nThanks for using EmberPlus-Console..." << Qt::endl;
    }
    if (retValBuffer)
        emit error(retValBuffer);
    else
        emit quitApp();
}
