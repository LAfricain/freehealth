/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2013 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *  Main developer: Eric MAEKER, <eric.maeker@gmail.com>                   *
 *  Contributors:                                                          *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
/*!
 * \class DDI::DDICore
 * \brief Central core singleton object for the DDIManager plugin.
 */

#include "ddicore.h"
#include <ddiplugin/constants.h>
#include <ddiplugin/atc/atctablemodel.h>
#include <ddiplugin/interactors/druginteractortablemodel.h>
#include <ddiplugin/ddi/drugdruginteractiontablemodel.h>
#include <ddiplugin/database/ddidatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/isettings.h>

#include <utils/log.h>
#include <utils/global.h>
#include <translationutils/constants.h>

#include <QDir>

#include <QDebug>

using namespace DDI;
using namespace Internal;
using namespace Trans::ConstantTranslations;

static inline Core::ISettings *settings()  { return Core::ICore::instance()->settings(); }

DDI::DDICore *DDI::DDICore::_instance = 0;

namespace DDI {
namespace Internal {
class DDICorePrivate
{
public:
    DDICorePrivate(DDICore *parent) :
        _initialized(false),
        _atcTableModel(0),
        _drugInteractorTableModel(0),
        _drugDrugInteractionTableModel(0),
        _base(0),
        q(parent)

    {
        Q_UNUSED(q);
    }

    ~DDICorePrivate()
    {
        if (_base)
            delete _base;
        _base = 0;
    }

public:
    bool _initialized;
    AtcTableModel *_atcTableModel;
    DrugInteractorTableModel *_drugInteractorTableModel;
    DrugDrugInteractionTableModel *_drugDrugInteractionTableModel;
    DDIDatabase *_base;

private:
    DDICore *q;
};
} // namespace Internal
} // namespace DDI

/*! Constructor of the DrugsDB::DDICore class */
DDICore::DDICore(QObject *parent) :
    QObject(parent),
    d(new DDICorePrivate(this))
{
    setObjectName("DDICore");
    _instance = this;
}

/** Return the single instance of the object */
DDICore *DDICore::instance()
{
    return _instance;
}

/*! Destructor of the DrugsDB::DDICore class */
DDICore::~DDICore()
{
    if (d)
        delete d;
    d = 0;
}

/*! Initializes the object with the default values. Return true if initialization was completed. */
bool DDICore::initialize()
{
    if (d->_initialized)
        return true;
    d->_base = new DDIDatabase;
    QString dbPath = settings()->path(Core::ISettings::UserDocumentsPath);

#ifdef WITH_TESTS
    // When we are running tests, you should not connect to the user database
    // but to a testing database. We can not put this code in the test/test_*.cpp
    // because all models are created and connect to the ddi database before
    // the test are ran.
    dbPath += "/test";
    QDir().mkpath(dbPath);
    // TODO: Copy non-testing db to the test path before initializing the database
#endif

    d->_base->initialize(dbPath, true);
    d->_atcTableModel = new AtcTableModel(this);
    d->_atcTableModel->initialize();

    d->_drugInteractorTableModel = new DrugInteractorTableModel(this);
    d->_drugInteractorTableModel->initialize();

    d->_drugDrugInteractionTableModel = new DrugDrugInteractionTableModel(this);
    d->_drugDrugInteractionTableModel->initialize();

    d->_initialized = true;
    return true;
}

bool DDICore::recreateDatabase()
{
    QString dbFileName = QString("%1/%2/%3").arg(settings()->path(Core::ISettings::UserDocumentsPath))
            .arg(Constants::DDIMANAGER_DATABASE_NAME)
            .arg(Constants::DDIMANAGER_DATABASE_FILENAME);;
    LOG(QString("Removing file %1").arg(dbFileName));
    if (QFile(dbFileName).exists()) {
        if (QFile(dbFileName).remove())
            LOG("Removed");
    }

    d->_base->forceFullDatabaseRefreshing();
    if (!d->_base->initialize(settings()->databasePath(), true)) {
        LOG_ERROR("Unable to initialize database");
        return false;
    }
    return true;
}

/**
 * Create a backup of the current database to the specified path.
 * Returns the absolute file path of the backuped database or a null QString
 * in case of an error.
 */
QString DDICore::backupDatabaseTo(const QString &absPath)
{
    if (QDir(absPath).isRelative())
        return QString::null;

    if (!QDir(absPath).exists())
        QDir().mkpath(absPath);
    QString fileName = QDir::cleanPath(QString("%1/%2_%3.db")
                                       .arg(absPath)
                                       .arg(qApp->applicationName().toLower().remove("_debug").remove("_alpha"))
                                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
    QFile currentDb(d->_base->database().databaseName());

    if (currentDb.exists())
        if (currentDb.copy(fileName))
            return fileName;

    return QString::null;
}

/** Returns the Atc table model single instance */
AtcTableModel *DDICore::atcTableModel() const
{
    return d->_atcTableModel;
}

DrugInteractorTableModel *DDICore::drugInteractorTableModel() const
{
    return d->_drugInteractorTableModel;
}

DrugDrugInteractionTableModel *DDICore::drugDrugInteractionTableModel() const
{
    return d->_drugDrugInteractionTableModel;
}

/** Returns the DDI database single instance */
Internal::DDIDatabase &DDICore::database() const
{
    return *d->_base;
}
