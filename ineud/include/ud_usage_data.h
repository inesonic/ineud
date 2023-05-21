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
* This header defines the \ref Ud::UsageData class.
***********************************************************************************************************************/

/* .. sphinx-project ineud */

#ifndef UD_USAGE_DATA_H
#define UD_USAGE_DATA_H

#include <QObject>
#include <QString>
#include <QtGlobal>
#include <QDateTime>
#include <QMutex>
#include <QHash>
#include <QJsonObject>
#include <QUrl>

#include <cstdint>

#include <wh_web_hook.h>

#include "ud_common.h"

class QTimer;
class QDate;
class QByteArray;
class QNetworkAccessManager;
class QSettings;

namespace Ud {
    /**
     * Class that tracks user activity within the application for future improvement.
     *
     * The UsageData class maintains two types of entries:
     *
     *     * Entries that contain simple counts.  These entries are intended to track the number of times a feature is
     *       used over a given period.
     *     * Entries that maintain running sums.  These entries are intended to track how much time is spent in a
     *       specific mode, in a specific dialog, etc.
     *
     * The class keeps track of the time between reporting events and provides that data as part of the generated
     * report and saved off on application exit.  The class also maintains a timer used to trigger flush events at
     * periodic intervals.
     *
     * To assist in tracking time spent on certain activities, the class provides methods to automatically start and
     * stop timers.  Stopping a timer automatically calculates the time spent and adds the value to an activity tracker
     * with the same name.
     *
     * The class provides a mechanism to enable or disable reporting.  Reporting status is maintained persistently.
     *
     * Lastly, the class maintains a randomly generated 64-bit value used as an anonymous secret value for tracking
     * purposes.  The use of the 64-bit random value allows the statistics to be anonymized.
     *
     * Reporting is disabled, by default.  After creating an instance of this object, you should load application
     * settings.
     *
     * Data is transmitted via HTTPS POST using an Inesonic standard web hook with rolling hash.
     */
    class UD_PUBLIC_API UsageData:public Wh::WebHook {
        Q_OBJECT

        public:
            /**
             * The default usage statistics reporting interval, in seconds.
             */
            static const unsigned long defaultReportingInterval;

            /**
             * Delay between enabling the timer and the first report.  The delay is used to avoid retriggering updates
             * at a high rate.  Value is in seconds.
             */
            static const unsigned enableReportDelay;

            /**
             * Delay between reporting retry attempts.  Value is in seconds.
             */
            static const unsigned reportRetrialPeriod;

            /**
             * The default settings group.
             */
            static const QString defaultSettingsGroup;

            /**
             * Constructor
             *
             * \param[in] settings             The settings class to load and store settings data with.
             *
             * \param[in] networkAccessManager The network settings manager.
             *
             * \param[in] sharedSecret         The HMAC secret used to validate messages.
             *
             * \param[in] webhookUrl           The URL to send reports to.
             *
             * \param[in] parent               Pointer to the parent object.
             */
            UsageData(
                QSettings*             settings,
                QNetworkAccessManager* networkAccessManager,
                const QByteArray&      sharedSecret,
                const QUrl&            webhookUrl,
                QObject*               parent = Q_NULLPTR);

            /**
             * Constructor
             *
             * \param[in] settings             The settings class to load and store settings data with.
             *
             * \param[in] settingsGroup        The group to use for usage data settings.
             *
             * \param[in] networkAccessManager The network settings manager.
             *
             * \param[in] sharedSecret         The HMAC secret used to validate messages.
             *
             * \param[in] webhookUrl           The URL to send reports to.
             *
             * \param[in] parent               Pointer to the parent object.
             */
            UsageData(
                QSettings*             settings,
                const QString&         settingsGroup,
                QNetworkAccessManager* networkAccessManager,
                const QByteArray&      sharedSecret,
                const QUrl&            webhookUrl,
                QObject*               parent = Q_NULLPTR
            );

            ~UsageData() override;

            /**
             * Method you can use to determine if usage data has been configured.  Call this method before calling
             * \ref loadSettings.
             *
             * \return Returns true if usage data as been configured (and possibly disabled).  Returns false if usage
             *         data has not been configured.
             */
            bool isConfigured() const;

            /**
             * Determines if reporting is enabled.
             *
             * \return Returns true if reporting is enabled.  Returns false if reporting is disabled.
             */
            bool reportingEnabled() const;

