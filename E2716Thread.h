#ifndef E2716THREAD_H
#define E2716THREAD_H

// *****************************************************************************
// File         [ E2716THREAD_H.h ]
// Description  [ Implementation of the E2716Thread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include "hexFile.h"

// *****************************************************************************
// Class        [ E2716Thread ]
// Description  [ ]
// *****************************************************************************
class E2716Thread : public QThread
{
    Q_OBJECT

public:
    explicit                E2716Thread(QObject* parent = nullptr);
                            ~E2716Thread();

    void                    transaction(const QString& portName,
                                        const QString& request,
                                        const QString& devType,
                                        int waitTimeout = 10000,
                                        int baudRate = 115200,
                                        int flowControl = 1,
                                        hexFile* file=nullptr);
    size_t                  bytesSent() { return m_bytesSent; }
    size_t                  bytesReceived() { return m_bytesReceived; }

signals:
    void                    response(const QString& s);
    void                    error(const QString& s);
    void                    timeout(const QString& s);
    void                    byteCount(int32_t c);

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
    hexFile              * m_HexFile;
};

#endif /* E2716THREAD_H */
