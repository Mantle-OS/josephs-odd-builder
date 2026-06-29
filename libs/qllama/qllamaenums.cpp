#include "qllamaenums.h"
QLlamaEnums::QLlamaEnums(QObject *parent) :
    QObject{parent},
    m_vocabTypesList{new QmlStringList{this}},
    m_ropeTypesList{new QmlStringList{this}},
    m_tokenTypesList{new QmlStringList{this}},
    m_tokenAttrsList{new QmlStringList{this}},
    m_fileTypesList{new QmlStringList{this}},
    m_ropeScalingTypesList{new QmlStringList{this}},
    m_poolingTypesList{new QmlStringList{this}},
    m_attentionTypesList{new QmlStringList{this}},
    m_flashAttnTypesList{new QmlStringList{this}},
    m_splitModesList{new QmlStringList{this}},
    m_contextTypesList{new QmlStringList{this}},
    m_kvOverrideTypesList{new QmlStringList{this}},
    m_modelMetaKeysList{new QmlStringList{this}},
    m_vocabPreTypesList{new QmlStringList{this}}
{
    fillAll();
}


QLlamaEnums::~QLlamaEnums()
{
    clearAll();
}



void QLlamaEnums::fillAll(){
    fillVocabTypes();
    fillRopeTypes();
    fillTokenTypes();
    fillTokenAttrs();
    fillVocabTypes();
    fillFileTypes();
    fillRopeScalingTypes();
    fillPoolingTypes();
    fillAttentionTypes();
    fillFlashAttnTypes();
    fillSplitModes();
    fillContextTypes();
    fillKvOverrideTypes();
    fillModelMetaKeys();
    fillVocabPreTypes();
}

void QLlamaEnums::clearAll(){
    m_vocabTypeMap.clear();
    m_vocabTypesList->clear();

    m_ropeTypeMap.clear();
    m_ropeTypesList->clear();

    m_tokenTypeMap.clear();
    m_tokenTypesList->clear();

    m_tokenAttrMap.clear();
    m_tokenAttrsList->clear();

    m_fileTypeMap.clear();
    m_fileTypesList->clear();

    m_ropeScalingTypeMap.clear();
    m_ropeScalingTypesList->clear();

    m_poolingTypeMap.clear();
    m_poolingTypesList->clear();

    m_attentionTypeMap.clear();
    m_attentionTypesList->clear();

    m_flashAttnTypeMap.clear();
    m_flashAttnTypesList->clear();

    m_splitModeMap.clear();
    m_splitModesList->clear();

    m_contextTypeMap.clear();
    m_contextTypesList->clear();

    m_kvOverrideTypeMap.clear();
    m_kvOverrideTypesList->clear();

    m_modelMetaKeyMap.clear();
    m_modelMetaKeysList->clear();

    m_vocabPreTypeMap.clear();
    m_vocabPreTypesList->clear();
}


QLlamaEnums::QLlamaVocabType QLlamaEnums::qLlamaVocabType(enum llama_vocab_type type)
{
    return static_cast<QLlamaVocabType>(type);
}

enum llama_vocab_type QLlamaEnums::llamaVocabType(QLlamaEnums::QLlamaVocabType type)
{
    return static_cast<enum llama_vocab_type>(type);
}
void QLlamaEnums::fillVocabTypes() {
    m_vocabTypeMap.clear();
    m_vocabTypesList->clear();
    m_vocabTypeMap = {
        { "None",     QLlamaVocabTypeNone },
        { "BPE (Byte Fallback)", QLlamaVocabTypeSPM },
        { "BPE",      QLlamaVocabTypeBPE },
        { "WordPiece",QLlamaVocabTypeWPM },
        { "Unigram",  QLlamaVocabTypeUGM },
        { "RWKV",     QLlamaVocabTypeRWKV },
        { "PLaMo-2",  QLlamaVocabTypePLAMO2 }
    };
    for (auto it = m_vocabTypeMap.constBegin(); it != m_vocabTypeMap.constEnd(); ++it)
        m_vocabTypesList->append(it.key());
}



