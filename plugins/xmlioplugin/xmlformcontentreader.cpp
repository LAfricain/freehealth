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
#include "xmlformcontentreader.h"
#include "constants.h"
#include "xmliobase.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/log.h>
#include <utils/global.h>
#include <utils/genericupdateinformation.h>
#include <translationutils/constants.h>
#include <translationutils/trans_filepathxml.h>
#include <translationutils/trans_msgerror.h>

#include <coreplugin/icore.h>
#include <coreplugin/ipatient.h>
#include <coreplugin/isettings.h>
#include <coreplugin/constants_tokensandsettings.h>

#include <formmanagerplugin/iformitem.h>
#include <formmanagerplugin/iformwidgetfactory.h>
#include <formmanagerplugin/iformio.h>

#include <categoryplugin/categoryitem.h>
#include <categoryplugin/categorycore.h>

#include <pmhplugin/pmhcore.h>
#include <pmhplugin/pmhcategorymodel.h>

#include <QApplication>
#include <QDir>
#include <QTextCodec>
#include <QFileInfo>
#include <QDomDocument>
#include <QDomElement>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptSyntaxCheckResult>

#include <QDebug>

using namespace XmlForms;
using namespace Internal;
using namespace Trans::ConstantTranslations;

//inline static Form::FormManager *formManager() { return Form::FormManager::instance(); }
inline static ExtensionSystem::PluginManager *pluginManager() {return ExtensionSystem::PluginManager::instance();}
static inline Core::ISettings *settings()  { return Core::ICore::instance()->settings(); }
static inline Category::CategoryCore *categoryCore() {return  Category::CategoryCore::instance();}
static inline PMH::PmhCore *pmhCore() {return PMH::PmhCore::instance();}
static inline Internal::XmlFormContentReader *reader() {return Internal::XmlFormContentReader::instance();}
static inline Internal::XmlIOBase *base() {return Internal::XmlIOBase::instance();}

static QString getNamespace(const Form::FormItem *item)
{
    QStringList ns;
    // Add all Form::FormMain Namespace
    Form::FormMain *form = item->parentFormMain();
    while (form) {
        if (form->useNameAsNSForSubItems())
            ns.prepend(form->uuid());
        form = form->parentFormMain();
    }
    if (ns.isEmpty())
        return QString::null;
    return ns.join("::") + "::";
}

XmlFormContentReader *XmlFormContentReader::m_Instance = 0;
XmlFormContentReader *XmlFormContentReader::instance()
{
    if (!m_Instance)
        m_Instance = new XmlFormContentReader;
    return m_Instance;
}

XmlFormContentReader::XmlFormContentReader() :
    m_Mute(false)
{
   m_ScriptsTypes.clear();
   m_ScriptsTypes.insert(Constants::TAG_SCRIPT_ONLOAD, Form::FormItemScripts::Script_OnLoad);
   m_ScriptsTypes.insert(Constants::TAG_SCRIPT_POSTLOAD, Form::FormItemScripts::Script_PostLoad);
   m_ScriptsTypes.insert(Constants::TAG_SCRIPT_ONDEMAND, Form::FormItemScripts::Script_OnDemand);
   m_ScriptsTypes.insert(Constants::TAG_SCRIPT_ONVALUECHANGED, Form::FormItemScripts::Script_OnValueChanged);
   m_ScriptsTypes.insert(Constants::TAG_SCRIPT_ONVALUEREQUIERED, Form::FormItemScripts::Script_OnValueRequiered);
   m_ScriptsTypes.insert(Constants::TAG_SCRIPT_ONDEPENDENCIESCHANGED, Form::FormItemScripts::Script_OnDependentValueChanged);
   m_ScriptsTypes.insert(Constants::TAG_SCRIPT_ONCLICKED, Form::FormItemScripts::Script_OnClicked);

   m_ValuesTypes.clear();
   m_ValuesTypes.insert(Constants::TAG_VALUE_UUID, Form::FormItemValues::Value_Uuid);
   m_ValuesTypes.insert(Constants::TAG_VALUE_NUMERICAL, Form::FormItemValues::Value_Numerical);
   m_ValuesTypes.insert(Constants::TAG_VALUE_SCRIPT, Form::FormItemValues::Value_Script);
   m_ValuesTypes.insert(Constants::TAG_VALUE_POSSIBLE, Form::FormItemValues::Value_Possible);
   m_ValuesTypes.insert(Constants::TAG_VALUE_DEPENDENCIES, Form::FormItemValues::Value_Dependency);
   m_ValuesTypes.insert(Constants::TAG_VALUE_PRINT, Form::FormItemValues::Value_Printing);
   m_ValuesTypes.insert(Constants::TAG_VALUE_DEFAULT, Form::FormItemValues::Value_Default);

   m_SpecsTypes.clear();
   m_SpecsTypes.insert(Constants::TAG_SPEC_PLUGINNAME, Form::FormItemSpec::Spec_Plugin);
   m_SpecsTypes.insert(Constants::TAG_SPEC_AUTHORS, Form::FormItemSpec::Spec_Author);
   m_SpecsTypes.insert(Constants::TAG_SPEC_CATEGORY, Form::FormItemSpec::Spec_Category);
   m_SpecsTypes.insert(Constants::TAG_SPEC_LICENSE, Form::FormItemSpec::Spec_License);
   m_SpecsTypes.insert(Constants::TAG_SPEC_CREATIONDATE, Form::FormItemSpec::Spec_CreationDate);
   m_SpecsTypes.insert(Constants::TAG_SPEC_BIBLIOGRAPHY, Form::FormItemSpec::Spec_Bibliography);
   m_SpecsTypes.insert(Constants::TAG_SPEC_DESCRIPTION, Form::FormItemSpec::Spec_Description);
   m_SpecsTypes.insert(Constants::TAG_SPEC_LABEL, Form::FormItemSpec::Spec_Label);
   m_SpecsTypes.insert(Constants::TAG_SPEC_VERSION, Form::FormItemSpec::Spec_Version);
   m_SpecsTypes.insert(Constants::TAG_SPEC_ICON, Form::FormItemSpec::Spec_IconFileName);
   m_SpecsTypes.insert(Constants::TAG_SPEC_TOOLTIP, Form::FormItemSpec::Spec_Tooltip);

   m_PatientDatas.clear();
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_DRUGSALLERGIES, Core::IPatient::DrugsAtcAllergies);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_DRUGSCHRONIC, Core::IPatient::DrugsChronicTherapeutics);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_WEIGHT, Core::IPatient::Weight);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_WEIGHTUNIT, Core::IPatient::WeightUnit);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_HEIGHT, Core::IPatient::Height);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_HEIGHTUNIT, Core::IPatient::HeightUnit);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_CREAT, Core::IPatient::Creatinine);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_CREATUNIT, Core::IPatient::CreatinineUnit);

   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_EMAIL, Core::IPatient::Mails);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_FAX, Core::IPatient::Faxes);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_TELS, Core::IPatient::Tels);
   m_PatientDatas.insert(Constants::TAG_DATAPATIENT_MOBILETEL, Core::IPatient::MobilePhone);
}

