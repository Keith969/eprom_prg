// *****************************************************************************
// File         [ E8755Thread.cpp ]
// Description  [ Implementation of the 8755Thread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include "E8755Thread.h"

#include <QtSerialPort/QSerialPort>
#include <QTime>

#define CMD_WRTE "$2"

// *****************************************************************************
// Function     [ constructor ]
// Description  [ ]
// *****************************************************************************
E8755Thread::E8755Thread(QObject* parent) :
    QThread(parent)
{
    moveToThread(this);
}

// *****************************************************************************
// Function     [ destructor ]
// Description  [ ]
// *****************************************************************************
E8755Thread::~E8755Thread()
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
E8755Thread::transaction(const QString& portName,
    const QString& request,
    const QString& devType,
    int waitTimeout,
    int baudRate,
    int flowControl,
    hexFile *file)
{
    m_portName = portName;
    m_waitTimeout = waitTimeout;
    m_baudrate = baudRate;
    m_flowControl = flowControl;
    m_request = request;
    m_devType = devType;
    m_HexFile = file;
    if (devType == "8755")
        m_byteCount = 2048;
    else if (devType == "8748")
        m_byteCount = 1024;

    if (! this->isRunning()) {
        start();
    }
}

// *****************************************************************************
// Function     [ run ]
// Description  [ The thread's run body. Called when we start() the thread. ]
// *****************************************************************************
void
E8755Thread::run()
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

    int32_t byte_count = 0;

    // Send the cmd, followed by the data.
    QString request(CMD_WRTE);
    // Send the cmd + data
    const QByteArray requestData = request.toUtf8();
    serial.write(requestData);

    // Write the size, max 64k. 2 hex chars.
    uint16_t size = m_HexFile->size();
    QString asc_size = QString("%1").arg(size, 2, 16, QChar('0'));
    serial.write(asc_size.toUtf8());

    // Send the data as bytes, using pairs of chars.
    std::vector<hexDataChunk> hData = m_HexFile->hexData();

    for (auto iter = hData.begin(); iter != hData.end(); ++iter) {
        hexDataChunk chunk = *iter;
        std::vector<uint8_t> data = chunk.data();
        uint8_t count = chunk.byteCount();
        for (int8_t i = 0; i < count; ++i) {
            const short d = data.at(i);
            QByteArray c = QString("%1").arg(d, 2, 16, QChar('0')).toUtf8();
            // If RTS is false, sleep
            //while (m_serialPort->isRequestToSend() == false) {
            //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            //}
            // Delay sending to the program pulse width, in this case 50mS
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            serial.write(c);
            serial.flush();
            byte_count++;
            if (byte_count % (m_byteCount / 100) == 0) {
                emit progress(byte_count * 100 / m_byteCount);
            }
        }
    }

    // Read response from the PIC, should be 'OK'
    if (serial.waitForReadyRead(m_waitTimeout)) {

        QByteArray responseData = serial.readAll();

        while (serial.waitForReadyRead(10)) {
            responseData += serial.readAll();
        }
        const QString response = QString::fromUtf8(responseData)+QString(' ')+QString("%1").arg(byte_count);
        emit this->response(response);
    }
    else {
        emit timeout(QString("Write cmd response timeout %1").arg(QTime::currentTime().toString()));
    }
}
