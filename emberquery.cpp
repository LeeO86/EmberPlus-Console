#include "emberquery.h"
#include <QDebug>

QTextStream out(stdout);


EmberQuery::EmberQuery(QObject *parent) : QObject(parent)
{
    m_libember = new libember_slim_wrapper(this);
    connect(m_libember, &libember_slim_wrapper::finishedEmber, this, &EmberQuery::finishedEmber);
    connect(m_libember, &libember_slim_wrapper::error, this, &EmberQuery::errorEmber);
}

EmberQuery::~EmberQuery()
{
    qDebug() << "Destruct EmberQuery";
    if(m_libember != nullptr)
            delete m_libember;
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
    qDebug() << "Starting to connect to " << m_url.toString() << " ...";
    m_libember->connectEmber(m_url, timeOut);
    m_libember->walkTree();
}

void EmberQuery::finishedEmber(QStringList output)
{
    if(!quietOut)
            out << m_url.toString() << "/ :\n";
    for(const auto& entry : output){
        out << entry << "\n";
    }
    if(!(quietOut || briefOut))
            out << "\nEmber+ Action finished.\nThanks for using EmberPlus-Console...\n";
    quit();
}

void EmberQuery::errorEmber(int retval, QString errorMsg)
{
    if(!quietOut)
            fputs(qPrintable(QString("Error: ")+errorMsg), stderr);
    if(m_libember != nullptr)
            delete m_libember;
    emit error(retval);
}

void EmberQuery::quit()
{
    qDebug() << "Starting EmberQuery::quit()";
    if(m_libember != nullptr)
            delete m_libember;
    emit quitApp();
}
