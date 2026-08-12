#pragma once
#include <QObject>
#include <QtMath>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <property-macros.h>

#include "qsdbaseparam.h"
#include "qmlsd_export.h"

class QMLSD_EXPORT QSdPulidParams : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(QString, idEmbeddingPath, ""    ) // Path to a precomputed PuLID identity-embedding file. PuLID itself is enabled separately via the loaded PuLID weights model.
    QP_RW(float,   idWeight,        1.0f  ) // Blend strength of the identity embedding on the generated output
    QML_ELEMENT

public:
    explicit QSdPulidParams(QObject *parent = nullptr);
    ~QSdPulidParams();

    sd_pulid_params_t pulidParams();
    void setPulidParams(const sd_pulid_params_t &other);
    void resetPulidParams();

private:
    QByteArray tmp_idEmbeddingPath = "";
    sd_pulid_params_t m_pulid_params = {nullptr, 1.0f};
};