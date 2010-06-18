/***************************************************************************
 *   FreeMedicalForms                                                      *
 *   (C) 2008-2010 by Eric MAEKER, MD                                     **
 *   eric.maeker@free.fr                                                   *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   This program is a free and open source software.                      *
 *   It is released under the terms of the new BSD License.                *
 *                                                                         *
 *   Redistribution and use in source and binary forms, with or without    *
 *   modification, are permitted provided that the following conditions    *
 *   are met:                                                              *
 *   - Redistributions of source code must retain the above copyright      *
 *   notice, this list of conditions and the following disclaimer.         *
 *   - Redistributions in binary form must reproduce the above copyright   *
 *   notice, this list of conditions and the following disclaimer in the   *
 *   documentation and/or other materials provided with the distribution.  *
 *   - Neither the name of the FreeMedForms' organization nor the names of *
 *   its contributors may be used to endorse or promote products derived   *
 *   from this software without specific prior written permission.         *
 *                                                                         *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS     *
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE        *
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,  *
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,  *
 *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      *
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER      *
 *   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT    *
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN     *
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
 *   POSSIBILITY OF SUCH DAMAGE.                                           *
 ***************************************************************************/
/***************************************************************************
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
/**
  \class UserWizard
  \brief Wizard for user creation.
  You can tell tkWiz to create the user itself or use a defined user. createUser() define the
  creation mode. If you set it to false, inform the row of the model to use with setModelRow(). By default,
  tkUserWizard create itself a new user.\n
  Usage :
  \code
    UserWizard wiz;
    wiz.createUser(true);   // will create the user (optionnal)
    wiz.show();
  \endcode

  \todo If row is defined --> populate all wizard pages with users values.
  \ingroup usertoolkit widget_usertoolkit usermanager
*/

#include "userwizard.h"

#include <texteditorplugin/texteditor.h>

#include <printerplugin/printer.h>
#include <printerplugin/textdocumentextra.h>

#include <listviewplugin/stringlistview.h>
#include <listviewplugin/stringlistmodel.h>

#include <coreplugin/icore.h>
#include <coreplugin/isettings.h>
#include <coreplugin/itheme.h>
#include <coreplugin/translators.h>
#include <coreplugin/iuser.h>
#include <coreplugin/constants_icons.h>

#include <utils/log.h>
#include <utils/global.h>
#include <utils/widgets/lineeditechoswitcher.h>

#include <usermanagerplugin/usermodel.h>
#include <usermanagerplugin/widgets/userrightswidget.h>

#include <translationutils/constanttranslations.h>

#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QCoreApplication>
#include <QEvent>
#include <QPushButton>
#include <QList>
#include <QCheckBox>

using namespace UserPlugin;
using namespace Trans::ConstantTranslations;

static inline UserPlugin::UserModel *userModel() { return UserModel::instance(); }
static inline Core::ITheme *theme()  { return Core::ICore::instance()->theme(); }
static inline Core::ISettings *settings()  { return Core::ICore::instance()->settings(); }


QHash<int, QString> UserWizard::m_Papers;

UserWizard::UserWizard(QWidget *parent)
    : QWizard(parent),
      m_Row(-1),
      m_Saved(false),
      m_CreateUser(false)
{
    setPage(LanguageSelectorPage, new UserLanguageSelectorPage(this));
    setPage(LoginPasswordPage, new UserLoginPasswordPage(this));
    setPage(IdentityPage, new UserIdentityPage(this));
    setPage(AdressPage, new UserAdressPage(this));
    setPage(TelsAndMailPage, new UserTelsAndMailPage(this));
    setPage(ProfilPage, new UserProfilPage(this));
    setPage(SpecialiesQualificationsPage, new UserSpecialiesQualificationsPage(this));
    setPage(RightsPage, new UserRightsPage(this));
    setPage(PaperGenericPage, new UserPaperPage("Generic", this));
    setPage(PaperAdministrativePage, new UserPaperPage("Administrative", this));
    setPage(PaperPrescriptionsPage, new UserPaperPage("Prescription", this));

    setWindowTitle(tr("User Creator Wizard"));
    QList<QWizard::WizardButton> layout;
    layout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton
            << QWizard::NextButton << QWizard::FinishButton;
    setButtonLayout(layout);
    //    setAttribute(Qt::WA_DeleteOnClose);
}

