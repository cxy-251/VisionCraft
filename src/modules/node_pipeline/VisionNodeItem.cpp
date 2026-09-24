#include "VisionNodeItem.h"
#include "ConnectionWireItem.h"
#include "core/ScreenCapture.h"
#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QUuid>
#include <QDateTime>
#include <chrono>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

VisionNodeItem::VisionNodeItem(VisionNodeType type, const QString &title, const QPointF &pos)
    : m_type(type), m_title(title) {
    m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    setPos(pos);
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setCursor(Qt::PointingHandCursor);

    switch (m_type) {
        case VisionNodeType::ScreenCapture:
            m_icon = "🖥️";
            m_headerColor = QColor("#059669");
            break;
        case VisionNodeType::SyntheticImage:
            m_icon = "🎲";
            m_headerColor = QColor("#0d9488");
            break;
        case VisionNodeType::Grayscale:
            m_icon = "⚫";
            m_headerColor = QColor("#0284c7");
            break;
        case VisionNodeType::GaussianBlur:
            m_icon = "💧";
            m_headerColor = QColor("#0284c7");
            break;
        case VisionNodeType::CannyEdge:
            m_icon = "⚡";
            m_headerColor = QColor("#d97706");
            break;
        case VisionNodeType::Threshold:
            m_icon = "🌓";
            m_headerColor = QColor("#d97706");
            break;
        case VisionNodeType::Morphology:
            m_icon = "🧱";
            m_headerColor = QColor("#475569");
            break;
        case VisionNodeType::ContourInspection:
            m_icon = "🎯";
            m_headerColor = QColor("#7c3aed");
            break;
        case VisionNodeType::QRCodeDetector:
            m_icon = "📱";
            m_headerColor = QColor("#2563eb");
            break;
    }

    initDefaultParams();
}

VisionNodeItem::~VisionNodeItem() {
    auto inWires = m_incomingWires;
    for (auto *w : inWires) delete w;
    auto outWires = m_outgoingWires;
    for (auto *w : outWires) delete w;
}

bool VisionNodeItem::hasInputSocket() const {
    return m_type != VisionNodeType::ScreenCapture && m_type != VisionNodeType::SyntheticImage;
}

QPointF VisionNodeItem::inputSocketScenePos() const {
    return mapToScene(QPointF(0, HEIGHT * 0.5));
}

QPointF VisionNodeItem::outputSocketScenePos() const {
    return mapToScene(QPointF(WIDTH, HEIGHT * 0.5));
}

void VisionNodeItem::addIncomingWire(ConnectionWireItem *wire) {
    if (wire && !m_incomingWires.contains(wire)) {
        m_incomingWires.append(wire);
    }
}

void VisionNodeItem::addOutgoingWire(ConnectionWireItem *wire) {
    if (wire && !m_outgoingWires.contains(wire)) {
        m_outgoingWires.append(wire);
    }
}

void VisionNodeItem::removeWire(ConnectionWireItem *wire) {
    m_incomingWires.removeAll(wire);
    m_outgoingWires.removeAll(wire);
}

void VisionNodeItem::initDefaultParams() {
    switch (m_type) {
        case VisionNodeType::ScreenCapture:
            m_params["mode"] = "center"; // "center", "mouse", "full"
            m_params["roiW"] = 640;
            m_params["roiH"] = 400;
            break;
        case VisionNodeType::SyntheticImage:
            m_params["hasDefect"] = true;
            m_params["holeCount"] = 4;
            break;
        case VisionNodeType::Grayscale:
            break;
        case VisionNodeType::GaussianBlur:
            m_params["ksize"] = 7;
            m_params["sigma"] = 2.0;
            break;
        case VisionNodeType::CannyEdge:
            m_params["lowThresh"] = 50;
            m_params["highThresh"] = 140;
            break;
        case VisionNodeType::Threshold:
            m_params["thresh"] = 128;
            m_params["otsu"] = true;
            break;
        case VisionNodeType::Morphology:
            m_params["op"] = "Close"; // Dilate, Erode, Open, Close
            m_params["ksize"] = 3;
            break;
        case VisionNodeType::ContourInspection:
            m_params["minArea"] = 80;
            m_params["maxArea"] = 80000;
            break;
        case VisionNodeType::QRCodeDetector:
            break;
    }
}

QVariant VisionNodeItem::param(const QString &key, const QVariant &defVal) const {
    return m_params.value(key, defVal);
}

void VisionNodeItem::setParam(const QString &key, const QVariant &val) {
    m_params[key] = val;
}

