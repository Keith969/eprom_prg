// *****************************************************************************
// File         [ guiMainWindow.cpp ]
// Description  [ Implementation of the guiMainWindow class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include "guiMainWindow.h"
#include "E8755Thread.h"
#include "E2708Thread.h"
#include "E2716Thread.h"
#include "E2532Thread.h"
#include "E2732Thread.h"
#include "TMS2716Thread.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtSerialPort/QSerialPortInfo>

#include <chrono>
#include <thread>

// *****************************************************************************
// Function     [ constructor ]
// Description  [ ]
// *****************************************************************************
guiMainWindow::guiMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    // Set the textEdit font to fixed spacing
    ui.textEdit->setFontFamily("Courier");

    // Set shortcut keys for actions
    ui.actionOpen_HEX_file->setShortcut(tr("Ctrl+O"));
    ui.actionSave_HEX_file->setShortcut(tr("Ctrl+S"));
    ui.actionQuit->setShortcut(tr("Ctrl+Q"));

    // Set connections
    QObject::connect(ui.actionOpen_HEX_file, SIGNAL(triggered()),                this, SLOT(openHexFile()));
    QObject::connect(ui.actionSave_HEX_file, SIGNAL(triggered()),                this, SLOT(saveHexFile()));
    QObject::connect(ui.actionQuit,          SIGNAL(triggered()),                this, SLOT(quit()));
    QObject::connect(ui.initButton,          SIGNAL(pressed()),                  this, SLOT(init()));
    QObject::connect(ui.readButton,          SIGNAL(pressed()),                  this, SLOT(read()));
    QObject::connect(ui.checkButton,         SIGNAL(pressed()),                  this, SLOT(check()));
    QObject::connect(ui.writeButton,         SIGNAL(pressed()),                  this, SLOT(write()));
    QObject::connect(ui.verifyButton,        SIGNAL(pressed()),                  this, SLOT(verify()));
    QObject::connect(ui.resetButton,         SIGNAL(pressed()),                  this, SLOT(reset()));
    QObject::connect(ui.loadHexFile,         SIGNAL(clicked()),                  this, SLOT(openHexFile()));
    QObject::connect(ui.saveHexFile,         SIGNAL(clicked()),                  this, SLOT(saveHexFile()));

    // Stuff the serial port combo box
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QString portName = info.portName();
        // Lets ignore bluetooth stuff, we don't use that
        if (! portName.contains("bluetooth", Qt::CaseInsensitive) && ! portName.contains("BLTH", Qt::CaseInsensitive)) {
            ui.serialPort->addItem(info.portName());
        }
    }

    // Baud rate settings
    ui.baudRate->addItem("115200");
    ui.baudRate->addItem("57600");
    ui.baudRate->addItem("38400");
    ui.baudRate->addItem("19200");
    ui.baudRate->addItem("9600");
    ui.baudRate->addItem("4800");
    ui.baudRate->addItem("2400");

    this->setStatusBar(&m_statusBar);

    QLabel* label = new QLabel("Status");
    statusBar()->insertPermanentWidget(0, label);

    m_ledWidget = new QLedWidget;
    setLedColour(Qt::green);
    setLedPower(true);
    statusBar()->insertPermanentWidget(1, m_ledWidget);

    statusBar()->showMessage("Ready");

    m_HexFile = new hexFile;
    m_HexFile->setMainWindow(this);
    m_initOK = false;

    // Until we have init the baud rate, disable the buttons
    ui.checkButton->setEnabled(false);
    ui.readButton->setEnabled(false);
    ui.writeButton->setEnabled(false);
    ui.verifyButton->setEnabled(false);
    ui.resetButton->setEnabled(false);
}

// *****************************************************************************
// Function     [ cdestructor ]
// Description  [ ]
// *****************************************************************************
guiMainWindow::~guiMainWindow()
{
    delete m_HexFile;
}

// *****************************************************************************
// Function     [ openHexFile ]
// Description  [ Reads a hex file into memory and displays it on the textEdit ]
// *****************************************************************************
void
guiMainWindow::openHexFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open HEX File...", ".", "*.hex");
    m_HexFile->readHex(fileName);

    // Display it in the textEdit widget
    clearText();
    std::vector<hexDataChunk> data = m_HexFile->hexData();
    for (auto iter = data.begin(); iter != data.end(); ++iter) {

        hexDataChunk chunk = *iter;
        QString line;

        // Write the address
        line.append(QString("%1:").arg(chunk.address(), 4, 16, QChar('0')));

        // Write the the data
        for (int32_t i = 0; i < chunk.byteCount(); ++i) {
            QString s = QString(" %1").arg(chunk.data().at(i), 2, 16, QChar('0'));
            line.append(s);
        }
        appendText(line);
    }
}

