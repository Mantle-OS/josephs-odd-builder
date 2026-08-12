#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <ggml.h>
#include <ggml-backend.h>
#include <ggml-opt.h>
#include <gguf.h>
#include <ggml-cpu.h>

namespace job::ggml {

enum class JobGgmlStatus : std::int8_t {
    AllocFailed = GGML_STATUS_ALLOC_FAILED,
    Failed      = GGML_STATUS_FAILED,
    Success     = GGML_STATUS_SUCCESS,
    Aborted     = GGML_STATUS_ABORTED
};

[[nodiscard]] static constexpr JobGgmlStatus fromGgmlStatus(enum ggml_status status) noexcept
{
    return static_cast<JobGgmlStatus>(status);
}

[[nodiscard]] static constexpr enum ggml_status toGgmlStatus(JobGgmlStatus status) noexcept
{
    return static_cast<enum ggml_status>(status);
}

enum class JobGgmlType : std::uint8_t {
    F32     = GGML_TYPE_F32,
    F16     = GGML_TYPE_F16,
    Q4_0    = GGML_TYPE_Q4_0,
    Q4_1    = GGML_TYPE_Q4_1,
    Q5_0    = GGML_TYPE_Q5_0,
    Q5_1    = GGML_TYPE_Q5_1,
    Q8_0    = GGML_TYPE_Q8_0,
    Q8_1    = GGML_TYPE_Q8_1,
    Q2_K    = GGML_TYPE_Q2_K,
    Q3_K    = GGML_TYPE_Q3_K,
    Q4_K    = GGML_TYPE_Q4_K,
    Q5_K    = GGML_TYPE_Q5_K,
    Q6_K    = GGML_TYPE_Q6_K,
    Q8_K    = GGML_TYPE_Q8_K,
    IQ2_XXS = GGML_TYPE_IQ2_XXS,
    IQ2_XS  = GGML_TYPE_IQ2_XS,
    IQ3_XXS = GGML_TYPE_IQ3_XXS,
    IQ1_S   = GGML_TYPE_IQ1_S,
    IQ4_NL  = GGML_TYPE_IQ4_NL,
    IQ3_S   = GGML_TYPE_IQ3_S,
    IQ2_S   = GGML_TYPE_IQ2_S,
    IQ4_XS  = GGML_TYPE_IQ4_XS,
    I8      = GGML_TYPE_I8,
    I16     = GGML_TYPE_I16,
    I32     = GGML_TYPE_I32,
    I64     = GGML_TYPE_I64,
    F64     = GGML_TYPE_F64,
    IQ1_M   = GGML_TYPE_IQ1_M,
    BF16    = GGML_TYPE_BF16,
    TQ1_0   = GGML_TYPE_TQ1_0,
    TQ2_0   = GGML_TYPE_TQ2_0,
    MXFP4   = GGML_TYPE_MXFP4,
    NVFP4   = GGML_TYPE_NVFP4,
    Q1_0    = GGML_TYPE_Q1_0,
    Count   = GGML_TYPE_COUNT
};
[[nodiscard]] static constexpr JobGgmlType fromGgmlType(enum ggml_type type) noexcept
{
    return static_cast<JobGgmlType>(type);
}

[[nodiscard]] static constexpr enum ggml_type toGgmlType(JobGgmlType type) noexcept
{
    return static_cast<enum ggml_type>(type);
}







// We intentionally do not use only `type >= 0 && type < GGML_TYPE_COUNT`.
// A range check would automatically accept any new ggml_type inserted by
// upstream before GGML_TYPE_COUNT, even though JOB has not reviewed or mapped
// that type yet. The explicit switch makes support opt-in: new types are
// rejected until they are deliberately added, while removed or renamed types
// produce a compile-time failure at the stale case label.
// The parameter uses `enum ggml_type` rather than an integer because callers
// should pass the native upstream type directly. This preserves type intent,
// avoids accepting arbitrary integer values at the API boundary, and keeps the
// function aligned one-to-one with GGML.
[[nodiscard]] constexpr bool isValidGgmlType(enum ggml_type type) noexcept
{
    switch (type) {
    case GGML_TYPE_F32:
    case GGML_TYPE_F16:
    case GGML_TYPE_Q4_0:
    case GGML_TYPE_Q4_1:
    case GGML_TYPE_Q5_0:
    case GGML_TYPE_Q5_1:
    case GGML_TYPE_Q8_0:
    case GGML_TYPE_Q8_1:
    case GGML_TYPE_Q2_K:
    case GGML_TYPE_Q3_K:
    case GGML_TYPE_Q4_K:
    case GGML_TYPE_Q5_K:
    case GGML_TYPE_Q6_K:
    case GGML_TYPE_Q8_K:
    case GGML_TYPE_IQ2_XXS:
    case GGML_TYPE_IQ2_XS:
    case GGML_TYPE_IQ3_XXS:
    case GGML_TYPE_IQ1_S:
    case GGML_TYPE_IQ4_NL:
    case GGML_TYPE_IQ3_S:
    case GGML_TYPE_IQ2_S:
    case GGML_TYPE_IQ4_XS:
    case GGML_TYPE_I8:
    case GGML_TYPE_I16:
    case GGML_TYPE_I32:
    case GGML_TYPE_I64:
    case GGML_TYPE_F64:
    case GGML_TYPE_IQ1_M:
    case GGML_TYPE_BF16:
    case GGML_TYPE_TQ1_0:
    case GGML_TYPE_TQ2_0:
    case GGML_TYPE_MXFP4:
    case GGML_TYPE_NVFP4:
    case GGML_TYPE_Q1_0:
        return true;

    case GGML_TYPE_COUNT:
    default:
        return false;
    }
}

enum class JobGgmlPrecision : std::uint8_t {
    Default = GGML_PREC_DEFAULT,
    F32     = GGML_PREC_F32
};

enum class JobGgmlOpHint : std::uint8_t {
    None           = GGML_HINT_NONE,
    Src0IsHadamard = GGML_HINT_SRC0_IS_HADAMARD
};

enum class JobGgmlFileType : std::int16_t {
    Unknown         = GGML_FTYPE_UNKNOWN,
    AllF32          = GGML_FTYPE_ALL_F32,
    MostlyF16       = GGML_FTYPE_MOSTLY_F16,
    MostlyQ4_0      = GGML_FTYPE_MOSTLY_Q4_0,
    MostlyQ4_1      = GGML_FTYPE_MOSTLY_Q4_1,
    MostlyQ4_1F16   = GGML_FTYPE_MOSTLY_Q4_1_SOME_F16,
    MostlyQ8_0      = GGML_FTYPE_MOSTLY_Q8_0,
    MostlyQ5_0      = GGML_FTYPE_MOSTLY_Q5_0,
    MostlyQ5_1      = GGML_FTYPE_MOSTLY_Q5_1,
    MostlyQ2_K      = GGML_FTYPE_MOSTLY_Q2_K,
    MostlyQ3_K      = GGML_FTYPE_MOSTLY_Q3_K,
    MostlyQ4_K      = GGML_FTYPE_MOSTLY_Q4_K,
    MostlyQ5_K      = GGML_FTYPE_MOSTLY_Q5_K,
    MostlyQ6_K      = GGML_FTYPE_MOSTLY_Q6_K,
    MostlyIQ2_XXS   = GGML_FTYPE_MOSTLY_IQ2_XXS,
    MostlyIQ2_XS    = GGML_FTYPE_MOSTLY_IQ2_XS,
    MostlyIQ3_XXS   = GGML_FTYPE_MOSTLY_IQ3_XXS,
    MostlyIQ1_S     = GGML_FTYPE_MOSTLY_IQ1_S,
    MostlyIQ4_NL    = GGML_FTYPE_MOSTLY_IQ4_NL,
    MostlyIQ3_S     = GGML_FTYPE_MOSTLY_IQ3_S,
    MostlyIQ2_S     = GGML_FTYPE_MOSTLY_IQ2_S,
    MostlyIQ4_XS    = GGML_FTYPE_MOSTLY_IQ4_XS,
    MostlyIQ1_M     = GGML_FTYPE_MOSTLY_IQ1_M,
    MostlyBF16      = GGML_FTYPE_MOSTLY_BF16,
    MostlyMXFP4     = GGML_FTYPE_MOSTLY_MXFP4,
    MostlyNVFP4     = GGML_FTYPE_MOSTLY_NVFP4,
    MostlyQ1_0      = GGML_FTYPE_MOSTLY_Q1_0
};


enum class JobGgmlOp : std::uint16_t {
    None                 = GGML_OP_NONE,

