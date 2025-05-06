#ifndef INITTHREAD_H
#define INITTHREAD_H

// *****************************************************************************
// File         [ initThread.h ]
// Description  [ Implementation of the initThread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

// Cmds for PIC
#define CMD_DONE "$0"
#define CMD_READ "$1"
#define CMD_WRTE "$2"
#define CMD_CHEK "$3"
#define CMD_IDEN "$4"
#define CMD_TYPE "$5"
#define CMD_RSET "$9"
#define CMD_INIT "U"

// Device type codes
#define DEV_2716  0
#define DEV_2732  1
#define DEV_2532  2
#define DEV_2708  3
#define DEV_T2716 4
#define DEV_8755  5
#define DEV_8748  6

// *****************************************************************************
// Class        [ initThread ]
// Description  [ ]
// *****************************************************************************
class initThread : public QThread
{
    Q_OBJECT

public:
    explicit                initThread(QObject *parent = nullptr);
                            ~initThread();

    void                    transaction(const QString &portName,
                                const QString &request,
                                const QString &devType,
                                int waitTimeout=10000,
                                int baudRate=115200,
                                int flowControl=0);

signals:
    void                    response(const QString &s);
    void                    type(const QString& s);
    void                    error(const QString &s);
    void                    timeout(const QString &s);

private:
    void                    run() override;

    QString                 m_portName;
    QString                 m_request;
    QString                 m_devType;
    int                     m_waitTimeout = 0;
    QMutex                  m_mutex;
    QWaitCondition          m_cond;
    int32_t                 m_baudrate = 115200;
    int32_t                 m_flowControl = 0;
};

#endif /* INITTHREAD_H */