void UserWizard::done(int r)
{
    validateCurrentPage();
    if (r == QDialog::Rejected) {
        m_Saved = false;
        bool yes = Utils::yesNoMessageBox(tr("WARNING ! You don't save this user."),
                               tr("If you continue changes will be lost.\n"
                                  "Do you really want to close this dialog ?"),
                               "", tr("Data losing"));
        if (yes)
            QDialog::done(r);
    } else if (m_Saved) {
        QDialog::done(r);
    } else {
        if ((m_CreateUser) || (m_Row == -1)) {
            m_Row = userModel()->rowCount();
            if (!userModel()->insertRows(m_Row, 1)) {
                Utils::warningMessageBox(tr("An error occured during database access."),
                                             tr("Logged errors saved. Please refer to the %1 to manage this error.")
                                       .arg(Utils::Log::saveLog()),
                                       "", tr("Error during database access"));
                QDialog::done(QDialog::Rejected);
            }
        }
        QModelIndex idx;
        idx = userModel()->index(m_Row, Core::IUser::DecryptedLogin);
        userModel()->setData(idx, field("Login"));
        idx = userModel()->index(m_Row, Core::IUser::Password);
        userModel()->setData(idx, UserPlugin::crypt(field("Password").toString()));

        idx = userModel()->index(m_Row, Core::IUser::Name);
        userModel()->setData(idx, field("Name"));
        idx = userModel()->index(m_Row, Core::IUser::SecondName);
        userModel()->setData(idx, field("SecondName"));
        idx = userModel()->index(m_Row, Core::IUser::Surname);
        userModel()->setData(idx, field("Surname"));
        idx = userModel()->index(m_Row, Core::IUser::Title);
        userModel()->setData(idx, field("Title"));
        idx = userModel()->index(m_Row, Core::IUser::Gender);
        userModel()->setData(idx, field("Gender"));

        idx = userModel()->index(m_Row, Core::IUser::Adress);
        userModel()->setData(idx, field("Adress"));
        idx = userModel()->index(m_Row, Core::IUser::Zipcode);
        userModel()->setData(idx, field("Zipcode"));
        idx = userModel()->index(m_Row, Core::IUser::City);
        userModel()->setData(idx, field("City"));
        idx = userModel()->index(m_Row, Core::IUser::Country);
        userModel()->setData(idx, field("Country"));
        idx = userModel()->index(m_Row, Core::IUser::LanguageIndex);
        userModel()->setData(idx, field("Language"));

        idx = userModel()->index(m_Row, Core::IUser::Tel1);
        userModel()->setData(idx, field("Tel1"));
        idx = userModel()->index(m_Row, Core::IUser::Tel2);
        userModel()->setData(idx, field("Tel2"));
        idx = userModel()->index(m_Row, Core::IUser::Tel3);
        userModel()->setData(idx, field("Tel3"));
        idx = userModel()->index(m_Row, Core::IUser::Fax);
        userModel()->setData(idx, field("Fax"));
        idx = userModel()->index(m_Row, Core::IUser::Mail);
        userModel()->setData(idx, field("Mail"));

        idx = userModel()->index(m_Row, Core::IUser::Specialities);
        userModel()->setData(idx, field("Specialities"));
        idx = userModel()->index(m_Row, Core::IUser::Qualifications);
        userModel()->setData(idx, field("Qualifications"));
        idx = userModel()->index(m_Row, Core::IUser::PractitionerId);
        userModel()->setData(idx, field("Identifiants"));

        idx = userModel()->index(m_Row, Core::IUser::ManagerRights);
        userModel()->setData(idx, field("UserManager"));
        idx = userModel()->index(m_Row, Core::IUser::DrugsRights);
        userModel()->setData(idx, field("DrugsManager"));
        idx = userModel()->index(m_Row, Core::IUser::MedicalRights);
        userModel()->setData(idx, field("Medical"));
        idx = userModel()->index(m_Row, Core::IUser::ParamedicalRights);
        userModel()->setData(idx, field("Paramedical"));
        idx = userModel()->index(m_Row, Core::IUser::AdministrativeRights);
        userModel()->setData(idx, field("Administrative"));

        const QString &uuid = userModel()->index(m_Row, Core::IUser::Uuid).data().toString();
        QHashIterator<int, QString> it(m_Papers);
        while (it.hasNext()) {
             it.next();
             userModel()->setPaper(uuid, it.key(), Print::TextDocumentExtra::fromXml(it.value()));
         }


#ifdef DEBUG
        // warn user
        idx = userModel()->index(m_Row, Core::IUser::Warn);
        idx.data();
#endif

        if (userModel()->submitRow(m_Row)) {
            Utils::informativeMessageBox(tr("User correctly saved into database."),
                                             tr("You can poursue with the current user %1 or set this new user to current user.")
                                             .arg(userModel()->currentUserData(Core::IUser::Name).toString()),
                                             "", tr("User correctly saved into database."));
            m_Saved = true;
            QDialog::done(r);
        } else {
            userModel()->removeRows(m_Row, 1);
            Utils::warningMessageBox(tr("An error occured during database access."),
                                         tr("Logged errors saved. Please refer to the %1 to manage this error.")
                                         .arg(Utils::Log::saveLog()),
                                         "", tr("Error during database access"));
            QDialog::done(r);
            m_Saved = false;
        }
    }
}

