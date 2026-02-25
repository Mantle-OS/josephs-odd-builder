#ifndef QPPSPARTICLE_H
#define QPPSPARTICLE_H

#include <QObject>
#include <QQmlEngine>

#include "property-macros.h"
#include "pointer-macros.h"

#include "qppsvec3f.h"
#include "qppscomposition.h"
#include "qppsdiskmodel.h"

#include <data/particle.h>
#include <data/disk.h>

class QPPSParticle : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QP_RO(quint64, idx, 0)
    QP_PTR_RO(QPPSVec3f, position)

    QP_RO(qreal, posX, 0.0)
    QP_RO(qreal, posY, 0.0)
    QP_RO(qreal, posZ, 0.0)

    QP_PTR_RO(QPPSVec3f, velocity)
    QP_RO(qreal, radius, 0.0f)
    QP_RO(qreal, mass, 0.0f)
    QP_RO(qreal, temperature , 0.0f)
    QP_PTR_RO(QPPSComposition, composition)
    QP_RO(QPPSDiskModel::DiskZone, zone, QPPSDiskModel::Inner_Disk)
    QP_RO(quint8, flags, 0)
    QML_UNCREATABLE("Please use QPPSEngine.ParticleModel")
public:
    explicit QPPSParticle(QObject *parent = nullptr):
        QObject{parent},
        m_position{new QPPSVec3f{this}},
        m_velocity{new QPPSVec3f{this}},
        m_composition{new QPPSComposition{this}}
    {}
    ~QPPSParticle()
    {
        if(m_position){
            m_position = nullptr;
        }
        delete m_position;

        if(m_velocity){
            m_velocity = nullptr;
        }
        delete m_velocity;

        if(m_composition){
            m_composition = nullptr;
        }
        delete m_composition;
    }

    job::science::data::Particle particle() const { return m_particle; }
    void updateFromPDL(const job::science::data::Particle &in)
    {
        m_particle = in;

        set_posX(static_cast<qreal>(job::science::data::DiskModelUtil::metersToAU(in.position.x)));
        set_posY(static_cast<qreal>(job::science::data::DiskModelUtil::metersToAU(in.position.y)));
        set_posZ(static_cast<qreal>(job::science::data::DiskModelUtil::metersToAU(in.position.z)));


        m_position->updateFromPDL(in.position);
        m_velocity->updateFromPDL(in.velocity);
        m_composition->updateFromPDL(in.composition);
        set_idx(in.id);
        set_radius(static_cast<qreal>(in.radius));
        set_mass(static_cast<qreal>(in.mass));
        set_temperature(static_cast<qreal>(in.temperature));
        set_flags(in.flags);
        set_zone(static_cast<QPPSDiskModel::DiskZone>(in.zone));
    }
private:
    job::science::data::Particle m_particle{};
};






#endif // QPPSPARTICLE_H
