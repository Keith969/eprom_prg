#ifndef GUIMAINWINDOW_H
#define GUIMAINWINDOW_H

// *****************************************************************************
// File         [ guiMainWindow.h ]
// Description  [ Implementation of the guiMainWindow class ]
// Author       [ Keith Sabine ]
// *****************************************************************************

#include <QtWidgets/QMainWindow>
#include <QSerialPort>
#include "ui_guiMainWindow.h"
#include "initThread.h"
#include "hexFile.h"
#include "qLedWidget.h"
#include "readThread.h"

// *****************************************************************************
// Class        [ guiMainWindow ]
// Description  [ ]
// *****************************************************************************
class guiMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    guiMainWindow(QWidget *parent = Q_NULLPTR);
    ~guiMainWindow();

public slots:
    void                   openHexFile();
    void                   saveHexFile();
    void                   init();
    void                   quit();
    void                   read();
    void                   check();
    void                   write();
    void                   verify();
    void                   reset();

    // General error slots
    void                   serialError(const QString &);
    void                   serialTimeout(const QString &);

    // Slots to receive cmd responses
    void                   readResponse(const QString &);
    void                   initResponse(const QString &);
    void                   typeResponse(const QString &);
    void                   checkResponse(const QString &);
    // Text window slots
    void                   appendText(const QString& s) {
        ui.textEdit->append(s);
    }
    void                   clearText() { ui.textEdit->clear(); }

    // LED control slots
    void                   setLedPower(bool pwr) { m_ledWidget->setPower(pwr); }
    void                   setLedColour(const QColor& color) { m_ledWidget->setColour(color); m_ledWidget->update(); }

private:
    size_t                 size() {return m_HexFile->size();}
    int32_t                getFlowControl();

    // ui
    Ui::guiMainWindowClass ui;

    // The hex file structure
    hexFile              * m_HexFile;

    // Sender thread
    QSerialPort          * m_serialPort;

    // Status bar
    QStatusBar             m_statusBar;
    QLabel                 m_statusMsg;
    QLedWidget           * m_ledWidget;

    bool                   m_initOK;

    // Device type
    QString                m_devType;
};

#endif /* GUIMAINWINDOW_H */
