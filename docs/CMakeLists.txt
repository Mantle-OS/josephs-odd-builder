add_custom_target(JOB_Documentation SOURCES
    ../README.md
    ../LICENSE.txt
    ../COMMERCIAL.txt
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
    job/usb_overview.md
    job/zstd_overview.md
    job/sound_overview.md
    job/job_token.md
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

add_custom_target(JOBGGML_Documentation SOURCES
    job_ggml/overview.md
    job_ggml/devices.md
    job_ggml/impl_devices.md
    job_ggml/backend.md
    job_ggml/context.md
    job_ggml/tensors.md
    job_ggml/operations.md
    job_ggml/gguf.md
    job_ggml/optimizer.md
    job_ggml/quant.md
    job_ggml/misc.md  # this is enums & type_traits and abort callback threadpool stuff;
)

add_custom_target(JOBSCHEMA_Documentation SOURCES
    job_schema/design/intro.md
    job_schema/design/job_schema_design.md
    job_schema/design/job_schema_gen_design.md
    job_schema/design/job_schema_gen_example_design.md
    job_schema/design/test_job_schema_design.md
    job_schema/design/job_schema_design_deps.md
)
