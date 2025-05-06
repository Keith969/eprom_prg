#ifndef TMS2716THREAD_H
#define TMS2716THREAD_H

// *****************************************************************************
// File         [ TMS2716Thread.h ]
// Description  [ Implementation of the T2716Thread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include "hexFile.h"

// *****************************************************************************
// Class        [ T2716Thread ]
// Description  [ ]
// *****************************************************************************
class T2716Thread : public QThread
{
    Q_OBJECT

public:
    explicit                T2716Thread(QObject* parent = nullptr);
                            ~T2716Thread();

    void                    transaction(const QString& portName,
                                        const QString& request,
                                        const QString& devType,
                                        int waitTimeout = 10000,
                                        int baudRate = 115200,
                                        int flowControl = 1,
                                        hexFile* file=nullptr);

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
    hexFile               * m_HexFile;
    int16_t                 m_byteCount;
};

#endif /* TMS2716THREAD_H */
