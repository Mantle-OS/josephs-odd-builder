cmake_minimum_required(VERSION 3.16)
project(job-inference-test
    VERSION 0.1
    DESCRIPTION "Ask Qwen3 4b it the for a fast sqrt in c++"
    LANGUAGES CXX
)

# Hard code these
# jmills@mantle-os:~/git/opensource/qwen_tmp$ ls
# config.json  generation_config.json  tokenizer_config.json  tokenizer.json  vocab.json
# /srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf


add_executable(job-inference-test
    main.cpp
)

target_link_libraries(job-inference-test
    PRIVATE
        Threads::Threads
        $<BUILD_INTERFACE:ggml>
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_Ggml
        JosephsOddBuilder_Token
        JosephsOddBuilder_Model

)

install(TARGETS job-inference-test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

