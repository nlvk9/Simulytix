#include "CanvasWidget.h"
#include "Component.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <cmath>

CanvasWidget::CanvasWidget(Canvas *canvas, Simulator *simulator, QWidget *parent)
    : QWidget(parent), m_canvas(canvas), m_simulator(simulator)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

void CanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int canvasWidth = width() - SIDEBAR_WIDTH;
    int canvasHeight = height();

    // Tell canvas where it lives on screen so hit-testing works
    m_canvas->canvasX = SIDEBAR_WIDTH;
    m_canvas->canvasY = 0;
    m_canvas->canvasW = canvasWidth;
    m_canvas->canvasH = canvasHeight;

    // Draw sidebar
    drawSidebar(painter);

    // Set clip rect to canvas area
    painter.setClipRect(SIDEBAR_WIDTH, 0, canvasWidth, canvasHeight);

    // Draw grid
    drawGrid(painter);

    // Draw wires
    drawWires(painter);

    // Draw wire in progress
    drawWireInProgress(painter);

    // Draw components
    for (const auto &comp : m_canvas->components())
    {
        bool ledOn = false;
        if (comp.kind == ComponentKind::LED && m_simulator->state() == SimState::Running)
        {
            ledOn = m_simulator->ledIsOn(comp.id);
        }
        bool selected = m_canvas->isComponentSelected(comp.id);
        drawComponent(painter, comp, selected, ledOn);
    }
}

void CanvasWidget::drawGrid(QPainter &painter)
{
    // Fill canvas with light gray background
    painter.fillRect(SIDEBAR_WIDTH, 0, width() - SIDEBAR_WIDTH, height(), QColor(240, 240, 240));

    // Draw subtle grid lines
    painter.setPen(QPen(QColor(220, 220, 220), 1));

    float zoom = m_canvas->zoom();
    float camX = m_canvas->camX();
    float camY = m_canvas->camY();

    int startX = SIDEBAR_WIDTH;
    int endX = width();
    int startY = 0;
    int endY = height();

    // Vertical lines
    for (float wx = std::floor(camX / 20) * 20;; wx += 20)
    {
        float sx = (wx - camX) * zoom + SIDEBAR_WIDTH;
        if (sx > endX)
            break;
        if (sx >= startX)
        {
            painter.drawLine(sx, startY, sx, endY);
        }
    }

    // Horizontal lines
    for (float wy = std::floor(camY / 20) * 20;; wy += 20)
    {
        float sy = (wy - camY) * zoom;
        if (sy > endY)
            break;
        if (sy >= startY)
        {
            painter.drawLine(startX, sy, endX, sy);
        }
    }
}

