#ifndef QPPSENGINE_H
#define QPPSENGINE_H

#include <QObject>
#include <QQmlEngine>
#include <QTimer>

#include <engine/engine.h>

#include "pointer-macros.h"
#include "qppsdiskmodel.h"
#include "qppszones.h"
#include "qppsparticle.h"
#include  "objectmodel.h"


class QPPSEngine : public QObject
{
    Q_OBJECT
    QP_MODEL_RO(QPPSParticle, particles)
    QP_PTR_RO(QPPSDiskModel, disk)
    QP_PTR_RO(QPPSZones, zones)
    QP_RO(bool, running, false)
    QP_RO(qreal, heartbeat, 1000.00f)
    QP_RO(bool, auto_start, false)
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit QPPSEngine(QObject *parent = nullptr) :
        QObject{parent},
        m_particles{new ObjectListModel<QPPSParticle>{this , "", "idx"}},
        m_disk{new QPPSDiskModel{this}},
        m_zones{new QPPSZones{this}},
        m_timer{new QTimer{this}}
    {

        auto cfg = job::science::engine::DefaultEngineConfig(job::threads::SchedulerType::WorkStealing);
        const size_t initialCount = 1024;
        m_engine = new job::science::engine::Engine(m_disk->disk(), m_zones->zones(), cfg, initialCount);

        for (size_t i = 0; i < initialCount; i++)
            m_particles->append(new QPPSParticle{this});
        connect(m_timer, &QTimer::timeout, this, [this](){
            // const float dt_s = static_cast<float>(m_engine->timeStep());
            // m_engine->step(dt_s);
            if (!m_engine)
                return;
            for (int i = 0; i < get_particles()->size(); i++)
                m_particles->at(i)->updateFromPDL(m_engine->particles()[i]); // Just get the partical at and update it
            set_running(m_engine->running());
        });

        m_timer->setInterval(static_cast<int>(m_heartbeat));
        if(m_auto_start)
            m_timer->start();

        // m_engine->setTimeStep(static_cast<float>(m_heartbeat / 1000.0f));
        m_disk->updateFromPDL(m_engine->disk());
        m_zones->updateFromPDL(m_engine->zones());
    }

    ~QPPSEngine()
    {
        if(m_disk){
            delete m_disk;
            m_disk = nullptr;
        }

        if(m_zones){
            delete m_zones;
            m_zones = nullptr;
        }

        // Clear the MVC thus deleteling all its children
        if(!m_particles->isEmpty())
            m_particles->clear();
        delete m_particles;
        m_particles = nullptr;

        if(m_timer && m_timer->isActive()){
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }

        if(m_engine){
            if(m_engine->running()){
                stop();
            }
            delete m_engine;
            m_engine = nullptr;
        }
    }

    Q_INVOKABLE void start()
    {
        if(m_engine) {
            m_engine->start();
            set_running(true);
            if(m_timer)
                m_timer->start();
        }
    }
    Q_INVOKABLE void stop()
    {
        if(m_engine) {
            m_engine->stop();
            set_running(false);
            if(m_timer && m_timer->isActive())
                m_timer->stop();
        }
    }

    Q_INVOKABLE bool play() const
    {
        if(m_engine)
            return m_engine->play();
        return false;
    }
    Q_INVOKABLE bool pause() const
    {
        if(m_engine)
            return m_engine->pause();
        return false;
    }

    Q_INVOKABLE void reset() const
    {
        if(m_engine)
            m_engine->reset();
    }

private:
    job::science::engine::Engine *m_engine = nullptr;
    QTimer *m_timer = nullptr;
};

#endif // QPPSENGINE_H