bool UserWizard::setCreatedUserAsCurrent() const
{
    if (!m_CreateUser)
        return false;
    return userModel()->setCurrentUser(loginForSQL(field("Login").toString()),
                                       crypt(field("Password").toString()));
}



UserLanguageSelectorPage::UserLanguageSelectorPage(QWidget *parent)
    : QWizardPage(parent), lbl(0)
{
    lbl = new QLabel(tr("Language"), this);
    retranslate();
    QComboBox * cbLanguage = new QComboBox(this);

    cbLanguage->addItems(Core::Translators::availableLocales());
    cbLanguage->setCurrentIndex(Core::Translators::availableLocales().indexOf(QLocale().name().left(2)));
    connect(cbLanguage, SIGNAL(activated(QString)), Core::Translators::instance(), SLOT(changeLanguage(const QString &)));

    registerField("Language", cbLanguage , "currentIndex");

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(lbl, 0, 0);
    layout->addWidget(cbLanguage, 0, 1);
    setLayout(layout);
}

void UserLanguageSelectorPage::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        retranslate();
}

void UserLanguageSelectorPage::retranslate()
{
    setTitle(tr("Please set your language."));
    setSubTitle(tr("Whole application will be translated to this language, forms too (when possible).\n"
                     "For instance, FreeMedForms is available in few languages.\n"
                     "New translations are planned."));
    if (lbl)
        lbl->setText(tr("Language"));
}

UserIdentityPage::UserIdentityPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Please enter your identity."));
    QLabel * lblTitle = new QLabel(tr("Title"), this);
    QLabel * lblName = new QLabel(tr("Name"), this);
    QLabel * lblSurName = new QLabel(tr("Surname"), this);
    QLabel * lblSecondName = new QLabel(tr("Second Name"), this);
    QLabel * lblGender = new QLabel(tr("Gender"), this);

    QLineEdit * leName = new QLineEdit(this);
    QLineEdit * leSurName = new QLineEdit(this);
    QLineEdit * leSecondName = new QLineEdit(this);
    QComboBox * cbTitle = new QComboBox(this);
    QComboBox * cbGender = new QComboBox(this);

    registerField("Name", leName, "text");
    registerField("Surname", leSurName, "text");
    registerField("SecondName", leSecondName, "text");
    registerField("Title", cbTitle, "currentIndex");
    registerField("Gender", cbGender, "currentIndex");

    cbTitle->addItems(titles());
    cbGender->addItems(genders());

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(lblTitle, 0, 0);
    layout->addWidget(cbTitle, 0, 1);
    layout->addWidget(lblName, 1, 0);
    layout->addWidget(leName, 1, 1);
    layout->addWidget(lblSurName, 2, 0);
    layout->addWidget(leSurName, 2, 1);
    layout->addWidget(lblSecondName, 3, 0);
    layout->addWidget(leSecondName, 3, 1);
    layout->addWidget(lblGender, 4, 0);
    layout->addWidget(cbGender, 4, 1);
    setLayout(layout);
}

bool UserIdentityPage::validatePage()
{
    if (field("Name").toString().isEmpty() || field("Surname").toString().isEmpty()) {
        Utils::warningMessageBox(tr("Forbidden anonymous user."),
                                 tr("All users must have at least a name and a surname.\n"
                                    "You can not poursue with an anonymous user."), "",
                                 tr("Forbidden anonymous user."));
        return false;
    }
    return true;
}

