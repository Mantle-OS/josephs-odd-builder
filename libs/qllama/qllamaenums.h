#ifndef QLLAMAENUMS_H
#define QLLAMAENUMS_H

#include <QObject>
#include <QQmlEngine>

#include <llama.h>


// from utils
#include <property-macros.h>
#include <pointer-macros.h>

#include <qmlstringlist.h>
#include "qllama_export.h"
class QLLAMA_EXPORT QLlamaEnums : public QObject
{
    Q_OBJECT
    // 1)  create the pointer mappings as a stringList    
    QP_PTR_RO(QmlStringList, vocabTypesList       ) // Human-readable llama vocabulary implementations for selection and inspection.
    QP_PTR_RO(QmlStringList, ropeTypesList        ) // Human-readable rotary-position embedding layouts supported by llama.cpp.
    QP_PTR_RO(QmlStringList, tokenTypesList       ) // Human-readable vocabulary token classifications.
    QP_PTR_RO(QmlStringList, tokenAttrsList       ) // Human-readable token attribute flags; individual values may be combined.
    QP_PTR_RO(QmlStringList, fileTypesList        ) // Human-readable model tensor and quantization file types.
    QP_PTR_RO(QmlStringList, ropeScalingTypesList ) // Human-readable RoPE scaling strategies.
    QP_PTR_RO(QmlStringList, poolingTypesList     ) // Human-readable embedding pooling strategies.
    QP_PTR_RO(QmlStringList, attentionTypesList   ) // Human-readable causal and non-causal attention modes.
    QP_PTR_RO(QmlStringList, flashAttnTypesList   ) // Human-readable Flash Attention selection modes.
    QP_PTR_RO(QmlStringList, splitModesList       ) // Human-readable multi-device model split strategies.
    QP_PTR_RO(QmlStringList, contextTypesList     ) // Human-readable context execution types.
    QP_PTR_RO(QmlStringList, kvOverrideTypesList  ) // Human-readable metadata override value types.
    QP_PTR_RO(QmlStringList, modelMetaKeysList    ) // Human-readable model metadata keys used for sampler defaults.
    QP_PTR_RO(QmlStringList, vocabPreTypesList    ) // Human-readable vocabulary pre-tokenizer implementations.
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit QLlamaEnums(QObject *parent = nullptr);

    ~QLlamaEnums();
    void fillAll();
    void clearAll();

    enum QLlamaVocabType {
        QLlamaVocabTypeNone   = static_cast<int>(LLAMA_VOCAB_TYPE_NONE),
        QLlamaVocabTypeSPM    = static_cast<int>(LLAMA_VOCAB_TYPE_SPM),
        QLlamaVocabTypeBPE    = static_cast<int>(LLAMA_VOCAB_TYPE_BPE),
        QLlamaVocabTypeWPM    = static_cast<int>(LLAMA_VOCAB_TYPE_WPM),
        QLlamaVocabTypeUGM    = static_cast<int>(LLAMA_VOCAB_TYPE_UGM),
        QLlamaVocabTypeRWKV   = static_cast<int>(LLAMA_VOCAB_TYPE_RWKV),
        QLlamaVocabTypePLAMO2 = static_cast<int>(LLAMA_VOCAB_TYPE_PLAMO2)
    };

    Q_ENUM(QLlamaVocabType)
    static QLlamaEnums::QLlamaVocabType qLlamaVocabType(enum llama_vocab_type type);
    static enum llama_vocab_type llamaVocabType(QLlamaEnums::QLlamaVocabType type);
    // ========