            /**
             * Determines if reporting is disabled.
             *
             * \return Returns true if reporting is disabled.  Returns false if reporting is enabled.
             */
            bool reportingDisabled() const;

            /**
             * Determines if reporting is in progress.
             *
             * \return Returns true if reporting is in progress.  Returns false if reporting is not in progress.
             */
            bool isReporting() const;

            /**
             * Determines if reporting is not in progress.
             *
             * \return Returns true if reporting is not in progress.  Returns false if reporting is in progress.
             */
            bool isNotReporting() const;

            /**
             * Reports the secret value used to report information about this user's statistics.
             *
             * \return Returns a randomly generated, perisistent, 64-bit value.
             */
            std::uint64_t userSecret() const;

            /**
             * Determines the URL where usage data will be sent.
             *
             * \return Returns the URL used for message data.
             */
            const QUrl& url() const;

            /**
             * Sets the URL where usage data will be sent.
             *
             * \param[in] newUrl The new URL to be used for messages.
             */
            void setUrl(const QUrl& newUrl);

            /**
             * Determines the settings group that will be used to load and save settings.
             *
             * \return Returns the settings group used to load and save usage data.
             */
            QString settingsGroup() const;

            /**
             * Sets the settings group to be used to load and save application settings.
             *
             * \param[in] newSettingsGroup The new settings group name to be used.
             */
            void setSettingsGroup(const QString& newSettingsGroup);

            /**
             * Determines the current reporting interval, in seconds.
             *
             * \return Returns the reporting interval, in seconds.
             */
            inline unsigned long long interval() const;

            /**
             * Sets the current reporting interval, in seconds.  Note that this method may trigger future reports.
             * This method exists primarily for test purposes and should generally not be needed during normal
             * operation.
             *
             * \param[in] newInterval The new reporting interval, in seconds.
             */
            void setInterval(unsigned long long newInterval);

            /**
             * Returns the date and time that the last report was made regarding user activity.
             *
             * \return Returns the date and time of the last report that was made.
             */
            QDateTime lastReportTime();

            /**
             * Returns the date and time that the next report is expected to be made regarding user activity.
             *
             * \return Returns the date and time when the application plans to make the next report regarding user
             *         activity.
             */
            QDateTime nextReportTime();

            /**
             * Determines if the last reporting operation was successful.
             *
             * \return Returns true if the last operation was successful.  Returns false if the last operation failed.
             */
            bool reportingSuccessful() const;

            /**
             * Determines if a specified timer is active.
             *
             * \param[in] timerName The name of the timer to start.
             */
            bool isTimerActive(const QString& timerName) const;

            /**
             * Loads stateful information related to customer usage.  This method is thread-safe.
             */
            void loadSettings();

            /**
             * Saves stateful information related to customer usage.  This method is thread safe.  Note that you may
             * want to call \ref stopTimers to terminate any running timers before calling this method.
             */
            void saveSettings();

        public slots:
            /**
             * Enables or disables reporting of usage statistics.
             *
             * \param[in] nowEnabled If true, reporting will be enabled.  If false, reporting will be disabled.
             */
            void setReportingEnabled(bool nowEnabled = true);

            /**
             * Disables or enables reporting of usage statistics.
             *
             * \param[in] nowDisabled If true, reporting will be disabled.  If false, reporting will be enabled.
             */
            void setReportingDisabled(bool nowDisabled = true);

            /**
             * Increments a usage event tracker.  This method is thread-safe.
             *
             * \param[in] eventName  The name of the event to be adjusted.
             *
             * \param[in] adjustment The adjustment to apply to the counter.
             */
            void adjustEvent(const QString& eventName, unsigned adjustment = 1);

            /**
             * Adds a value to a usage time tracker.  This method is thread-safe.
             *
             * \param[in] activityName The name of the activity being tracked.
             *
             * \param[in] adjustment   The adjustment amount.
             */
            void adjustActivity(const QString& activityName, std::int64_t adjustment);

            /**
             * Starts an activity timer.  The function will assert if the timer is already running.
             *
             * \param[in] timerName The name of the timer to start.
             */
            void startTimer(const QString& timerName);