void VisionNodeItem::process(const cv::Mat &inputMat) {
    auto t1 = std::chrono::high_resolution_clock::now();
    m_isSuccess = true;

    try {
        switch (m_type) {
            case VisionNodeType::ScreenCapture: {
                QString mode = param("mode", "center").toString();
                int rw = param("roiW", 640).toInt();
                int rh = param("roiH", 400).toInt();
                QImage qimg;
                if (mode == "mouse") {
                    QPoint p = QCursor::pos();
                    qimg = ScreenCapture::grabScreenRegion(p.x() - rw/2, p.y() - rh/2, rw, rh);
                } else if (mode == "center") {
                    auto screens = QGuiApplication::screens();
                    if (!screens.isEmpty()) {
                        QRect geo = screens.first()->geometry();
                        qimg = ScreenCapture::grabScreenRegion(geo.center().x() - rw/2, geo.center().y() - rh/2, rw, rh);
                    }
                } else {
                    qimg = ScreenCapture::grabScreen(0);
                }
                if (!qimg.isNull()) {
                    m_outputMat = ScreenCapture::qImageToMat(qimg);
                    m_statusMessage = QString("%1x%2 实时流").arg(m_outputMat.cols).arg(m_outputMat.rows);
                } else {
                    m_outputMat = cv::Mat(400, 600, CV_8UC3, cv::Scalar(40, 40, 40));
                    m_statusMessage = "抓取失败";
                    m_isSuccess = false;
                }
                break;
            }

            case VisionNodeType::SyntheticImage: {
                // 生成工业工件金属板合成图像
                m_outputMat = cv::Mat(400, 600, CV_8UC3, cv::Scalar(210, 215, 220));
                
                // 绘制工件外轮廓倒角矩形
                cv::rectangle(m_outputMat, cv::Rect(50, 40, 500, 320), cv::Scalar(160, 165, 170), -1);
                cv::rectangle(m_outputMat, cv::Rect(50, 40, 500, 320), cv::Scalar(80, 85, 90), 3);

                // 中央主装配通孔
                cv::circle(m_outputMat, cv::Point(300, 200), 55, cv::Scalar(40, 45, 50), -1);
                cv::circle(m_outputMat, cv::Point(300, 200), 65, cv::Scalar(110, 115, 120), 2);

                // 四周螺丝沉孔
                cv::circle(m_outputMat, cv::Point(100, 90), 16, cv::Scalar(50, 50, 50), -1);
                cv::circle(m_outputMat, cv::Point(500, 90), 16, cv::Scalar(50, 50, 50), -1);
                cv::circle(m_outputMat, cv::Point(100, 310), 16, cv::Scalar(50, 50, 50), -1);
                cv::circle(m_outputMat, cv::Point(500, 310), 16, cv::Scalar(50, 50, 50), -1);

                // 可选划痕缺陷
                if (param("hasDefect", true).toBool()) {
                    cv::line(m_outputMat, cv::Point(320, 130), cv::Point(440, 240), cv::Scalar(30, 30, 220), 3);
                    cv::circle(m_outputMat, cv::Point(190, 210), 9, cv::Scalar(20, 20, 200), -1);
                }

                // 激光雕刻文字
                cv::putText(m_outputMat, "PART-QC-2026-OK", cv::Point(180, 75), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(50, 50, 50), 2);
                m_statusMessage = "合成工件就绪";
                break;
            }

            case VisionNodeType::Grayscale: {
                if (inputMat.empty()) {
                    m_outputMat = cv::Mat();
                    break;
                }
                if (inputMat.channels() == 3 || inputMat.channels() == 4) {
                    cv::cvtColor(inputMat, m_outputMat, cv::COLOR_BGR2GRAY);
                } else {
                    m_outputMat = inputMat.clone();
                }
                m_statusMessage = QString("单通道灰度 (%1x%2)").arg(m_outputMat.cols).arg(m_outputMat.rows);
                break;
            }

            case VisionNodeType::GaussianBlur: {
                if (inputMat.empty()) break;
                int k = param("ksize", 7).toInt();
                if (k % 2 == 0) k += 1;
                double sigma = param("sigma", 2.0).toDouble();
                cv::GaussianBlur(inputMat, m_outputMat, cv::Size(k, k), sigma);
                m_statusMessage = QString("核: %1x%1 | σ: %2").arg(k).arg(sigma, 0, 'f', 1);
                break;
            }

            case VisionNodeType::CannyEdge: {
                if (inputMat.empty()) break;
                cv::Mat gray;
                if (inputMat.channels() == 3) {
                    cv::cvtColor(inputMat, gray, cv::COLOR_BGR2GRAY);
                } else {
                    gray = inputMat;
                }
                double l = param("lowThresh", 50).toDouble();
                double h = param("highThresh", 140).toDouble();
                cv::Canny(gray, m_outputMat, l, h);
                m_statusMessage = QString("边缘提取 (%1-%2)").arg(static_cast<int>(l)).arg(static_cast<int>(h));
                break;
            }

            case VisionNodeType::Threshold: {
                if (inputMat.empty()) break;
                cv::Mat gray;
                if (inputMat.channels() == 3) {
                    cv::cvtColor(inputMat, gray, cv::COLOR_BGR2GRAY);
                } else {
                    gray = inputMat;
                }
                bool otsu = param("otsu", true).toBool();
                double t = param("thresh", 128).toDouble();
                int flags = cv::THRESH_BINARY;
                if (otsu) flags |= cv::THRESH_OTSU;
                cv::threshold(gray, m_outputMat, t, 255, flags);
                m_statusMessage = otsu ? "Otsu 自适应二值化" : QString("固定阈值 %1").arg(static_cast<int>(t));
                break;
            }

            case VisionNodeType::Morphology: {
                if (inputMat.empty()) break;
                int k = param("ksize", 3).toInt();
                if (k < 1) k = 1;
                cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(k, k));
                QString opStr = param("op", "Close").toString();
                int op = cv::MORPH_CLOSE;
                if (opStr == "Open") op = cv::MORPH_OPEN;
                else if (opStr == "Dilate") op = cv::MORPH_DILATE;
                else if (opStr == "Erode") op = cv::MORPH_ERODE;
                cv::morphologyEx(inputMat, m_outputMat, op, element);
                m_statusMessage = QString("形态学 %1 (k=%2)").arg(opStr).arg(k);
                break;
            }

            case VisionNodeType::ContourInspection: {
                if (inputMat.empty()) break;
                cv::Mat gray;
                if (inputMat.channels() == 3) {
                    cv::cvtColor(inputMat, gray, cv::COLOR_BGR2GRAY);
                } else {
                    gray = inputMat.clone();
                }

                cv::Mat bin;
                if (gray.type() != CV_8UC1) {
                    gray.convertTo(bin, CV_8UC1);
                } else {
                    bin = gray;
                }

                std::vector<std::vector<cv::Point>> contours;
                cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

                if (inputMat.channels() == 1) {
                    cv::cvtColor(inputMat, m_outputMat, cv::COLOR_GRAY2BGR);
                } else {
                    m_outputMat = inputMat.clone();
                }

                double minA = param("minArea", 80).toDouble();
                double maxA = param("maxArea", 80000).toDouble();
                int validCount = 0;
                for (size_t i = 0; i < contours.size(); ++i) {
                    double area = cv::contourArea(contours[i]);
                    if (area >= minA && area <= maxA) {
                        validCount++;
                        cv::Rect rect = cv::boundingRect(contours[i]);
                        cv::rectangle(m_outputMat, rect, cv::Scalar(16, 185, 129), 2);
                        cv::putText(m_outputMat, QString("A:%1").arg(static_cast<int>(area)).toStdString(),
                                    cv::Point(rect.x, std::max(15, rect.y - 4)),
                                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(56, 189, 248), 1);
                    }
                }
                m_statusMessage = QString("质检检出 %1 处目标").arg(validCount);
                break;
            }

            case VisionNodeType::QRCodeDetector: {
                if (inputMat.empty()) break;
                if (inputMat.channels() == 1) {
                    cv::cvtColor(inputMat, m_outputMat, cv::COLOR_GRAY2BGR);
                } else {
                    m_outputMat = inputMat.clone();
                }

                cv::QRCodeDetector detector;
                std::vector<cv::Point2f> points;
                std::string data = detector.detectAndDecode(m_outputMat, points);
                if (!points.empty()) {
                    for (size_t i = 0; i < 4; ++i) {
                        cv::line(m_outputMat, points[i], points[(i + 1) % 4], cv::Scalar(0, 255, 0), 3);
                    }
                    if (!data.empty()) {
                        cv::putText(m_outputMat, "QR: " + data, cv::Point(points[0].x, std::max(25.f, points[0].y - 8)),
                                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 200, 0), 2);
                        m_statusMessage = "解码: " + QString::fromStdString(data);
                    } else {
                        m_statusMessage = "已定位二维码角点";
                    }
                } else {
                    m_statusMessage = "未发现二维码";
                }
                break;
            }
        }
    } catch (...) {
        m_isSuccess = false;
        m_statusMessage = "执行异常";
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    m_latencyMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    updateThumbnail();
    update();
}