    enum QLlamaRopeType {
        QLlamaRopeTypeNone   = static_cast<int>(LLAMA_ROPE_TYPE_NONE),
        QLlamaRopeTypeNorm   = static_cast<int>(LLAMA_ROPE_TYPE_NORM),
        QLlamaRopeTypeNeoX   = static_cast<int>(LLAMA_ROPE_TYPE_NEOX),
        QLlamaRopeTypeMRope  = static_cast<int>(LLAMA_ROPE_TYPE_MROPE),
        QLlamaRopeTypeIMRope = static_cast<int>(LLAMA_ROPE_TYPE_IMROPE),
        QLlamaRopeTypeVision = static_cast<int>(LLAMA_ROPE_TYPE_VISION)
    };
    Q_ENUM(QLlamaRopeType)
    static QLlamaRopeType qLlamaRopeType(enum llama_rope_type type);
    static enum llama_rope_type llamaRopeType(QLlamaRopeType type);
    // --------


    enum QLlamaTokenType {
        QLlamaTokenTypeUndefined   = static_cast<int>(LLAMA_TOKEN_TYPE_UNDEFINED),
        QLlamaTokenTypeNormal      = static_cast<int>(LLAMA_TOKEN_TYPE_NORMAL),
        QLlamaTokenTypeUnknown     = static_cast<int>(LLAMA_TOKEN_TYPE_UNKNOWN),
        QLlamaTokenTypeControl     = static_cast<int>(LLAMA_TOKEN_TYPE_CONTROL),
        QLlamaTokenTypeUserDefined = static_cast<int>(LLAMA_TOKEN_TYPE_USER_DEFINED),
        QLlamaTokenTypeUnused      = static_cast<int>(LLAMA_TOKEN_TYPE_UNUSED),
        QLlamaTokenTypeByte        = static_cast<int>(LLAMA_TOKEN_TYPE_BYTE)
    };
    Q_ENUM(QLlamaTokenType)
    static QLlamaTokenType qLlamaTokenType(enum llama_token_type type);
    static enum llama_token_type llamaTokenType(QLlamaTokenType type);
    // =============

    enum QLlamaTokenAttr {
        QLlamaTokenAttrUndefined   = LLAMA_TOKEN_ATTR_UNDEFINED,
        QLlamaTokenAttrUnknown     = LLAMA_TOKEN_ATTR_UNKNOWN,
        QLlamaTokenAttrUnused      = LLAMA_TOKEN_ATTR_UNUSED,
        QLlamaTokenAttrNormal      = LLAMA_TOKEN_ATTR_NORMAL,
        QLlamaTokenAttrControl     = LLAMA_TOKEN_ATTR_CONTROL,
        QLlamaTokenAttrUserDefined = LLAMA_TOKEN_ATTR_USER_DEFINED,
        QLlamaTokenAttrByte        = LLAMA_TOKEN_ATTR_BYTE,
        QLlamaTokenAttrNormalized  = LLAMA_TOKEN_ATTR_NORMALIZED,
        QLlamaTokenAttrLStrip      = LLAMA_TOKEN_ATTR_LSTRIP,
        QLlamaTokenAttrRStrip      = LLAMA_TOKEN_ATTR_RSTRIP,
        QLlamaTokenAttrSingleWord  = LLAMA_TOKEN_ATTR_SINGLE_WORD
    };
    Q_DECLARE_FLAGS(QLlamaTokenAttrs, QLlamaTokenAttr)
    Q_FLAG(QLlamaTokenAttrs)
    static QLlamaTokenAttrs qLlamaTokenAttrs(int nativeAttr);
    static int nativeTokenAttr(QLlamaTokenAttrs attrs);
    // =============