UserLoginPasswordPage::UserLoginPasswordPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Please enter your login and password."));
    QLabel * lblL = new QLabel(tr("Login"), this);
    QLabel * lblP = new QLabel(tr("Password"), this);
    QLabel * lblCP = new QLabel(tr("Confirm Password"), this);

    leLogin = new Utils::LineEditEchoSwitcher(this);
    lePassword = new Utils::LineEditEchoSwitcher(this);
    lePasswordConfirm = new Utils::LineEditEchoSwitcher(this);
    leLogin->toogleEchoMode();
    lePassword->toogleEchoMode();
    lePasswordConfirm->toogleEchoMode();
    leLogin->setIcon(theme()->icon(Core::Constants::ICONEYES));
    lePassword->setIcon(theme()->icon(Core::Constants::ICONEYES));
    lePasswordConfirm->setIcon(theme()->icon(Core::Constants::ICONEYES));

    registerField("Login", leLogin, "text");
    registerField("Password", lePassword, "text");
    registerField("ConfirmPassword", lePasswordConfirm, "text");

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(lblL, 0, 0);
    layout->addWidget(leLogin, 0, 1);
    layout->addWidget(lblP, 1, 0);
    layout->addWidget(lePassword, 1, 1);
    layout->addWidget(lblCP, 2, 0);
    layout->addWidget(lePasswordConfirm, 2, 1);
    setLayout(layout);
}

bool UserLoginPasswordPage::validatePage()
{
    if (field("Password").toString() != field("ConfirmPassword")) {
        Utils::warningMessageBox(tr("Password confirmation error."),
                                 tr("You must correctly confirm your password to go throw this page."),
                                 "", tr("Wrong password"));
        return false;
    }
    if (field("Login").toString().isEmpty()) {
        Utils::warningMessageBox(tr("Login error."),
                                 tr("You must specify a valid login. Empty login is forbidden."),
                                 "", tr("Wrong login"));
        return false;
    }
    // log/pass already used ?
    if (userModel()->isCorrectLogin(UserPlugin::loginForSQL(field("Login").toString()),
                            UserPlugin::crypt(field("Password").toString()))) {
        Utils::warningMessageBox(tr("Login and password already used"),
                                 tr("The users' database already contains the same login/password couple.\n"
                                    "You must specify a different login/password."),
                                 "", tr("Login/Password already used"));
        return false;
    }
    return true;
}

UserAdressPage::UserAdressPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Please enter your complete adress."));
    setSubTitle(tr("This represents your professionnal adress."));
    QLabel * lblAdress = new QLabel(tr("Adress"), this);
    QLabel * lblCity = new QLabel(tr("City"), this);
    QLabel * lblZipcode = new QLabel(tr("Zipcode"), this);
    QLabel * lblCountry = new QLabel(tr("Country"), this);

    QTextEdit * teAdress = new QTextEdit(this);
    QLineEdit * leCity = new QLineEdit(this);
    QLineEdit * leZipcode = new QLineEdit(this);
    QLineEdit * leCountry = new QLineEdit(this);

    registerField("Adress", teAdress , "plainText");
    registerField("City", leCity , "text");
    registerField("Zipcode", leZipcode , "text");
    registerField("Country", leCountry , "text");

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(lblAdress, 0, 0);
    layout->addWidget(teAdress, 0, 1);
    layout->addWidget(lblCity, 1, 0);
    layout->addWidget(leCity, 1, 1);
    layout->addWidget(lblZipcode, 2, 0);
    layout->addWidget(leZipcode, 2, 1);
    layout->addWidget(lblCountry, 3, 0);
    layout->addWidget(leCountry, 3, 1);
    setLayout(layout);
}

