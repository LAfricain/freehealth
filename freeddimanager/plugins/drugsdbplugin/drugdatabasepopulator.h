/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2016 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *  along with this program.                                               *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main developers : Eric Maeker
 *  Contributors:                                                          *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef DDIMANAGER_DRUGSDBPLUGIN_INTERNAL_DATABASEPOPULATOR_H
#define DDIMANAGER_DRUGSDBPLUGIN_INTERNAL_DATABASEPOPULATOR_H

#include <QObject>
#include <QMultiHash>

/**
 * \file drugdatabasepopulator.h
 * \author Eric Maeker
 * \version 0.10.0
 * \date 16 Jan 2014
*/

namespace DrugsDB {
namespace Internal {
class DrugBaseEssentials;
}
}

namespace DrugsDb {
namespace Internal {
class DrugDatabasePopulatorPrivate;

class DrugDatabasePopulator : public QObject
{
    Q_OBJECT

public:
    explicit DrugDatabasePopulator(QObject *parent = 0);
    bool initialize();

public:
    ~DrugDatabasePopulator();

    bool saveAtcClassification(DrugsDB::Internal::DrugBaseEssentials *database);
    bool saveDrugDrugInteractions(DrugsDB::Internal::DrugBaseEssentials *database);
    bool saveComponentAtcLinks(DrugsDB::Internal::DrugBaseEssentials *database, const QMultiHash<int, int> &componentIdToAtcId, const int sid);

private:
    Internal::DrugDatabasePopulatorPrivate *d;
};

} // namespace Internal
} // namespace DrugsDb

#endif // DDIMANAGER_DRUGSDBPLUGIN_INTERNAL_DATABASEPOPULATOR_H

