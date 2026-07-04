#include "qmlsecurememinput.h"

#include <algorithm>

QmlSecureMemInput::QmlSecureMemInput(QQuickItem *parent) :
    QQuickItem{parent},
    m_secureBuffer{new QmlSecureMem{this}}
{
    setImplicitWidth(240);
    setImplicitHeight(32);
    // m_secureBuffer->mem()->() (64) // Pre-allocates QmlSecureMem -->  via QSecureMem{64} -> JobSecureMem
    setFlag(ItemHasContents, true);
    setFlag(ItemIsFocusScope, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setActiveFocusOnTab(true);
}

QmlSecureMemInput::~QmlSecureMemInput()
{
    secureWipe();
}

QColor QmlSecureMemInput::borderColor() const noexcept noexcept
{
    return m_borderColor;
}

QSGNode *QmlSecureMemInput::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    // Add extra guards to return if impossible to draw
    // if visible, (if width < 0 && if height < 0)


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
    float const startX = 15.0f;
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