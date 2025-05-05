#ifndef READTHREAD_H
#define READTHREAD_H

// *****************************************************************************
// File         [ ReadThread.h ]
// Description  [ Implementation of the SenderThread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

// *****************************************************************************
// Class        [ readThread ]
// Description  [ ]
// *****************************************************************************
class readThread : public QThread
{
    Q_OBJECT

public:
    explicit readThread(QObject *parent = nullptr);
    ~readThread();

    void                      transaction(const QString &portName,
                                            const QString &request,
                                            const QString &devType,
                                            int waitTimeout=10000,
                                            int baudRate=115200,
                                            int flowControl=0);
    size_t                    bytesSent() {return m_bytesSent;}
    size_t                    bytesReceived() {m_bytesReceived;}

signals:
    void                      response(const QString &s);
    void                      error(const QString &s);
    void                      timeout(const QString &s);

private:
    void                      run() override;

    QString                   m_portName;
    QString                   m_request;
    QString                   m_devType;
    int                       m_waitTimeout = 0;
    QMutex                    m_mutex;
    QWaitCondition            m_cond;
    int32_t                   m_baudrate = 115200;
    int32_t                   m_flowControl = 0;
    size_t                    m_bytesSent;
    size_t                    m_bytesReceived;
};

#endif /* READTHREAD_H */
