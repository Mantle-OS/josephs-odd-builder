#ifndef QSDSLGPARAMS_H
#define QSDSLGPARAMS_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include "qsdbaseparam.h"
class QSdSlgParams : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(qsizetype,    layerCount,     0       )
    QP_RW(float,        layerStart,     0.01f   )
    QP_RW(float,        layerEnd,       0.2f    )
    QP_RW(float,        scale,          0.f     )
    QML_ELEMENT
public:
    explicit QSdSlgParams(QObject *parent = nullptr);
    ~QSdSlgParams();

    sd_slg_params_t slgParams() const;
    void setSlgParams(sd_slg_params_t other);
    void resetSlgParams();

private:
    sd_slg_params_t     m_slgParams{ nullptr, 0,  0.01f, 0.2f, 0.f };
    int                 *m_layers = nullptr;
// QP_RW(int *layers READ layers WRITE setLayers NOTIFY layersChanged FINAL)
};

#endif // QSDSLGPARAMS_H