/*-*-c++-*-*************************************************************************************************************
* Copyright 2016 - 2022 Inesonic, LLC.
* 
* This file is licensed under two licenses.
*
* Inesonic Commercial License, Version 1:
*   All rights reserved.  Inesonic, LLC retains all rights to this software, including the right to relicense the
*   software in source or binary formats under different terms.  Unauthorized use under the terms of this license is
*   strictly prohibited.
*
* GNU Public License, Version 2:
*   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
*   License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later
*   version.
*   
*   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
*   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
*   details.
*   
*   You should have received a copy of the GNU General Public License along with this program; if not, write to the Free
*   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
********************************************************************************************************************//**
* \file
*
* This file implements the \ref Ud::UsageData class.
***********************************************************************************************************************/

#include <QObject>
#include <QTimer>
#include <QSettings>
#include <QCoreApplication>
#include <QString>
#include <QtGlobal>
#include <QDateTime>
#include <QDate>
#include <QMutex>
#include <QMap>
#include <QByteArray>
#include <QMutexLocker>
#include <QThread>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSysInfo>
#include <QUrl>

#include <cstring>

#include <crypto_trng.h>
#include <crypto_aes_cbc_encryptor.h>
#include <crypto_hmac.h>

#include "ud_usage_data.h"

namespace Ud {
    const unsigned long UsageData::defaultReportingInterval = 7 * 24 * 60 * 60;
    const unsigned      UsageData::enableReportDelay = 60;
    const unsigned      UsageData::reportRetrialPeriod = 30 * 60;
    const QString       UsageData::defaultSettingsGroup("usageData");

    UsageData::UsageData(
            QSettings*             settings,
            QNetworkAccessManager* networkAccessManager,
            const QByteArray&      sharedSecret,
            const QUrl&            webhookUrl,
            QObject*               parent
        ):Wh::WebHook(
            networkAccessManager,
            sharedSecret,
            parent
        ) {
        configure(settings, webhookUrl);
    }


    UsageData::UsageData(
            QSettings*             settings,
            const QString&         settingsGroup,
            QNetworkAccessManager* networkAccessManager,
            const QByteArray&      sharedSecret,
            const QUrl&            webhookUrl,
            QObject*               parent
        ):Wh::WebHook(
            networkAccessManager,
            sharedSecret,
            parent
        ) {
        configure(settings, webhookUrl);
        setSettingsGroup(settingsGroup);
    }


    UsageData::~UsageData() {}


    bool UsageData::reportingEnabled() const {
        return enabled;
    }


    bool UsageData::reportingDisabled() const {
        return !enabled;
    }


    bool UsageData::isReporting() const {
        return currentlyIsReporting;
    }


    bool UsageData::isNotReporting() const {
        return !currentlyIsReporting;
    }


    std::uint64_t UsageData::userSecret() const {
        return secret;
    }


    const QUrl& UsageData::url() const {
        return currentDestinationUrl;
    }


    void UsageData::setUrl(const QUrl& newUrl) {
        currentDestinationUrl = newUrl;
    }


    QString UsageData::settingsGroup() const {
        return currentSettingsGroup;
    }


    void UsageData::setSettingsGroup(const QString& newSettingsGroup) {
        currentSettingsGroup = newSettingsGroup;
    }


    unsigned long long UsageData::interval() const {
        return reportInterval;
    }


    void UsageData::setInterval(unsigned long long newInterval) {
        reportInterval = newInterval;

        if (enabled && isNotReporting()) {
            nextOperation = lastOperation.addSecs(reportInterval);

            QDateTime earliestDateTime = QDateTime::currentDateTimeUtc().addSecs(enableReportDelay);
            if (nextOperation < earliestDateTime) {
                nextOperation = earliestDateTime;
            }

            scheduleReport(nextOperation);
        }
    }


    QDateTime UsageData::lastReportTime() {
        return lastOperation;
    }


    QDateTime UsageData::nextReportTime() {
        return nextOperation;
    }


    bool UsageData::reportingSuccessful() const {
        return lastReportSuccessful;
    }


    bool UsageData::isTimerActive(const QString& timerName) const {
        QMutexLocker locker(&timersMutex);
        return timers.contains(timerName);
    }