// *****************************************************************************
// Function     [ saveHexFile ]
// Description  [ Save the contents of the textEdit to a hex file ]
// *****************************************************************************
void
guiMainWindow::saveHexFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save HEX File As...", ".", "*.hex");
    if (false == fileName.endsWith(".hex")) {
        fileName += ".hex";
    }

    // Clear any existing hex file
    m_HexFile->clear();

    // Read the textEdit and fill hexfile
    QString text = ui.textEdit->toPlainText();
    QStringList lines = text.split("\n", Qt::SkipEmptyParts);

    uint16_t address=0;
    const int8_t blocksize = 16;
    hexDataChunk chunk;
    bool ok=false;

    // foreach line
    for (auto line_iter = lines.begin(); line_iter != lines.end(); ++line_iter) {
        QString line = *line_iter;
        std::vector<uint8_t> data;
        uint32_t checksum=0;

        // split lines by spaces
        QStringList textlist = line.split(" ", Qt::SkipEmptyParts);
        for (auto token_iter = textlist.begin(); token_iter != textlist.end(); ++token_iter) {

            QString addr = *token_iter;
            // The first item is the address followed by ':'
            if (addr.contains(QChar(':'))) {
                addr.remove(':');
                address = addr.toUShort(&ok, 16);
                chunk.setByteCount(blocksize);
                checksum += blocksize;
                chunk.setAddress(address);
                checksum += address & 0xff;
                checksum += (address >> 8) & 0xff;
                chunk.setRecordType(0);
                checksum += 0;
                token_iter++;
            }

            QString item = *token_iter;
            uint8_t d = (uint8_t) item.toUShort(&ok, 16);
            data.push_back(d);
            checksum += d;
            if (!ok) {
                QString message = QString("Invalid byte %1").arg(item);
                QMessageBox::warning(nullptr, "Not a valid hex value", message);
                return;
            }
        }
        chunk.setData(data);
        uint8_t lsb = ~(checksum & 0xff)+1;
        chunk.setCheckSum(lsb);
        address += blocksize;
        m_HexFile->addChunk(chunk);
    }

    m_HexFile->writeHex(fileName);
}

// *****************************************************************************
// Function     [ getFlowControl ]
// Description  [ Should really always use RTS/CTS ]
// *****************************************************************************
int32_t
guiMainWindow::getFlowControl()
{
    if (ui.flowNone->isChecked())
        return 0;
    /*else if (ui.flowRtsCts->isChecked())*/
        return 1;
}

// *****************************************************************************
// Function     [ reset ]
// Description  [ Reset the programmer ]
// *****************************************************************************
void
guiMainWindow::reset()
{
    QString portName = ui.serialPort->currentText();
    int32_t baudRate = ui.baudRate->currentText().toInt();
    int32_t flowControl = getFlowControl();

    QSerialPort serial;

    serial.setPortName(portName);
    serial.setBaudRate(baudRate);
    serial.setFlowControl((QSerialPort::FlowControl)flowControl);

    if (!serial.open(QIODevice::ReadWrite)) {
        clearText();
        appendText(QString("Can't open %1, error code %2").arg(portName).arg(serial.error()));
        return;
    }

    // Send the PIC a reset cmd
    serial.write(CMD_RSET);
    serial.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    m_initOK = false;

    // Until we have init the baud rate, disable the buttons
    ui.checkButton->setEnabled(false);
    ui.readButton->setEnabled(false);
    ui.writeButton->setEnabled(false);
    ui.verifyButton->setEnabled(false);
    ui.resetButton->setEnabled(false);
    ui.initButton->setEnabled(true);

    clearText();
    statusBar()->showMessage("Ready");
}