    Dup                  = GGML_OP_DUP,
    Add                  = GGML_OP_ADD,
    AddId                = GGML_OP_ADD_ID,
    Add1                 = GGML_OP_ADD1,
    Acc                  = GGML_OP_ACC,
    Sub                  = GGML_OP_SUB,
    Mul                  = GGML_OP_MUL,
    Div                  = GGML_OP_DIV,
    Sqr                  = GGML_OP_SQR,
    Sqrt                 = GGML_OP_SQRT,
    Log                  = GGML_OP_LOG,
    Sin                  = GGML_OP_SIN,
    Cos                  = GGML_OP_COS,
    Sum                  = GGML_OP_SUM,
    SumRows              = GGML_OP_SUM_ROWS,
    CumSum               = GGML_OP_CUMSUM,
    Mean                 = GGML_OP_MEAN,
    ArgMax               = GGML_OP_ARGMAX,
    CountEqual           = GGML_OP_COUNT_EQUAL,
    Repeat               = GGML_OP_REPEAT,
    RepeatBack           = GGML_OP_REPEAT_BACK,
    Concat               = GGML_OP_CONCAT,
    SiluBack             = GGML_OP_SILU_BACK,
    Norm                 = GGML_OP_NORM,
    RmsNorm              = GGML_OP_RMS_NORM,
    RmsNormBack          = GGML_OP_RMS_NORM_BACK,
    GroupNorm            = GGML_OP_GROUP_NORM,
    L2Norm               = GGML_OP_L2_NORM,

