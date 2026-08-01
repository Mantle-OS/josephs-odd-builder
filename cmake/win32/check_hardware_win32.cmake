if(MSVC)
    # Enable higher warning level
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
    )

    # Frame Pointer Preservation (/Oy- disables frame pointer omission)
    check_cxx_compiler_flag("/Oy-" COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER)
    if(COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER)
        add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:/Oy->)
    endif()

    # Fast Math
    check_cxx_compiler_flag("/fp:fast" CXX_SUPPORTS_FP_FAST)
    if(CXX_SUPPORTS_FP_FAST)
        message(STATUS "Enabling fast math (/fp:fast)")
        list(APPEND JOB_CXX_FLAGS "/fp:fast")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/fp:fast>)
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
        if(JOB_AVX_512_VNNI_FLAG)
            add_compile_definitions(HAS_AVX_512_VNNI HAS_AVX_512 __AVX512VNNI__)
            list(APPEND JOB_CXX_FLAGS "/arch:AVX512")
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/arch:AVX512>)

        elseif(JOB_AVX_512_FLAG)
            add_compile_definitions(HAS_AVX_512)
            list(APPEND JOB_CXX_FLAGS "/arch:AVX512")
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/arch:AVX512>)

        elseif(JOB_AVX_VNNI_FLAG)
            add_compile_definitions(HAS_AVX_VNNI HAS_AVX_TWO __AVXVNNI__)
            list(APPEND JOB_CXX_FLAGS "/arch:AVX2")
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/arch:AVX2>)

        elseif(JOB_AVX_TWO_FLAG)
            add_compile_definitions(HAS_AVX_TWO)
            list(APPEND JOB_CXX_FLAGS "/arch:AVX2")
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/arch:AVX2>)

        elseif(JOB_AVX_FLAG)
            add_compile_definitions(HAS_AVX)
            list(APPEND JOB_CXX_FLAGS "/arch:AVX")
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/arch:AVX>)
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64")
        add_compile_definitions(HAS_NEON)
    endif()

else()
    message(FATAL_ERROR "Only MSVC is currently suppored on windows")
endif()