UserTelsAndMailPage::UserTelsAndMailPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Please, enter tels, fax and mail adress."));
    QLabel * lblT1 = new QLabel(tr("Tel1"), this);
    QLabel * lblT2 = new QLabel(tr("Tel2"), this);
    QLabel * lblT3 = new QLabel(tr("Tel3"), this);
    QLabel * lblFax = new QLabel(tr("Fax"), this);
    QLabel * lblMail = new QLabel(tr("Mail"), this);

    QLineEdit * leT1 = new QLineEdit(this);
    QLineEdit * leT2 = new QLineEdit(this);
    QLineEdit * leT3 = new QLineEdit(this);
    QLineEdit * leFax = new QLineEdit(this);
    QLineEdit * leMail = new QLineEdit(this);

    registerField("Tel1", leT1, "text");
    registerField("Tel2", leT2, "text");
    registerField("Tel3", leT3, "text");
    registerField("Fax", leFax, "text");
    registerField("Mail", leMail, "text");

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(lblT1, 0, 0);
    layout->addWidget(leT1, 0, 1);
    layout->addWidget(lblT2, 1, 0);
    layout->addWidget(leT2, 1, 1);
    layout->addWidget(lblT3, 2, 0);
    layout->addWidget(leT3, 2, 1);
    layout->addWidget(lblFax, 3, 0);
    layout->addWidget(leFax, 3, 1);
    layout->addWidget(lblMail, 4, 0);
    layout->addWidget(leMail, 4, 1);
    setLayout(layout);
}

bool UserTelsAndMailPage::validatePage()
{
    // TODO check mail and tel/fax formatting
    return true;
}


UserProfilPage::UserProfilPage(QWidget *parent) :
        QWizardPage(parent)
{
    setTitle(tr("Select a profil"));
    setSubTitle(tr("FreeMedForms allows you to create users using predefined profils. Select your profil and options."));

    registerField("isMedical", new QWidget(this));
    registerField("UserManager", new QWidget(this));
    registerField("DrugsManager", new QWidget(this));
    registerField("Medical", new QWidget(this));
    registerField("Paramedical", new QWidget(this));
    registerField("Administrative", new QWidget(this));

    Views::StringListModel *model = new Views::StringListModel(this);
    model->setStringList(QStringList() << tkTr(Trans::Constants::DOCTOR) << tr("Software administrator"));
    model->setCheckable(true);
    model->setReadOnly(true);
    view = new Views::StringListView(this);
    view->setModel(model);
    view->setActions(0);

    box = new QCheckBox(tr("Define all rights"), this);

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(view, 0, 0);
    layout->addWidget(box, 1, 0);
}

bool UserProfilPage::validatePage()
{
    setField("UserManager",  Core::IUser::NoRights);
    setField("DrugsManager",  Core::IUser::NoRights);
    setField("Medical", Core::IUser::NoRights);
    setField("Paramedical", Core::IUser::NoRights);
    setField("Administrative", Core::IUser::NoRights);
    setField("isMedical", false);
    next = UserWizard::PaperGenericPage;
    QStringList result = view->getCheckedStringList().toStringList();
    if (result.contains(tkTr(Trans::Constants::DOCTOR))) {
        setField("DrugsManager",  Core::IUser::AllRights);
        setField("Medical", Core::IUser::AllRights);
        setField("Paramedical", int(Core::IUser::ReadAll | Core::IUser::Print));
        setField("Administrative", Core::IUser::NoRights);
        setField("isMedical", true);
        next = UserWizard::SpecialiesQualificationsPage;
    }
    if (result.contains(tr("Software administrator"))) {
        setField("UserManager",  Core::IUser::AllRights);
    }
    if (box->isChecked()) {
        next = UserWizard::RightsPage;
        return true;
    }
    return true;
}

UserSpecialiesQualificationsPage::UserSpecialiesQualificationsPage(QWidget *parent)
        : QWizardPage(parent)
{
    setTitle(tr("Define user's specialties and qualifications."));
    setSubTitle(tr("Use the context menu to add, remove, move up or down items."));
    QTabWidget *tab = new QTabWidget(this);

    QStringListModel * modelspe = new QStringListModel(this);
    Views::StringListView * speView = new Views::StringListView(this);
    speView->setModel(modelspe);
    QStringListModel * modelqual = new QStringListModel(this);
    Views::StringListView * quaView = new Views::StringListView(this);
    quaView->setModel(modelqual);
    QStringListModel * modelids = new QStringListModel(this);
    Views::StringListView * idsView = new Views::StringListView(this);
    idsView->setModel(modelids);

    tab->addTab(speView, tr("Specialities"));
    tab->addTab(quaView, tr("Qualifications"));
    tab->addTab(idsView, tr("Identifiants"));

    registerField("Specialities", speView, "stringList");
    registerField("Qualifications", quaView, "stringList");
    registerField("Identifiants", idsView, "stringList");

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(tab, 0, 1);
    setLayout(layout);
}

