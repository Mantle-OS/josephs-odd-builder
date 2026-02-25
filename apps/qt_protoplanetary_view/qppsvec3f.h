#ifndef QPPSVEC3F_H
#define QPPSVEC3F_H

#include <QObject>
#include <QQmlEngine>
#include "property-macros.h"
#include <data/vec3f.h>
class QPPSVec3f : public QObject
{
    Q_OBJECT
    QP_RO(qreal, posX, 0.0f)
    QP_RO(qreal, posY, 0.0f)
    QP_RO(qreal, posZ, 0.0f)
    QML_ELEMENT
    QML_UNCREATABLE("Use QPPSEngine.ParticleModel.get(INDEX).Vec")
public:
    explicit QPPSVec3f(QObject *parent = nullptr) : QObject{parent} {}
    void updateFromPDL(const job::science::data::Vec3f &in)
    {
        m_vec = in;
        set_posX(static_cast<qreal>(m_vec.x));
        set_posY(static_cast<qreal>(m_vec.y));
        set_posZ(static_cast<qreal>(m_vec.z));
    }
    job::science::data::Vec3f vec() const { return m_vec; }
private:
    job::science::data::Vec3f m_vec;
};

#endif // QPPSVEC3F_H
