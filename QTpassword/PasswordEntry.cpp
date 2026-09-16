#include "PasswordEntry.h"

PasswordEntry::PasswordEntry(){}

PasswordEntry::PasswordEntry(const QString &title,const QString &username,
        const QString &password,const QString &url, const QString &notes,const QString &category,bool favorite):
        m_title(title),m_username(username),m_password(password),
    m_url(url),m_notes(notes),m_category(category),m_favorite(favorite){ }

QString PasswordEntry::title() const { return m_title; }

QString PasswordEntry::username() const { return m_username; }

QString PasswordEntry::password() const { return m_password; }

QString PasswordEntry::url() const { return m_url; }

QString PasswordEntry::notes() const { return m_notes; }

QString PasswordEntry::category() const{ return m_category;}


void PasswordEntry::setTitle(const QString &title) { m_title = title; }

void PasswordEntry::setUsername(const QString &username) { m_username = username; }

void PasswordEntry::setPassword(const QString &password) { m_password = password; }

void PasswordEntry::setUrl(const QString &url) { m_url = url; }

void PasswordEntry::setNotes(const QString &notes) { m_notes = notes; }

bool PasswordEntry::favorite()const{  return m_favorite;}

void PasswordEntry::setFavorite(bool favorite){  m_favorite = favorite;}