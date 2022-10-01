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
* This header provides a wrapper for tests pulling the QtTest framework more cleanly into the signal/slot mechanism.
***********************************************************************************************************************/

#ifndef APPLICATION_WRAPPER_H
#define APPLICATION_WRAPPER_H

#include <QtGlobal>
#include <QObject>
#include <QList>

class QCoreApplication;
class QTimer;

/**
 * Class that pulls the QtTest functions under an instance of \ref UniqueApplication.  The class allows tests to be
 * run under the application framework expected by the various classes.
 *
 * This approach was inspired by comments on the web page:
 *
 *     http://stackoverflow.com/questions/1524390/what-unit-testing-framework-should-i-use-for-qt
 */
class ApplicationWrapper:public QObject {
    Q_OBJECT

    public:
        /**
         * Constructor
         *
         * \param[in] argumentCount The number of command line arguments.
         *
         * \param[in] argumentValues The command line arguments.
         *
         * \param[in] parent Pointer to the parent object.
         */
        ApplicationWrapper(
            int&     argumentCount,
            char**   argumentValues,
            QObject* parent = Q_NULLPTR
        );

        ~ApplicationWrapper() override;

        /**
         * Adds a new test class to the test framework.
         *
         * \param[in] testInstance The test instance to be added.
         */
        void includeTest(QObject* testInstance);

        /**
         * Executes all registered tests for the application.
         *
         * \return Returns 0 on success, non-zero on error.
         */
        int exec();

    private slots:
        /**
         * Starts the tests, in succession.  Triggered by the signal/slot event loop.
         */
        void runNextTest();

    private:
        void startNextTest();

        QCoreApplication*         applicationInstance;
        QTimer*                   startTimer;
        QList<QObject*>           registeredTests;
        QList<QObject*>::iterator nextTest;
        int                       currentStatus;
};

#endif