UserRightsPage::UserRightsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Define user's rights."));
    setSubTitle(tr("Role by role, define the user's rights."));

    QTabWidget *tab = new QTabWidget(this);

    Internal::UserRightsWidget *um = new Internal::UserRightsWidget(this);
    Internal::UserRightsWidget *drugs = new Internal::UserRightsWidget(this);
    Internal::UserRightsWidget *med = new Internal::UserRightsWidget(this);
    Internal::UserRightsWidget *paramed = new Internal::UserRightsWidget(this);
    Internal::UserRightsWidget *administ = new Internal::UserRightsWidget(this);

    um->setRights(field("UserManager").toInt());
    um->setRights(field("DrugsManager").toInt());
    um->setRights(field("Medical").toInt());
    um->setRights(field("Paramedical").toInt());
    um->setRights(field("Administrative").toInt());

    tab->addTab(um, tr("Users"));
    tab->addTab(drugs, tr("Drugs"));
    tab->addTab(med, tr("Medicals"));
    tab->addTab(paramed, tr("Paramedicals"));
    tab->addTab(administ, tr("Administrative"));

    /** \todo set the values of the rights */
    registerField("UserManager", um, "rights");
    registerField("DrugsManager", drugs, "rights");
    registerField("Medical", med, "rights");
    registerField("Paramedical", paramed, "rights");
    registerField("Administrative", administ, "rights");

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(tab, 0, 0);
    setLayout(layout);
}



static inline QString defaultHeader()
{
    return Utils::readTextFile(settings()->path(Core::ISettings::BundleResourcesPath) + "/textfiles/default_user_header.htm");
}

static inline QString defaultFooter()
{
    return Utils::readTextFile(settings()->path(Core::ISettings::BundleResourcesPath) + "/textfiles/default_user_footer.htm");
}

UserPaperPage::UserPaperPage(const QString &paperName, QWidget *parent) :
        QWizardPage(parent), type(paperName)
{
    QString title;
    if (type=="Generic")
        title = tr("Generic");
    else if (type == "Prescription")
        title = tr("Prescription");
    else if (type == "Administrative")
            title = tr("Administrative");
    setTitle(tr("%1 headers and footers").arg(title));

    header = new Print::TextDocumentExtra;
    footer = new Print::TextDocumentExtra;
    wm = new Print::TextDocumentExtra;
    previewer = Print::Printer::previewer(this);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(previewer, 0, 0);
    setLayout(layout);

    header->setHtml(defaultHeader());
    footer->setHtml(defaultFooter());

    if (type=="Prescription") {
        wm->setPresence(Print::Printer::DuplicataOnly);
    }

    previewer->setHeader(header);
    previewer->setFooter(footer);
    previewer->setWatermark(wm);
}

bool UserPaperPage::validatePage()
{
    Print::TextDocumentExtra *tmp = new Print::TextDocumentExtra;
    int header, footer, wmk;
    if (type=="Generic") {
        header = Core::IUser::GenericHeader;
        footer = Core::IUser::GenericFooter;
        wmk = Core::IUser::GenericWatermark;
    } else if (type == "Prescription") {
        header = Core::IUser::PrescriptionHeader;
        footer = Core::IUser::PrescriptionFooter;
        wmk = Core::IUser::PrescriptionWatermark;
    } else if (type == "Administrative") {
        header = Core::IUser::AdministrativeHeader;
        footer = Core::IUser::AdministrativeFooter;
        wmk = Core::IUser::AdministrativeWatermark;
    }

    previewer->headerToPointer(tmp);
    UserWizard::setUserPaper(header, tmp->toXml());

    previewer->footerToPointer(tmp);
    UserWizard::setUserPaper(footer, tmp->toXml());

    previewer->watermarkToPointer(tmp);
    UserWizard::setUserPaper(wmk, tmp->toXml());

    return true;
}


int UserPaperPage::nextId() const
{
    if (field("isMedical").toBool() && wizard()->page(UserWizard::PaperPrescriptionsPage)!=this)
        return UserWizard::PaperPrescriptionsPage;
    return -1;
}
