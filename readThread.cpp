// *****************************************************************************
// File         [ readThread.cpp ]
// Description  [ Implementation of the readThread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include "readThread.h"

#include <QtSerialPort/QSerialPort>
#include <QTime>

// *****************************************************************************
// Function     [ constructor ]
// Description  [ ]
// *****************************************************************************
readThread::readThread(QObject *parent) :
    QThread(parent)
{
}

// *****************************************************************************
// Function     [ destructor ]
// Description  [ ]
// *****************************************************************************
readThread::~readThread()
{
    m_mutex.lock();
    m_cond.wakeOne();
    m_mutex.unlock();
    wait();
}

// *****************************************************************************
// Function     [ transaction ]
// Description  [ The transaction for the thread to carry out. ]
// *****************************************************************************
void
readThread::transaction(const QString &portName,
                               const QString &request,
                               const QString &devType,
                               int waitTimeout,
                               int baudRate,
                               int flowControl)
{
    //const QMutexLocker locker(&m_mutex);
    m_portName = portName;
    m_waitTimeout = waitTimeout;
    m_baudrate = baudRate;
    m_flowControl = flowControl;
    m_request = request;
    m_bytesSent = 0;
    m_bytesReceived = 0;
    m_devType = devType;
}

// *****************************************************************************
// Function     [ run ]
// Description  [ The thread's run body. Called when we start() the thread. ]
// *****************************************************************************
void
readThread::run()
{
    m_mutex.lock();
    m_mutex.unlock();

    QSerialPort serial;

    if (m_portName.isEmpty()) {
        emit error(tr("No port name specified"));
        return;
    }

    serial.setPortName(m_portName);
    serial.setBaudRate(m_baudrate);
    serial.setFlowControl((QSerialPort::FlowControl) m_flowControl);

    if (!serial.open(QIODevice::ReadWrite)) {
        emit error(tr("Can't open %1, error code %2")
                    .arg(m_portName).arg(serial.error()));
        return;
    }

    // write request to the PIC
    const QByteArray requestData = m_request.toUtf8();
    
    // Read or verify cmds are just 2 bytes.
    serial.write(requestData);

    // Did we get a response?
    if (serial.waitForBytesWritten(m_waitTimeout)) {

        // read response from the PIC
        if (serial.waitForReadyRead(m_waitTimeout)) {

            QByteArray responseData = serial.readAll();

            while (serial.waitForReadyRead(100)) {
                responseData += serial.readAll();
            }

            const QString response = QString::fromUtf8(responseData);
            emit this->response(response);
        } else {
            emit timeout(tr("Wait read response timeout %1")
                            .arg(QTime::currentTime().toString()));
        }
    } else {
        emit timeout(tr("Wait write request timeout %1")
                        .arg(QTime::currentTime().toString()));
    }
}