    MulMat               = GGML_OP_MUL_MAT,
    MulMatId             = GGML_OP_MUL_MAT_ID,
    OutProd              = GGML_OP_OUT_PROD,

    Scale                = GGML_OP_SCALE,
    Set                  = GGML_OP_SET,
    Copy                 = GGML_OP_CPY,
    Contiguous           = GGML_OP_CONT,
    Reshape              = GGML_OP_RESHAPE,
    View                 = GGML_OP_VIEW,
    Permute              = GGML_OP_PERMUTE,
    Transpose            = GGML_OP_TRANSPOSE,
    GetRows              = GGML_OP_GET_ROWS,
    GetRowsBack          = GGML_OP_GET_ROWS_BACK,
    SetRows              = GGML_OP_SET_ROWS,
    Diag                 = GGML_OP_DIAG,
    DiagMaskInf          = GGML_OP_DIAG_MASK_INF,
    DiagMaskZero         = GGML_OP_DIAG_MASK_ZERO,
    SoftMax              = GGML_OP_SOFT_MAX,
    SoftMaxBack          = GGML_OP_SOFT_MAX_BACK,
    Rope                 = GGML_OP_ROPE,
    RopeBack             = GGML_OP_ROPE_BACK,
    Clamp                = GGML_OP_CLAMP,
    ConvTranspose1d      = GGML_OP_CONV_TRANSPOSE_1D,
    Im2Col               = GGML_OP_IM2COL,
    Im2ColBack           = GGML_OP_IM2COL_BACK,
    Im2Col3d             = GGML_OP_IM2COL_3D,
    Col2Im1d             = GGML_OP_COL2IM_1D,
    Conv2d               = GGML_OP_CONV_2D,
    Conv3d               = GGML_OP_CONV_3D,
    Conv2dDepthwise      = GGML_OP_CONV_2D_DW,
    ConvTranspose2d      = GGML_OP_CONV_TRANSPOSE_2D,
    Pool1d               = GGML_OP_POOL_1D,
    Pool2d               = GGML_OP_POOL_2D,
    Pool2dBack           = GGML_OP_POOL_2D_BACK,
    Upscale              = GGML_OP_UPSCALE,
    Pad                  = GGML_OP_PAD,
    PadReflect1d         = GGML_OP_PAD_REFLECT_1D,
    Roll                 = GGML_OP_ROLL,
    Arange               = GGML_OP_ARANGE,
    TimestepEmbedding    = GGML_OP_TIMESTEP_EMBEDDING,
    ArgSort              = GGML_OP_ARGSORT,
    TopK                 = GGML_OP_TOP_K,
    LeakyRelu            = GGML_OP_LEAKY_RELU,
    Tri                  = GGML_OP_TRI,
    Fill                 = GGML_OP_FILL,

