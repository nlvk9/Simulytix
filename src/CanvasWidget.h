#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>

#include "Canvas.h"
#include "Simulator.h"

class CanvasWidget : public QWidget
{
    Q_OBJECT

public:
    CanvasWidget(Canvas *canvas, Simulator *simulator, QWidget *parent = nullptr);

    void setGlowPhase(float phase) { m_glowPhase = phase; }

signals:
    void componentSelected(int kind);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void drawGrid(QPainter &painter);
    void drawComponent(QPainter &painter, const Component &comp, bool selected, bool ledOn);
    void drawWires(QPainter &painter);
    void drawWireInProgress(QPainter &painter);
    void drawSidebar(QPainter &painter);
    void drawPinLabels(QPainter &painter, const Component &comp);
    void drawPinLeg(QPainter &painter, float sx, float sy, float compW, float compH, float zoom, const Pin &pin);

    void fillCircle(QPainter &painter, int cx, int cy, int rad);
    void drawCircleOutline(QPainter &painter, int cx, int cy, int rad, int thickness = 1);
    void fillRoundedRect(QPainter &painter, int x, int y, int w, int h, int radius);

    Canvas *m_canvas;
    Simulator *m_simulator;
    float m_glowPhase = 0.0f;
    QPoint m_lastMousePos;

    // Sidebar dimensions
    static const int SIDEBAR_WIDTH = 130;
};