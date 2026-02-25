#ifndef POINTER_MACROS_H
#define POINTER_MACROS_H
#include <QObject>

#define QP_PTR_RW(type, name) \
    protected: \
    Q_PROPERTY (type * name READ get_##name  WRITE set_##name NOTIFY name##Changed) \
    private: \
    type * m_##name = nullptr; \
    public: \
    type * get_##name  (void) const { return m_##name ; } \
    public Q_SLOTS: \
    void set_##name (type * name) { \
        if (m_##name != name) { \
            m_##name = name; \
            Q_EMIT name##Changed (m_##name); \
        } \
    } \
    Q_SIGNALS: \
    void name##Changed (type * name); \
    private:

#define QP_PTR_RO(type, name) \
    protected: \
    Q_PROPERTY (type * name READ get_##name  NOTIFY name##Changed) \
    private: \
    type * m_##name = nullptr ; \
    public: \
    type * get_##name  (void) const { return m_##name ; } \
    void set_##name (type * name) { \
        if (m_##name != name) { \
            m_##name = name; \
            Q_EMIT name##Changed (m_##name); \
        } \
    } \
    Q_SIGNALS: \
    void name##Changed (type * name); \
    private:

#define QP_PTR_CONST(type, name) \
    protected: \
    Q_PROPERTY (type * name READ get_##name  CONSTANT) \
    private: \
    type * m_##name = nullptr; \
    public: \
    type * get_##name  (void) const { return m_##name ; } \
    private:

#endif // POINTER-MACROS_H
