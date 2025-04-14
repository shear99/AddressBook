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
                 const QString& nickname)
        : m_name(name),
        m_phoneNumber(phoneNumber),
        m_email(email),
        m_company(company),
        m_position(position),
        m_nickname(nickname)
    {}

    // Getters
    QString name() const { return m_name; }
    QString phoneNumber() const { return m_phoneNumber; }
    QString email() const { return m_email; }
    QString company() const { return m_company; }
    QString position() const { return m_position; }
    QString nickname() const { return m_nickname; }

    // Setters
    void setName(const QString& name) { m_name = name; }
    void setPhoneNumber(const QString& phoneNumber) { m_phoneNumber = phoneNumber; }
    void setEmail(const QString& email) { m_email = email; }
    void setCompany(const QString& company) { m_company = company; }
    void setPosition(const QString& position) { m_position = position; }
    void setNickname(const QString& nickname) { m_nickname = nickname; }

private:
    QString m_name;
    QString m_phoneNumber;
    QString m_email;
    QString m_company;
    QString m_position;
    QString m_nickname;
};

#endif // ADDRESSENTRY_H
