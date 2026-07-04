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

    void focusOutEvent(QFocusEvent *event) override
    {
        update();
        QQuickItem::focusOutEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override
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
                std::memset( m_secureBuffer->mem()->data() + m_byteCount, 0, bytesToRemove );

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
            if (static_cast<size_t>(m_byteCount + rawBytes.size()) <= static_cast<size_t>(m_secureBuffer->mem()->size())) {
                std::memcpy(
                    m_secureBuffer->mem()->data() + m_byteCount,
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
