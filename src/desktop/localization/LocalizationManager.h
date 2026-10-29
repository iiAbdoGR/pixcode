#pragma once

#include <QtCore/QObject>
#include <QString>
#include <unordered_map>
#include <memory>

namespace pixcode::desktop {

class LocalizationManager : public QObject {
    Q_OBJECT

public:
    static LocalizationManager& instance();

    void setLanguage(const QString& languageCode);
    [[nodiscard]] QString currentLanguage() const noexcept { return m_currentLanguage; }
    [[nodiscard]] bool isRtl() const noexcept { return m_currentLanguage == "ar"; }

    [[nodiscard]] QString translate(const QString& key) const;
    [[nodiscard]] QString get(const QString& key, const QString& fallback = QString()) const {
        QString res = translate(key);
        if (res == key && !fallback.isEmpty()) {
            return fallback;
        }
        return res;
    }

signals:
    void languageChanged(const QString& newLanguageCode);

private:
    LocalizationManager();
    void initDictionary();

    QString m_currentLanguage;
    std::unordered_map<std::string, std::unordered_map<std::string, QString>> m_dictionary;
};

// Convenience inline macro
inline QString trText(const QString& key) {
    return LocalizationManager::instance().translate(key);
}

} // namespace pixcode::desktop

