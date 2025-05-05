// *****************************************************************************
// File         [ initThread.cpp ]
// Description  [ Implementation of the initThread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include "initThread.h"

#include <QtSerialPort/QSerialPort>
#include <QTime>

// *****************************************************************************
// Function     [ constructor ]
// Description  [ ]
// *****************************************************************************
initThread::initThread(QObject *parent) :
    QThread(parent)
{
    moveToThread(this);
}

// *****************************************************************************
// Function     [ destructor ]
// Description  [ ]
// *****************************************************************************
initThread::~initThread()
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
initThread::transaction(const QString &portName,
                        const QString &request,
                        const QString &devType,
                        int waitTimeout,
                        int baudRate,
                        int flowControl)
{
    m_portName = portName;
    m_waitTimeout = waitTimeout;
    m_baudrate = baudRate;
    m_flowControl = flowControl;
    m_request = request;
    m_bytesSent = 0;
    m_bytesReceived = 0;
    m_devType = devType;

    if (!this->isRunning()) {
        start();
    }
}

// *****************************************************************************
// Function     [ run ]
// Description  [ The thread's run body. Called when we start() the thread. ]
// *****************************************************************************
void
initThread::run()
{
    QSerialPort serial;

    if (m_portName.isEmpty()) {
        emit error(tr("No port name specified"));
        return;
    }

    serial.setPortName(m_portName);
    serial.setBaudRate(m_baudrate);
    serial.setFlowControl((QSerialPort::FlowControl)m_flowControl);

    if (!serial.open(QIODevice::ReadWrite)) {
        emit error(tr("Can't open %1, error code %2")
            .arg(m_portName).arg(serial.error()));
        return;
    }

    // write request to the PIC
    const QByteArray requestData = m_request.toUtf8();

    // Send the cmd, ascii U or 0x55.
    serial.write(requestData);

    // Did we get a response?
    if (serial.waitForBytesWritten(m_waitTimeout)) {

        // read response from the PIC
        if (serial.waitForReadyRead(m_waitTimeout)) {

            // Try and read some data
            QByteArray responseData = serial.readAll();

            // ... and wait for rest of the data.
            while (serial.waitForReadyRead(10)) {
                responseData += serial.readAll();
            }

            const QString response = QString::fromUtf8(responseData);
            emit this->response(response);

        } else {
                emit timeout(QString("Read baud rate timeout %1")
                    .arg(QTime::currentTime().toString()));
        }
    } else {
            emit timeout(QString("Send init brg timeout %1")
                .arg(QTime::currentTime().toString()));
    }

    // Now send a device type cmd
    if (m_devType == "2716"    ||
        m_devType == "2732"    ||
        m_devType == "2532"    ||
        m_devType == "2708"    ||
        m_devType == "TMS2716" ||
        m_devType == "8755"    ||
        m_devType == "8748") {

        // Write the cmd
        serial.write(CMD_TYPE);

        // Send the cmd arg as per pic code
        QByteArray requestData;
        if (m_devType == "2716")
            requestData = QString("0").toUtf8();
        else if (m_devType == "2732")
            requestData = QString("1").toUtf8();
        else if (m_devType == "2532")
            requestData = QString("2").toUtf8();
        else if (m_devType == "2708")
            requestData = QString("3").toUtf8();
        else if (m_devType == "TMS2716")
            requestData = QString("4").toUtf8();
        else if (m_devType == "8755")
            requestData = QString("5").toUtf8();
        else if (m_devType == "8748")
            requestData = QString("6").toUtf8();

        serial.write(requestData);


        // Read response from the PIC
        if (serial.waitForReadyRead(m_waitTimeout)) {

            QByteArray responseData = serial.readAll();

            while (serial.waitForReadyRead(10)) {
                responseData += serial.readAll();
            }

            const QString response = QString::fromUtf8(responseData);
            emit this->type(response);

        } else {
            emit timeout(QString("Read devType timeout %1").arg(QTime::currentTime().toString()));
        }
    }
}
