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
    size_t                  bytesSent() {return m_bytesSent;}
    size_t                  bytesReceived() {return m_bytesReceived;}

signals:
    void                    response(const QString &s);
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
    size_t                  m_bytesSent;
    size_t                  m_bytesReceived;
};

#endif /* INITTHREAD_H */