    void UsageData::loadSettings() {
        currentSettings->beginGroup(currentSettingsGroup);

        enabled = currentSettings->value("enabled").toBool();

        if (currentSettings->contains("secret")) {
            secret = currentSettings->value("secret").toULongLong();
        } else {
            secret = Crypto::random64();
            currentSettings->setValue("secret", static_cast<unsigned long long>(secret));
        }

        lastOperation = QDateTime::currentDateTimeUtc();
        nextOperation = lastOperation.addSecs(reportInterval);

        QDateTime defaultLastOperation = QDateTime::currentDateTimeUtc();
        QDateTime defaultNextOperation = defaultLastOperation.addSecs(reportInterval);

        lastOperation = currentSettings->value("last_operation", defaultLastOperation).toDateTime();
        nextOperation = currentSettings->value("next_operation", defaultNextOperation).toDateTime();

        events.clear();
        currentSettings->beginGroup("events");

        eventsMutex.lock();

        QStringList keys = currentSettings->allKeys();
        for (QStringList::const_iterator it=keys.begin(), end=keys.end() ; it!=end ; ++it) {
            std::uint64_t value = currentSettings->value(*it,0).toULongLong();
            events.insert(*it, value);
        }

        eventsMutex.unlock();

        currentSettings->endGroup();

        activities.clear();
        currentSettings->beginGroup("activities");

        activitiesMutex.lock();

        keys = currentSettings->allKeys();
        for (QStringList::const_iterator it=keys.begin(), end=keys.end() ; it!=end ; ++it) {
            std::uint64_t value = currentSettings->value(*it,0).toULongLong();
            events.insert(*it, value);
        }

        activitiesMutex.unlock();

        currentSettings->endGroup();

        currentSettings->endGroup();

        if (enabled) {
            scheduleReport(nextOperation);
        } else {
            timer->stop();
        }
    }


    void UsageData::saveSettings() {
        currentSettings->beginGroup(currentSettingsGroup);

        currentSettings->setValue("enabled", enabled);
        currentSettings->setValue("secret", static_cast<unsigned long long>(secret));
        currentSettings->setValue("lastOperation", lastOperation);
        currentSettings->setValue("nextOperation", nextOperation);

        currentSettings->beginGroup("events");
        for (auto it=events.begin(), end=events.end() ; it!=end ; ++it) {
            currentSettings->setValue(it.key(), static_cast<unsigned long long>(it.value()));
        }
        currentSettings->endGroup();

        currentSettings->beginGroup("activities");
        for (auto it=activities.begin(), end=activities.end() ; it!=end ; ++it) {
            currentSettings->setValue(it.key(), static_cast<unsigned long long>(it.value()));
        }
        currentSettings->endGroup();

        currentSettings->endGroup();
    }


    void UsageData::setReportingEnabled(bool nowEnabled) {
        if (!enabled && nowEnabled) {
            QDateTime minimumNextOperation = QDateTime::currentDateTimeUtc().addSecs(enableReportDelay);

            if (minimumNextOperation > nextOperation) {
                nextOperation = minimumNextOperation;
            }

            scheduleReport(nextOperation);
            }
        else if (enabled && !nowEnabled) {
            timer->stop();
        }

        enabled = nowEnabled;
    }


    void UsageData::setReportingDisabled(bool nowDisabled) {
        setReportingEnabled(!nowDisabled);
    }


    void UsageData::adjustEvent(const QString& eventName, unsigned adjustment) {
        QMutexLocker locker(&eventsMutex);

        if (events.contains(eventName)) {
            events[eventName] += adjustment;
        } else {
            events.insert(eventName, adjustment);
        }
    }


    void UsageData::adjustActivity(const QString& activityName, std::int64_t adjustment) {
        QMutexLocker locker(&activitiesMutex);

        if (activities.contains(activityName)) {
            activities[activityName] += adjustment;
        } else {
            activities.insert(activityName, adjustment);
        }
    }


    void UsageData::startTimer(const QString& timerName) {
        QMutexLocker locker(&timersMutex);

        Q_ASSERT(!timers.contains(timerName));
        timers.insert(timerName, QDateTime::currentDateTimeUtc());
    }


    void UsageData::stopTimer(const QString& timerName, bool doStop) {
        QMutexLocker locker(&timersMutex);
        Q_ASSERT(timers.contains(timerName));

        QDateTime endTime = QDateTime::currentDateTimeUtc();
        QDateTime startTime;

        if (doStop) {
            startTime = timers.take(timerName);
        } else {
            startTime = timers.value(timerName);
            timers[timerName] = endTime;
        }

        qint64 elapsedTime = startTime.secsTo(endTime);

        adjustActivity(timerName, elapsedTime);
    }


    void UsageData::stopTimers() {
        QMutexLocker locker(&timersMutex);
        QDateTime endTime = QDateTime::currentDateTimeUtc();

        for (auto it=timers.begin(), end=timers.end() ; it!=end ; ++it) {
            QString   timerName   = it.key();
            QDateTime startTime   = it.value();
            qint64    elapsedTime = startTime.secsTo(endTime);

            adjustActivity(timerName, elapsedTime);
        }

        timers.clear();
    }


    void UsageData::jsonResponseWasReceived(const QJsonDocument& jsonDocument) {
        Wh::WebHook::jsonResponseWasReceived(jsonDocument); // For test purposes.

        adjustEventsAndActivities();

        lastOperation = nextOperation;
        nextOperation = QDateTime::currentDateTimeUtc().addSecs(reportInterval);

        lastReportSuccessful = true;

        if (enabled) {
            scheduleReport(nextOperation);
        }

        emit reportingFinished(lastReportSuccessful);
        currentlyIsReporting = false;
    }