// *****************************************************************************
// Function     [ init ]
// Description  [ Send a init command to the PIC via the serial port
//                The init cmd is 'U' which sets the PIC baud rate.
//                The PIC will respond with:
//                - a signal initResponse, data = baud rate
//                - a signal via typeResponse = device type
//              ]
// *****************************************************************************
void
guiMainWindow::init()
{
    QString portName = ui.serialPort->currentText();
    int32_t timeout = ui.timeOut->value() * 1000;
    int32_t baudRate = ui.baudRate->currentText().toInt();
    int32_t flowControl = getFlowControl();
    QString devType = ui.deviceType->currentText();

    if (m_initOK) {
        QMessageBox::warning(this, "Initialisation", "Serial link already set up!", QMessageBox::Ok);
        return;
    }
    else {
        statusBar()->showMessage(QString("Initialising PIC programmer"));
        setLedColour(Qt::red);
        qApp->processEvents();
    }

    initThread init_thread;
    QObject::connect(&init_thread, SIGNAL(error(const QString&)), this, SLOT(serialError(const QString&)));
    QObject::connect(&init_thread, SIGNAL(timeout(const QString&)), this, SLOT(serialTimeout(const QString&)));
    QObject::connect(&init_thread, SIGNAL(response(const QString&)), this, SLOT(initResponse(const QString&)));
    QObject::connect(&init_thread, SIGNAL(type(const QString&)), this, SLOT(typeResponse(const QString&)));
    init_thread.transaction(portName,
                            CMD_INIT,
                            devType,
                            timeout,
                            baudRate,
                            flowControl);
}

// *****************************************************************************
// Function     [ initResponse ]
// Description  [ The response string should be the baud rate info ]
// *****************************************************************************
void
guiMainWindow::initResponse(const QString &s)
{
    int32_t baudRate = ui.baudRate->currentText().toInt();

    clearText();
    bool ok = true;
    int32_t val = 20.0e6 / (4 * (s.toInt(&ok) + 1));
    if (std::abs(100 * (val - baudRate) / baudRate) < 5) {
        QString ss = QString("Initialised serial link to %1 baud").arg(baudRate);
        appendText(ss);
    }
    else {
        QString ss = QString("Error, serial link not %1 baud").arg(baudRate);
        appendText(ss);
    }
    m_initOK = true;

    if (ok == true) {
        // Enable the buttons
        ui.checkButton->setEnabled(true);
        ui.readButton->setEnabled(true);
        ui.writeButton->setEnabled(true);
        ui.verifyButton->setEnabled(true);
        ui.resetButton->setEnabled(true);
        ui.initButton->setEnabled(false);
        statusBar()->showMessage("Initialise OK");
    }
    else {
        serialError(QString("Failed to initialise serial link to %1 baud").arg(baudRate));
    }
}

// *****************************************************************************
// Function     [ typeResponse ]
// Description  [ The response string should be the device type ]
// *****************************************************************************
void
guiMainWindow::typeResponse(const QString& s)
{
    if (s == "OK") {
        QString devType = ui.deviceType->currentText();
        statusBar()->showMessage("Init OK");
        appendText(QString("Set device type to %1").arg(devType));
    }
    else {
        serialError(QString("Failed to write %1 bytes)").arg(s.size()));
    }
    
    setLedColour(Qt::green);
}

// *****************************************************************************
// Function     [ read ]
// Description  [ Send a read command to the PIC ]
// *****************************************************************************
void
guiMainWindow::read()
{
    if (! m_initOK) {
        QMessageBox::critical(this, "Baud rate", "Init baud rate first!", QMessageBox::Ok);
        return;
    }
    else {
        statusBar()->showMessage(QString("Reading from DUT"));
        setLedColour(Qt::red);
        qApp->processEvents();
    }

    QString portName = ui.serialPort->currentText();
    int32_t timeout = ui.timeOut->value() * 1000;
    int32_t baudRate = ui.baudRate->currentText().toInt();
    int32_t flowControl = getFlowControl();
    QString devType = ui.deviceType->currentText();

    readThread read_thread;
    QObject::connect(&read_thread, SIGNAL(error(const QString &)), this, SLOT(serialError(const QString &)));
    QObject::connect(&read_thread, SIGNAL(timeout(const QString &)), this, SLOT(serialTimeout(const QString&)));
    QObject::connect(&read_thread, SIGNAL(response(const QString &)), this, SLOT(readResponse(const QString&)));
    read_thread.transaction(portName,
                            CMD_READ,
                            devType,
                            timeout,
                            baudRate,
                            flowControl);
}

// *****************************************************************************
// Function     [ readResponse ]
// Description  [ The PIC response is the data, already formatted.
//                It might be better to receive raw data and format it here.
//              ]
// *****************************************************************************
void
guiMainWindow::readResponse(const QString &s)
{
    clearText();
    appendText(s);
    statusBar()->showMessage("Ready");
    setLedColour(Qt::green);
}