XmlFormContentReader::~XmlFormContentReader()
{}

void XmlFormContentReader::clearCache()
{
    m_ReadableForms.clear();
    m_DomDocFormCache.clear();
    m_ActualForm = 0;
}

/** Refresh the IFormWidgetFactories from the plugin manager */
void XmlFormContentReader::refreshPluginFactories()
{
    m_PlugsFactories.clear();
    foreach(Form::IFormWidgetFactory *fact, pluginManager()->getObjects<Form::IFormWidgetFactory>()) {
        foreach(const QString &widgetname, fact->providedWidgets()) {
           m_PlugsFactories.insert(widgetname,fact);
        }
    }
}

/** Return true is formUid is already tested and present in cache */
bool XmlFormContentReader::isInCache(const QString &formUid) const
{
    return m_ReadableForms.contains(formUid);
}

/** Return the cached readable QDomDocument from the cache system or 0 if the \e formuid is not present in cache. */
QDomDocument *XmlFormContentReader::fromCache(const QString &formUid) const
{
    if (m_DomDocFormCache.contains(formUid))
        return m_DomDocFormCache[formUid];
//    qWarning() << "NOT IN CACHE" << formUid << m_DomDocFormCache.keys();
    return 0;
}

/** Warn XML reading errors */
void XmlFormContentReader::warnXmlReadError(bool muteUserWarnings, const QString &file, const QString &msg, const int line, const int col) const
{
    QString m = Trans::ConstantTranslations::tkTr(Trans::Constants::FILE_1_ISNOT_READABLE).arg(file) + " ; " +
                  Trans::ConstantTranslations::tkTr(Trans::Constants::ERROR_1_LINE_2_COLUMN_3)
                  .arg(msg).arg(line).arg(col);
    LOG_ERROR_FOR("XmlFormContentReader", m);
    m_Error.append(Trans::ConstantTranslations::tkTr(Trans::Constants::ERROR_1_LINE_2_COLUMN_3)
                   .arg(msg).arg(line).arg(col));

    if (!muteUserWarnings)
        Utils::warningMessageBox(
            Trans::ConstantTranslations::tkTr(Trans::Constants::FILE_1_ISNOT_READABLE).arg(file),
            Trans::ConstantTranslations::tkTr(Trans::Constants::ERROR_1_LINE_2_COLUMN_3)
            .arg(msg).arg(line).arg(col),"",qApp->applicationName());
}

