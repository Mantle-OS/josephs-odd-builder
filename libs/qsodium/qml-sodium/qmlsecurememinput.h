#pragma once

#include <cstring>
#include <vector>

// core
#include <QChar>
#include <QByteArray>
#include <QString>
//gui
#include <QColor>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>

//qml /quick
#include <QQuickItem>
#include <QSGNode>
#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>

#include <qqmlregistration.h>

#include <pointer-macros.h>

#include <qmlsecuremem.h>
#include <qsecuremem.h>

class QmlSecureMemInput : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int length READ length NOTIFY lengthChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)
    Q_PROPERTY(QColor maskColor READ maskColor WRITE setMaskColor NOTIFY maskColorChanged)
    QP_PTR_RO(QmlSecureMem, secureBuffer)
    QML_ELEMENT
    static constexpr size_t kSecureInputCapacity = 64;

public:
    explicit QmlSecureMemInput(QQuickItem *parent = nullptr);
    ~QmlSecureMemInput() override;

    int length() const noexcept;
    QColor borderColor() const noexcept;
    void setBorderColor(const QColor &color);

    QColor maskColor() const noexcept;
    void setMaskColor(const QColor &color);

    Q_INVOKABLE void secureWipe() noexcept;

Q_SIGNALS:
    void lengthChanged();
    void borderColorChanged();
    void maskColorChanged();
    void returnPressed();
    void secureWipeExecuted(); // new

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override
    {
        forceActiveFocus();
        event->accept();
    }

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private:
    // QmlSecureMem m_secureBuffer;
    int m_byteCount = 0;
    int m_maskCount = 0;
    std::vector<uint8_t> m_inputByteLengths;

    QColor m_borderColor = QColor("#555555");
    QColor m_maskColor = QColor("#00FF66");
};
