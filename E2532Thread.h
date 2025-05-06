#ifndef E2532THREAD_H
#define E2532THREAD_H

// *****************************************************************************
// File         [ E2532Thread.h ]
// Description  [ Implementation of the E2532Thread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include "hexFile.h"

// *****************************************************************************
// Class        [ E2532Thread ]
// Description  [ ]
// *****************************************************************************
class E2532Thread : public QThread
{
    Q_OBJECT

public:
    explicit                E2532Thread(QObject* parent = nullptr);
    ~E2532Thread();

    void                    transaction(const QString& portName,
                                        const QString& request,
                                        const QString& devType,
                                        int waitTimeout = 10000,
                                        int baudRate = 115200,
                                        int flowControl = 1,
                                        hexFile *file = nullptr);
    size_t                  bytesSent() { return m_bytesSent; }
    size_t                  bytesReceived() { return m_bytesReceived; }

signals:
    void                    response(const QString& s);
    void                    error(const QString& s);
    void                    timeout(const QString& s);
    void                    byteCount(int32_t c);
    void                    progress(int32_t val);

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
    hexFile               * m_HexFile;
};

#endif /* E2532THREAD_H */
