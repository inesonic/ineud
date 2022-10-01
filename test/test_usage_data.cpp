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
* This file implements tests for the \ref UsageData class.  Note that the class expects a properly configured local web
* server to receive usage data packets.
***********************************************************************************************************************/

#include <QDebug>
#include <QObject>
#include <QtTest/QtTest>
#include <QEventLoop>
#include <QTimer>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QSettings>

#if (defined(Q_OS_WIN32))

    #include <Windows.h>

#else

    #include <unistd.h>

#endif

#include <cstdint>

#include <ud_usage_data.h>

#include "test_usage_data.h"

const char         TestUsageData::testWebhook[] = "https://autonoma.inesonic.com/v2/test_usage_data";
const std::uint8_t TestUsageData::testUsageDataHmacSecret[] = {
    0xB1, 0xD7, 0xAC, 0x38,   0x6C, 0xE4, 0xD3, 0x19,
    0x4F, 0xCC, 0x35, 0xE0,   0xA8, 0xFB, 0x65, 0x41,
    0x0F, 0xBE, 0x39, 0x41,   0x43, 0xF0, 0x47, 0x63,
    0x1D, 0xAC, 0x4C, 0xA1,   0x84, 0x30, 0x90, 0xC0,
    0xA2, 0x3E, 0x80, 0xA9,   0x07, 0x6F, 0x97, 0xA2,
    0x72, 0x52, 0x57, 0xC3,   0xC2, 0x03, 0xDC, 0x2F,
    0xAD, 0x2D, 0x6B, 0xE1
};

#if (defined(Q_OS_WIN))

    /* Because Windows just has to be different in annoyingly meaningless ways */
    void sleep(float timeInSeconds) {
        Sleep(static_cast<int>(1000 * timeInSeconds + 0.5));
    }

#endif

TestUsageData::TestUsageData() {
    eventLoop          = new QEventLoop(this);
    failureTimer       = new QTimer(this);
    operationTimedOut  = false;

    networkAccessManager = new QNetworkAccessManager(this);
    settings             = new QSettings("Inesonic, LLC", "test_ineud", this);
    usageData            = new Ud::UsageData(
        settings,
        networkAccessManager,
        QByteArray(reinterpret_cast<const char*>(testUsageDataHmacSecret), sizeof(testUsageDataHmacSecret)),
        QUrl(testWebhook),
        this
    );

    connect(usageData, SIGNAL(reportingFinished(bool)), this, SLOT(reportingFinished(bool)));
    connect(failureTimer, SIGNAL(timeout()), this, SLOT(timedOut()));
}


TestUsageData::~TestUsageData() {}


void TestUsageData::reportingFinished(bool success) {
    operationTimedOut = false;
    operationFinished = success;

    usageData->setReportingDisabled();
    failureTimer->stop();

    eventLoop->quit();
}


void TestUsageData::timedOut() {
    operationTimedOut = true;
    operationFinished = false;

    usageData->setReportingDisabled();
    failureTimer->stop();

    eventLoop->quit();
}


void TestUsageData::initTestCase() {
    usageData->loadSettings();
    usageData->setReportingDisabled();
}


void TestUsageData::testPrimaryInstance() {
    usageData->adjustEvent("test_event_1");
    usageData->adjustEvent("test_event_2");
    usageData->adjustEvent("test_event_1");

    usageData->startTimer("activity_1");
    usageData->startTimer("activity_2");
    sleep(2);
    usageData->stopTimer("activity_2");
    sleep(2);
    usageData->stopTimer("activity_1");

    usageData->setReportingEnabled();
    usageData->setInterval(1);
    eventLoop->exec();

    QCOMPARE(operationTimedOut, false);
    QCOMPARE(usageData->reportingSuccessful(), true);
}


void TestUsageData::cleanupTestCase() {
    usageData->saveSettings();
}
