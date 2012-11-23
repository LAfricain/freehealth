/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2012 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *   Main developers : Eric MAEKER, <eric.maeker@gmail.com>                *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef DRUGSTEMPLATEPRINTER_H
#define DRUGSTEMPLATEPRINTER_H

#include <drugsbaseplugin/drugsbase_exporter.h>
#include <templatesplugin/itemplateprinter.h>

namespace DrugsDB {
class DrugsModel;

namespace Internal {

class DRUGSBASE_EXPORT DrugsTemplatePrinter : public Templates::ITemplatePrinter
{
    Q_OBJECT
public:
    DrugsTemplatePrinter(QObject *parent) : Templates::ITemplatePrinter(parent) {}
    ~ DrugsTemplatePrinter() {}

    QString mimeType() const;
    bool printTemplates(const QList<const Templates::ITemplate *> iTemplates) const;
};

}  // End namespace Internal
}  // End namespace DrugsDB

#endif // DRUGSTEMPLATEPRINTER_H