QLlamaEnums::QLlamaRopeType QLlamaEnums::qLlamaRopeType(enum llama_rope_type type)
{
    return static_cast<QLlamaEnums::QLlamaRopeType>(type);
}
enum llama_rope_type QLlamaEnums::llamaRopeType(QLlamaEnums::QLlamaRopeType type)
{
    return static_cast<enum llama_rope_type>(type);
}
void QLlamaEnums::fillRopeTypes() {
    m_ropeTypeMap.clear();
    m_ropeTypesList->clear();
    m_ropeTypeMap = {
        { "None",               QLlamaRopeTypeNone },
        { "Normal",             QLlamaRopeTypeNorm },
        { "NeoX (GPT-NeoX)",    QLlamaRopeTypeNeoX },
        { "MROPE (Multimodal)", QLlamaRopeTypeMRope },
        { "IMROPE (Vision)",    QLlamaRopeTypeIMRope },
        { "Vision Context",     QLlamaRopeTypeVision }
    };
    for (auto it = m_ropeTypeMap.constBegin(); it != m_ropeTypeMap.constEnd(); ++it)
        m_ropeTypesList->append(it.key());
}


QLlamaEnums::QLlamaTokenType QLlamaEnums::qLlamaTokenType(enum llama_token_type type)
{
    return static_cast<QLlamaEnums::QLlamaTokenType>(type);
}
enum llama_token_type QLlamaEnums::llamaTokenType(QLlamaEnums::QLlamaTokenType type)
{
    return static_cast<enum llama_token_type>(type);
}
void QLlamaEnums::fillTokenTypes() {
    m_tokenTypeMap.clear();
    m_tokenTypesList->clear();
    m_tokenTypeMap = {
        { "Undefined",    QLlamaTokenTypeUndefined },
        { "Normal Text",  QLlamaTokenTypeNormal },
        { "Unknown",      QLlamaTokenTypeUnknown },
        { "Control Sequence", QLlamaTokenTypeControl },
        { "User Defined", QLlamaTokenTypeUserDefined },
        { "Unused Slot",  QLlamaTokenTypeUnused },
        { "Raw UTF-8 Byte", QLlamaTokenTypeByte }
    };
    for (auto it = m_tokenTypeMap.constBegin(); it != m_tokenTypeMap.constEnd(); ++it)
        m_tokenTypesList->append(it.key());
}


QLlamaEnums::QLlamaTokenAttrs QLlamaEnums::qLlamaTokenAttrs(int nativeAttr)
{
    return QLlamaEnums::QLlamaTokenAttrs(static_cast<QLlamaEnums::QLlamaTokenAttr>(nativeAttr));
}
int QLlamaEnums::nativeTokenAttr(QLlamaEnums::QLlamaTokenAttrs attrs)
{
    return static_cast<int>(attrs);
}
void QLlamaEnums::fillTokenAttrs() {
    m_tokenAttrMap.clear();
    m_tokenAttrsList->clear();
    m_tokenAttrMap = {
        { "Undefined",        QLlamaTokenAttrUndefined },
        { "Unknown",          QLlamaTokenAttrUnknown },
        { "Unused",           QLlamaTokenAttrUnused },
        { "Normal",           QLlamaTokenAttrNormal },
        { "Control",          QLlamaTokenAttrControl },
        { "User Defined",     QLlamaTokenAttrUserDefined },
        { "Byte",             QLlamaTokenAttrByte },
        { "Normalized",       QLlamaTokenAttrNormalized },
        { "Left Strip (LSTRIP)",  QLlamaTokenAttrLStrip },
        { "Right Strip (RSTRIP)", QLlamaTokenAttrRStrip },
        { "Single Word",      QLlamaTokenAttrSingleWord }
    };
    for (auto it = m_tokenAttrMap.constBegin(); it != m_tokenAttrMap.constEnd(); ++it)
        m_tokenAttrsList->append(it.key());
}

