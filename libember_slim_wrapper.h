#ifndef LIBEMBER_SLIM_WRAPPER_H
#define LIBEMBER_SLIM_WRAPPER_H

#include <QObject>
#include <QtNetwork>
#include <QDebug>

extern "C" {
    #include "emberplus.h"
    #include "emberinternal.h"
}


#define EMBER_DEF_PORT 9000
#define IDENT_PATH_BUFFER 256

// forward Declaration of Class
class libember_slim_wrapper;

typedef struct SPtrListNode
{
   voidptr value;
   struct SPtrListNode *pNext;
} PtrListNode;

typedef struct SPtrList
{
   PtrListNode *pHead;
   PtrListNode *pLast;
   int count;
} PtrList;

typedef struct STarget
{
   berint number;
   berint *pConnectedSources;
   int connectedSourcesCount;
} Target;

typedef struct SSource
{
   berint number;
} Source;

typedef struct SElement
{
   berint number;
   GlowElementType type;
   GlowFieldFlags paramFields;

   union
   {
      GlowNode node;
      GlowParameter param;

      struct
      {
         GlowMatrix matrix;
         PtrList targets;
         PtrList sources;
      } matrix;

      GlowFunction function;
   } glow;

   PtrList children;
   struct SElement *pParent;
} Element;

typedef struct SSession
{
   libember_slim_wrapper *obj;
   pcstr remoteAddress;
   int remotePort;
   Element root;
   Element *pCursor;
   berint cursorPathBuffer[GLOW_MAX_TREE_DEPTH];
   berint *pCursorPath;
   int cursorPathLength;

   // some reading state
   pstr pEnumeration;
   pstr pFormula;
   pstr pFormat;
} Session;

class libember_slim_wrapper : public QObject
{
    Q_OBJECT
public:
    explicit libember_slim_wrapper(QObject *parent = nullptr);
    ~libember_slim_wrapper();
    void connectEmber(QUrl url, int timeOut);
    void walkTree();

private:
    void send(QByteArray msg);
    void getDirectory(Element *pElement);
    void callChild(Element *pElement);
    void nodeReturned(Element *pElement);
    void findParams(Element *pStart);
    void writeParam(Element *param);

    static void onNode(const GlowNode *pNode, GlowFieldFlags fields, const berint *pPath, int pathLength, voidptr state);
    static void onParameter(const GlowParameter *pParameter, GlowFieldFlags fields, const berint *pPath, int pathLength, voidptr state);
    static void onMatrix(const GlowMatrix *pMatrix, const berint *pPath, int pathLength, voidptr state);
    static void onTarget(const GlowSignal *pSignal, const berint *pPath, int pathLength, voidptr state);
    static void onSource(const GlowSignal *pSignal, const berint *pPath, int pathLength, voidptr state);
    static void onConnection(const GlowConnection *pConnection, const berint *pPath, int pathLength, voidptr state);
    static void onFunction(const GlowFunction *pFunction, const berint *pPath, int pathLength, voidptr state);
    static void onInvocationResult(const GlowInvocationResult *pInvocationResult, voidptr state);
    static void onOtherPackageReceived(const byte *pPackage, int length, voidptr state);
    static void onUnsupportedTltlv(const BerReader *pReader, const berint *pPath, int pathLength, GlowReaderPosition position, voidptr state);
    static void onLastPackageRecieved(const byte *pPackage, int length, voidptr state);

    QTcpSocket *tcpSock;
    GlowReader p_reader;
    byte *pRxBuffer;
    Session m_session;
    QTimer *readingTimeOut;
    bool m_walk = false;
    bool m_numPathOut = false;
    QStringList output;

private slots:
    void readSocket();
    void socketError(QAbstractSocket::SocketError socketError);
    void runFinished();

signals:
    void finishedEmber(QStringList output);
    void error(int retval, QString errorMsg);

};

#endif // LIBEMBER_SLIM_WRAPPER_H