void CanvasWidget::drawComponent(QPainter &painter, const Component &comp, bool selected, bool ledOn)
{
    float zoom = m_canvas->zoom();
    float camX = m_canvas->camX();
    float camY = m_canvas->camY();
    int ox = m_canvas->canvasX;

    float sx = (comp.x - camX) * zoom + ox;
    float sy = (comp.y - camY) * zoom;

    painter.save();

    if (selected)
    {
        painter.setPen(QPen(Qt::red, 2));
    }
    else
    {
        painter.setPen(QPen(Qt::black, 1));
    }

    switch (comp.kind)
    {
    case ComponentKind::ArduinoNano:
    case ComponentKind::ArduinoUno:
    {
        // Draw microcontroller as a proper rectangle with pins
        float w = comp.w * zoom;
        float h = comp.h * zoom;

        // Main board
        painter.setBrush(QBrush(QColor(50, 100, 50)));
        painter.drawRoundedRect(sx, sy, w, h, 8, 8);

        // Board label
        painter.setPen(QPen(Qt::black));
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        QString label = (comp.kind == ComponentKind::ArduinoNano) ? "Arduino Nano" : "Arduino Uno";
        painter.drawText(sx, sy, w, 25 * zoom, Qt::AlignCenter, label);

        // Draw pin legs and pins
        painter.setFont(QFont("Arial", 7));
        for (const auto &pin : comp.pins)
        {
            float py = sy + pin.localY * zoom;
            drawPinLeg(painter, sx, sy, comp.w, comp.h, zoom, pin);

            painter.setPen(QPen(Qt::black));
            painter.setBrush(Qt::NoBrush);
            QRectF labelRect;
            if (pin.localX < comp.w * 0.5f)
            {
                labelRect = QRectF(sx - 50, py - 6, 45, 12);
                painter.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, QString::fromStdString(pin.label));
            }
            else
            {
                labelRect = QRectF(sx + w + 5, py - 6, 45, 12);
                painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, QString::fromStdString(pin.label));
            }
        }
        break;
    }

    case ComponentKind::LED:
    {
        // Draw LED as a circle centered within the component bounds
        float centerX = comp.x + comp.w * 0.5f;
        float centerY = comp.y + comp.h * 0.5f;
        float scx = (centerX - camX) * zoom + ox;
        float scy = (centerY - camY) * zoom;
        float radius = 15 * zoom;
        QColor ledColor = ledOn ? QColor(255, 100, 100) : QColor(150, 100, 100);

        painter.setBrush(QBrush(ledColor));
        painter.drawEllipse(scx - radius, scy - radius, radius * 2, radius * 2);

        // Draw pin legs with + / - labels
        painter.setPen(QPen(Qt::black, 1));
        for (const auto &pin : comp.pins)
        {
            drawPinLeg(painter, sx, sy, comp.w, comp.h, zoom, pin);

            // Draw + or - label next to pin dot
            float px = sx + pin.localX * zoom;
            float py = sy + pin.localY * zoom;
            painter.setFont(QFont("Arial", 9, QFont::Bold));
            painter.setPen(QPen(pin.type == PinType::Anode ? QColor(180, 0, 0) : QColor(0, 0, 180)));
            QString lbl = (pin.type == PinType::Anode) ? "+" : "-";
            painter.drawText(QRectF(px + 6, py - 8, 14, 14), Qt::AlignCenter, lbl);
        }
        break;
    }

    case ComponentKind::PushButton:
    {
        // Draw push button from top-left (sx, sy)
        float w = comp.w * zoom;
        float h = comp.h * zoom;
        painter.setBrush(QBrush(QColor(150, 150, 200)));
        painter.drawRoundedRect(sx, sy, w, h, 3, 3);

        painter.setPen(QPen(Qt::black));
        painter.setFont(QFont("Arial", 8));
        painter.drawText(sx, sy, w, h, Qt::AlignCenter, "BTN");

        // Draw pin legs
        painter.setPen(QPen(Qt::black, 1));
        for (const auto &pin : comp.pins)
        {
            drawPinLeg(painter, sx, sy, comp.w, comp.h, zoom, pin);
        }
        break;
    }

    case ComponentKind::Resistor:
    {
        // Draw resistor body from top-left (sx, sy)
        float w = comp.w * zoom;
        float h = comp.h * zoom;
        float cx = sx + w * 0.5f;
        float bodyTop = sy + h * 0.2f;
        float bodyBot = sy + h * 0.8f;
        float bodyH = bodyBot - bodyTop;

        // Lead lines
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(cx, sy, cx, bodyTop);
        painter.drawLine(cx, bodyBot, cx, sy + h);

        // Body rectangle
        painter.setBrush(QBrush(QColor(200, 160, 80)));
        painter.setPen(QPen(Qt::black, 1));
        painter.drawRect(sx + w * 0.1f, bodyTop, w * 0.8f, bodyH);

        // Color bands
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor(220, 0, 0)));
        painter.drawRect(sx + w * 0.25f, bodyTop, w * 0.08f, bodyH);
        painter.setBrush(QBrush(QColor(220, 0, 0)));
        painter.drawRect(sx + w * 0.42f, bodyTop, w * 0.08f, bodyH);
        painter.setBrush(QBrush(QColor(80, 50, 20)));
        painter.drawRect(sx + w * 0.59f, bodyTop, w * 0.08f, bodyH);

        // Pin dots
        painter.setPen(QPen(Qt::black, 1));
        for (const auto &pin : comp.pins)
        {
            drawPinLeg(painter, sx, sy, comp.w, comp.h, zoom, pin);
        }
        break;
    }

    case ComponentKind::Potentiometer:
    {
        // Draw potentiometer
        float w = comp.w * zoom;
        float h = comp.h * zoom;
        painter.setBrush(QBrush(QColor(200, 150, 100)));
        painter.drawRoundedRect(sx, sy, w, h, 3, 3);

        painter.setPen(QPen(Qt::black));
        painter.setFont(QFont("Arial", 7));
        painter.drawText(sx, sy, w, h, Qt::AlignCenter, "POT");

        // Draw pin legs
        painter.setPen(QPen(Qt::black, 1));
        for (const auto &pin : comp.pins)
        {
            drawPinLeg(painter, sx, sy, comp.w, comp.h, zoom, pin);
        }
        break;
    }

    case ComponentKind::Servo:
    {
        // Draw servo body
        float w = comp.w * zoom;
        float h = comp.h * zoom;
        painter.setBrush(QBrush(QColor(100, 100, 150)));
        painter.drawRoundedRect(sx, sy, w, h, 3, 3);

        // Draw rotating fan at center
        float cx = sx + w * 0.5f;
        float cy = sy + h * 0.4f;
        painter.save();
        painter.translate(cx, cy);
        painter.rotate(comp.servoAngle);

        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(QBrush(QColor(200, 200, 200)));

        // Draw fan blades (3 blades at 120 degrees)
        for (int i = 0; i < 3; ++i)
        {
            QPointF blade[3];
            float angle = (i * 120.f) * 3.14159f / 180.f;
            blade[0] = QPointF(0, 0);
            blade[1] = QPointF(6 * zoom * cos(angle), 6 * zoom * sin(angle));
            blade[2] = QPointF(4 * zoom * cos(angle + 30.f * 3.14159f / 180.f),
                               4 * zoom * sin(angle + 30.f * 3.14159f / 180.f));
            painter.drawPolygon(blade, 3);
        }

        // Draw hub
        painter.setBrush(QBrush(Qt::black));
        painter.drawEllipse(-3 * zoom, -3 * zoom, 6 * zoom, 6 * zoom);

        painter.restore();

        painter.setPen(QPen(Qt::black));
        painter.setFont(QFont("Arial", 7));
        painter.drawText(sx, sy, w, h, Qt::AlignCenter, "SERVO");

        // Draw pin legs and labels
        painter.setPen(QPen(Qt::black, 1));
        painter.setFont(QFont("Arial", 7));
        for (const auto &pin : comp.pins)
        {
            drawPinLeg(painter, sx, sy, comp.w, comp.h, zoom, pin);
            float px = sx + pin.localX * zoom;
            float py = sy + pin.localY * zoom;
            painter.setPen(QPen(Qt::black));
            painter.drawText(QPointF(px - 10.f, py + 15.f), QString::fromStdString(pin.label));
        }
        break;
    }

    default:
    {
        // Generic component
        float w = comp.w * zoom;
        float h = comp.h * zoom;
        painter.setBrush(QBrush(QColor(150, 150, 150)));
        painter.drawRoundedRect(sx, sy, w, h, 3, 3);

        painter.setPen(QPen(Qt::black));
        painter.setFont(QFont("Arial", 8));
        painter.drawText(sx, sy, w, h, Qt::AlignCenter, "COMP");

        // Draw pin legs
        painter.setPen(QPen(Qt::black, 1));
        for (const auto &pin : comp.pins)
        {
            drawPinLeg(painter, sx, sy, comp.w, comp.h, zoom, pin);
        }
        break;
    }
    }

    painter.restore();
}