QLlamaEnums::QLlamaFType QLlamaEnums::qLlamaFType(enum llama_ftype type)
{
    return static_cast<QLlamaEnums::QLlamaFType>(type);
}
enum llama_ftype QLlamaEnums::llamaFType(QLlamaEnums::QLlamaFType type)
{
    return static_cast<enum llama_ftype>(type);
}
void QLlamaEnums::fillFileTypes() {
    m_fileTypeMap.clear();
    m_fileTypesList->clear();
    m_fileTypeMap = {
        { "F32 (Full Precision)",          QLlamaFTypeAllF32 },
        { "F16 (Half Precision)",          QLlamaFTypeMostlyF16 },
        { "BF16 (Brain Float 16)",         QLlamaFTypeMostlyBF16 },
        { "Q4_0 (Standard 4-bit)",         QLlamaFTypeMostlyQ4_0 },
        { "Q4_1 (4-bit with bias)",        QLlamaFTypeMostlyQ4_1 },
        { "Q5_0 (5-bit legacy)",           QLlamaFTypeMostlyQ5_0 },
        { "Q5_1 (5-bit with bias)",        QLlamaFTypeMostlyQ5_1 },
        { "Q8_0 (8-bit quantized)",        QLlamaFTypeMostlyQ8_0 },
        { "Q2_K (2-bit K-Quant)",          QLlamaFTypeMostlyQ2_K },
        { "Q2_K_S (2-bit K-Quant Small)",    QLlamaFTypeMostlyQ2_K_S },
        { "Q3_K_S (3-bit K-Quant Small)",    QLlamaFTypeMostlyQ3_K_S },
        { "Q3_K_M (3-bit K-Quant Medium)",   QLlamaFTypeMostlyQ3_K_M },
        { "Q3_K_L (3-bit K-Quant Large)",    QLlamaFTypeMostlyQ3_K_L },
        { "Q4_K_S (4-bit K-Quant Small)",    QLlamaFTypeMostlyQ4_K_S },
        { "Q4_K_M (4-bit K-Quant Medium)",   QLlamaFTypeMostlyQ4_K_M },
        { "Q5_K_S (5-bit K-Quant Small)",    QLlamaFTypeMostlyQ5_K_S },
        { "Q5_K_M (5-bit K-Quant Medium)",   QLlamaFTypeMostlyQ5_K_M },
        { "Q6_K (6-bit K-Quant)",          QLlamaFTypeMostlyQ6_K },
        { "Q1_0 (1-bit legacy)",           QLlamaFTypeMostlyQ1_0 },
        { "IQ1_S (1-bit I-Matrix Small)",  QLlamaFTypeMostlyIQ1_S },
        { "IQ1_M (1-bit I-Matrix Medium)", QLlamaFTypeMostlyIQ1_M },
        { "IQ2_XXS (2-bit I-Matrix Ultra Small)", QLlamaFTypeMostlyIQ2_XXS },
        { "IQ2_XS (2-bit I-Matrix Extra Small)",  QLlamaFTypeMostlyIQ2_XS },
        { "IQ2_S (2-bit I-Matrix Small)",  QLlamaFTypeMostlyIQ2_S },
        { "IQ2_M (2-bit I-Matrix Medium)", QLlamaFTypeMostlyIQ2_M },
        { "IQ3_XXS (3-bit I-Matrix Ultra Small)", QLlamaFTypeMostlyIQ3_XXS },
        { "IQ3_XS (3-bit I-Matrix Extra Small)",  QLlamaFTypeMostlyIQ3_XS },
        { "IQ3_S (3-bit I-Matrix Small)",  QLlamaFTypeMostlyIQ3_S },
        { "IQ3_M (3-bit I-Matrix Medium)", QLlamaFTypeMostlyIQ3_M },
        { "IQ4_XS (4-bit I-Matrix Extra Small)",  QLlamaFTypeMostlyIQ4_XS },
        { "IQ4_NL (4-bit Non-Linear)",     QLlamaFTypeMostlyIQ4_NL },
        { "TQ1_0 (Ternary 1-bit)",         QLlamaFTypeMostlyTQ1_0 },
        { "TQ2_0 (Ternary 2-bit)",         QLlamaFTypeMostlyTQ2_0 },
        { "MXFP4 (OCP Microscaling MoE)",  QLlamaFTypeMostlyMxfp4Moe },
        { "NVFP4 (NVIDIA Hardware FP4)",   QLlamaFTypeMostlyNvfp4 },
        { "Guessed / Unknown Format",      QLlamaFTypeGuessed }
    };
    for (auto it = m_fileTypeMap.constBegin(); it != m_fileTypeMap.constEnd(); ++it)
        m_fileTypesList->append(it.key());
}


QLlamaEnums::QLlamaRopeScalingType QLlamaEnums::qLlamaRopeScalingType(enum llama_rope_scaling_type type)
{
    return static_cast<QLlamaEnums::QLlamaRopeScalingType>(type);
}
enum llama_rope_scaling_type QLlamaEnums::llamaRopeScalingType(QLlamaEnums::QLlamaRopeScalingType type)
{
    return static_cast<enum llama_rope_scaling_type>(type);
}
void QLlamaEnums::fillRopeScalingTypes() {
    m_ropeScalingTypeMap.clear();
    m_ropeScalingTypesList->clear();
    m_ropeScalingTypeMap = {
        { "Unspecified / Auto", QLlamaRopeScalingTypeUnspecified },
        { "None",               QLlamaRopeScalingTypeNone },
        { "Linear Scaling",     QLlamaRopeScalingTypeLinear },
        { "YaRN Engine",        QLlamaRopeScalingTypeYaRN },
        { "LongRoPE Optimizer", QLlamaRopeScalingTypeLongRoPE }
    };
    for (auto it = m_ropeScalingTypeMap.constBegin(); it != m_ropeScalingTypeMap.constEnd(); ++it)
        m_ropeScalingTypesList->append(it.key());
}

