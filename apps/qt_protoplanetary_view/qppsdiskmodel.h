#ifndef QPPSDISKMODEL_H
#define QPPSDISKMODEL_H

#include <QObject>
#include <property-macros.h>
#include <QQmlEngine>

#include <data/disk.h>

class QPPSDiskModel : public QObject
{
    Q_OBJECT
    QP_RO(qreal,  stellarMass,              1.0f        )
    QP_RO(qreal,  stellarLuminosity,        1.0f        )
    QP_RO(qreal,  stellarTemp,              5778.0f     )
    QP_RO(qreal,  innerRadius,              0.05f       )
    QP_RO(qreal,  outerRadius,              100.0f      )
    QP_RO(qreal,  scaleHeight,              0.05f       )
    QP_RO(qreal,  t0,                       280.0f      )
    QP_RO(qreal,  rho0,                     1e-9f       )
    QP_RO(qreal,  pressure0,                1e-3f       )
    QP_RO(qreal,  tempExponent,             -0.5f       )
    QP_RO(qreal,  densityExponent,          -2.75f      )
    QP_RO(qreal,  radiationPressureCoeff,   1.0f        )
    QML_ELEMENT
public:
    explicit QPPSDiskModel(QObject *parent = nullptr):
        QObject{parent},
        m_disk{job::science::data::DiskModel{}}
    {
    }
    ~QPPSDiskModel(){
    }
    enum DiskZone{Inner_Disk = 0, Mid_Disk, Outer_Disk };
    Q_ENUMS(DiskZone)

    void updateFromPDL(const job::science::data::DiskModel &disk)
    {
        m_disk = disk;
        set_stellarMass(static_cast<qreal>(m_disk.stellarMass));
        set_stellarLuminosity(static_cast<qreal>(m_disk.stellarLuminosity));
        set_stellarTemp(static_cast<qreal>(m_disk.stellarTemp));
        set_innerRadius(static_cast<qreal>(m_disk.innerRadius));
        set_outerRadius(static_cast<qreal>(m_disk.outerRadius));
        set_scaleHeight(static_cast<qreal>(m_disk.scaleHeight));
        set_t0(static_cast<qreal>(m_disk.T0)); // NOTE: Correcting T0 field name case
        set_rho0(static_cast<qreal>(m_disk.rho0));
        set_pressure0(static_cast<qreal>(m_disk.pressure0));
        set_tempExponent(static_cast<qreal>(m_disk.tempExponent));
        set_densityExponent(static_cast<qreal>(m_disk.densityExponent));
        set_radiationPressureCoeff(static_cast<qreal>(m_disk.radiationPressureCoeff));
    }
    job::science::data::DiskModel disk() const { return m_disk; }
private:
    job::science::data::DiskModel m_disk;
};

#endif // QPPSDISKMODEL_H
