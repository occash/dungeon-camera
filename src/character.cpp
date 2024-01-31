#include "character.h"

#include <cmath>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

int Ability::totalScore() const
{
    if (overrideScore > 0)
        return overrideScore;
    else if (setScore > 0)
        return setScore;
    else
        return baseScore + bonusScore;
}

int Ability::modifier() const
{
    return std::floor((totalScore() - 10) / 2);
}

void Ability::reset()
{
    baseScore = 10;
    bonusScore = 0;
    overrideScore = 0;
    setScore = 0;
}

Character::Character(QObject *parent) :
    QObject(parent),
    m_manager{ new QNetworkAccessManager(this) },
    m_id{ -1 },
    m_level{ 1 },
    m_abilities{ 6 },
    m_armorType{ ArmorType::None },
    m_armorClass{ 0 },
    m_bonusArmorClass{ 0 },
    m_mediumArmorExpert{ false },
    m_baseHitPoints{ 8 },
    m_bonusHitPoints{ 0 },
    m_overrideHitPoints{ 0 },
    m_removedHitPoints{ 0 },
    m_temporaryHitPoints{ 0 }
{}

Character::~Character()
{}

int Character::armorClass() const
{
    if (m_armorType != ArmorType::None) {
        const int maxDexModifier = maxArmorClassModifier();
        const int dexModifier = std::clamp(m_abilities[1].modifier(), -5, maxDexModifier);
        return m_armorClass + dexModifier + m_bonusArmorClass;
    } else {
        // TODO: unarmored AC
        return 10 + m_abilities[1].modifier() + m_bonusArmorClass;
    }
}

int Character::maxHitPoints() const
{
    if (m_overrideHitPoints > 0)
        return m_overrideHitPoints;
    else {
        return m_baseHitPoints +
            m_bonusHitPoints +
            m_temporaryHitPoints +
            m_abilities[2].modifier() * m_level;
    }
}

int Character::currenthitPoints() const
{
    return std::clamp(maxHitPoints() - m_removedHitPoints, 0, 2000);
}

void Character::load()
{
    QFile file{ "data.json" };

    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file for loading";
        return;
    }

    loadFromJson(file.readAll());
    emit updated();
    loadPortrait();
}

void Character::loadPortrait()
{
    QNetworkRequest request{ m_portraitUrl };
    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray bytes = reply->readAll();
            m_portrait = QImage::fromData(bytes);
            emit portraitUpdated();
        }

        reply->deleteLater();
    });
}

void Character::reload(const int id)
{
    QUrl url{ QString("https://character-service.dndbeyond.com/character/v5/character/%1").arg(id) };
    QNetworkRequest request{ url };
    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray bytes = reply->readAll();
            QFile file{ "data.json" };

            if (!file.open(QIODevice::WriteOnly)) {
                qCritical() << "Failed to open file for saving";
                return;
            }

            file.write(bytes);
            loadFromJson(bytes);
            emit updated();
            loadPortrait();
        }

        reply->deleteLater();
    });
}