QLlamaEnums::QLlamaPoolingType QLlamaEnums::qLlamaPoolingType(enum llama_pooling_type type)
{
    return static_cast<QLlamaEnums::QLlamaPoolingType>(type);
}
enum llama_pooling_type QLlamaEnums::llamaPoolingType(QLlamaEnums::QLlamaPoolingType type)
{
    return static_cast<enum llama_pooling_type>(type);
}
void QLlamaEnums::fillPoolingTypes() {
    m_poolingTypeMap.clear();
    m_poolingTypesList->clear();
    m_poolingTypeMap = {
        { "Unspecified / Auto", QLlamaPoolingTypeUnspecified },
        { "None (No Pooling)",  QLlamaPoolingTypeNone },
        { "Mean (Average)",     QLlamaPoolingTypeMean },
        { "CLS Token Target",   QLlamaPoolingTypeCls },
        { "Last Token Target",  QLlamaPoolingTypeLast },
        { "Rank (Cross-Encoder)", QLlamaPoolingTypeRank }
    };
    for (auto it = m_poolingTypeMap.constBegin(); it != m_poolingTypeMap.constEnd(); ++it)
        m_poolingTypesList->append(it.key());
}


QLlamaEnums::QLlamaAttentionType QLlamaEnums::qLlamaAttentionType(enum llama_attention_type type)
{
    return static_cast<QLlamaEnums::QLlamaAttentionType>(type);
}
enum llama_attention_type QLlamaEnums::llamaAttentionType(QLlamaEnums::QLlamaAttentionType type)
{
    return static_cast<enum llama_attention_type>(type);
}
void QLlamaEnums::fillAttentionTypes() {
    m_attentionTypeMap.clear();
    m_attentionTypesList->clear();
    m_attentionTypeMap = {
        { "Unspecified / Auto", QLlamaAttentionTypeUnspecified },
        { "Causal (Autoregressive)", QLlamaAttentionTypeCausal },
        { "Non-Causal (Bi-directional)", QLlamaAttentionTypeNonCausal }
    };
    for (auto it = m_attentionTypeMap.constBegin(); it != m_attentionTypeMap.constEnd(); ++it)
        m_attentionTypesList->append(it.key());
}


QLlamaEnums::QLlamaFlashAttnType QLlamaEnums::qLlamaFlashAttnType(enum llama_flash_attn_type type)
{
    return static_cast<QLlamaEnums::QLlamaFlashAttnType>(type);
}
enum llama_flash_attn_type QLlamaEnums::llamaFlashAttnType(QLlamaEnums::QLlamaFlashAttnType type)
{
    return static_cast<enum llama_flash_attn_type>(type);
}
void QLlamaEnums::fillFlashAttnTypes() {
    m_flashAttnTypeMap.clear();
    m_flashAttnTypesList->clear();
    m_flashAttnTypeMap = {
        { "Auto (Hardware Detect)", QLlamaFlashAttnTypeAuto },
        { "Disabled",               QLlamaFlashAttnTypeDisabled },
        { "Enabled (Flash Attention)", QLlamaFlashAttnTypeEnabled }
    };
    for (auto it = m_flashAttnTypeMap.constBegin(); it != m_flashAttnTypeMap.constEnd(); ++it)
        m_flashAttnTypesList->append(it.key());
}