    FlashAttentionExt    = GGML_OP_FLASH_ATTN_EXT,
    FlashAttentionBack   = GGML_OP_FLASH_ATTN_BACK,
    SsmConv              = GGML_OP_SSM_CONV,
    SsmScan              = GGML_OP_SSM_SCAN,
    WindowPartition      = GGML_OP_WIN_PART,
    WindowUnpartition    = GGML_OP_WIN_UNPART,
    GetRelativePosition  = GGML_OP_GET_REL_POS,
    AddRelativePosition  = GGML_OP_ADD_REL_POS,
    RwkvWkv6             = GGML_OP_RWKV_WKV6,
    GatedLinearAttention = GGML_OP_GATED_LINEAR_ATTN,
    RwkvWkv7             = GGML_OP_RWKV_WKV7,
    SolveTriangular      = GGML_OP_SOLVE_TRI,
    GatedDeltaNet        = GGML_OP_GATED_DELTA_NET,

    Unary                = GGML_OP_UNARY,

    MapCustom1           = GGML_OP_MAP_CUSTOM1,
    MapCustom2           = GGML_OP_MAP_CUSTOM2,
    MapCustom3           = GGML_OP_MAP_CUSTOM3,

    Custom               = GGML_OP_CUSTOM,

    CrossEntropyLoss     = GGML_OP_CROSS_ENTROPY_LOSS,
    CrossEntropyLossBack = GGML_OP_CROSS_ENTROPY_LOSS_BACK,
    OptimizerStepAdamW   = GGML_OP_OPT_STEP_ADAMW,
    OptimizerStepSgd     = GGML_OP_OPT_STEP_SGD,

    Glu                  = GGML_OP_GLU,

