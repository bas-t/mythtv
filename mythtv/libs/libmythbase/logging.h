#ifndef LOGGING_H_
#define LOGGING_H_

#include <QMutexLocker>
#include <QMutex>
#include <QQueue>
#include <QTime>

#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "mythbaseexp.h"  //  MBASE_PUBLIC , etc.
#include "verbosedefs.h"
#include "mthread.h"
#include "nzmqt.hpp"

#define LOGLINE_MAX (2048-120)

class QString;
class MSqlQuery;
class LoggingItem;

void loggingRegisterThread(const QString &name);
void loggingDeregisterThread(void);

class QWaitCondition;

typedef enum {
    kMessage       = 0x01,
    kRegistering   = 0x02,
    kDeregistering = 0x04,
    kFlush         = 0x08,
    kStandardIO    = 0x10,
    kInitializing  = 0x20,
} LoggingType;

class LoggerThread;

typedef struct tm tmType;

/// \brief The logging items that are generated by LOG() and are sent to the
///        console and to mythlogserver via ZeroMQ
class LoggingItem: public QObject
{
    Q_OBJECT

    Q_PROPERTY(int pid READ pid WRITE setPid)
    Q_PROPERTY(qlonglong tid READ tid WRITE setTid)
    Q_PROPERTY(qulonglong threadId READ threadId WRITE setThreadId)
    Q_PROPERTY(uint usec READ usec WRITE setUsec)
    Q_PROPERTY(int line READ line WRITE setLine)
    Q_PROPERTY(int type READ type WRITE setType)
    Q_PROPERTY(int level READ level WRITE setLevel)
    Q_PROPERTY(int facility READ facility WRITE setFacility)
    Q_PROPERTY(qlonglong epoch READ epoch WRITE setEpoch)
    Q_PROPERTY(QString file READ file WRITE setFile)
    Q_PROPERTY(QString function READ function WRITE setFunction)
    Q_PROPERTY(QString threadName READ threadName WRITE setThreadName)
    Q_PROPERTY(QString appName READ appName WRITE setAppName)
    Q_PROPERTY(QString table READ table WRITE setTable)
    Q_PROPERTY(QString logFile READ logFile WRITE setLogFile)
    Q_PROPERTY(QString message READ message WRITE setMessage)
    Q_ENUMS(LoggingType)
    Q_ENUMS(LogLevel_t)

    friend class LoggerThread;
    friend void LogPrintLine(uint64_t, LogLevel_t, const char *, int,
                             const char *, int, const char *, ... );

  public:
    char *getThreadName(void);
    int64_t getThreadTid(void);
    void setThreadTid(void);
    static LoggingItem *create(const char *, const char *, int, LogLevel_t,
                               LoggingType);
    static LoggingItem *create(QByteArray &buf);
    void deleteItem(void);
    QByteArray toByteArray(void);

    QAtomicInt          refcount;

    int                 pid() const         { return m_pid; };
    qlonglong           tid() const         { return m_tid; };
    qulonglong          threadId() const    { return m_threadId; };
    uint                usec() const        { return m_usec; };
    int                 line() const        { return m_line; };
    int                 type() const        { return (int)m_type; };
    int                 level() const       { return (int)m_level; };
    int                 facility() const    { return m_facility; };
    qlonglong           epoch() const       { return m_epoch; };
    QString             file() const        { return QString(m_file); };
    QString             function() const    { return QString(m_function); };
    QString             threadName() const  { return QString(m_threadName); };
    QString             appName() const     { return QString(m_appName); };
    QString             table() const       { return QString(m_table); };
    QString             logFile() const     { return QString(m_logFile); };
    QString             message() const     { return QString(m_message); };