// *****************************************************************************
// Function     [ check ]
// Description  [ Check if the DUT is programmed ]
// *****************************************************************************
void
guiMainWindow::check()
{
    if (! m_initOK) {
        QMessageBox::critical(this, "Baud rate", "Init baud rate first!", QMessageBox::Ok);
        return;
    }
    else {
        statusBar()->showMessage(QString("Checking DUT"));
        setLedColour(Qt::red);
        qApp->processEvents();
    }

    QString portName = ui.serialPort->currentText();
    int32_t timeout = ui.timeOut->value() * 1000;
    int32_t baudRate = ui.baudRate->currentText().toInt();
    int32_t flowControl = getFlowControl();
    QString devType = ui.deviceType->currentText();

    readThread read_thread;
    QObject::connect(&read_thread, SIGNAL(error(const QString &)), this, SLOT(serialError(const QString &)));
    QObject::connect(&read_thread, SIGNAL(timeout(const QString &)), this, SLOT(serialTimeout(const QString&)));
    QObject::connect(&read_thread, SIGNAL(response(const QString &)), this, SLOT(checkResponse(const QString&)));
    read_thread.transaction(portName,
                            CMD_READ,
                            devType,
                            timeout,
                            baudRate,
                            flowControl);
}

// *****************************************************************************
// Function     [ checkResponse ]
// Description  [ Check the read data is 0x00 or 0xff depending on devType ]
// *****************************************************************************
void
guiMainWindow::checkResponse(const QString &response)
{
    QString devType = ui.deviceType->currentText();

    // Check all bytes read
    clearText();
    int32_t fails=0;

    // Go thru all response data, checking bytes are 0xff
    for (int32_t j=6; j < response.size(); j+=6) {
        // += 6 is to skip addr e.g. '0000: '
        for (int32_t i=0; i < 16; ++i) {
            QString ss = response.mid(j, 2);
            // Convert to hex char
            bool ok = false;
            uint8_t dev_chr = ss.toInt(&ok, 16);
            // Compare with 0xff for all but 8748 which erases to 0x00
            if (devType == "8748") {
                if (0x00 != dev_chr) {
                    fails++;
                }
            }
            else {
                if (0xff != dev_chr) {
                    fails++;
                }
            }
            j += 3;
        }
    }

    // for now just check if we got some
    if (fails == 0) {
        statusBar()->showMessage("Check OK");
        appendText(QString("Blank check passed"));
    }
    else {
        statusBar()->showMessage("Check failed");
        appendText(QString("Blank check failed for %1 bytes").arg(fails));
    }

    setLedColour(Qt::green);
}

