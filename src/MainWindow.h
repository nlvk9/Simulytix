#pragma once

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QKeyEvent>

#include "Canvas.h"
#include "CanvasWidget.h"
#include "Simulator.h"
#include "AiTipsPanel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onUploadClicked();
    void onStopClicked();
    void onTimer();

private:
    void setupUI();
    void setupConnections();

    // Core components
    Canvas m_canvas;
    Simulator m_simulator;

    // UI components
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    CanvasWidget *m_canvasWidget;
    QWidget *m_rightPanel;
    QTextEdit *m_codeEditor;
    QPushButton *m_uploadButton;
    QPushButton *m_stopButton;
    QLabel *m_statusLabel;
    AiTipsPanel *m_aiTipsPanel;

    QTimer *m_animationTimer;
    float m_glowPhase = 0.0f;

protected:
    void keyPressEvent(QKeyEvent *event) override;
};