void VisionNodeItem::updateThumbnail() {
    if (m_outputMat.empty()) {
        m_thumbnail = QImage();
        return;
    }

    cv::Mat small;
    int tw = 156;
    int th = 70;
    cv::resize(m_outputMat, small, cv::Size(tw, th), 0, 0, cv::INTER_LINEAR);

    if (small.channels() == 1) {
        m_thumbnail = QImage(small.data, small.cols, small.rows, static_cast<int>(small.step), QImage::Format_Grayscale8).copy();
    } else if (small.channels() == 3) {
        cv::Mat rgb;
        cv::cvtColor(small, rgb, cv::COLOR_BGR2RGB);
        m_thumbnail = QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    } else if (small.channels() == 4) {
        m_thumbnail = QImage(small.data, small.cols, small.rows, static_cast<int>(small.step), QImage::Format_ARGB32).copy();
    }
}

QRectF VisionNodeItem::boundingRect() const {
    qreal margin = SOCKET_RADIUS + 4.0;
    return QRectF(-margin, -margin, WIDTH + 2 * margin, HEIGHT + 2 * margin);
}

void VisionNodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    bool selected = isSelected();

    // 1. 卡片主体阴影与圆角底色
    QRectF bodyRect(0, 0, WIDTH, HEIGHT);
    painter->setPen(QPen(selected ? QColor("#38bdf8") : QColor("#334155"), selected ? 2.5 : 1.2));
    painter->setBrush(QColor("#1e293b"));
    painter->drawRoundedRect(bodyRect, 8, 8);

    // 2. 顶部标题栏
    QRectF headerRect(0, 0, WIDTH, 30);
    QPainterPath headerPath;
    headerPath.moveTo(0, 8);
    headerPath.quadTo(0, 0, 8, 0);
    headerPath.lineTo(WIDTH - 8, 0);
    headerPath.quadTo(WIDTH, 0, WIDTH, 8);
    headerPath.lineTo(WIDTH, 30);
    headerPath.lineTo(0, 30);
    headerPath.closeSubpath();

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_headerColor);
    painter->drawPath(headerPath);

    // 标题文字与图标
    painter->setPen(QColor("#ffffff"));
    painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
    QString titleText = QString("%1 %2").arg(m_icon, m_title);
    painter->drawText(headerRect.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, titleText);

    // 3. 中间缩略图预览窗口
    QRectF thumbRect(12, 36, 156, 72);
    painter->setPen(QPen(QColor("#475569"), 1));
    painter->setBrush(QColor("#0f172a"));
    painter->drawRoundedRect(thumbRect, 4, 4);

    if (!m_thumbnail.isNull()) {
        painter->drawImage(thumbRect.adjusted(1, 1, -1, -1), m_thumbnail);
    } else {
        painter->setPen(QColor("#64748b"));
        painter->setFont(QFont("Segoe UI", 8));
        painter->drawText(thumbRect, Qt::AlignCenter, "待执行...");
    }

    // 4. 底部状态与耗时
    QRectF footerRect(10, 112, WIDTH - 20, 20);
    painter->setFont(QFont("Segoe UI", 8));
    painter->setPen(m_isSuccess ? QColor("#94a3b8") : QColor("#ef4444"));
    QString footerText = QString("%1 | %2ms").arg(m_statusMessage).arg(m_latencyMs, 0, 'f', 1);
    painter->drawText(footerRect, Qt::AlignVCenter | Qt::AlignLeft, footerText);

    // 5. 左右引脚端口绘制 (Sockets)
    // 左输入端口
    if (hasInputSocket()) {
        QPointF inPin(0, HEIGHT * 0.5);
        painter->setPen(QPen(QColor("#38bdf8"), 2));
        painter->setBrush(m_incomingWires.isEmpty() ? QColor("#0f172a") : QColor("#38bdf8"));
        painter->drawEllipse(inPin, SOCKET_RADIUS, SOCKET_RADIUS);
    }

    // 右输出端口
    QPointF outPin(WIDTH, HEIGHT * 0.5);
    painter->setPen(QPen(QColor("#10b981"), 2));
    painter->setBrush(m_outgoingWires.isEmpty() ? QColor("#0f172a") : QColor("#10b981"));
    painter->drawEllipse(outPin, SOCKET_RADIUS, SOCKET_RADIUS);
}

QVariant VisionNodeItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged) {
        for (auto *wire : m_incomingWires) wire->updatePath();
        for (auto *wire : m_outgoingWires) wire->updatePath();
    }
    return QGraphicsItem::itemChange(change, value);
}
