// *****************************************************************************
// File         [ TMS2716Thread.cpp ]
// Description  [ Implementation of the T2716Thread class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include "TMS2716Thread.h"

#include <QtSerialPort/QSerialPort>
#include <QTime>

#define CMD_WRTE "$2"

// *****************************************************************************
// Function     [ constructor ]
// Description  [ ]
// *****************************************************************************
T2716Thread::T2716Thread(QObject* parent) :
    QThread(parent)
{
    moveToThread(this);
}

// *****************************************************************************
// Function     [ destructor ]
// Description  [ ]
// *****************************************************************************
T2716Thread::~T2716Thread()
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
T2716Thread::transaction(const QString& portName,
    const QString& request,
    const QString& devType,
    int waitTimeout,
    int baudRate,
    int flowControl,
    hexFile* file)
{
    m_portName = portName;
    m_waitTimeout = waitTimeout;
    m_baudrate = baudRate;
    m_flowControl = flowControl;
    m_request = request;
    m_devType = devType;
    m_HexFile = file;
    m_byteCount = 2048;

    if (!this->isRunning()) {
        start();
    }
}

// *****************************************************************************
// Function     [ run ]
// Description  [ The thread's run body. Called when we start() the thread. ]
// *****************************************************************************
void
T2716Thread::run()
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


    // Repeat write 100 times...
    for (int32_t j = 0; j < 100; ++j) {

        int32_t byte_count = 0;
        emit progress(j);

        // Send the cmd, followed by the data.
        QString request(CMD_WRTE);
        // Send the cmd + data
        const QByteArray requestData = request.toUtf8();
        serial.write(requestData);

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
                //    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                //}
                // Delay sending to the program pulse width, in this case 1mS
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                serial.write(c);
                serial.flush();
                byte_count++;
            }
        }

#if 1 // Changes 050925 KAS to allow writing blanks to fill EPROM
        int16_t size = 2048;
        // If we wrote less than the EPROM capacity, fill with 1's
        if (m_HexFile->size() < size) {
            for (int8_t i = byte_count; i < size; ++i) {
                const short d = 0xff;
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
                    emit progress(byte_count * 100 / size);
                }
            }
        }
#endif

        // Read response from the PIC
        if (serial.waitForReadyRead(m_waitTimeout)) {

            QByteArray responseData = serial.readAll();

            while (serial.waitForReadyRead(10)) {
                responseData += serial.readAll();
            }

            const QString response = QString::fromUtf8(responseData) + QString(' ') + QString("%1").arg(byte_count);
            emit this->response(response);
        }
        else {
            emit timeout(QString("Write cmd response timeout %1").arg(QTime::currentTime().toString()));
        }
    }
}
