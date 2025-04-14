#ifndef ADDRESSENTRY_H
#define ADDRESSENTRY_H

#include <QString>

class AddressEntry {
public:
    AddressEntry() = default;

    AddressEntry(const QString& name,
                 const QString& phoneNumber,
                 const QString& email,
                 const QString& company,
                 const QString& position,
                 const QString& nickname,
                 bool favorite = false)
        : m_name(name),
        m_phoneNumber(phoneNumber),
        m_email(email),
        m_company(company),
        m_position(position),
        m_nickname(nickname),
        m_favorite(favorite),
        m_originalName(name),
        m_originalPhoneNumber(phoneNumber)
    {}

    // Getters
    QString name() const { return m_name; }
    QString phoneNumber() const { return m_phoneNumber; }
    QString email() const { return m_email; }
    QString company() const { return m_company; }
    QString position() const { return m_position; }
    QString nickname() const { return m_nickname; }
    bool favorite() const { return m_favorite; }
    QString originalName() const { return m_originalName; }
    QString originalPhoneNumber() const { return m_originalPhoneNumber; }

    // Setters
    void setName(const QString& name) { m_name = name; }
    void setPhoneNumber(const QString& phoneNumber) { m_phoneNumber = phoneNumber; }
    void setEmail(const QString& email) { m_email = email; }
    void setCompany(const QString& company) { m_company = company; }
    void setPosition(const QString& position) { m_position = position; }
    void setNickname(const QString& nickname) { m_nickname = nickname; }
    void setFavorite(bool favorite) { m_favorite = favorite; }

    // 원본 키 값은 보통 처음 로드할 때 설정되므로, 변경하지 않도록 함
    void setOriginalName(const QString& name) { m_originalName = name; }
    void setOriginalPhoneNumber(const QString& phoneNumber) { m_originalPhoneNumber = phoneNumber; }

private:
    QString m_name;
    QString m_phoneNumber;
    QString m_email;
    QString m_company;
    QString m_position;
    QString m_nickname;
    bool m_favorite = false;
    // 원본 키 값 (초기 로드 시 저장)
    QString m_originalName;
    QString m_originalPhoneNumber;

};

#endif // ADDRESSENTRY_H