void CanvasWidget::drawPinLeg(QPainter &painter, float sx, float sy, float compW, float compH, float zoom, const Pin &pin)
{
    // pin.localX/Y are in world units; sx/sy are screen coords of component top-left
    float px = sx + pin.localX * zoom;
    float py = sy + pin.localY * zoom;

    // Find the nearest component edge in screen space
    float startX = px;
    float startY = py;

    if (pin.localX < 0)
    {
        startX = sx; // left edge
    }
    else if (pin.localX > compW)
    {
        startX = sx + compW * zoom; // right edge
    }

    if (pin.localY < 0)
    {
        startY = sy; // top edge
    }
    else if (pin.localY > compH)
    {
        startY = sy + compH * zoom; // bottom edge
    }

    painter.setBrush(Qt::NoBrush);
    painter.drawLine(startX, startY, px, py);
    painter.drawEllipse(px - 3, py - 3, 6, 6);
}

void CanvasWidget::drawWires(QPainter &painter)
{
    painter.setPen(QPen(Qt::blue, 2));

    for (const auto &wire : m_canvas->wires())
    {
        int idxA = m_canvas->componentIndexById(wire.compA);
        int idxB = m_canvas->componentIndexById(wire.compB);
        if (idxA < 0 || idxB < 0)
            continue;

        const Component &comp1 = m_canvas->components()[idxA];
        const Component &comp2 = m_canvas->components()[idxB];

        const Pin &pin1 = comp1.pins[wire.pinA];
        const Pin &pin2 = comp2.pins[wire.pinB];

        float zoom = m_canvas->zoom();
        float camX = m_canvas->camX();
        float camY = m_canvas->camY();
        int ox = m_canvas->canvasX;

        float x1 = (comp1.x + pin1.localX - camX) * zoom + ox;
        float y1 = (comp1.y + pin1.localY - camY) * zoom;
        float x2 = (comp2.x + pin2.localX - camX) * zoom + ox;
        float y2 = (comp2.y + pin2.localY - camY) * zoom;

        painter.drawLine(x1, y1, x2, y2);
    }
}

