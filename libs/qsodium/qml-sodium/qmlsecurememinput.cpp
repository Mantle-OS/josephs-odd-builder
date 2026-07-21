#include "qmlsecurememinput.h"

#include <algorithm>

//  m_memory->mem()->() (64)
// Pre-allocates QmlSecureMem ->  via QSecureMem{64} -> JobSecureMem
QmlSecureMemInput::QmlSecureMemInput(QQuickItem *parent) :
    QQuickItem{parent},
     m_memory{new QmlSecureMem{this}}
{
    setImplicitWidth(240);
    setImplicitHeight(32);
    setFlag(ItemHasContents, true);
    setFlag(ItemIsFocusScope, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setActiveFocusOnTab(true);
}

QmlSecureMemInput::~QmlSecureMemInput()
{
    secureWipe();
}

int QmlSecureMemInput::length() const noexcept
{
    return m_maskCount;
}

QColor QmlSecureMemInput::borderColor() const noexcept
{
    return m_borderColor;
}

void QmlSecureMemInput::setBorderColor(const QColor &color)
{
    if (m_borderColor != color) {
        m_borderColor = color;
        Q_EMIT borderColorChanged();
        update();
    }
}

void QmlSecureMemInput::setMaskColor(const QColor &color) {
    if (m_maskColor != color) {
        m_maskColor = color;
        Q_EMIT maskColorChanged();
        update();
    }
}

void QmlSecureMemInput::secureWipe() noexcept
{
    bool rearmed = false;

    if ( m_memory &&  m_memory->mem()) {
         m_memory->mem()->clear();
        rearmed =  m_memory->mem()->allocate(kSecureInputCapacity);
    }

    m_byteCount = 0;
    m_maskCount = 0;
    m_inputByteLengths.clear();

    Q_EMIT lengthChanged();
    Q_EMIT secureWipeExecuted();
    update();

    if (!rearmed){
        // LOG: QmlSecureMemInput failed to re-arm secure buffer after wipe.
    }
}

void QmlSecureMemInput::focusInEvent(QFocusEvent *event)
{
    update();
    QQuickItem::focusInEvent(event);
}

void QmlSecureMemInput::focusOutEvent(QFocusEvent *event)
{
    update();
    QQuickItem::focusOutEvent(event);
}

void QmlSecureMemInput::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        Q_EMIT  returnPressed();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Backspace) {
        if (!m_inputByteLengths.empty()) {
            uint8_t const bytesToRemove = m_inputByteLengths.back();
            m_inputByteLengths.pop_back();

            m_byteCount -= bytesToRemove;
            --m_maskCount;

            // Zero out memory slice "safely"
            sodium_memzero(m_memory->mem()->data() + m_byteCount, bytesToRemove);

            Q_EMIT lengthChanged();
            update();
        }

        event->accept();
        return;
    }

    QString const text = event->text();
    if (!text.isEmpty()) {
        QByteArray const rawBytes = text.toUtf8();

        if (rawBytes.isEmpty()) {
            event->accept();
            return;
        }

        if (rawBytes.size() > std::numeric_limits<uint8_t>::max()) {
            event->accept();
            return;
        }

        // Check bounds
        if (static_cast<size_t>(m_byteCount + rawBytes.size()) <= static_cast<size_t>( m_memory->mem()->size())) {
            std::memcpy(
                 m_memory->mem()->data() + m_byteCount,
                rawBytes.constData(),
                static_cast<size_t>(rawBytes.size())
                );

            m_byteCount += rawBytes.size();
            ++m_maskCount;
            m_inputByteLengths.push_back(static_cast<uint8_t>(rawBytes.size()));

            Q_EMIT lengthChanged();
            update();
        }

        event->accept();
        return;
    }

    QQuickItem::keyPressEvent(event);
}

void QmlSecureMemInput::mousePressEvent(QMouseEvent *event)
{
    forceActiveFocus();
    event->accept();
}

