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
initThread::run()
{
    QString portName = ui.serialPort->currentText();
    int32_t timeout = ui.timeOut->value() * 1000;
    int32_t baudRate = ui.baudRate->currentText().toInt();
    int32_t flowControl = getFlowControl();
    QString devType = ui.deviceType->currentText();
    m_devType = devType;

    if (m_initOK) {
        QMessageBox::warning(this, "Initialisation", "Serial link already set up!", QMessageBox::Ok);
    }
    else {
        statusBar()->showMessage(QString("Status: Connecting to port %1.")
                                     .arg(portName));

        // Set the serial port
        m_serialPort = new QSerialPort(this);
        m_serialPort->setPortName(portName);
        m_serialPort->setBaudRate(baudRate);
        m_serialPort->setFlowControl((QSerialPort::FlowControl) flowControl);

        // Now open the port
        if (!m_serialPort->open(QIODevice::ReadWrite)) {
            appendText( QString("Can't open %1, error code %2").arg(portName).arg(m_serialPort->error()) );
            statusBar()->showMessage("Ready");
            return;
        }

        statusBar()->showMessage(QString("Status: Connected to port %1.")
                                     .arg(portName));

        setLedColour(Qt::red);
        qApp->processEvents();

        // Send the cmd, ascii U or 0x55.
        m_serialPort->write(CMD_INIT);

        // Did we get a response?
        if (m_serialPort->waitForBytesWritten(timeout)) {

            // read response from the PIC
            if (m_serialPort->waitForReadyRead(timeout)) {

                // Try and read some data
                QByteArray responseData = m_serialPort->readAll();

                // ... and wait for rest of the data.
                while (m_serialPort->waitForReadyRead(10)) {
                    responseData += m_serialPort->readAll();
                }

                const QString response = QString::fromUtf8(responseData);

                // The response string should be the baud rate info
                clearText();
                bool ok=true;
                int32_t val=20.0e6 / (4 * (response.toInt(&ok) + 1));
                if (std::abs(100*(val - baudRate)/baudRate) < 5) {
                    QString ss = QString("Initialised serial link to %1 baud").arg(baudRate);
                    appendText(ss);
                }
                else {
                    QString ss = QString("Error, serial link not %1 baud").arg(baudRate);
                    appendText(ss);
                }
                m_initOK = true;

                // Enable the buttons
                ui.checkButton->setEnabled(true);
                ui.readButton->setEnabled(true);
                ui.writeButton->setEnabled(true);
                ui.verifyButton->setEnabled(true);
                ui.initButton->setEnabled(false);

                if (ok == true) {
                    statusBar()->showMessage("Initialise OK");
                }
                else {
                    serialError(QString("Failed to initialise serial link to %1 baud").arg(baudRate));
                }
            } else {
                serialTimeout(QString("Read baud rate timeout %1").arg(QTime::currentTime().toString()));
            }
        } else {
            serialTimeout(QString("Send init brg timeout %1").arg(QTime::currentTime().toString()));
        }
    }


    // Now send a device type cmd
    if (m_devType == "2716" ||
        m_devType == "2732" ||
        m_devType == "2532" ||
        m_devType == "2708" ||
        m_devType == "TMS2716" ||
        m_devType == "8755" ||
        m_devType == "8748") {

        // Write the cmd
        m_serialPort->write(CMD_TYPE);

        // Send the cmd arg as per pic code
        // DEV_2716 0
        // DEV_2732 1
        // DEV_2532 2
        // DEV_2708 3
        // DEV_T2716 4
        // DEV_8755 5
        // DEV_8748 6

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

        m_serialPort->write(requestData);


        // Read response from the PIC
        if (m_serialPort->waitForReadyRead(timeout)) {

            QByteArray responseData = m_serialPort->readAll();

            while (m_serialPort->waitForReadyRead(10)) {
                responseData += m_serialPort->readAll();
            }
            const QString response = QString::fromUtf8(responseData);
            if (response == "OK") {
                statusBar()->showMessage("Write OK");
            }
            else {
                serialError(QString("Failed to write %1 bytes)").arg(requestData.size()));
            }
        } else {
            serialTimeout(QString("Read devType timeout %1").arg(QTime::currentTime().toString()));
        }
        appendText(QString("Set device type to %1").arg(m_devType));
    }
}
