#include "MainWindow.h"
#include "CanvasWidget.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QKeyEvent>
#include "Component.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    setupConnections();

    m_statusLabel->setText("Place components from the sidebar, wire pins, then Upload.");

    m_animationTimer->start(16); // ~60 FPS
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("Simulytix");
    setMinimumSize(1280, 800);
    resize(1280, 800);

    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);

    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_canvasWidget = new CanvasWidget(&m_canvas, &m_simulator, this);
    m_rightPanel = new QWidget;

    m_mainSplitter->addWidget(m_canvasWidget);
    m_mainSplitter->addWidget(m_rightPanel);

    // Set splitter proportions (canvas gets more space)
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);

    QHBoxLayout *centralLayout = new QHBoxLayout(m_centralWidget);
    centralLayout->addWidget(m_mainSplitter);

    // Right panel layout
    QVBoxLayout *rightLayout = new QVBoxLayout(m_rightPanel);

    // Code editor
    m_codeEditor = new QTextEdit;
    m_codeEditor->setFont(QFont("Monaco", 12));
    m_codeEditor->setPlainText("// Servo example\n// Connect servo DATA to D9, VCC to 5V, GND to GND\n\nvoid setup() {\n  pinMode(9, OUTPUT);\n}\n\nvoid loop() {\n  // Move to one position\n  analogWrite(9, 64);\n  delay(1000);\n\n  // Move to the other position\n  analogWrite(9, 192);\n  delay(1000);\n}\n");

    // Buttons
    QWidget *buttonWidget = new QWidget;
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);

    m_uploadButton = new QPushButton("Upload");
    m_stopButton = new QPushButton("Stop");
    m_stopButton->setEnabled(false);

    buttonLayout->addWidget(m_uploadButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();

    // Status label
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("QLabel { color: blue; }");

    rightLayout->addWidget(m_codeEditor);
    rightLayout->addWidget(buttonWidget);
    rightLayout->addWidget(m_statusLabel);
    m_aiTipsPanel = new AiTipsPanel(m_rightPanel);
    rightLayout->addWidget(m_aiTipsPanel, 1);

    // Animation timer
    m_animationTimer = new QTimer(this);
}

void MainWindow::setupConnections()
{
    connect(m_uploadButton, &QPushButton::clicked, this, &MainWindow::onUploadClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_animationTimer, &QTimer::timeout, this, &MainWindow::onTimer);
    connect(m_canvasWidget, &CanvasWidget::componentSelected, [this](int index)
            {
        if (index >= 0 && index < 7) {
            ComponentKind kinds[] = {
                ComponentKind::ArduinoNano,
                ComponentKind::ArduinoUno,
                ComponentKind::LED,
                ComponentKind::Resistor,
                ComponentKind::PushButton,
                ComponentKind::Potentiometer,
                ComponentKind::Servo
            };
            // Place components at the center of the visible canvas
            float visibleCanvasWidth = m_canvasWidget->width() - 130;
            float centerX = m_canvas.screenToWorldX(visibleCanvasWidth * 0.5f);
            float centerY = m_canvas.screenToWorldY(m_canvasWidget->height() * 0.5f);
            m_canvas.addComponent(kinds[index],
                                centerX - 50.f,
                                centerY - 50.f);
            m_canvasWidget->update(); // Trigger repaint
        } });
}

void MainWindow::onUploadClicked()
{
    QString code = m_codeEditor->toPlainText();
    QStringList compNames;
    for (const auto &c : m_canvas.components())
        compNames << QString::fromStdString(componentKindName(c.kind));
    m_aiTipsPanel->setContext(code, compNames);
    m_statusLabel->setText("Starting simulation...");
    m_simulator.start(code.toStdString(),
                      m_canvas.components(),
                      m_canvas.wires());

    if (m_simulator.state() == SimState::Running)
    {
        m_statusLabel->setText("Simulation running.");
        m_uploadButton->setEnabled(false);
        m_stopButton->setEnabled(true);
    }
    else
    {
        m_statusLabel->setText("Error: " + QString::fromStdString(m_simulator.errorMsg()));
    }
}

void MainWindow::onStopClicked()
{
    m_simulator.stop();
    m_statusLabel->setText("Simulation stopped.");
    m_uploadButton->setEnabled(true);
    m_stopButton->setEnabled(false);
}

void MainWindow::onTimer()
{
    m_glowPhase += 0.06f;
    if (m_glowPhase > 6.28f)
        m_glowPhase -= 6.28f;

    m_canvasWidget->setGlowPhase(m_glowPhase);
    m_canvasWidget->update(); // Trigger repaint
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        close();
    }
    else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        m_canvas.deleteSelected();
        m_canvasWidget->update();
    }
    QMainWindow::keyPressEvent(event);
}