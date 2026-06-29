#ifndef QLLAMAVOCAB_H
#define QLLAMAVOCAB_H

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <vector>

#include <llama.h>
#include <property-macros.h>

#include "qllamabase.h"
#include "qllamaenums.h"

class QLlamaTokenData : public QLlamaBase
{
    Q_OBJECT
    QML_ELEMENT
    QP_RW(QString,                      text,  "")
    QP_RW(float,                        score, 0.0f)
    QP_RW(QLlamaEnums::QLlamaTokenAttr, attr,  QLlamaEnums::QLlamaTokenAttrUndefined)

public:
    explicit QLlamaTokenData(QObject *parent = nullptr) : QLlamaBase{parent} {}
};

class QLlamaVocab : public QLlamaBase
{
    Q_OBJECT
    QML_ELEMENT

    QP_RW(QLlamaEnums::QLlamaVocabType,    type,    QLlamaEnums::QLlamaVocabTypeNone)
    QP_RW(QLlamaEnums::QLlamaVocabPreType, preType, QLlamaEnums::QLlamaVocabPreTypeDefault)
    QP_RO(quint32,                         tokenCount, 0)

public:
    explicit QLlamaVocab(QObject *parent = nullptr);
    ~QLlamaVocab() override = default;

    void linkNativeVocab(const struct llama_vocab* vocab);

    // "High performance" string/token translation matrix layer entries
    Q_INVOKABLE QList<int> tokenize(const QString &text, bool addSpecial, bool parseSpecial = false) const;
    Q_INVOKABLE QString detokenize(const QList<int> &tokens, bool special) const;
    Q_INVOKABLE QString tokenToPiece(int tokenId) const;

    // the odd ones of the "bunch"
    Q_INVOKABLE int tokenBos() const;
    Q_INVOKABLE int tokenEos() const;
    Q_INVOKABLE int tokenNl() const;

private:
    const struct llama_vocab *m_nativeVocab = nullptr;
};


#endif // QLLAMAVOCAB_H