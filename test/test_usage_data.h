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
* This header provides tests for the \ref UsageData class.  Note that the class expects a properly configured local
* web server to receive usage data packets.
***********************************************************************************************************************/

#ifndef TEST_USAGE_DATA_H
#define TEST_USAGE_DATA_H

#include <QObject>
#include <QtTest/QtTest>

#include <cstdint>

class QNetworkAccessManager;
class QSettings;
class QEventLoop;
class QTimer;

namespace Ud {
    class UsageData;
}

class TestUsageData:public QObject {
    Q_OBJECT

    public:
        TestUsageData();

        ~TestUsageData() override;

    protected slots: // protected to keep the test framework from thinking these are test cases.
        void reportingFinished(bool successful);

        void timedOut();

    private slots:
        void initTestCase();

        void testPrimaryInstance();

        void cleanupTestCase();

    private:
        static const unsigned     reportingInterval = 2; // Report every 2 seconds.
        static const unsigned     reportingTimeout = 60; // Give 60 seconds for things to happen.
        static const char         testWebhook[];
        static const std::uint8_t testUsageDataHmacSecret[];

        QNetworkAccessManager* networkAccessManager;
        QSettings*             settings;
        Ud::UsageData*         usageData;

        QEventLoop*            eventLoop;
        QTimer*                failureTimer;
        bool                   operationTimedOut;
        bool                   operationFinished;
};

#endif
