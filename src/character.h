#pragma once

#include <QObject>
#include <QImage>

class QNetworkAccessManager;

struct Ability
{
    int baseScore = 10;
    int bonusScore = 0;
    int overrideScore = 0;
    int setScore = 0;

    int totalScore() const;
    int modifier() const;
    void reset();
};

enum class ArmorType
{
    None,
    Light,
    Medium,
    Heavy
};

class Character : public QObject
{
    Q_OBJECT

public:
    Character(QObject *parent);
    ~Character();

    int id() const { return m_id; }
    QImage portrait() const { return m_portrait; }
    QString name() const { return m_name; }
    int level() const { return m_level; }
    QString race() const { return m_race; }
    QString playerClass() const { return m_class; }
    int armorClass() const;
    int maxHitPoints() const;
    int currenthitPoints() const;

    void load();

public slots:
    void reload(const int id);
    void loadPortrait();

signals:
    void updated();
    void portraitUpdated();

private:
    void loadFromJson(const QByteArray &bytes);
    int maxArmorClassModifier() const;

private:
    QNetworkAccessManager *m_manager;
    int m_id;
    QString m_portraitUrl;
    QImage m_portrait;
    QString m_name;
    int m_level;
    QString m_race;
    QString m_class;
    QList<Ability> m_abilities;
    ArmorType m_armorType;
    int m_armorClass;
    int m_bonusArmorClass;
    bool m_mediumArmorExpert;
    int m_baseHitPoints;
    int m_bonusHitPoints;
    int m_overrideHitPoints;
    int m_removedHitPoints;
    int m_temporaryHitPoints;

};
