add_custom_target(JOB_Documentation SOURCES
    ../README.md
    coding_style.md
    job/job.md
    job/ansi_overview.md
    job/core_overview.md
    job/crypto_overview.md
    job/io_overview.md
    job/net_overview.md
    job/science_overview.md
    job/simd_overview.md
    job/serializer_overview.md
    job/threading_overview.md
    job/tui_overview.md
    job/uart_overview.md
)

add_custom_target(JOB_AI_Documentation SOURCES
    job/ai.md
    job/ai_benchmarks.md
    job/ai/cords.md
    job/ai/comp.md
    job/ai/evo.md
    job/ai/layers.md
    job/ai/router.md
    job/ai/moe.md
    job/ai/adapter.md
    job/ai/coach.md
    job/ai/infer.md
    job/ai/learn.md
    job/ai/token.md
    job/ai/cache.md
)

add_custom_target(QTAI_Documentation SOURCES
    qtai/qtai.md
    qtai/qt-ai-utils.md
    qtai/qml-ai-utils.md
    qtai/qsodium.md
    qtai/qml-sodium.md
    qtai/qzstd.md
    qtai/qml-zstd.md
    qtai/qhuggingface.md
    qtai/qllama.md
    qtai/qstable-diffusion.md
    #
    qtai/qai-schema.md # Working on main landing page for generated schema code
    qtai/qai-schema/ailedger.md # Done
    qtai/qai-schema/aipkg.md    # done
    qtai/qai-schema/aisession.md #DONE

    # qtai/qsession.md
    # qtai/qaipkg.md
)