/** Check if the xmlContent is conform to the XML Form IO. formUid must be an absolute path if corresponding to a file. */
bool XmlFormContentReader::checkFileContent(const QString &formUidOrFullAbsPath, const QString &contents) const
{
//    qWarning() << "CHECK" << formUidOrFullAbsPath;
    bool ok = true;
    if (contents.isEmpty()) {
        if (!m_Mute)
            Utils::warningMessageBox(tkTr(Trans::Constants::FILE_1_ISEMPTY).arg(formUidOrFullAbsPath),"",qApp->applicationName());
        m_Error.append(tkTr(Trans::Constants::FILE_1_ISEMPTY).arg(formUidOrFullAbsPath));
        return false;
    }
    // Check XML files
    if (formUidOrFullAbsPath.endsWith(".xml", Qt::CaseInsensitive)) {
        if (contents.count("<"+QString(Constants::TAG_NEW_FORM)) != contents.count("</"+QString(Constants::TAG_NEW_FORM)+">")) {
            ok = false;
            m_Error.append(tkTr(Trans::Constants::XML_WRONG_NUMBER_OF_TAG_1).arg(Constants::TAG_NEW_FORM));
            LOG_ERROR_FOR("XmlFormContentReader", Trans::ConstantTranslations::tkTr(Trans::Constants::FILE_1_ISNOT_READABLE).arg(formUidOrFullAbsPath));
        }
        if ((contents.count(QString("<%1>").arg(Constants::TAG_MAINXMLTAG)) != 1) ||
                (contents.count(QString("</%1>").arg(Constants::TAG_MAINXMLTAG)) != 1)) {
            m_Error.append(tkTr(Trans::Constants::XML_WRONG_NUMBER_OF_TAG_1).arg(Constants::TAG_NEW_FORM));
            ok = false;
        }

        // load document
        QString errorMsg;
        int errorLine, errorColumn;
        QDomDocument *doc = new QDomDocument;
        if (!doc->setContent(contents, &errorMsg, &errorLine, &errorColumn)) {
            warnXmlReadError(m_Mute, formUidOrFullAbsPath, errorMsg, errorLine, errorColumn);
            ok = false;
        }

        // Check doctype name
        if (doc->doctype().name().compare(Constants::DOCTYPE_NAME,Qt::CaseInsensitive)!=0) {
            const QString &error = tkTr(Trans::Constants::XML_DOCUMENT_TYPE_MISMATCH);
            m_Error << error;
            warnXmlReadError(m_Mute, formUidOrFullAbsPath, error);
            ok = false;
        }
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        if (ok)
            m_DomDocFormCache.insert(formUidOrFullAbsPath, doc);
    }
    // Check JS files
    else if (formUidOrFullAbsPath.endsWith(".js", Qt::CaseInsensitive)) {
        QScriptEngine engine;
        QScriptSyntaxCheckResult result = engine.checkSyntax(contents);
        if (result.state()==QScriptSyntaxCheckResult::Error) {
            QString msg = QString("Script error: (%1:%2) %3 -- in file %4")
                    .arg(result.errorLineNumber())
                    .arg(result.errorColumnNumber())
                    .arg(result.errorMessage())
                    .arg(formUidOrFullAbsPath);
            if (!m_Mute)
                Utils::warningMessageBox(msg, "", qApp->applicationName());
            m_Error.append(msg);
            ok = false;
        }
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    return ok;
}

static void setPathToDescription(QString path, Form::FormIODescription *desc)
{
    path.replace(settings()->path(Core::ISettings::CompleteFormsPath), Core::Constants::TAG_APPLICATION_COMPLETEFORMS_PATH);
    path.replace(settings()->path(Core::ISettings::SubFormsPath), Core::Constants::TAG_APPLICATION_SUBFORMS_PATH);
    desc->setData(Form::FormIODescription::UuidOrAbsPath, path);
}

QMultiHash<QString, QString> XmlFormContentReader::readUuidEquivalence(const QDomDocument *doc) const
{
    QMultiHash<QString, QString> oldToNew;
    QDomElement item = doc->firstChildElement(Constants::TAG_MAINXMLTAG);
    item = item.firstChildElement(Constants::TAG_UUID_EQUIVALENCE_MAIN);
    item = item.firstChildElement(Constants::TAG_UUID_EQUIVALENCE_ITEM);
    while (!item.isNull()) {
        if (item.attribute(Constants::ATTRIB_OLD_UUID).isEmpty() || item.attribute(Constants::ATTRIB_NEW_UUID).isEmpty()) {
            LOG_ERROR_FOR("XmlFormContentReader", "Field equivalence missing old/new attributes");
            continue;
            item = item.nextSiblingElement(Constants::TAG_UUID_EQUIVALENCE_ITEM);
        }
        oldToNew.insertMulti(item.attribute(Constants::ATTRIB_OLD_UUID), item.attribute(Constants::ATTRIB_NEW_UUID));
        item = item.nextSiblingElement(Constants::TAG_UUID_EQUIVALENCE_ITEM);
    }
    return oldToNew;
}

/** Return the Form::FormIODescription according to the XML QDomElement \e xmlDescr. The \e xmlDescr must point to the first description tag of the document. */
Form::FormIODescription *XmlFormContentReader::readXmlDescription(const QDomElement &xmlDescr, const QString &formUid)
{
    Form::FormIODescription *ioDesc = new Form::FormIODescription;
    ioDesc->setRootTag(Constants::TAG_FORM_DESCRIPTION);
    ioDesc->fromDomElement(xmlDescr);
    setPathToDescription(formUid, ioDesc);
    return ioDesc;
}

/** Return the Form description. Call this member just after the checkFormFileContent() so that the form xml content will be cached */
Form::FormIODescription *XmlFormContentReader::readFileInformation(const QString &formUidOrFullAbsPath, const Form::FormIOQuery &query)
{
    Form::FormIODescription *toReturn = 0;
    // Get the QDomDocument from cache
    QDomDocument *doc = m_DomDocFormCache[formUidOrFullAbsPath];
    if (!doc) {
        LOG_ERROR_FOR("XmlFormContentReader", "No document in cache call canReadForm before. Form: " + formUidOrFullAbsPath);
        return toReturn;
    }

    // get from cache and read description
    QDomElement root = doc->documentElement();
    root = root.firstChildElement(Constants::TAG_FORM_DESCRIPTION);
    toReturn = readXmlDescription(root, formUidOrFullAbsPath);

    // get screenshots if requiered
    XmlFormName form(formUidOrFullAbsPath);
    if (query.getScreenShots()) {
        if (query.forceFileReading()) {
            // Get from local files
            QString shotPath = form.absPath + QDir::separator() + "shots" + QDir::separator();
            QStringList lang;
            lang << QLocale().name().left(2).toLower() << "en" << "xx" << "all";
            bool found = false;
            foreach(const QString &l, lang) {
                if (QDir(shotPath + l).exists()) {
                    found = true;
                    shotPath = shotPath + l;
                    break;
                }
            }
            if (found) {
                QDir dir(shotPath);
                //            qWarning() << "Trying to read shots" << dir.absolutePath();
                foreach(const QFileInfo &file, dir.entryInfoList(QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.gif")) {
                    QPixmap pix(file.absoluteFilePath());
                    toReturn->addScreenShot(file.absoluteFilePath().remove(shotPath), pix);
                }
            }
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        } else {
            // Get from database
            QHash<QString, QPixmap> pix = base()->getScreenShots(form.uid, QLocale().name().left(2).toLower());
//            qWarning() << "xxxxxxxx FROMBASE" << formUidOrFullAbsPath << pix.keys();
            foreach(const QString &k, pix.keys()) {
                toReturn->addScreenShot(k, pix.value(k));
            }
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }
    return toReturn;
}

QList<Form::FormIODescription *> XmlFormContentReader::getFormFileDescriptions(const Form::FormIOQuery &query)
{
    Q_UNUSED(query);
    QList<Form::FormIODescription *> toReturn;

    // TODO: code here

//    QString startPath;
//    if (query.typeOfForms() & Form::FormIOQuery::UserForms) {
//        // TODO: manage user forms path and default path
//    } else {
//        // TODO: manage user forms path and default path
//    }
//    if (query.typeOfForms() & Form::FormIOQuery::CompleteForms) {
//        startPath = settings()->path(Core::ISettings::CompleteFormsPath);
//        getAllFormsFromDir(startPath, &toReturn);
//        for(int i = 0; i < toReturn.count(); ++i) {
//            toReturn.at(i)->setData(Form::FormIODescription::IsCompleteForm, true);
//        }
//    }
//    if (query.typeOfForms() & Form::FormIOQuery::SubForms) {
//        startPath = settings()->path(Core::ISettings::SubFormsPath);
//        getAllFormsFromDir(startPath, &toReturn);
//        for(int i = 0; i < toReturn.count(); ++i) {
//            toReturn.at(i)->setData(Form::FormIODescription::IsSubForm, true);
//        }
//    }
//    // TODO: Add IFormIO to descr && check all forms for params of Query
//    for(int i = 0; i < toReturn.count(); ++i) {
//        toReturn.at(i)->setIoFormReader(this);
//    }
    return toReturn;
}

bool XmlFormContentReader::loadForm(const XmlFormName &form, Form::FormMain *rootForm)
{
//    qWarning() << Q_FUNC_INFO << form.uid << form.absFileName << m_ActualForm;

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QDomDocument *doc = 0;
    if (!m_DomDocFormCache.keys().contains(form.absFileName)) {
        LOG_ERROR_FOR("XmlFormContentReader","Form not in cache: " + form.absFileName);
        return false;
    }
    doc = m_DomDocFormCache[form.absFileName];
    QDomElement root = doc->firstChildElement(Constants::TAG_MAINXMLTAG);
    QDomElement newForm = root.firstChildElement(Constants::TAG_NEW_FORM);
//    QDomElement newMode = root.firstChildElement(Constants::TAG_NEW_MODE);
    QDomElement addFile = root.firstChildElement(Constants::TAG_ADDFILE);

    // in case of no rootForm is passed --> XML must start with a file inclusion or a newform tag
    if (!rootForm) {
        if (addFile.isNull() && newForm.isNull()) {
            warnXmlReadError(m_Mute, form.uid, tkTr(Trans::Constants::XML_WRONG_ROOT_TAG_1_2).arg(root.tagName()).arg(Constants::TAG_NEW_FORM));
            return false;
        }
//        rootForm = createNewForm(newForm, m_ActualForm);
    }
    m_ActualForm = rootForm;

    if (!loadElement(rootForm, root, form)) {
        LOG_ERROR_FOR("XmlFormContentReader", "Unable to load form " + form.uid);
        return false;
    }

//    rootForm->createDebugPage();
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    createWidgets(rootForm);

    // Manage uuid equivalence
    QMultiHash<QString, QString> oldToNew = readUuidEquivalence(doc);
    if (!oldToNew.isEmpty()) {
        QStringList newUids = oldToNew.values();
        newUids.removeDuplicates();
        foreach(Form::FormMain *main, rootForm->flattenFormMainChildren()) {
            if (newUids.contains(main->uuid(), Qt::CaseInsensitive)) {
                main->spec()->setEquivalentUuid(oldToNew.keys(main->uuid()));
            }
            foreach(Form::FormItem *item, main->flattenFormItemChildren()) {
                if (newUids.contains(item->uuid(), Qt::CaseInsensitive)) {
                    item->spec()->setEquivalentUuid(oldToNew.keys(item->uuid()));
                }
            }
        }
    }
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    return true;
}

bool XmlFormContentReader::loadElement(Form::FormItem *item, QDomElement &rootElement, const XmlFormName &form)
{
    bool descriptionPassed = false; // for speed improvements
    QDomElement element = rootElement.firstChildElement();
    while (!element.isNull()) {
        int i=0;

        // Do not proceed form description here
        if (!descriptionPassed) {
            if (element.tagName().compare(::Constants::TAG_FORM_DESCRIPTION, Qt::CaseInsensitive)==0) {
                descriptionPassed = true; // speed improvements
                element = element.nextSiblingElement();
                continue;
            }
        }

        // Create a nem FormItem ?
        i = Constants::createTags.indexOf(element.tagName());
        if (i != -1) {
            createElement(item, element, form);
            element = element.nextSiblingElement();
            continue;
        }

        // if there is no item defined then go nextSibling till we find a new form, item, page...

        if (!item) {
            element = element.nextSiblingElement();
            continue;
        }

        // Values ?
        if (element.tagName().compare(Constants::TAG_VALUE, Qt::CaseInsensitive)==0) {
            populateValues(item, element, form);
            element = element.nextSiblingElement();
            continue;
        }

        // Script ?
        if (element.tagName().compare(Constants::TAG_SCRIPT, Qt::CaseInsensitive)==0) {
            populateScripts(item, element, form);
            element = element.nextSiblingElement();
            continue;
        }

        // Spec ?
        i = m_SpecsTypes.value(element.tagName(), -1);
        if (i != -1) {
            QString lang = element.attribute(Constants::ATTRIB_LANGUAGE, Trans::Constants::ALL_LANGUAGE);
            item->spec()->setValue(m_SpecsTypes.value(element.tagName()), element.text(), lang);
            element = element.nextSiblingElement();
            continue;
        }

        // UiLink ?
        if (element.tagName().compare(Constants::TAG_UI_UILINK, Qt::CaseInsensitive)==0) {
            item->spec()->setValue(Form::FormItemSpec::Spec_UiLabel, element.attribute(Constants::ATTRIB_UI_UILINK_LABEL));
            item->spec()->setValue(Form::FormItemSpec::Spec_UiWidget, element.attribute(Constants::ATTRIB_UI_UILINK_WIDGET));
            item->spec()->setValue(Form::FormItemSpec::Spec_UiInsertIntoLayout, element.attribute(Constants::ATTRIB_UI_UILINK_INSERTINTOLAYOUT));
            element = element.nextSiblingElement();
            continue;
        }

        // Name/UUID ?
        if ((element.tagName().compare(Constants::ATTRIB_NAME, Qt::CaseInsensitive)==0) ||
            (element.tagName().compare(Constants::ATTRIB_UUID, Qt::CaseInsensitive)==0)) {
            QString uidNS = getNamespace(item);
            item->setUuid(uidNS + element.text());
            element = element.nextSiblingElement();
            continue;
        }

        // Patient Data Representation ?
        if (element.tagName().compare(Constants::TAG_DATAPATIENT, Qt::CaseInsensitive)==0) {
            i = m_PatientDatas.value(element.text(), -1);
            if (i != -1) {
                item->setPatientDataRepresentation(i);
            }
            element = element.nextSiblingElement();
            continue;
        }

    //             // optional?
//            if (element.hasAttribute(Constants::ATTRIB_OPTIONNAL)) {
//                if (element.attribute(Constants::ATTRIB_OPTIONNAL).compare("true",Qt::CaseInsensitive)==0)
//                    item->valueReferences()->setOptionnal(true);
//                else
//                    item->valueReferences()->setOptionnal(false);

        // Add a form file ?
        if (element.tagName().compare(Constants::TAG_ADDFILE, Qt::CaseInsensitive)==0) {
            addFile(element, form);
            element = element.nextSiblingElement();
            continue;
        }

        // Options
        if (element.tagName().compare(Constants::TAG_OPTIONS, Qt::CaseInsensitive)==0 ||
                element.tagName().compare(Constants::TAG_OPTION, Qt::CaseInsensitive)==0) {
            const QString &options = element.text();
            if (options.contains(Constants::TAG_OPTIONS_UNIQUE_EPISODE, Qt::CaseInsensitive)) {
                if (item==m_ActualForm)
                    m_ActualForm->setEpisodePossibilities(Form::FormMain::UniqueEpisode);
            } else if (options.contains(Constants::TAG_OPTIONS_NO_EPISODE, Qt::CaseInsensitive)) {
                if (item==m_ActualForm)
                    m_ActualForm->setEpisodePossibilities(Form::FormMain::NoEpisode);
            }
            if (options.contains(Constants::OPTION_USEFORMNAMEASNS, Qt::CaseInsensitive)) {
                if (item==m_ActualForm)
                    m_ActualForm->setUseNameAsNSForSubItems(true);
            }
            item->addExtraData(element.tagName(), options);
            element = element.nextSiblingElement();
            continue;
        }

        // All others add them in extraData
        item->addExtraData(element.tagName(), element.text());
//        qWarning() << "°°°°°°°°°°°°°° XML adding other tag" << element.tagName() << element.text() << item->extraDatas();

        element = element.nextSiblingElement();
    }
    return true;
}

bool XmlFormContentReader::createElement(Form::FormItem *item, QDomElement &element, const XmlFormName &form)
{
//    qWarning() << "XmlFormIO create element" << m_ActualForm->useNameAsNSForSubItems() << m_ActualForm->uuid();
    // new item
    if (element.tagName().compare(Constants::TAG_NEW_ITEM, Qt::CaseInsensitive)==0) {
        if (item) {
            Form::FormItem *child = item->createChildItem();
            QString uidNS = getNamespace(child);
            // read attributes (type, uid/name, patient representation...)
            if (element.hasAttribute(Constants::ATTRIB_UUID))
                child->setUuid(uidNS + element.attribute(Constants::ATTRIB_UUID));

            if (element.hasAttribute(Constants::ATTRIB_NAME))
                child->setUuid(uidNS + element.attribute(Constants::ATTRIB_NAME));

            if (element.hasAttribute(Constants::ATTRIB_TYPE))
                child->spec()->setValue(Form::FormItemSpec::Spec_Plugin, element.attribute(Constants::ATTRIB_TYPE), Trans::Constants::ALL_LANGUAGE);

            if (element.hasAttribute(Constants::ATTRIB_PATIENTREPRESENTATION)) {
                int i = m_PatientDatas.value(element.attribute(Constants::ATTRIB_PATIENTREPRESENTATION), -1);
                if (i != -1) {
                    child->setPatientDataRepresentation(i);
                }
            }
            loadElement(child, element, form);
            return true;
        }
        else
            return false; // XML ERROR --> an item MUST have a parent
    }

    // new form
    if (element.tagName().compare(Constants::TAG_NEW_FORM, Qt::CaseInsensitive)==0) {
        // create a new form
        Form::FormMain *oldRootForm = m_ActualForm;
        m_ActualForm = m_ActualForm->createChildForm(element.firstChildElement(Constants::TAG_NAME).text());
        item = m_ActualForm;
        if (item) {
            QString uidNS = getNamespace(m_ActualForm);
            item->spec()->setValue(Form::FormItemSpec::Spec_Plugin, "form", Trans::Constants::ALL_LANGUAGE);
            // read attributes (type, uid/name, patient representation...)
            if (element.hasAttribute(Constants::ATTRIB_UUID))
                item->setUuid(uidNS + element.attribute(Constants::ATTRIB_UUID));

            if (element.hasAttribute(Constants::ATTRIB_NAME))
                item->setUuid(uidNS + element.attribute(Constants::ATTRIB_NAME));

            if (element.hasAttribute(Constants::ATTRIB_TYPE))
                item->spec()->setValue(Form::FormItemSpec::Spec_Plugin, element.attribute(Constants::ATTRIB_TYPE), Trans::Constants::ALL_LANGUAGE);

            if (element.hasAttribute(Constants::ATTRIB_UIFILE)) {
                QString content = base()->getFormContent(form.uid, XmlIOBase::UiFile, element.attribute(Constants::ATTRIB_UIFILE));
                item->spec()->setValue(Form::FormItemSpec::Spec_UiFileContent, content, Trans::Constants::ALL_LANGUAGE);
            }

            loadElement(item, element, form);
            // read specific form's data
            m_ActualForm = oldRootForm;
            return true;
        }
        else
            return false;
    }

    // TODO xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx MODE

//    if (element.tagName().compare(Constants::TAG_NEW_MODE, Qt::CaseInsensitive)==0) {
//        // create a new page
//        item = item->createPage(element.firstChildElement(Constants::TAG_NAME).text());
//        // TODO: add page to a form
//        if (item) {
//            QString uidNS = getNamespace(item);
//            // read attributes (type, uid/name, patient representation...)
//            if (element.hasAttribute(Constants::ATTRIB_UUID))
//                item->setUuid(uidNS + element.attribute(Constants::ATTRIB_UUID));

//            if (element.hasAttribute(Constants::ATTRIB_NAME))
//                item->setUuid(uidNS + element.attribute(Constants::ATTRIB_NAME));

//            if (element.hasAttribute(Constants::ATTRIB_TYPE))
//                item->spec()->setValue(Form::FormItemSpec::Spec_Plugin, element.attribute(Constants::ATTRIB_TYPE), Trans::Constants::ALL_LANGUAGE);

//            loadElement(item, element, form);
//            // read specific page's data
//            return true;
//        }
//        else
//            return false;
//    }

    return false;
}

/** Read the values of an element. */
bool XmlFormContentReader::populateValues(Form::FormItem *item, const QDomElement &root, const XmlFormName &form)
{
    Q_UNUSED(form);
    QDomElement element = root.firstChildElement();
    while (!element.isNull()) {
        const QString &lang = element.attribute(Constants::ATTRIB_LANGUAGE, Trans::Constants::ALL_LANGUAGE);
//        qWarning() << "Values" << root.tagName() << element.tagName() << lang;
        int id = element.attribute(Constants::ATTRIB_ID, 0).toInt();
        const QString &val = element.text();
        int type = m_ValuesTypes.value(element.tagName(), -1);
        if (type==Form::FormItemValues::Value_Printing) {
            // read file
            // TODO: ...
//            if (!element.attribute(Constants::ATTRIB_FILE).isEmpty()) {
//                QString content = readExtraFile(form, element.attribute(Constants::ATTRIB_FILE));
//                item->valueReferences()->setValue(type, id, content, lang);
//            } else {
                item->valueReferences()->setValue(type, id, val, lang);
//            }
        } else if (type != -1) {
            item->valueReferences()->setValue(type, id, val, lang);
        }
        element = element.nextSiblingElement();
    }
    return true;
}

/** Read the scripts of an element. */
bool XmlFormContentReader::populateScripts(Form::FormItem *item, const QDomElement &root, const XmlFormName &form)
{
    Q_UNUSED(form);
    QDomElement element = root.firstChildElement();
    QString lang = root.attribute(Constants::ATTRIB_LANGUAGE, Trans::Constants::ALL_LANGUAGE).left(2);
    while (!element.isNull()) {
        QString script = element.text();
        QString file = root.attribute(Constants::ATTRIB_FILE);
        // TODO: HERE
        int type = m_ScriptsTypes.value(element.tagName().toLower(), Form::FormItemScripts::Script_OnDemand);
        item->scripts()->setScript(type, script, lang);
        element = element.nextSiblingElement();
    }
    return true;
}

/** Read a new file. Called when matched XML tag \e file. */
bool XmlFormContentReader::addFile(const QDomElement &element, const XmlFormName &formName)
{
    XmlFormName form = formName;
    // Get file content from database
    QString fileName = element.text();
    // Guess the type of file according to its extension
    int basetype = XmlIOBase::ExtraFiles;
    if (fileName.endsWith(".xml", Qt::CaseInsensitive)) {
        basetype = XmlIOBase::FullContent;
        if (fileName.startsWith("./")) {
            form.modeName = fileName;
            form.modeName = form.modeName.mid(2);
            form.modeName.chop(4);
        } else {
            form = XmlFormName(fileName);
            fileName = form.modeName;
        }
    }
    else if (fileName.endsWith(".js", Qt::CaseInsensitive))
        basetype = XmlIOBase::ScriptFile;
    else if (fileName.endsWith(".ui", Qt::CaseInsensitive))
        basetype = XmlIOBase::UiFile;
    else if (fileName.endsWith(".html", Qt::CaseInsensitive))
        basetype = XmlIOBase::HtmlFile;
//    else if (fileName.endsWith(".qml", Qt::CaseInsensitive))
//        basetype = XmlIOBase::QmlContent;

    QString content = base()->getFormContent(form.uid, basetype, fileName);
    if (content.isEmpty()) {
        LOG_ERROR_FOR("XmlReader", "Content not in base??? " + form.uid + "; mode:" + form.modeName + ";filename:" + fileName);
        Q_ASSERT(false);
    }

    fileName = element.text();
    fileName = fileName.replace(Core::Constants::TAG_APPLICATION_COMPLETEFORMS_PATH, settings()->path(Core::ISettings::CompleteFormsPath), Qt::CaseInsensitive);
    fileName = fileName.replace(Core::Constants::TAG_APPLICATION_SUBFORMS_PATH, settings()->path(Core::ISettings::SubFormsPath), Qt::CaseInsensitive);
    if (QFileInfo(fileName).isRelative())
        fileName.prepend(QFileInfo(form.absFileName).absolutePath() + QDir::separator());
    fileName = QDir::cleanPath(fileName);
//    QString content = Utils::readTextFile(fileName, Utils::DontWarnUser);

    // Check file content for script addition
    const QString &type = element.attribute(Constants::ATTRIB_TYPE);
    if (type.compare(Constants::FILETYPE_SCRIPT, Qt::CaseInsensitive)==0) {
        if (checkFileContent(fileName, content)) {
            // Add script to the empty root FormMain
            Form::FormMain *parent = m_ActualForm;
            while (m_ActualForm->parentFormMain()) {
                parent = m_ActualForm->parentFormMain();
            }
            const QString &lang = element.attribute(Constants::ATTRIB_LANGUAGE, Trans::Constants::ALL_LANGUAGE);
            const QString &scriptType = element.attribute(Constants::ATTRIB_NAME, Trans::Constants::ALL_LANGUAGE);

//            qWarning()<< "\n\n" << lang << type << scriptType << parent->uuid();

            if (scriptType.compare(Constants::TAG_SCRIPT_ONLOAD, Qt::CaseInsensitive)==0)
                parent->scripts()->setScript(Form::FormItemScripts::Script_OnLoad, content, lang);
            else if (scriptType.compare(Constants::TAG_SCRIPT_POSTLOAD, Qt::CaseInsensitive)==0)
                parent->scripts()->setScript(Form::FormItemScripts::Script_PostLoad, content, lang);
            return true;
        }
    }

    // Check file content (for forms file)
    if (checkFileContent(fileName, content)) {
        if (!loadForm(fileName, m_ActualForm)) {
            LOG_ERROR_FOR("XmlReader", "Unable to add form file " + element.text());
            return false;
        } else {
            // TODO: code here
//            saveFormToDatabase(fileName, XmlIOBase::FullContent, content);
            return true;

        }
    }
    return true;
}

bool XmlFormContentReader::createItemWidget(Form::FormItem *item, QWidget *parent)
{
//    qWarning() << Q_FUNC_INFO;
//    qWarning() << item << item->uuid() << item->spec()->pluginName();
    QString requestedWidget = item->spec()->pluginName().toLower();
    Form::IFormWidgetFactory *factory = m_PlugsFactories.value(requestedWidget);
    Form::IFormWidget *w = 0;

    // does plugin was inform in the xml file ?
    if (item->spec()->pluginName().isEmpty()) {
        LOG_ERROR_FOR("XmlFormContentReader", "No plugin name for item: " + item->uuid());
        factory = m_PlugsFactories.value("helptext");
        w = factory->createWidget("helptext", item);
        item->spec()->setValue(Form::FormItemSpec::Spec_Label, QString("XML FORM ERROR: Requested widget does not exist for item '%1'").arg(item->uuid()));
        return false;
    }

    // does requested widget exist in the plugins ?
    factory = m_PlugsFactories.value(requestedWidget);
    if (!m_PlugsFactories.keys().contains(requestedWidget)) {
        LOG_ERROR_FOR("XmlFormContentReader", QString("Form error in item '%1': Requested widget '%2' does not exist in plugin's widgets list.").arg(item->uuid()).arg(requestedWidget));
        // Add a default widget for the error log
        factory = m_PlugsFactories.value("helptext");
        item->spec()->setValue(Form::FormItemSpec::Spec_Label, QString("XML FORM ERROR: Requested widget does not exist for item '%1'").arg(item->uuid()));
        w = factory->createWidget("helptext", item);
        return false;
    }

    // get the widget
    w = factory->createWidget(requestedWidget, item);
    if (w->isContainer()) {
        foreach(Form::FormItem *child, item->formItemChildren()) {
//            Form::IFormWidget *wchild = factory->createWidget(child->spec()->pluginName(),child,w);
//            w->addWidgetToContainer(wchild);
//            child->setFormWidget(wchild);
            createItemWidget(child, w);
        }
    }
    Form::IFormWidget *p = qobject_cast<Form::IFormWidget*>(parent);
    if (p)
        p->addWidgetToContainer(w);
    return true;
}

bool XmlFormContentReader::createFormWidget(Form::FormMain *form)
{
    if (!createItemWidget(form, 0))
        return false;
    return true;
}

bool XmlFormContentReader::createWidgets(const Form::FormMain *rootForm)
{
    // foreach FormMain children
    foreach(Form::FormMain *form, rootForm->flattenFormMainChildren()) {
        // create the form
        createFormWidget(form);
    }
    return true;
}

bool XmlFormContentReader::loadPmhCategories(const QString &uuidOrAbsPath)
{
    Q_UNUSED(uuidOrAbsPath);
//    QString file = QFileInfo(uuidOrAbsPath).absolutePath() + QDir::separator() + Constants::PMHXCATOGORIES_FILENAME;
//    // replace path TAGs
//    file.replace(Core::Constants::TAG_APPLICATION_COMPLETEFORMS_PATH, settings()->path(Core::ISettings::CompleteFormsPath));
//    file.replace(Core::Constants::TAG_APPLICATION_SUBFORMS_PATH, settings()->path(Core::ISettings::SubFormsPath));
//    file.replace(Core::Constants::TAG_APPLICATION_RESOURCES_PATH, settings()->path(Core::ISettings::BundleResourcesPath));

//    QDomDocument *doc = 0;
//    if (!reader()->isInCache(file)) {
//        if (!canReadForms(file)) {
//            LOG_ERROR("Unable to read Pmh Category file: " + file);
//            return false;
//        }
//    }
//    categoryCore()->removeAllExistingCategories("PMHx");
//    doc = reader()->fromCache(file);
//    Q_ASSERT(doc);
//    if (!doc) {
//        LOG_ERROR("No category document in XmlFormIO::loadPmhCategories("+file+")");
//        return false;
//    }
//    QDomElement root = doc->firstChildElement(Constants::TAG_MAINXMLTAG);
//    QDomElement element = root.firstChildElement(Constants::TAG_PMHX_CATEGORIES);
//    element = element.firstChildElement(::Constants::TAG_CATEGORY);
//    while (!element.isNull()) {
//        createCategory(element, 0);
//        element = element.nextSiblingElement(::Constants::TAG_CATEGORY);
//    }
//    pmhCore()->pmhCategoryModel()->refreshFromDatabase();
    return true;
}

bool XmlFormContentReader::createCategory(const QDomElement &element, Category::CategoryItem *parent)
{
    Q_UNUSED(element);
    Q_UNUSED(parent);
//    // create the category
//    Category::CategoryItem *item = new Category::CategoryItem;
//    item->setData(Category::CategoryItem::DbOnly_Mime, "PMHx");
//    item->setData(Category::CategoryItem::ThemedIcon, element.attribute(::Constants::ATTRIB_ICON));

//    // read the labels
//    QDomElement label = element.firstChildElement(::Constants::TAG_SPEC_LABEL);
//    while (!label.isNull()) {
//        item->setLabel(label.text(), label.attribute(::Constants::ATTRIB_LANGUAGE, Trans::Constants::ALL_LANGUAGE));
//        label = label.nextSiblingElement(::Constants::TAG_SPEC_LABEL);
//    }

//    // get ExtraTag content -> CategoryItem::ExtraXml
//    QDomElement extra = element.firstChildElement(::Constants::TAG_SPEC_EXTRA);
//    if (!extra.isNull()) {
//        item->setData(Category::CategoryItem::ExtraXml, extra.toDocument().toString(2));
//    }

//    // save to database
//    if (parent) {
//        parent->addChild(item);
//        item->setParent(parent);
//    }
//    categoryCore()->saveCategory(item);

//    // has children ?
//    QDomElement child = element.firstChildElement(::Constants::TAG_CATEGORY);
//    while (!child.isNull()) {
//        createCategory(child, item);
//        child = child.nextSiblingElement(::Constants::TAG_CATEGORY);
//    }
    return true;
}
