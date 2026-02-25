#ifndef QPPSZONES_H
#define QPPSZONES_H

#include <QObject>
#include "property-macros.h"
#include <QQmlEngine>

#include <data/zones.h>

class QPPSZones : public QObject
{
    Q_OBJECT
    QP_RO(qreal, innerBoundaryAU,   0.3f    )
    QP_RO(qreal, midBoundaryAU,     5.0f    )
    QP_RO(qreal, outerBoundaryAU,   30.0f   )
    QP_RO(qreal, t_silicateSub,     1200.0f )
    QP_RO(qreal, t_iceCondense,     170.0f  )
    QP_RO(qreal, t_COCondense,      30.0f   )
    QML_ELEMENT
    QML_UNCREATABLE(".......")
public:
    explicit QPPSZones(QObject *parent = nullptr) :
        QObject{parent},
        m_zones{job::science::data::Zones{}}
    {}
    job::science::data::Zones zones() const { return m_zones; }
    void updateFromPDL(const job::science::data::Zones &in)
    {
        m_zones = in;
        set_innerBoundaryAU(static_cast<qreal>(m_zones.innerBoundaryAU));
        set_midBoundaryAU(static_cast<qreal>(m_zones.midBoundaryAU));
        set_outerBoundaryAU(static_cast<qreal>(m_zones.outerBoundaryAU));
        set_t_silicateSub(static_cast<qreal>(m_zones.T_silicateSub));
        set_t_iceCondense(static_cast<qreal>(m_zones.T_iceCondense));
        set_t_COCondense(static_cast<qreal>(m_zones.T_COCondense));
    }
private:
    job::science::data::Zones m_zones;
};
#endif // QPPSZONES_H