    enum QLlamaFType {
        QLlamaFTypeAllF32       = static_cast<int>(LLAMA_FTYPE_ALL_F32),
        QLlamaFTypeMostlyF16    = static_cast<int>(LLAMA_FTYPE_MOSTLY_F16),
        QLlamaFTypeMostlyQ4_0   = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q4_0),
        QLlamaFTypeMostlyQ4_1   = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q4_1),
        QLlamaFTypeMostlyQ8_0   = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q8_0),
        QLlamaFTypeMostlyQ5_0   = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q5_0),
        QLlamaFTypeMostlyQ5_1   = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q5_1),
        QLlamaFTypeMostlyQ2_K   = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q2_K),
        QLlamaFTypeMostlyQ3_K_S = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q3_K_S),
        QLlamaFTypeMostlyQ3_K_M = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q3_K_M),
        QLlamaFTypeMostlyQ3_K_L = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q3_K_L),
        QLlamaFTypeMostlyQ4_K_S = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q4_K_S),
        QLlamaFTypeMostlyQ4_K_M = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q4_K_M),
        QLlamaFTypeMostlyQ5_K_S = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q5_K_S),
        QLlamaFTypeMostlyQ5_K_M = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q5_K_M),
        QLlamaFTypeMostlyQ6_K   = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q6_K),
        QLlamaFTypeMostlyIQ2_XXS= static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ2_XXS),
        QLlamaFTypeMostlyIQ2_XS = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ2_XS),
        QLlamaFTypeMostlyQ2_K_S = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q2_K_S),
        QLlamaFTypeMostlyIQ3_XS = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ3_XS),
        QLlamaFTypeMostlyIQ3_XXS= static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ3_XXS),
        QLlamaFTypeMostlyIQ1_S  = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ1_S),
        QLlamaFTypeMostlyIQ4_NL = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ4_NL),
        QLlamaFTypeMostlyIQ3_S  = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ3_S),
        QLlamaFTypeMostlyIQ3_M  = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ3_M),
        QLlamaFTypeMostlyIQ2_S  = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ2_S),
        QLlamaFTypeMostlyIQ2_M  = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ2_M),
        QLlamaFTypeMostlyIQ4_XS = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ4_XS),
        QLlamaFTypeMostlyIQ1_M  = static_cast<int>(LLAMA_FTYPE_MOSTLY_IQ1_M),
        QLlamaFTypeMostlyBF16   = static_cast<int>(LLAMA_FTYPE_MOSTLY_BF16),
        QLlamaFTypeMostlyTQ1_0  = static_cast<int>(LLAMA_FTYPE_MOSTLY_TQ1_0),
        QLlamaFTypeMostlyTQ2_0  = static_cast<int>(LLAMA_FTYPE_MOSTLY_TQ2_0),
        QLlamaFTypeMostlyMxfp4Moe=static_cast<int>(LLAMA_FTYPE_MOSTLY_MXFP4_MOE),
        QLlamaFTypeMostlyNvfp4  = static_cast<int>(LLAMA_FTYPE_MOSTLY_NVFP4),
        QLlamaFTypeMostlyQ1_0   = static_cast<int>(LLAMA_FTYPE_MOSTLY_Q1_0),
        QLlamaFTypeGuessed      = static_cast<int>(LLAMA_FTYPE_GUESSED)
    };
    Q_ENUM(QLlamaFType)
    static QLlamaFType qLlamaFType(enum llama_ftype type);
    static enum llama_ftype llamaFType(QLlamaFType type);
    // ===========================


    enum QLlamaRopeScalingType {
        QLlamaRopeScalingTypeUnspecified = static_cast<int>(LLAMA_ROPE_SCALING_TYPE_UNSPECIFIED),
        QLlamaRopeScalingTypeNone        = static_cast<int>(LLAMA_ROPE_SCALING_TYPE_NONE),
        QLlamaRopeScalingTypeLinear      = static_cast<int>(LLAMA_ROPE_SCALING_TYPE_LINEAR),
        QLlamaRopeScalingTypeYaRN        = static_cast<int>(LLAMA_ROPE_SCALING_TYPE_YARN),
        QLlamaRopeScalingTypeLongRoPE    = static_cast<int>(LLAMA_ROPE_SCALING_TYPE_LONGROPE)
    };
    Q_ENUM(QLlamaRopeScalingType)
    static QLlamaRopeScalingType qLlamaRopeScalingType(enum llama_rope_scaling_type type);
    static enum llama_rope_scaling_type llamaRopeScalingType(QLlamaRopeScalingType type);
    // ===========================

    enum QLlamaPoolingType {
        QLlamaPoolingTypeUnspecified = static_cast<int>(LLAMA_POOLING_TYPE_UNSPECIFIED),
        QLlamaPoolingTypeNone        = static_cast<int>(LLAMA_POOLING_TYPE_NONE),
        QLlamaPoolingTypeMean        = static_cast<int>(LLAMA_POOLING_TYPE_MEAN),
        QLlamaPoolingTypeCls         = static_cast<int>(LLAMA_POOLING_TYPE_CLS),
        QLlamaPoolingTypeLast        = static_cast<int>(LLAMA_POOLING_TYPE_LAST),
        QLlamaPoolingTypeRank        = static_cast<int>(LLAMA_POOLING_TYPE_RANK)
    };
    Q_ENUM(QLlamaPoolingType)
    static QLlamaPoolingType qLlamaPoolingType(enum llama_pooling_type type);
    static enum llama_pooling_type llamaPoolingType(QLlamaPoolingType type);
    // ===========================


    enum QLlamaAttentionType {
        QLlamaAttentionTypeUnspecified = static_cast<int>(LLAMA_ATTENTION_TYPE_UNSPECIFIED),
        QLlamaAttentionTypeCausal      = static_cast<int>(LLAMA_ATTENTION_TYPE_CAUSAL),
        QLlamaAttentionTypeNonCausal   = static_cast<int>(LLAMA_ATTENTION_TYPE_NON_CAUSAL)
    };
    Q_ENUM(QLlamaAttentionType)
    static QLlamaAttentionType qLlamaAttentionType(enum llama_attention_type type);
    static enum llama_attention_type llamaAttentionType(QLlamaAttentionType type);
    // ===========================

    enum QLlamaFlashAttnType {
        QLlamaFlashAttnTypeAuto     = static_cast<int>(LLAMA_FLASH_ATTN_TYPE_AUTO),
        QLlamaFlashAttnTypeDisabled = static_cast<int>(LLAMA_FLASH_ATTN_TYPE_DISABLED),
        QLlamaFlashAttnTypeEnabled  = static_cast<int>(LLAMA_FLASH_ATTN_TYPE_ENABLED)
    };
    Q_ENUM(QLlamaFlashAttnType)
    static QLlamaFlashAttnType qLlamaFlashAttnType(enum llama_flash_attn_type type);
    static enum llama_flash_attn_type llamaFlashAttnType(QLlamaFlashAttnType type);
    // ===========================

    enum QLlamaSplitMode {
        QLlamaSplitModeNone   = static_cast<int>(LLAMA_SPLIT_MODE_NONE),
        QLlamaSplitModeLayer  = static_cast<int>(LLAMA_SPLIT_MODE_LAYER),
        QLlamaSplitModeRow    = static_cast<int>(LLAMA_SPLIT_MODE_ROW),
        QLlamaSplitModeTensor = static_cast<int>(LLAMA_SPLIT_MODE_TENSOR)
    };
    Q_ENUM(QLlamaSplitMode)
    static QLlamaSplitMode qLlamaSplitMode(enum llama_split_mode mode);
    static enum llama_split_mode llamaSplitMode(QLlamaSplitMode mode);
    // ===========================

    enum QLlamaContextType {
        QLlamaContextTypeDefault = static_cast<int>(LLAMA_CONTEXT_TYPE_DEFAULT),
        QLlamaContextTypeMTP     = static_cast<int>(LLAMA_CONTEXT_TYPE_MTP)
    };
    Q_ENUM(QLlamaContextType)
    static  QLlamaContextType qLlamaContextType(enum llama_context_type type);
    static  enum llama_context_type llamaContextType(QLlamaContextType type);
    // ===========================

    enum QLlamaKvOverrideType {
        QLlamaKvOverrideTypeInt   = static_cast<int>(LLAMA_KV_OVERRIDE_TYPE_INT),
        QLlamaKvOverrideTypeFloat = static_cast<int>(LLAMA_KV_OVERRIDE_TYPE_FLOAT),
        QLlamaKvOverrideTypeBool  = static_cast<int>(LLAMA_KV_OVERRIDE_TYPE_BOOL),
        QLlamaKvOverrideTypeStr   = static_cast<int>(LLAMA_KV_OVERRIDE_TYPE_STR)
    };
    Q_ENUM(QLlamaKvOverrideType)
    static QLlamaKvOverrideType qLlamaKvOverrideType(enum llama_model_kv_override_type type);
    static enum llama_model_kv_override_type llamaKvOverrideType(QLlamaKvOverrideType type);
    // ===========================


    enum QLlamaModelMetaKey {
        QLlamaModelMetaKeySamplingSequence     = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_SEQUENCE),
        QLlamaModelMetaKeySamplingTopK         = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_TOP_K),
        QLlamaModelMetaKeySamplingTopP         = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_TOP_P),
        QLlamaModelMetaKeySamplingMinP         = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_MIN_P),
        QLlamaModelMetaKeySamplingXtcProbability = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_XTC_PROBABILITY),
        QLlamaModelMetaKeySamplingXtcThreshold   = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_XTC_THRESHOLD),
        QLlamaModelMetaKeySamplingTemp         = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_TEMP),
        QLlamaModelMetaKeySamplingPenaltyLastN = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_PENALTY_LAST_N),
        QLlamaModelMetaKeySamplingPenaltyRepeat = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_PENALTY_REPEAT),
        QLlamaModelMetaKeySamplingMirostat     = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_MIROSTAT),
        QLlamaModelMetaKeySamplingMirostatTau  = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_MIROSTAT_TAU),
        QLlamaModelMetaKeySamplingMirostatEta  = static_cast<int>(LLAMA_MODEL_META_KEY_SAMPLING_MIROSTAT_ETA)
    };
    Q_ENUM(QLlamaModelMetaKey)
    static QLlamaModelMetaKey qLlamaModelMetaKey(enum llama_model_meta_key key);
    static enum llama_model_meta_key llamaModelMetaKey(QLlamaModelMetaKey key);
    // --------------------------

    enum QLlamaVocabPreType {
        QLlamaVocabPreTypeDefault         = 0,
        QLlamaVocabPreTypeLlama3          = 1,
        QLlamaVocabPreTypeDeepseekLlm     = 2,
        QLlamaVocabPreTypeDeepseekCoder   = 3,
        QLlamaVocabPreTypeFalcon          = 4,
        QLlamaVocabPreTypeMpt             = 5,
        QLlamaVocabPreTypeStarcoder       = 6,
        QLlamaVocabPreTypeGpt2            = 7,
        QLlamaVocabPreTypeRefact          = 8,
        QLlamaVocabPreTypeCommandR        = 9,
        QLlamaVocabPreTypeStablelm2       = 10,
        QLlamaVocabPreTypeQwen2           = 11,
        QLlamaVocabPreTypeOlmo            = 12,
        QLlamaVocabPreTypeDbrx            = 13,
        QLlamaVocabPreTypeSmaug           = 14,
        QLlamaVocabPreTypePoro            = 15,
        QLlamaVocabPreTypeChatglm3        = 16,
        QLlamaVocabPreTypeChatglm4        = 17,
        QLlamaVocabPreTypeViking          = 18,
        QLlamaVocabPreTypeJais            = 19,
        QLlamaVocabPreTypeTekken          = 20,
        QLlamaVocabPreTypeSmollm          = 21,
        QLlamaVocabPreTypeCodeshell       = 22,
        QLlamaVocabPreTypeBloom           = 23,
        QLlamaVocabPreTypeGpt3Finnish     = 24,
        QLlamaVocabPreTypeExaone          = 25,
        QLlamaVocabPreTypeChameleon       = 26,
        QLlamaVocabPreTypeMinerva         = 27,
        QLlamaVocabPreTypeDeepseek3Llm    = 28,
        QLlamaVocabPreTypeGpt4o           = 29,
        QLlamaVocabPreTypeSuperbpe        = 30,
        QLlamaVocabPreTypeTrillion        = 31,
        QLlamaVocabPreTypeBailingmoe      = 32,
        QLlamaVocabPreTypeLlama4          = 33,
        QLlamaVocabPreTypePixtral         = 34,
        QLlamaVocabPreTypeSeedCoder       = 35,
        QLlamaVocabPreTypeHunyuan         = 36,
        QLlamaVocabPreTypeKimiK2          = 37,
        QLlamaVocabPreTypeHunyuanDense    = 38,
        QLlamaVocabPreTypeGrok2           = 39,
        QLlamaVocabPreTypeGraniteDocling  = 40,
        QLlamaVocabPreTypeMinimaxM2       = 41,
        QLlamaVocabPreTypeAfmoe           = 42,
        QLlamaVocabPreTypeSolarOpen       = 43,
        QLlamaVocabPreTypeYoutu           = 44,
        QLlamaVocabPreTypeExaoneMoe       = 45,
        QLlamaVocabPreTypeQwen35          = 46,
        QLlamaVocabPreTypeTinyAya         = 47,
        QLlamaVocabPreTypeJoyaiLlm        = 48,
        QLlamaVocabPreTypeJais2           = 49,
        QLlamaVocabPreTypeGemma4          = 50,
        QLlamaVocabPreTypeSarvamMoe       = 51,
    };
    Q_ENUM(QLlamaVocabPreType)
    static QLlamaVocabPreType qLlamaVocabPreType(int type) {  return static_cast<QLlamaVocabPreType>(type);  }
    static int llamaVocabPreType(QLlamaVocabPreType type)  {  return static_cast<int>(type);  }