    void UsageData::failed(int networkError) {
        Wh::WebHook::failed(networkError); // For test purposes.

        nextOperation        = QDateTime::currentDateTimeUtc().addSecs(reportRetrialPeriod);
        lastReportSuccessful = false;

        if (enabled) {
            scheduleReport(nextOperation);
        }

        emit reportingFinished(false);
        currentlyIsReporting = false;
    }


    void UsageData::reportUsageData() {
        Q_ASSERT(enabled);
        Q_ASSERT(isNotReporting());

        currentlyIsReporting = true;
        emit reportingStarted();

        QJsonObject top;

        top.insert("product", QCoreApplication::applicationName());
        top.insert("version", QCoreApplication::applicationVersion());
        top.insert("cpu_architecture", QSysInfo::currentCpuArchitecture());
        top.insert("kernel_type", QSysInfo::kernelType());
        top.insert("kernel_version", QSysInfo::kernelVersion());
        top.insert("os_product_type", QSysInfo::productType());
        top.insert("os_product_version", QSysInfo::productVersion());
        top.insert("number_logical_cores", QThread::idealThreadCount());
        top.insert("secret_id_low", static_cast<double>(static_cast<std::uint32_t>(secret      )));
        top.insert("secret_id_high", static_cast<double>(static_cast<std::uint32_t>(secret >> 32)));

        top.insert("elapsed_time", lastOperation.secsTo(nextOperation));

        eventsMutex.lock();
        QStringList keys = events.keys();
        eventsMutex.unlock();

        eventsAdjustment.clear();
        QJsonObject eventsData;
        for (QStringList::const_iterator it=keys.constBegin(),end=keys.constEnd() ; it!=end ; ++it) {
            eventsMutex.lock();
            std::uint64_t value = events.value(*it);
            eventsMutex.unlock();

            eventsData.insert(*it, static_cast<double>(value));
            eventsAdjustment.insert(*it, value);
        }

        top.insert("events", eventsData);

        timersMutex.lock();
        keys = timers.keys();
        timersMutex.unlock();

        for (QStringList::const_iterator it=keys.constBegin(),end=keys.constEnd() ; it!=end ; ++it) {
            stopTimer(*it, false);
        }

        activitiesMutex.lock();
        keys = activities.keys();
        activitiesMutex.unlock();

        activitiesAdjustment.clear();
        QJsonObject activitiesData;
        for (QStringList::const_iterator it=keys.constBegin(),end=keys.constEnd() ; it!=end ; ++it) {
            activitiesMutex.lock();
            std::uint64_t value = activities.value(*it);
            activitiesMutex.unlock();

            activitiesData.insert(*it, static_cast<double>(value));
            activitiesAdjustment.insert(*it, value);
        }

        top.insert("activities", activitiesData);

        send(currentDestinationUrl, top);
    }


    void UsageData::scheduleReport(const QDateTime& reportTime) {
        QDateTime     currentTime     = QDateTime::currentDateTimeUtc();
        std::uint64_t secondsToReport = currentTime.secsTo(reportTime);

        timer->start(1000 * secondsToReport);
    }


    void UsageData::configure(QSettings* settings, const QUrl& destinationUrl) {
        currentSettings       = settings;
        currentDestinationUrl = destinationUrl;
        currentSettingsGroup  = defaultSettingsGroup;
        enabled               = false;
        reportInterval        = defaultReportingInterval;
        currentlyIsReporting  = false;
        lastReportSuccessful  = false;

        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setTimerType(Qt::VeryCoarseTimer);

        connect(timer, &QTimer::timeout, this, &UsageData::reportUsageData);
    }


    void UsageData::adjustEventsAndActivities() {
        for (  QHash<QString, std::uint64_t>::const_iterator eventsIterator    = eventsAdjustment.constBegin(),
                                                             eventsEndIterator = eventsAdjustment.constEnd()
             ; eventsIterator != eventsEndIterator
             ; ++eventsIterator
            ) {
            const QString& key        = eventsIterator.key();
            std::uint64_t  adjustment = eventsIterator.value();

            eventsMutex.lock();
            std::uint64_t value = events.value(key);

            if (value == adjustment) {
                events.remove(key);
            } else {
                Q_ASSERT(adjustment < value);
                events[key] = value - adjustment;
            }

            eventsMutex.unlock();
        }

        for (  QHash<QString, std::uint64_t>::const_iterator activitiesIterator    = activitiesAdjustment.constBegin(),
                                                             activitiesEndIterator = activitiesAdjustment.constEnd()
             ; activitiesIterator != activitiesEndIterator
             ; ++activitiesIterator
            ) {
            const QString& key        = activitiesIterator.key();
            std::uint64_t  adjustment = activitiesIterator.value();

            activitiesMutex.lock();
            std::uint64_t value = activities.value(key);

            if (value == adjustment) {
                activities.remove(key);
            } else {
                Q_ASSERT(adjustment < value);
                activities[key] = value - adjustment;
            }

            activitiesMutex.unlock();
        }

        eventsAdjustment.clear();
        activitiesAdjustment.clear();
    }
}