    Count                = GGML_OP_COUNT
};


[[nodiscard]] static constexpr JobGgmlOp fromGgmlOp(enum ggml_op op) noexcept
{
    return static_cast<JobGgmlOp>(op);
}

[[nodiscard]] static constexpr enum ggml_op toGgmlOp(JobGgmlType op) noexcept
{
    return static_cast<enum ggml_op>(op);
}








enum class JobGgmlUnaryOp : std::uint8_t {
    Abs       = GGML_UNARY_OP_ABS,
    Sign      = GGML_UNARY_OP_SGN,
    Negate    = GGML_UNARY_OP_NEG,
    Step      = GGML_UNARY_OP_STEP,
    Tanh      = GGML_UNARY_OP_TANH,
    Elu       = GGML_UNARY_OP_ELU,
    Relu      = GGML_UNARY_OP_RELU,
    Sigmoid   = GGML_UNARY_OP_SIGMOID,
    Gelu      = GGML_UNARY_OP_GELU,
    GeluQuick = GGML_UNARY_OP_GELU_QUICK,
    Silu      = GGML_UNARY_OP_SILU,
    HardSwish = GGML_UNARY_OP_HARDSWISH,
    HardSigmoid = GGML_UNARY_OP_HARDSIGMOID,
    Exp       = GGML_UNARY_OP_EXP,
    Expm1     = GGML_UNARY_OP_EXPM1,
    SoftPlus  = GGML_UNARY_OP_SOFTPLUS,
    GeluErf   = GGML_UNARY_OP_GELU_ERF,
    Xielu     = GGML_UNARY_OP_XIELU,
    Floor     = GGML_UNARY_OP_FLOOR,
    Ceil      = GGML_UNARY_OP_CEIL,
    Round     = GGML_UNARY_OP_ROUND,
    Truncate  = GGML_UNARY_OP_TRUNC,
    Count     = GGML_UNARY_OP_COUNT
};

enum class JobGgmlGluOp : std::uint8_t {
    ReGlu      = GGML_GLU_OP_REGLU,
    GeGlu      = GGML_GLU_OP_GEGLU,
    SwiGlu     = GGML_GLU_OP_SWIGLU,
    SwiGluOai  = GGML_GLU_OP_SWIGLU_OAI,
    GeGluErf   = GGML_GLU_OP_GEGLU_ERF,
    GeGluQuick = GGML_GLU_OP_GEGLU_QUICK,
    Count      = GGML_GLU_OP_COUNT
};

enum class JobGgmlObjectType : std::uint8_t {
    Tensor     = GGML_OBJECT_TYPE_TENSOR,
    Graph      = GGML_OBJECT_TYPE_GRAPH,
    WorkBuffer = GGML_OBJECT_TYPE_WORK_BUFFER
};

enum class JobGgmlLogLevel : std::uint8_t {
    None     = GGML_LOG_LEVEL_NONE,
    Debug    = GGML_LOG_LEVEL_DEBUG,
    Info     = GGML_LOG_LEVEL_INFO,
    Warning  = GGML_LOG_LEVEL_WARN,
    Error    = GGML_LOG_LEVEL_ERROR,
    Continue = GGML_LOG_LEVEL_CONT
};

enum class JobGgmlTensorFlag : std::uint32_t {
    None    = 0,
    Input   = GGML_TENSOR_FLAG_INPUT,
    Output  = GGML_TENSOR_FLAG_OUTPUT,
    Param   = GGML_TENSOR_FLAG_PARAM,
    Loss    = GGML_TENSOR_FLAG_LOSS,
    Compute = GGML_TENSOR_FLAG_COMPUTE
};

enum class JobGgmlTriType : std::uint8_t {
    UpperDiagonal = GGML_TRI_TYPE_UPPER_DIAG,
    Upper         = GGML_TRI_TYPE_UPPER,
    LowerDiagonal = GGML_TRI_TYPE_LOWER_DIAG,
    Lower         = GGML_TRI_TYPE_LOWER
};

enum class JobGgmlPoolOp : std::uint8_t {
    Max   = GGML_OP_POOL_MAX,
    Avg   = GGML_OP_POOL_AVG,
    Count = GGML_OP_POOL_COUNT
};

enum class JobGgmlScaleMode : std::uint8_t {
    Nearest  = GGML_SCALE_MODE_NEAREST,
    Bilinear = GGML_SCALE_MODE_BILINEAR,
    Bicubic  = GGML_SCALE_MODE_BICUBIC,
    Count    = GGML_SCALE_MODE_COUNT
};

enum class JobGgmlScaleFlag : std::uint32_t {
    None         = 0,
    AlignCorners = GGML_SCALE_FLAG_ALIGN_CORNERS,
    Antialias    = GGML_SCALE_FLAG_ANTIALIAS
};

enum class JobGgmlSortOrder : std::uint8_t {
    Ascending  = GGML_SORT_ORDER_ASC,
    Descending = GGML_SORT_ORDER_DESC
};

enum class JobGgmlSchedulerPriority : std::int8_t {
    Low      = GGML_SCHED_PRIO_LOW,
    Normal   = GGML_SCHED_PRIO_NORMAL,
    Medium   = GGML_SCHED_PRIO_MEDIUM,
    High     = GGML_SCHED_PRIO_HIGH,
    Realtime = GGML_SCHED_PRIO_REALTIME
};

enum class JobGgmlBackendBufferUsage : std::uint8_t {
    Any     = GGML_BACKEND_BUFFER_USAGE_ANY,
    Weights = GGML_BACKEND_BUFFER_USAGE_WEIGHTS,
    Compute = GGML_BACKEND_BUFFER_USAGE_COMPUTE
};

enum class JobGgmlDeviceType : std::uint8_t {
    Cpu   = GGML_BACKEND_DEVICE_TYPE_CPU,
    Gpu   = GGML_BACKEND_DEVICE_TYPE_GPU,
    IGpu  = GGML_BACKEND_DEVICE_TYPE_IGPU,
    Accel = GGML_BACKEND_DEVICE_TYPE_ACCEL,
    Meta  = GGML_BACKEND_DEVICE_TYPE_META
};

[[nodiscard]] static constexpr JobGgmlDeviceType fromGgmlBackendDeviceType(enum ggml_backend_dev_type devType) noexcept
{
    return static_cast<JobGgmlDeviceType>(devType);
}

[[nodiscard]] static constexpr enum ggml_backend_dev_type toGgmlBackendDeviceType(JobGgmlDeviceType devType) noexcept
{
    return static_cast<enum ggml_backend_dev_type>(devType);
}


enum class JobGgmlMetaSplitAxis : std::uint8_t {
    Axis0    = GGML_BACKEND_SPLIT_AXIS_0,
    Axis1    = GGML_BACKEND_SPLIT_AXIS_1,
    Axis2    = GGML_BACKEND_SPLIT_AXIS_2,
    Axis3    = GGML_BACKEND_SPLIT_AXIS_3,
    Mirrored = GGML_BACKEND_SPLIT_AXIS_MIRRORED,
    Partial  = GGML_BACKEND_SPLIT_AXIS_PARTIAL,
    None     = GGML_BACKEND_SPLIT_AXIS_NONE,
    Unknown  = GGML_BACKEND_SPLIT_AXIS_UNKNOWN
};

[[nodiscard]] constexpr JobGgmlTensorFlag operator|(JobGgmlTensorFlag lhs, JobGgmlTensorFlag rhs) noexcept
{
    return static_cast<JobGgmlTensorFlag>(
        static_cast<std::uint32_t>(lhs) |
        static_cast<std::uint32_t>(rhs)
        );
}

[[nodiscard]] constexpr JobGgmlTensorFlag operator&(JobGgmlTensorFlag lhs, JobGgmlTensorFlag rhs) noexcept
{
    return static_cast<JobGgmlTensorFlag>(
        static_cast<std::uint32_t>(lhs) &
        static_cast<std::uint32_t>(rhs)
        );
}

constexpr JobGgmlTensorFlag &operator|=(JobGgmlTensorFlag &lhs, JobGgmlTensorFlag rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr JobGgmlTensorFlag &operator&=(JobGgmlTensorFlag &lhs, JobGgmlTensorFlag rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

[[nodiscard]] constexpr bool hasFlag(JobGgmlTensorFlag value, JobGgmlTensorFlag flag) noexcept
{
    return (value & flag) == flag;
}

[[nodiscard]] constexpr JobGgmlScaleFlag operator|(JobGgmlScaleFlag lhs, JobGgmlScaleFlag rhs) noexcept
{
    return static_cast<JobGgmlScaleFlag>(
        static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs)
        );
}

[[nodiscard]] constexpr JobGgmlScaleFlag operator&(JobGgmlScaleFlag lhs, JobGgmlScaleFlag rhs) noexcept
{
    return static_cast<JobGgmlScaleFlag>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

constexpr JobGgmlScaleFlag &operator|=(JobGgmlScaleFlag &lhs, JobGgmlScaleFlag rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr JobGgmlScaleFlag &operator&=(JobGgmlScaleFlag &lhs, JobGgmlScaleFlag rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

[[nodiscard]] constexpr bool hasFlag(JobGgmlScaleFlag value, JobGgmlScaleFlag flag) noexcept
{
    return (value & flag) == flag;
}

// START OPT
enum class JobGgmlOptLossType : std::int32_t {
    Mean             = GGML_OPT_LOSS_TYPE_MEAN,
    Sum              = GGML_OPT_LOSS_TYPE_SUM,
    CrossEntropy     = GGML_OPT_LOSS_TYPE_CROSS_ENTROPY,
    MeanSquaredError = GGML_OPT_LOSS_TYPE_MEAN_SQUARED_ERROR
};

enum class JobGgmlOptBuildType : std::int32_t {
    Forward = GGML_OPT_BUILD_TYPE_FORWARD,
    Grad    = GGML_OPT_BUILD_TYPE_GRAD,
    Opt     = GGML_OPT_BUILD_TYPE_OPT
};

enum class JobGgmlOptOptimizerType : std::int32_t {
    AdamW = GGML_OPT_OPTIMIZER_TYPE_ADAMW,
    Sgd   = GGML_OPT_OPTIMIZER_TYPE_SGD,
    Count = GGML_OPT_OPTIMIZER_TYPE_COUNT
};

[[nodiscard]] constexpr enum ggml_opt_loss_type toGgmlOptLossType(JobGgmlOptLossType type) noexcept
{
    return static_cast<enum ggml_opt_loss_type>(type);
}

[[nodiscard]] constexpr JobGgmlOptLossType fromGgmlOptLossType(enum ggml_opt_loss_type type) noexcept
{
    return static_cast<JobGgmlOptLossType>(type);
}

[[nodiscard]] constexpr enum ggml_opt_build_type toGgmlOptBuildType(JobGgmlOptBuildType type) noexcept
{
    return static_cast<enum ggml_opt_build_type>(type);
}

[[nodiscard]] constexpr JobGgmlOptBuildType fromGgmlOptBuildType(enum ggml_opt_build_type type) noexcept
{
    return static_cast<JobGgmlOptBuildType>(type);
}

[[nodiscard]] constexpr enum ggml_opt_optimizer_type toGgmlOptOptimizerType(JobGgmlOptOptimizerType type) noexcept
{
    return static_cast<enum ggml_opt_optimizer_type>(type);
}

[[nodiscard]] constexpr JobGgmlOptOptimizerType fromGgmlOptOptimizerType(enum ggml_opt_optimizer_type type) noexcept
{
    return static_cast<JobGgmlOptOptimizerType>(type);
}


static_assert(static_cast<std::int32_t>(JobGgmlOptLossType::Mean) == GGML_OPT_LOSS_TYPE_MEAN);
static_assert(static_cast<std::int32_t>(JobGgmlOptBuildType::Opt) == GGML_OPT_BUILD_TYPE_OPT);
static_assert(static_cast<std::int32_t>(JobGgmlOptOptimizerType::Count) == GGML_OPT_OPTIMIZER_TYPE_COUNT);
// END OPT

// START GGUF
enum class JobGgufType : std::uint32_t {
    UInt8   = GGUF_TYPE_UINT8,
    Int8    = GGUF_TYPE_INT8,
    UInt16  = GGUF_TYPE_UINT16,
    Int16   = GGUF_TYPE_INT16,
    UInt32  = GGUF_TYPE_UINT32,
    Int32   = GGUF_TYPE_INT32,
    Float32 = GGUF_TYPE_FLOAT32,
    Bool    = GGUF_TYPE_BOOL,
    String  = GGUF_TYPE_STRING,
    Array   = GGUF_TYPE_ARRAY,
    UInt64  = GGUF_TYPE_UINT64,
    Int64   = GGUF_TYPE_INT64,
    Float64 = GGUF_TYPE_FLOAT64,
    Count   = GGUF_TYPE_COUNT
};

inline constexpr std::array<std::size_t, GGUF_TYPE_COUNT> JobGgufTypeSizes {
    sizeof(std::uint8_t),
    sizeof(std::int8_t),
    sizeof(std::uint16_t),
    sizeof(std::int16_t),
    sizeof(std::uint32_t),
    sizeof(std::int32_t),
    sizeof(float),
    sizeof(std::int8_t), // GGUF bool is serialized as int8_t
    0,                   // string: variable-sized
    0,                   // array: variable-sized
    sizeof(std::uint64_t),
    sizeof(std::int64_t),
    sizeof(double)
};

inline constexpr std::array<std::string_view, GGUF_TYPE_COUNT> JobGgufTypeNames {
    "u8", "i8",
    "u16", "i16",
    "u32", "i32", "f32",
    "bool", "str", "arr",
    "u64", "i64", "f64"
};

static_assert(JobGgufTypeSizes.size() == static_cast<std::size_t>(GGUF_TYPE_COUNT));
static_assert(JobGgufTypeNames.size() == static_cast<std::size_t>(GGUF_TYPE_COUNT));

[[nodiscard]] constexpr bool isValidGgufType(enum gguf_type type) noexcept
{
    const auto index = static_cast<std::int32_t>(type);
    return index >= 0 && index < GGUF_TYPE_COUNT;
}

[[nodiscard]] constexpr bool isValidGgufType(JobGgufType type) noexcept
{
    const auto index = static_cast<std::int32_t>(type);
    return index >= 0 && index < GGUF_TYPE_COUNT;
}

[[nodiscard]] constexpr std::size_t ggufTypeSize(enum gguf_type type) noexcept
{
    if (!isValidGgufType(type))
        return 0;

    return JobGgufTypeSizes[static_cast<std::size_t>(type)];
}

[[nodiscard]] constexpr std::size_t ggufTypeSize(JobGgufType type) noexcept
{
    if (!isValidGgufType(type))
        return 0;

    return JobGgufTypeSizes[static_cast<std::size_t>(type)];
}

[[nodiscard]] constexpr std::string_view ggufTypeName(enum gguf_type type) noexcept
{
    if (!isValidGgufType(type))
        return "unknown";

    return JobGgufTypeNames[static_cast<std::size_t>(type)];
}

[[nodiscard]] constexpr std::string_view ggufTypeName(JobGgufType type) noexcept
{
    if (!isValidGgufType(type))
        return "unknown";

    return JobGgufTypeNames[static_cast<std::size_t>(type)];
}

[[nodiscard]] constexpr enum gguf_type toGgufType(JobGgufType type) noexcept
{
    return static_cast<enum gguf_type>(type);
}

[[nodiscard]] constexpr JobGgufType fromGgufType(enum gguf_type type) noexcept
{
    return static_cast<JobGgufType>(type);
}
// END GGUF

// START CPU (ggml-cpu.h)
enum class JobGgmlNumaStrategy : std::uint8_t {
    Disabled   = GGML_NUMA_STRATEGY_DISABLED,
    Distribute = GGML_NUMA_STRATEGY_DISTRIBUTE,
    Isolate    = GGML_NUMA_STRATEGY_ISOLATE,
    Numactl    = GGML_NUMA_STRATEGY_NUMACTL,
    Mirror     = GGML_NUMA_STRATEGY_MIRROR
};
[[nodiscard]] constexpr enum ggml_numa_strategy toGgmlNumaStrategy(JobGgmlNumaStrategy strategy) noexcept
{
    return static_cast<enum ggml_numa_strategy>(strategy);
}

[[nodiscard]] constexpr JobGgmlNumaStrategy fromGgmlNumaStrategy(enum ggml_numa_strategy strategy) noexcept
{
    return static_cast<JobGgmlNumaStrategy>(strategy);
}

enum class JobGgmlSchedPriority : std::int8_t {
    Low      = GGML_SCHED_PRIO_LOW,
    Normal   = GGML_SCHED_PRIO_NORMAL,
    Medium   = GGML_SCHED_PRIO_MEDIUM,
    High     = GGML_SCHED_PRIO_HIGH,
    Realtime = GGML_SCHED_PRIO_REALTIME
};
[[nodiscard]] constexpr enum ggml_sched_priority toGgmlSchedPriority(JobGgmlSchedPriority priority) noexcept
{
    return static_cast<enum ggml_sched_priority>(priority);
}

[[nodiscard]] constexpr JobGgmlSchedPriority fromGgmlSchedPriority(enum ggml_sched_priority priority) noexcept
{
    return static_cast<JobGgmlSchedPriority>(priority);
}
// END CPU



// START LOCAL
enum class JobGgmlDeviceImpl : std::uint8_t {
    Fallback,
    Cpu,
    Blas,
    Cuda,
    Vulkan,
    WebGpu,
    Zdnn,
    VirtGpu,
    Metal,
    Sycl,
    OpenVino,
    OpenCl,
    Hexagon,
    ZenDnn,
    Cann,
    Rpc,

    Count
};

// What they are called in the cpp file they MUST MATCH or we have no lookup table
inline constexpr std::array<std::string_view, 16> JobGgmlDeviceImplNames{
    "Fallback",
    "CPU",
    "BLAS",
    "CUDA",
    "Vulkan",
    "WebGPU",
    "zDNN",
    "VirtGPU",
    "Metal",
    "SYCL",
    "OpenVINO",
    "OpenCL",
    "Hexagon",
    "ZenDNN",
    "CANN",
    "RPC"
};

[[nodiscard]] constexpr std::string_view deviceImplName(JobGgmlDeviceImpl impl) noexcept
{
    const auto index = static_cast<std::size_t>(impl);
    return index < JobGgmlDeviceImplNames.size() ? JobGgmlDeviceImplNames[index] : "Fallback";
}

[[nodiscard]] constexpr JobGgmlDeviceImpl deviceImplFromName(std::string_view name) noexcept
{
    if (name == "CPU")      return JobGgmlDeviceImpl::Cpu;
    if (name == "BLAS")     return JobGgmlDeviceImpl::Blas;
    if (name == "CUDA")     return JobGgmlDeviceImpl::Cuda;
    if (name == "Vulkan")   return JobGgmlDeviceImpl::Vulkan;
    if (name == "OpenCL")   return JobGgmlDeviceImpl::OpenCl;
    if (name == "Metal")    return JobGgmlDeviceImpl::Metal;
    if (name == "SYCL")     return JobGgmlDeviceImpl::Sycl;
    if (name == "OpenVINO") return JobGgmlDeviceImpl::OpenVino;
    if (name == "WebGPU")   return JobGgmlDeviceImpl::WebGpu;
    if (name == "VirtGPU")  return JobGgmlDeviceImpl::VirtGpu;
    if (name == "Hexagon")  return JobGgmlDeviceImpl::Hexagon;
    if (name == "zDNN")     return JobGgmlDeviceImpl::Zdnn;
    if (name == "ZenDNN")   return JobGgmlDeviceImpl::ZenDnn;
    if (name == "CANN")     return JobGgmlDeviceImpl::Cann;
    if (name == "RPC")      return JobGgmlDeviceImpl::Rpc;

    return JobGgmlDeviceImpl::Fallback;
}

static_assert(JobGgmlDeviceImplNames.size() == static_cast<std::size_t>(JobGgmlDeviceImpl::Count));

// Device mangers "state machine"
enum class DeviceManagerState : uint8_t
{
    Uninitialized = 0,
    Scanning,
    Ready,
    Error
};

} // namespace job::ggml