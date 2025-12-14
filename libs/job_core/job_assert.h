#pragma once
#include <cstdio>
#include <cstdlib>

#if !defined(JOB_ASSERT)
#if defined(NDEBUG)
#define JOB_ASSERT(expr) \
    do { \
        if( ! ( expr ) ) std::abort ( ) ; } \
    while ( 0 )
#else
#define JOB_ASSERT(expr) do { \
if( ! ( expr ) ) { \
        JOB_LOG_ERROR( "[JOB_ASSERT] {}:{}: {}", __FILE__, __LINE__, #expr) ; \
        std::abort ( ); \
} \
} while( 0 )
#endif
#endif
