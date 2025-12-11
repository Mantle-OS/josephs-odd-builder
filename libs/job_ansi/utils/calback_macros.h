#define API_CALLBACK_CONST(type, name, defaultValue) \
private: \
    type m_##name = defaultValue ; \
    public: \
    std::function < void ( const type & name ) > on_##name##Changed ; \
    type get_##name ( void ) const { return m_##name ; } \
    void set_##name ( const type & name ) \
{ \
        if ( m_##name != name ){ \
            m_##name = name; \
            if ( on_##name##Changed ) \
            on_##name##Changed ( m_##name ) ; \
    } \
} \
    private:

#define API_CALLBACK(type, name, defaultValue) \
              private: \
    type m_##name = defaultValue ; \
    public: \
    std::function < void ( type  name ) > on_##name##Changed ; \
    type get_##name ( void ) const { return m_##name ; } \
    void set_##name ( type name ) \
{ \
        if ( m_##name != name ){ \
            m_##name = name; \
            if ( on_##name##Changed ) \
            on_##name##Changed ( m_##name ) ; \
    } \
} \
    private:


