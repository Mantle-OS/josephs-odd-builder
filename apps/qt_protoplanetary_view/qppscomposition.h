#ifndef QPPSCOMPOSITION_H
#define QPPSCOMPOSITION_H

#include <data/composition.h>

#include <QObject>
#include <QQmlEngine>

#include "property-macros.h"
class QPPSComposition : public QObject
{
    Q_OBJECT
    QP_RO(qreal, silicate, 0.0f)
    QP_RO(qreal, metal, 0.0f)
    QP_RO(qreal, ice, 0.0f )
    QP_RO(qreal, carbon, 0.0f)
    QP_RO(qreal, density, 0.0f)
    QP_RO(qreal, heatCapacity, 0.0f)
    QP_RO(qreal, emissivity, 0.0f)
    QP_RO(qreal, absorptivity, 0.0f)
    QML_ELEMENT
    QML_UNCREATABLE("......")
public:
    explicit QPPSComposition(QObject *parent = nullptr):
        QObject{parent}
    {
    }
    enum MaterialType { Silicate = 0, Metallic, Icy, Carbonaceous, Sulfidic, Oxidized, Mixed };
    Q_ENUMS(MaterialType)

    job::science::data::Composition comp() const { return m_comp; }
    void updateFromPDL(job::science::data::Composition in)
    {
        m_comp = in;
        set_silicate(static_cast<qreal>(m_comp.silicate));
        set_metal(static_cast<qreal>(m_comp.metal));
        set_ice(static_cast<qreal>(m_comp.ice));
        set_carbon(static_cast<qreal>(m_comp.carbon));
        set_density(static_cast<qreal>(m_comp.density));
        set_heatCapacity(static_cast<qreal>(m_comp.heatCapacity));
        set_emissivity(static_cast<qreal>(m_comp.emissivity));
        set_absorptivity(static_cast<qreal>(m_comp.absorptivity));
        set_materialType(static_cast<MaterialType>(m_comp.type));
    }
private:
    QP_RO(MaterialType, materialType, MaterialType::Mixed)    
    job::science::data::Composition m_comp;
};

#endif // QPPSCOMPOSITION_H
