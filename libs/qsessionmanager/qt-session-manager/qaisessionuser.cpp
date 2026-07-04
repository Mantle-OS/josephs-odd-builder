#include "qaisessionuser.h"

QAiSessionUser::QAiSessionUser(QObject *parent) :
    QObject{parent}
{
}

QAiSessionUser::~QAiSessionUser() noexcept
{
}

void QAiSessionUser::updateFromStruct(const jsgen::AiSessionUser &userStruct) noexcept
{
    m_user = userStruct;
    set_userId(QString::fromStdString(m_user.user_id));
    set_displayName(QString::fromStdString(m_user.display_name));
    set_icon(QString::fromStdString(m_user.icon));
    set_vaultPath(QString::fromStdString(m_user.vault_path));
}