    void setPid(const int val)              { m_pid = val; };
    void setTid(const qlonglong val)        { m_tid = val; };
    void setThreadId(const qulonglong val)  { m_threadId = val; };
    void setUsec(const uint val)            { m_usec = val; };
    void setLine(const int val)             { m_line = val; };
    void setType(const int val)             { m_type = (LoggingType)val; };
    void setLevel(const int val)            { m_level = (LogLevel_t)val; };
    void setFacility(const int val)         { m_facility = val; };
    void setEpoch(const qlonglong val)      { m_epoch = val; };
    void setFile(const QString val)
            { m_file = strdup(val.toLocal8Bit().constData()); };
    void setFunction(const QString val)
            { m_function = strdup(val.toLocal8Bit().constData()); };
    void setThreadName(const QString val)
            { m_threadName = strdup(val.toLocal8Bit().constData()); };
    void setAppName(const QString val)
            { m_appName = strdup(val.toLocal8Bit().constData()); };
    void setTable(const QString val)
            { m_table = strdup(val.toLocal8Bit().constData()); };
    void setLogFile(const QString val)
            { m_logFile = strdup(val.toLocal8Bit().constData()); };
    void setMessage(const QString val)        
    {
        strncpy(m_message, val.toLocal8Bit().constData(), LOGLINE_MAX);
        m_message[LOGLINE_MAX] = '\0';
    };

    const char *rawFile() const        { return m_file; };
    const char *rawFunction() const    { return m_function; };
    const char *rawThreadName() const  { return m_threadName; };
    const char *rawAppName() const     { return m_appName; };
    const char *rawTable() const       { return m_table; };
    const char *rawLogFile() const     { return m_logFile; };
    const char *rawMessage() const     { return m_message; };

  protected:
    int                 m_pid;
    qlonglong           m_tid;
    qulonglong          m_threadId;
    uint                m_usec;
    int                 m_line;
    LoggingType         m_type;
    LogLevel_t          m_level;
    int                 m_facility;
    qlonglong           m_epoch;
    const char         *m_file;
    const char         *m_function;
    char               *m_threadName;
    const char         *m_appName;
    const char         *m_table;
    const char         *m_logFile;
    char                m_message[LOGLINE_MAX+1];

  private:
    LoggingItem();
    LoggingItem(const char *_file, const char *_function,
                int _line, LogLevel_t _level, LoggingType _type);
};

/// \brief The logging thread that consumes the logging queue and dispatches
///        each LoggingItem to mythlogserver via ZeroMQ
class LoggerThread : public QObject, public MThread
{
    Q_OBJECT
  public:
    LoggerThread(QString filename, bool progress, bool quiet, QString table,
                 int facility);
    ~LoggerThread();
    void run(void);
    void stop(void);
    bool flush(int timeoutMS = 200000);
    void handleItem(LoggingItem *item);
    void fillItem(LoggingItem *item);
  private:
    bool logConsole(LoggingItem *item);
    QWaitCondition *m_waitNotEmpty; ///< Condition variable for waiting
                                    ///  for the queue to not be empty
                                    ///  Protected by logQueueMutex
    QWaitCondition *m_waitEmpty;    ///< Condition variable for waiting
                                    ///  for the queue to be empty
                                    ///  Protected by logQueueMutex
    bool m_aborted;                 ///< Flag to abort the thread.
                                    ///  Protected by logQueueMutex
    bool m_initialWaiting;          ///< Waiting for the initial response from
                                    ///  mythlogserver
    QString m_filename; ///< Filename of debug logfile
    bool m_progress;    ///< show only LOG_ERR and more important (console only)
    int  m_quiet;       ///< silence the console (console only)
    QString m_appname;      ///< Cached application name
    QString m_tablename;    ///< Cached table name for db logging
    int m_facility;         ///< Cached syslog facility (or -1 to disable)
    pid_t m_pid;            ///< Cached pid value

    nzmqt::ZMQContext *m_zmqContext; ///< ZeroMQ context to use in this logger
    nzmqt::ZMQSocket  *m_zmqSocket;  ///< ZeroMQ socket to talk to mythlogserver

  protected slots:
    void messageReceived(const QList<QByteArray>&);
};

#endif

/*
 * vim:ts=4:sw=4:ai:et:si:sts=4
 */
