/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2011 by Eric MAEKER, MD (France) <eric.maeker@free.fr>        *
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
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
#ifndef ACCOUNTVIEW_H
#define ACCOUNTVIEW_H

#include <QObject>
#include <QWidget>
#include <QDate>

/**
 * \file accountview.h
 * \author Eric MAEKER <eric.maeker@free.fr>
 * \version 0.1.0
 * \date 03 Apr 2010
*/

namespace Account {
namespace Internal {
class AccountViewPrivate;
}

class AccountView : public QWidget
{
    Q_OBJECT
public:
    AccountView(QWidget *parent);
    ~AccountView();

private:
    void filterChanged();

private Q_SLOTS:
    void on_startDate_dateChanged(const QDate &date);
    void on_endDate_dateChanged(const QDate &date);
    void on_periodCombo_currentIndexChanged(int index);

private:
    Internal::AccountViewPrivate *d;
};


}  // End namespace Account

#endif    // End ACCOUNT_PLUGIN_H
