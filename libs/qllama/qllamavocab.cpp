#include "qllamavocab.h"
#include <QDebug>

QLlamaVocab::QLlamaVocab(QObject *parent)
    : QLlamaBase(parent)
{
}

void QLlamaVocab::linkNativeVocab(const struct llama_vocab* vocab)
{
    m_nativeVocab = vocab;
    if (m_nativeVocab) {
        set_tokenCount(llama_vocab_n_tokens(m_nativeVocab));
        set_type(static_cast<QLlamaEnums::QLlamaVocabType>(llama_vocab_type(m_nativeVocab)));
        // set_preType(static_cast<QLlamaEnums::QLlamaVocabPreType>(llama_vocab_get_pre_type(m_nativeVocab))); ... I wish not exposed to public API. Talk to developers about this TODO  Joseph Look at later
    } else {
        set_tokenCount(0);
        set_type(QLlamaEnums::QLlamaVocabTypeNone);
        set_preType(QLlamaEnums::QLlamaVocabPreTypeDefault);
    }
}

QList<int> QLlamaVocab::tokenize(const QString &text, bool addSpecial, bool parseSpecial) const
{
    QList<int> result;
    if (!m_nativeVocab)
        return result;

    QByteArray utf8 = text.toUtf8();
    std::vector<llama_token> tokens(utf8.size() + (addSpecial ? 2 : 0));

    int32_t n_tokens = llama_tokenize(
        m_nativeVocab,
        utf8.constData(),
        utf8.size(),
        tokens.data(),
        static_cast<int32_t>(tokens.size()),
        addSpecial,
        parseSpecial
    );

    if (n_tokens < 0) {
        // Grow vector block dynamically if token count exceeded estimation
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(
            m_nativeVocab,
            utf8.constData(),
            utf8.size(),
            tokens.data(),
            static_cast<int32_t>(tokens.size()),
            addSpecial,
            parseSpecial
        );
    }

    result.reserve(n_tokens);
    for (int32_t i = 0; i < n_tokens; ++i) {
        result.append(static_cast<int>(tokens[i]));
    }
    return result;
}

QString QLlamaVocab::detokenize(const QList<int> &tokens, [[maybe_unused]]bool special) const
{
    if (!m_nativeVocab || tokens.isEmpty())
        return QString();

    QString result;
    for (int token : tokens)
        result.append(tokenToPiece(token)); // Accurately appends special pieces

    // spical later

    return result;
}

QString QLlamaVocab::tokenToPiece(int tokenId) const
{
    if (!m_nativeVocab)
        return QString();

    // local 256-byte static scratch buffer to grab string pieces "safely"
    char buffer[256];
    int32_t len = llama_token_to_piece(m_nativeVocab, static_cast<llama_token>(tokenId), buffer, sizeof(buffer), 0, true);

    if (len < 0) {
        // Fallback or grow allocation dynamically if string boundary overflows ?
        std::vector<char> bigBuffer(-len);
        llama_token_to_piece(m_nativeVocab, static_cast<llama_token>(tokenId), bigBuffer.data(), static_cast<int32_t>(bigBuffer.size()), 0, true);
        return QString::fromUtf8(bigBuffer.data(), -len);
    }

    return QString::fromUtf8(buffer, len);
}

int QLlamaVocab::tokenBos() const
{
    return m_nativeVocab ? static_cast<int>(llama_vocab_bos(m_nativeVocab)) : -1;
}

int QLlamaVocab::tokenEos() const
{
    return m_nativeVocab ? static_cast<int>(llama_vocab_eos(m_nativeVocab)) : -1;
}

int QLlamaVocab::tokenNl()  const
{
    return m_nativeVocab ? static_cast<int>(llama_vocab_nl(m_nativeVocab))  : -1;
}