QColor QmlSecureMemInput::maskColor() const noexcept
{
    return m_maskColor;
}

QSGNode *QmlSecureMemInput::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{

    if (!isVisible() || width() <= 0.0 || height() <= 0.0) {
        delete oldNode;
        return nullptr;
    }

    QSGNode *rootNode = oldNode;

    if (!rootNode)
        rootNode = new QSGNode();

    QSGGeometryNode *borderNode = nullptr;

    if (rootNode->childCount() == 0) {
        borderNode = new QSGGeometryNode();
        auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 5);
        geometry->setDrawingMode(QSGGeometry::DrawLineStrip);

        borderNode->setGeometry(geometry);
        borderNode->setFlag(QSGNode::OwnsGeometry);

        auto *material = new QSGFlatColorMaterial();
        borderNode->setMaterial(material);
        borderNode->setFlag(QSGNode::OwnsMaterial);

        rootNode->appendChildNode(borderNode);
    } else {
        borderNode = static_cast<QSGGeometryNode *>(rootNode->childAtIndex(0));
    }

    auto *borderGeom = borderNode->geometry();
    borderGeom->setLineWidth(hasActiveFocus() ? 2.0f : 1.0f);

    float const w = static_cast<float>(width());
    float const h = static_cast<float>(height());

    QSGGeometry::Point2D *borderVertices = borderGeom->vertexDataAsPoint2D();

    borderVertices[0].set(0.0f, 0.0f);
    borderVertices[1].set(w, 0.0f);
    borderVertices[2].set(w, h);
    borderVertices[3].set(0.0f, h);
    borderVertices[4].set(0.0f, 0.0f);

    borderGeom->markVertexDataDirty();

    auto *borderMat = static_cast<QSGFlatColorMaterial *>(borderNode->material());
    borderMat->setColor(m_borderColor);

    borderNode->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

    QSGGeometryNode *dotsNode = nullptr;

    if (rootNode->childCount() == 1) {
        dotsNode = new QSGGeometryNode();

        auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);

        dotsNode->setGeometry(geometry);
        dotsNode->setFlag(QSGNode::OwnsGeometry);

        auto *material = new QSGFlatColorMaterial();
        dotsNode->setMaterial(material);
        dotsNode->setFlag(QSGNode::OwnsMaterial);

        rootNode->appendChildNode(dotsNode);
    } else {
        dotsNode = static_cast<QSGGeometryNode *>(rootNode->childAtIndex(1));
    }

    auto *dotsGeom = dotsNode->geometry();

    float const dotSize = 10.0f;
    float const spacing = 16.0f;
    float const startX  = 15.0f;
    float const centerY = h / 2.0f;

    int const visibleCount = std::max(0, std::min(m_maskCount, static_cast<int>((w - startX) / spacing)));

    if (visibleCount == 0) {
        dotsGeom->allocate(0);
    } else {
        int const totalVertices = visibleCount * 6;
        dotsGeom->allocate(totalVertices);
        dotsGeom->setDrawingMode(QSGGeometry::DrawTriangles);

        QSGGeometry::Point2D *dotsVertices = dotsGeom->vertexDataAsPoint2D();

        int vIdx = 0;

        for (int i = 0; i < visibleCount; ++i) {
            float const x = startX + (static_cast<float>(i) * spacing);
            float const y = centerY - (dotSize / 2.0f);

            dotsVertices[vIdx++].set(x, y);
            dotsVertices[vIdx++].set(x + dotSize, y);
            dotsVertices[vIdx++].set(x, y + dotSize);

            dotsVertices[vIdx++].set(x + dotSize, y);
            dotsVertices[vIdx++].set(x + dotSize, y + dotSize);
            dotsVertices[vIdx++].set(x, y + dotSize);
        }
    }

    dotsGeom->markVertexDataDirty();

    auto *dotsMat = static_cast<QSGFlatColorMaterial *>(dotsNode->material());
    dotsMat->setColor(m_maskColor);

    dotsNode->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

    return rootNode;
}