// *****************************************************************************
// Function     [ write ]
// Description  [ Write the data we read in from a hex file to the PIC ]
// *****************************************************************************
void
guiMainWindow::write()
{
    if (! m_initOK) {
        QMessageBox::critical(this, "Baud rate", "Init baud rate first!", QMessageBox::Ok);
        return;
    }
    if (m_HexFile->size() > 0) {
        QString portName = ui.serialPort->currentText();
        int32_t timeout = ui.timeOut->value() * 1000;
        int32_t baudRate = ui.baudRate->currentText().toInt();
        int32_t flowControl = getFlowControl();
        QString devType = ui.deviceType->currentText();

        statusBar()->showMessage(QString("Writing to DUT"));
        setLedColour(Qt::red);
        qApp->processEvents();

        if (devType == "8755" || devType == "8748") {

            // Check hex file size is 2kb
            if ((devType == "8755") && (m_HexFile->size() != 2048)) {
                clearText();
                appendText("HEX file size is not 2048 bytes!\n");
                return;
            }
            if ((devType == "8748") && (m_HexFile->size() != 1024)) {
                clearText();
                appendText("HEX file size is not 1024 bytes!\n");
                return;
            }

            QObject::connect(&e8755_thread, SIGNAL(error(const QString&)), this, SLOT(serialError(const QString&)));
            QObject::connect(&e8755_thread, SIGNAL(timeout(const QString&)), this, SLOT(serialTimeout(const QString&)));
            QObject::connect(&e8755_thread, SIGNAL(response(const QString&)), this, SLOT(writeResponse(const QString&)));
            e8755_thread.transaction(portName,
                CMD_READ,
                devType,
                timeout,
                baudRate,
                flowControl,
                m_HexFile);
        }

        else if (devType == "2708") {

            // Check hex file size is 1kb
            if (m_HexFile->size() != 1024) {
                clearText();
                appendText("HEX file size is not 1024 bytes!\n");
                return;
            }

            QObject::connect(&e2708_thread, SIGNAL(error(const QString&)), this, SLOT(serialError(const QString&)));
            QObject::connect(&e2708_thread, SIGNAL(timeout(const QString&)), this, SLOT(serialTimeout(const QString&)));
            QObject::connect(&e2708_thread, SIGNAL(response(const QString&)), this, SLOT(writeResponse(const QString&)));
            e2708_thread.transaction(portName,
                CMD_READ,
                devType,
                timeout,
                baudRate,
                flowControl,
                m_HexFile);
        }

        else if (devType == "TMS2716") {

            // Check hex file size is 2kb
            if (m_HexFile->size() != 2048) {
                clearText();
                appendText("HEX file size is not 2048 bytes!\n");
                return;
            }

            QObject::connect(&t2716_thread, SIGNAL(error(const QString&)), this, SLOT(serialError(const QString&)));
            QObject::connect(&t2716_thread, SIGNAL(timeout(const QString&)), this, SLOT(serialTimeout(const QString&)));
            QObject::connect(&t2716_thread, SIGNAL(response(const QString&)), this, SLOT(writeResponse(const QString&)));
            t2716_thread.transaction(portName,
                CMD_READ,
                devType,
                timeout,
                baudRate,
                flowControl,
                m_HexFile);
        }

        else if (devType == "2716") {

            // Check hex file size is 2kb
            if (m_HexFile->size() != 2048) {
                clearText();
                appendText("HEX file size is not 2048 bytes!\n");
                return;
            }

            QObject::connect(&e2716_thread, SIGNAL(error(const QString&)), this, SLOT(serialError(const QString&)));
            QObject::connect(&e2716_thread, SIGNAL(timeout(const QString&)), this, SLOT(serialTimeout(const QString&)));
            QObject::connect(&e2716_thread, SIGNAL(response(const QString&)), this, SLOT(writeResponse(const QString&)));
            e2716_thread.transaction(portName,
                CMD_READ,
                devType,
                timeout,
                baudRate,
                flowControl,
                m_HexFile);
        }

        else if (devType == "2532") {

            // Check hex file size is 4kb
            if (m_HexFile->size() != 4096) {
                clearText();
                appendText("HEX file size is not 4096 bytes!\n");
                return;
            }

            QObject::connect(&e2532_thread, SIGNAL(error(const QString&)), this, SLOT(serialError(const QString&)));
            QObject::connect(&e2532_thread, SIGNAL(timeout(const QString&)), this, SLOT(serialTimeout(const QString&)));
            QObject::connect(&e2532_thread, SIGNAL(response(const QString&)), this, SLOT(writeResponse(const QString&)));
            e2532_thread.transaction(portName,
                CMD_READ,
                devType,
                timeout,
                baudRate,
                flowControl,
                m_HexFile);
        }

        else if (devType == "2732") {

            // Check hex file size is 4kb
            if (m_HexFile->size() != 4096) {
                clearText();
                appendText("HEX file size is not 4096 bytes!\n");
                return;
            }

            QObject::connect(&e2732_thread, SIGNAL(error(const QString&)), this, SLOT(serialError(const QString&)));
            QObject::connect(&e2732_thread, SIGNAL(timeout(const QString&)), this, SLOT(serialTimeout(const QString&)));
            QObject::connect(&e2732_thread, SIGNAL(response(const QString&)), this, SLOT(writeResponse(const QString&)));
            e2732_thread.transaction(portName,
                CMD_READ,
                devType,
                timeout,
                baudRate,
                flowControl,
                m_HexFile);
        }
    }
    else {
        clearText();
        appendText("No HEX data - please open a HEX file!\n");
    }
}

