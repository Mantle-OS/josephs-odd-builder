#pragma once
#include <cassert>
#include <cstdlib>
#include "job_logger.h"

#if !defined(JOB_ASSERT)
#ifdef NDEBUG
#define JOB_ASSERT(expr) \
    do { \
        if( ! ( expr ) ) \
            std::abort ( ) ; \
    } \
    while ( 0 )
#else
#define JOB_ASSERT(expr) do { \
if( ! ( expr ) ) { \
        JOB_LOG_ASSERT("[JOB_ASSERT] {}:{}: {}", __FILE__, __LINE__, #expr); \
        std::abort ( ); \
} \
} while( 0 )
#endif
#endif


#ifndef JOB_ASSUME
#if defined(__GNUC__) || defined(__clang__)
#define JOB_ASSUME(cond) do { if ( ! ( cond ) ) __builtin_unreachable ( ) ;  } while ( 0 )
#else
#define JOB_ASSUME(cond) do { ( void ) sizeof ( cond ) ; } while (0)
#endif
#endif

#ifndef JOB_UNREACHABLE
#if defined(__GNUC__) || defined(__clang__)
#define JOB_UNREACHABLE() do { __builtin_unreachable ( ) ; } while ( 0 )
#else
#define JOB_UNREACHABLE() do { std::abort ( ) ; } while ( 0 )
#endif
#endif