private:
    QMap<QString, QLlamaVocabType> m_vocabTypeMap;
    void fillVocabTypes();

    QMap<QString, QLlamaRopeType> m_ropeTypeMap;
    void fillRopeTypes();

    QMap<QString, QLlamaTokenType> m_tokenTypeMap;
    void fillTokenTypes();

    QMap<QString, QLlamaTokenAttr> m_tokenAttrMap;
    void fillTokenAttrs();

    QMap<QString, QLlamaFType> m_fileTypeMap;
    void fillFileTypes();

    QMap<QString, QLlamaRopeScalingType> m_ropeScalingTypeMap;
    void fillRopeScalingTypes();

    QMap<QString, QLlamaPoolingType> m_poolingTypeMap;
    void fillPoolingTypes();

    QMap<QString, QLlamaAttentionType> m_attentionTypeMap;
    void fillAttentionTypes();

    QMap<QString, QLlamaFlashAttnType> m_flashAttnTypeMap;
    void fillFlashAttnTypes();

    QMap<QString, QLlamaSplitMode> m_splitModeMap;
    void fillSplitModes();

    QMap<QString, QLlamaContextType> m_contextTypeMap;
    void fillContextTypes();

    QMap<QString, QLlamaKvOverrideType> m_kvOverrideTypeMap;
    void fillKvOverrideTypes();

    QMap<QString, QLlamaModelMetaKey> m_modelMetaKeyMap;
    void fillModelMetaKeys();

    QMap<QString, QLlamaVocabPreType> m_vocabPreTypeMap;
    void fillVocabPreTypes() {
        m_vocabPreTypeMap = {
            { "Default / Native", QLlamaVocabPreTypeDefault },
            { "LLaMA 3",          QLlamaVocabPreTypeLlama3 },
            { "DeepSeek LLM",     QLlamaVocabPreTypeDeepseekLlm },
            { "DeepSeek Coder",   QLlamaVocabPreTypeDeepseekCoder },
            { "Falcon",           QLlamaVocabPreTypeFalcon },
            { "MPT",              QLlamaVocabPreTypeMpt },
            { "StarCoder",        QLlamaVocabPreTypeStarcoder },
            { "GPT-2",            QLlamaVocabPreTypeGpt2 },
            { "Refact",           QLlamaVocabPreTypeRefact },
            { "Command R",        QLlamaVocabPreTypeCommandR },
            { "StableLM 2",       QLlamaVocabPreTypeStablelm2 },
            { "Qwen 2",           QLlamaVocabPreTypeQwen2 },
            { "OLMo",             QLlamaVocabPreTypeOlmo },
            { "DBRX",             QLlamaVocabPreTypeDbrx },
            { "Smaug",            QLlamaVocabPreTypeSmaug },
            { "Poro",             QLlamaVocabPreTypePoro },
            { "ChatGLM 3",        QLlamaVocabPreTypeChatglm3 },
            { "ChatGLM 4",        QLlamaVocabPreTypeChatglm4 },
            { "Viking",           QLlamaVocabPreTypeViking },
            { "JAIS",             QLlamaVocabPreTypeJais },
            { "Tekken",           QLlamaVocabPreTypeTekken },
            { "SmolLM",           QLlamaVocabPreTypeSmollm },
            { "CodeShell",        QLlamaVocabPreTypeCodeshell },
            { "BLOOM",            QLlamaVocabPreTypeBloom },
            { "GPT-3 Finnish",    QLlamaVocabPreTypeGpt3Finnish },
            { "EXAONE",           QLlamaVocabPreTypeExaone },
            { "Chameleon",        QLlamaVocabPreTypeChameleon },
            { "Minerva",          QLlamaVocabPreTypeMinerva },
            { "DeepSeek V3 / R1", QLlamaVocabPreTypeDeepseek3Llm },
            { "GPT-4o",           QLlamaVocabPreTypeGpt4o },
            { "SuperBPE",         QLlamaVocabPreTypeSuperbpe },
            { "Trillion",         QLlamaVocabPreTypeTrillion },
            { "Bailing MoE",      QLlamaVocabPreTypeBailingmoe },
            { "LLaMA 4",          QLlamaVocabPreTypeLlama4 },
            { "Pixtral",          QLlamaVocabPreTypePixtral },
            { "Seed Coder",       QLlamaVocabPreTypeSeedCoder },
            { "Hunyuan",          QLlamaVocabPreTypeHunyuan },
            { "Kimi K2",          QLlamaVocabPreTypeKimiK2 },
            { "Hunyuan Dense",    QLlamaVocabPreTypeHunyuanDense },
            { "Grok 2",           QLlamaVocabPreTypeGrok2 },
            { "Granite Docling",  QLlamaVocabPreTypeGraniteDocling },
            { "MiniMax M2",       QLlamaVocabPreTypeMinimaxM2 },
            { "AFMoE",            QLlamaVocabPreTypeAfmoe },
            { "Solar Open",       QLlamaVocabPreTypeSolarOpen },
            { "YouTu",            QLlamaVocabPreTypeYoutu },
            { "EXAONE MoE",       QLlamaVocabPreTypeExaoneMoe },
            { "Qwen 3.5",         QLlamaVocabPreTypeQwen35 },
            { "Tiny Aya",         QLlamaVocabPreTypeTinyAya },
            { "JoyAI LLM",        QLlamaVocabPreTypeJoyaiLlm },
            { "JAIS 2",           QLlamaVocabPreTypeJais2 },
            { "Gemma 4",          QLlamaVocabPreTypeGemma4 },
            { "Sarvam MoE",       QLlamaVocabPreTypeSarvamMoe }
        };
        for (auto it = m_vocabPreTypeMap.constBegin(); it != m_vocabPreTypeMap.constEnd(); ++it)
            m_vocabPreTypesList->append(it.key());
    }
};

#endif // QLLAMAENUMS_H