QLlamaEnums::QLlamaSplitMode QLlamaEnums::qLlamaSplitMode(enum llama_split_mode mode)
{
    return static_cast<QLlamaEnums::QLlamaSplitMode>(mode);
}
enum llama_split_mode QLlamaEnums::llamaSplitMode(QLlamaEnums::QLlamaSplitMode mode)
{
    return static_cast<enum llama_split_mode>(mode);
}
void QLlamaEnums::fillSplitModes() {
    m_splitModeMap.clear();
    m_splitModesList->clear();
    m_splitModeMap = {
        { "None (Single GPU Only)",              QLlamaSplitModeNone },
        { "Layer-wise Split (Pipeline Parallel)", QLlamaSplitModeLayer },
        { "Row-wise Split (Basic Tensor Parallel)", QLlamaSplitModeRow },
        { "Pure Tensor Parallelism (TP Matrix)",  QLlamaSplitModeTensor }
    };
    for (auto it = m_splitModeMap.constBegin(); it != m_splitModeMap.constEnd(); ++it)
        m_splitModesList->append(it.key());
}


QLlamaEnums::QLlamaContextType QLlamaEnums::qLlamaContextType(enum llama_context_type type)
{
    return static_cast<QLlamaEnums::QLlamaContextType>(type);
}
enum llama_context_type QLlamaEnums::llamaContextType(QLlamaEnums::QLlamaContextType type)
{
    return static_cast<enum llama_context_type>(type);
}
void QLlamaEnums::fillContextTypes() {
    m_contextTypeMap.clear();
    m_contextTypesList->clear();
    m_contextTypeMap = {
        { "Default (Standard)",             QLlamaContextTypeDefault },
        { "MTP (Multi-Token Prediction)",   QLlamaContextTypeMTP }
    };
    for (auto it = m_contextTypeMap.constBegin(); it != m_contextTypeMap.constEnd(); ++it)
        m_contextTypesList->append(it.key());
}


QLlamaEnums::QLlamaKvOverrideType QLlamaEnums::qLlamaKvOverrideType(enum llama_model_kv_override_type type)
{
    return static_cast<QLlamaEnums::QLlamaKvOverrideType>(type);
}
enum llama_model_kv_override_type QLlamaEnums::llamaKvOverrideType(QLlamaEnums::QLlamaKvOverrideType type)
{
    return static_cast<enum llama_model_kv_override_type>(type);
}
void QLlamaEnums::fillKvOverrideTypes() {
    m_kvOverrideTypeMap.clear();
    m_kvOverrideTypesList->clear();
    m_kvOverrideTypeMap = {
        { "Integer", QLlamaKvOverrideTypeInt },
        { "Float",   QLlamaKvOverrideTypeFloat },
        { "Boolean", QLlamaKvOverrideTypeBool },
        { "String",  QLlamaKvOverrideTypeStr }
    };
    for (auto it = m_kvOverrideTypeMap.constBegin(); it != m_kvOverrideTypeMap.constEnd(); ++it)
        m_kvOverrideTypesList->append(it.key());
}


QLlamaEnums::QLlamaModelMetaKey QLlamaEnums::qLlamaModelMetaKey(enum llama_model_meta_key key)
{
    return static_cast<QLlamaEnums::QLlamaModelMetaKey>(key);
}
enum llama_model_meta_key QLlamaEnums::llamaModelMetaKey(QLlamaEnums::QLlamaModelMetaKey key)
{
    return static_cast<enum llama_model_meta_key>(key);
}
void QLlamaEnums::fillModelMetaKeys() {
    m_modelMetaKeyMap.clear();
    m_modelMetaKeysList->clear();
    m_modelMetaKeyMap = {
        { "Sampling Sequence",       QLlamaModelMetaKeySamplingSequence },
        { "Top-K",                  QLlamaModelMetaKeySamplingTopK },
        { "Top-P",                  QLlamaModelMetaKeySamplingTopP },
        { "Min-P",                  QLlamaModelMetaKeySamplingMinP },
        { "XTC Probability",        QLlamaModelMetaKeySamplingXtcProbability },
        { "XTC Threshold",          QLlamaModelMetaKeySamplingXtcThreshold },
        { "Temperature",            QLlamaModelMetaKeySamplingTemp },
        { "Penalty Last N Tokens",  QLlamaModelMetaKeySamplingPenaltyLastN },
        { "Repeat Penalty Coeff",   QLlamaModelMetaKeySamplingPenaltyRepeat },
        { "Mirostat Toggle",        QLlamaModelMetaKeySamplingMirostat },
        { "Mirostat Tau (Target)",  QLlamaModelMetaKeySamplingMirostatTau },
        { "Mirostat Eta (Rate)",    QLlamaModelMetaKeySamplingMirostatEta }
    };
    for (auto it = m_modelMetaKeyMap.constBegin(); it != m_modelMetaKeyMap.constEnd(); ++it)
        m_modelMetaKeysList->append(it.key());
}