// *****************************************************************************
// Function     [ Verify ]
// Description  [ Verify device ]
// *****************************************************************************
void
guiMainWindow::verify()
{
    if (!m_initOK) {
        QMessageBox::critical(this, "Baud rate", "Init baud rate first!", QMessageBox::Ok);
        return;
    }
    else {
        statusBar()->showMessage(QString("Verifying DUT"));
        setLedColour(Qt::red);
        qApp->processEvents();
    }

    QString portName = ui.serialPort->currentText();
    int32_t timeout = ui.timeOut->value() * 1000;
    int32_t baudRate = ui.baudRate->currentText().toInt();
    int32_t flowControl = getFlowControl();
    QString devType = ui.deviceType->currentText();

    readThread read_thread;
    QObject::connect(&read_thread, SIGNAL(error(const QString&)), this, SLOT(serialError(const QString&)));
    QObject::connect(&read_thread, SIGNAL(timeout(const QString&)), this, SLOT(serialTimeout(const QString&)));
    QObject::connect(&read_thread, SIGNAL(response(const QString&)), this, SLOT(verifyResponse(const QString&)));
    read_thread.transaction(portName,
        CMD_READ,
        devType,
        timeout,
        baudRate,
        flowControl);
}

// *****************************************************************************
// Function     [ verifyResponse ]
// Description  [ ]
// *****************************************************************************
void
guiMainWindow::verifyResponse(const QString& s)
{
    if (s.size() > 2) {
        clearText();
        // Compare each char of s to the hexfile
        std::vector<hexDataChunk> hexdata = m_HexFile->hexData();
        int32_t j = 0;
        int32_t bad = 0;
        bool ok = true;
        for (auto iter = hexdata.begin(); iter != hexdata.end(); ++iter, ++j) {
            hexDataChunk chunk = *iter;
            // Write the address of the chunk
            QString ss; ss.setNum(chunk.address(), 16);
            ui.textEdit->insertPlainText(QString("%1: ").arg(ss, 4, QChar('0')));
            j += 6; // skip addr e.g. '0000: '
            std::vector<uint8_t> data = chunk.data();
            for (int32_t i = 0; i < data.size(); ++i) {
                // chunk's hex characters given by data.at(i)
                uint8_t hex_chr = data.at(i);
                // dev's data is 2 chars
                QString ss = s.mid(j, 2);
                // Convert to hex char
                uint8_t dev_chr = ss.toInt(&ok, 16);
                // Compare the data. If equal, write the data,
                // if not equal, write the data in red.
                if (hex_chr == dev_chr) {
                    ui.textEdit->insertPlainText(QString("%1 ").arg(ss, 2, QChar('0')));
                }
                else {
                    ui.textEdit->setTextColor(Qt::red);
                    ui.textEdit->insertPlainText(QString("%1 ").arg(ss, 2, QChar('0')));
                    ui.textEdit->setTextColor(Qt::black);
                    bad++;
                }
                // increment j pointer
                if (i == 15) {
                    j += 2;
                }
                else {
                    j += 3;
                }
            }
            ui.textEdit->insertPlainText("\n");
        }
        if (bad != 0) {
            statusBar()->showMessage(QString("DUT has %1 differences with hex file!").arg(bad));
        }
        else {
            statusBar()->showMessage("DUT verified correct.");
        }
    }
    setLedColour(Qt::green);
}

// *****************************************************************************
// Function     [ writeResponse ]
// Description  [ ]
// *****************************************************************************
void
guiMainWindow::writeResponse(const QString &s)
{
    if (s.startsWith("OK")) {
        statusBar()->showMessage("Write OK");
        QStringList bc = s.split(' ');
        int32_t byte_count = bc.at(1).toInt();
        appendText(QString("Wrote %1 bytes").arg(byte_count));
    }
    else {
        QStringList bc = s.split(' ');
        int32_t byte_count = bc.at(1).toInt();
        serialError(QString("Failed to write %1 bytes)").arg(byte_count));
    }
    setLedColour(Qt::green);
}

// *****************************************************************************
// Function     [ quit ]
// Description  [ ]
// *****************************************************************************
void
guiMainWindow::quit()
{
    QCoreApplication::exit();
}

// *****************************************************************************
// Function     [ serialError ]
// Description  [ ]
// *****************************************************************************
void
guiMainWindow::serialError(const QString &s)
{
    QString message = QString("Error: %1").arg(s);
    QMessageBox::warning(nullptr, "Programmer error", message);
    statusBar()->showMessage("Ready");
    setLedColour(Qt::green);
}

// *****************************************************************************
// Function     [ serialTimeout ]
// Description  [ ]
// *****************************************************************************
void
guiMainWindow::serialTimeout(const QString &s)
{
    QString message = QString("Timeout: %1").arg(s);
    QMessageBox::warning(nullptr, "Programmer timeout", message);
    statusBar()->showMessage("Ready");
    setLedColour(Qt::green);
}