void CanvasWidget::drawWireInProgress(QPainter &painter)
{
    WireInProgress wip = m_canvas->wip();
    if (!wip.active)
        return;

    painter.setPen(QPen(Qt::green, 2, Qt::DashLine));

    int idx = m_canvas->componentIndexById(wip.compId);
    if (idx < 0)
        return;

    const Component &comp = m_canvas->components()[idx];
    const Pin &pin = comp.pins[wip.pinIdx];

    float zoom = m_canvas->zoom();
    float camX = m_canvas->camX();
    float camY = m_canvas->camY();
    int ox = m_canvas->canvasX;

    float x1 = (comp.x + pin.localX - camX) * zoom + ox;
    float y1 = (comp.y + pin.localY - camY) * zoom;
    float x2 = (wip.mouseX - camX) * zoom + ox;
    float y2 = (wip.mouseY - camY) * zoom;

    painter.drawLine(x1, y1, x2, y2);
}

void CanvasWidget::drawSidebar(QPainter &painter)
{
    // Sidebar background
    painter.fillRect(0, 0, SIDEBAR_WIDTH, height(), QColor(240, 240, 240));

    // Component palette
    painter.setFont(QFont("Arial", 10));
    painter.setPen(QPen(Qt::black));

    int y = 10;
    const char *componentNames[] = {
        "Arduino Nano",
        "Arduino Uno",
        "LED",
        "Resistor",
        "Push Button",
        "Potentiometer",
        "Servo"};

    for (int i = 0; i < 7; ++i)
    {
        // Draw component icon (simple rectangle for now)
        painter.setBrush(QBrush(QColor(200, 200, 200)));
        painter.drawRoundedRect(10, y, 110, 30, 5, 5);

        painter.drawText(10, y, 110, 30, Qt::AlignCenter, componentNames[i]);
        y += 40;
    }
}

void CanvasWidget::drawPinLabels(QPainter &painter, const Component &comp)
{
    float zoom = m_canvas->zoom();
    float camX = m_canvas->camX();
    float camY = m_canvas->camY();

    float sx = (comp.x - camX) * zoom + SIDEBAR_WIDTH;
    float sy = (comp.y - camY) * zoom;

    painter.setFont(QFont("Arial", 7));
    painter.setPen(QPen(Qt::black));

    for (size_t i = 0; i < comp.pins.size(); ++i)
    {
        const Pin &pin = comp.pins[i];
        float px = sx + pin.localX * zoom;
        float py = sy + pin.localY * zoom;

        QString label = QString::number(i);
        painter.drawText(px + 5, py - 5, label);
    }
}

void CanvasWidget::fillCircle(QPainter &painter, int cx, int cy, int rad)
{
    painter.setBrush(QBrush(painter.pen().color()));
    painter.drawEllipse(cx - rad, cy - rad, rad * 2, rad * 2);
}

void CanvasWidget::drawCircleOutline(QPainter &painter, int cx, int cy, int rad, int thickness)
{
    QPen pen = painter.pen();
    pen.setWidth(thickness);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(cx - rad, cy - rad, rad * 2, rad * 2);
}

void CanvasWidget::fillRoundedRect(QPainter &painter, int x, int y, int w, int h, int radius)
{
    painter.drawRoundedRect(x, y, w, h, radius, radius);
}

void CanvasWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();

    int x = event->x();
    int y = event->y();

    // Always keep canvas bounds in sync before hit-testing
    m_canvas->canvasX = SIDEBAR_WIDTH;
    m_canvas->canvasY = 0;
    m_canvas->canvasW = width() - SIDEBAR_WIDTH;
    m_canvas->canvasH = height();

    if (x < SIDEBAR_WIDTH)
    {
        // Sidebar click - determine which component
        int itemHeight = 40;
        int itemIndex = (y - 10) / itemHeight;
        if (itemIndex >= 0 && itemIndex < 7)
        {
            emit componentSelected(itemIndex);
        }
        return;
    }

    // Canvas click - pass actual Qt button enum and full x coordinate
    m_canvas->onMouseDown(x, y, (int)event->button());
    update();
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_canvas->canvasX = SIDEBAR_WIDTH;
    m_canvas->canvasY = 0;
    m_canvas->canvasW = width() - SIDEBAR_WIDTH;
    m_canvas->canvasH = height();

    m_canvas->onMouseUp(event->x(), event->y(), (int)event->button());
    update();
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_canvas->canvasX = SIDEBAR_WIDTH;
    m_canvas->canvasY = 0;
    m_canvas->canvasW = width() - SIDEBAR_WIDTH;
    m_canvas->canvasH = height();

    QPoint pos = event->pos();
    int dx = pos.x() - m_lastMousePos.x();
    int dy = pos.y() - m_lastMousePos.y();
    m_lastMousePos = pos;

    m_canvas->onMouseMove(event->x(), event->y(), dx, dy);
    update();
}

void CanvasWidget::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() > 0 ? 1.0f : -1.0f;
    QPointF pos = event->position();
    m_canvas->onScroll((int)pos.x(), (int)pos.y(), delta);
    update();
}

void CanvasWidget::keyPressEvent(QKeyEvent *event)
{
    m_canvas->onKey(event->key());
    update();
}