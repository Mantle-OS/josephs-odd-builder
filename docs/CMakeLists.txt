add_custom_target(Documentation SOURCES
    ansi_overview.md
    coding_style.md
    core_overview.md
    crypto_overview.md
    io_overview.md
    net_overview.md
    serializer_overview.md
    threading_overview.md
    uart_overview.md
)

add_custom_target(AI_Documentation SOURCES
    ai.md               # needs rewrite after all other things are done
    ai_benchmarks.md    # done
    odd_portal.md       # done
    ai/cords.md         # done
    ai/comp.md          # done
    ai/evo.md           # done
    ai/layers.md        # done
    ai/router.md        # done
    ai/moe.md           # done
    ai/adapter.md       # done
    ai/coach.md         # done
    ai/infer.md         #
    ai/learn.md
    ai/cache.md
)