void Character::loadFromJson(const QByteArray &bytes)
{
    auto document = QJsonDocument::fromJson(bytes);
    QJsonObject object = document.object();
    QJsonObject data = object["data"].toObject();
    
    m_id = data["id"].toInt();
    QJsonObject decorations = data["decorations"].toObject();
    m_portraitUrl = decorations["avatarUrl"].toString();
    m_name = data["name"].toString();
    QJsonObject race = data["race"].toObject();
    m_race = race["fullName"].toString();
    QJsonArray classes = data["classes"].toArray();
    QJsonObject mainClass = classes[0].toObject();
    m_level = mainClass["level"].toInt();
    QJsonObject definition = mainClass["definition"].toObject();
    m_class = definition["name"].toString();

    for (auto &ability : m_abilities)
        ability.reset();

    QJsonArray stats = data["stats"].toArray();
    QJsonArray bonusStats = data["bonusStats"].toArray();
    QJsonArray overrideStats = data["overrideStats"].toArray();

    for (int i = 0; i < stats.size(); ++i) {
        QJsonObject stat = stats[i].toObject();
        QJsonObject bonusStat = bonusStats[i].toObject();
        QJsonObject overrideStat = overrideStats[i].toObject();

        m_abilities[i].baseScore = stat["value"].toInt();
        m_abilities[i].bonusScore = bonusStat["value"].toInt();
        m_abilities[i].overrideScore = overrideStat["value"].toInt();
    }

    m_baseHitPoints = data["baseHitPoints"].toInt();
    m_bonusHitPoints = data["bonusHitPoints"].toInt();
    m_overrideHitPoints = data["overrideHitPoints"].toInt();
    m_removedHitPoints = data["removedHitPoints"].toInt();
    m_temporaryHitPoints = data["temporaryHitPoints"].toInt();

    // Adjust stats from race, class and items
    // read modifiers/race and modifiers/class
    // type: bonus
    // subtype: strength-score

    QJsonObject modifiers = data["modifiers"].toObject();
    QJsonArray raceModifiers = modifiers["race"].toArray();

    for (QJsonValueRef value : raceModifiers) {
        QJsonObject modifier = value.toObject();
        const int modifierTypeId = modifier["modifierTypeId"].toInt();
        const int modifierSubTypeId = modifier["modifierSubTypeId"].toInt();

        if (modifierTypeId == 1) {
            const int index = modifierSubTypeId - 2;

            if (index >= 0 && index <= 6)
                m_abilities[index].bonusScore += modifier["value"].toInt();
        } else if (modifierTypeId == 9) {
            const int index = modifierSubTypeId - 175;

            if (index >= 0 && index <= 6)
                m_abilities[index].setScore = modifier["value"].toInt();
        }
    }

    QJsonArray classModifiers = modifiers["class"].toArray();

    for (QJsonValueRef value : classModifiers) {
        QJsonObject modifier = value.toObject();
        const int modifierTypeId = modifier["modifierTypeId"].toInt();
        const int modifierSubTypeId = modifier["modifierSubTypeId"].toInt();

        if (modifierTypeId == 1) {
            const int index = modifierSubTypeId - 2;

            if (index >= 0 && index <= 6)
                m_abilities[index].bonusScore += modifier["value"].toInt();
        } else if (modifierTypeId == 9) {
            const int index = modifierSubTypeId - 175;

            if (index >= 0 && index <= 6)
                m_abilities[index].setScore = modifier["value"].toInt();
        }
    }

    QJsonArray itemModifiers = modifiers["item"].toArray();

    for (QJsonValueRef value : itemModifiers) {
        QJsonObject modifier = value.toObject();
        const int modifierTypeId = modifier["modifierTypeId"].toInt();
        const int modifierSubTypeId = modifier["modifierSubTypeId"].toInt();

        if (modifierTypeId == 1) {
            const int index = modifierSubTypeId - 2;

            if (index >= 0 && index <= 6)
                m_abilities[index].bonusScore += modifier["value"].toInt();
            else if (modifierSubTypeId == 1)
                m_bonusArmorClass += modifier["value"].toInt();
        } else if (modifierTypeId == 9) {
            const int index = modifierSubTypeId - 175;

            if (index >= 0 && index <= 6)
                m_abilities[index].setScore = modifier["value"].toInt();
        }
    }

    QJsonArray inventory = data["inventory"].toArray();

    for (QJsonValueRef value : inventory) {
        QJsonObject item = value.toObject();
        const bool equipped = item["equipped"].toBool();

        if (equipped) {
            QJsonObject definition = item["definition"].toObject();
            const int armorTypeId = definition["armorTypeId"].toInt();

            if (armorTypeId > 0) {
                m_armorType = static_cast<ArmorType>(armorTypeId);
                m_armorClass = definition["armorClass"].toInt();
            }
        }
    }

    QJsonArray feats = data["feats"].toArray();

    for (QJsonValueRef value : feats) {
        QJsonObject feat = value.toObject();
        QJsonObject definition = feat["definition"].toObject();
        const int id = definition["id"].toInt();

        if (id == 33)
            m_mediumArmorExpert = true;
    }
}

int Character::maxArmorClassModifier() const
{
    switch (m_armorType) {
    case ArmorType::Light:
        return 10;
    case ArmorType::Medium:
        return m_mediumArmorExpert ? 3 : 2;
    case ArmorType::Heavy:
        return 0;
    default:
        return 10;
    }
}