            /**
             * Stop or updates an activity timer, updating the activity with the time delta, in seconds.
             *
             * \param[in] timerName The name of the timer to start.
             *
             * \param[in] doStop    If true, the timer will be stopped.  If false, elapsed time will be accounted for
             *                      and the timer will continue to run.
             */
            void stopTimer(const QString& timerName, bool doStop = true);

            /**
             * Stops and processes all active timers.
             */
            void stopTimers();

        signals:
            /**
             * Signal that is emitted when reporting is started.  This signal exists primary for test purposes.
             */
            void reportingStarted();

            /**
             * Signal that is emitted when reporting has finished.  This signal exists primarily for test purposes.
             *
             * \param[in] successful Holds true if the reporting operation was successful.  Holds false if the
             *                      reporting operation failed.
             */
            void reportingFinished(bool successful);

        protected:
            /**
             * Method you can overload to intercept valid responses.
             *
             * \param[in] jsonDocument A JSON document holding the received response.
             */
            void jsonResponseWasReceived(const QJsonDocument& jsonDocument) override;

            /**
             * Method you can overload to intercept failure notifications.  The default implementation triggers the
             * \ref Wh::WebHook::failedToSend signal.
             *
             * \param[in] networkError The last reported network error.  This is the value of
             *                         QNetworkReply::NetworkError cast to an integer.
             */
            void failed(int networkError) override;

        private slots:
            /**
             * Slot that triggers reporting of user activity.
             */
            void reportUsageData();

        private:
            /**
             * Schedules reports to occur at a specified time.
             *
             * \param[in] reportTime The time for the report to be issued.
             */
            void scheduleReport(const QDateTime& reportTime);

            /**
             * Method called to perform configuration that is common to all constructors.
             *
             * \param[in] settings       The settings group to load/store settings to.
             *
             * \param[in] destinationUrl The destination URL for the webhook.
             */
            void configure(QSettings* settings, const QUrl& destinationUrl);

            /**
             * Adjusts the events and activities values downward after reporting.  If the resulting value is zero, the
             * event or activity is removed from the database.
             */
            void adjustEventsAndActivities();

            /**
             * The settings class used to load/store data.
             */
            QSettings* currentSettings;

            /**
             * The group within the settings class to hold the usage data between reports.
             */
            QString currentSettingsGroup;

            /**
             * The network access manager.
             */
            QNetworkAccessManager* currentNetworkAccessManager;

            /**
             * The destination URL.
             */
            QUrl currentDestinationUrl;

            /**
             * Flag indicating if usage data reporting is enabled.
             */
            bool enabled;

            /**
             * Secret used to identify this machine in an anonymous way.
             */
            std::uint64_t secret;

            /**
             * Mutex used to allow multi-threaded access to the usage data events.
             */
            mutable QMutex eventsMutex;

            /**
             * Mutex used to allow multi-threaded access to the usage data activities.
             */
            mutable QMutex activitiesMutex;

            /**
             * Mutex used to allow multi-threaded access to the usage data timers.
             */
            mutable QMutex timersMutex;

            /**
             * Value indicating when the last update was performed.
             */
            QDateTime lastOperation;

            /**
             * Value indicating when the next update will be performed.
             */
            QDateTime nextOperation;

            /**
             * Hash tracking count of events by event name.
             */
            QHash<QString, std::uint64_t> events;

            /**
             * Hash tracking activity counts.
             */
            QHash<QString, std::uint64_t> activities;

            /**
             * Hash tracking timer start date/times.
             */
            QHash<QString, QDateTime> timers;

            /**
             * Hash used to track adjustments to events during updates.  This hash exists so we don't need to block
             * updates to the tracking mechanisms during reporting.
             */
            QHash<QString, std::uint64_t> eventsAdjustment;

            /**
             * Hash used to track adjustments to activities during updates.  This hash exists so we don't need to block
             * updates to the tracking mechanisms during reporting.
             */
            QHash<QString, std::uint64_t> activitiesAdjustment;

            /**
             * Timer used to trigger updates.
             */
            QTimer* timer;

            /**
             * The desired interval between reports, in seconds.
             */
            std::uint64_t reportInterval;

            /**
             * Flag that indicates if this class is actively reporting.
             */
            bool currentlyIsReporting;

            /**
             * Flag indicating if the last reporting operation was successful.
             */
            bool lastReportSuccessful;
    };
}

#endif
