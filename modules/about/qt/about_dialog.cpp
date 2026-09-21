#include "modules/about/about_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "aui/qt/image_util.h"
#include "modules/about/about_info.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "project.h"
#include "scada/session_service.h"

#include "client_build_stamp.h"
#include "ui_about_dialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFontDatabase>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QLocale>
#include <QPushButton>
#include <QSysInfo>
#include <QTimer>

#include <utility>

namespace {

// How long the copy button states that it copied before reverting. The
// confirmation is the button's own label rather than a toast: the client has
// no toast surface, and a message box over a modal would be a second thing to
// dismiss for an action that cannot fail quietly.
constexpr int kCopiedForMs = 2500;

QString ToQString(const std::u16string& value) {
  return QString::fromStdU16String(value);
}

std::u16string ToU16String(const QString& value) {
  return value.toStdU16String();
}

}  // namespace

// The one dialog whose content carries the product mark.
//
// `dialogs.md` §1 forbids a brand lockup in dialog content and names this
// dialog as the place the application IS identified, so the mark, the product
// name and the version belong here and nowhere else. It still does not repeat
// its own window title.
//
// What the rest of it is for is being quoted back. Every row below is
// something a fault report needs and an operator cannot otherwise find, and
// `Copy details` puts the lot on the clipboard as plain text because the
// alternative is somebody retyping a commit hash over the phone. The shared
// screen for both clients is `docs/product/ui-mockups/screens/about.html`.
class AboutDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AboutDialog(AboutInfo info, QWidget* parent = nullptr)
      : QDialog(parent), info_{std::move(info)} {
    ui.setupUi(this);

    // The product's own mark, not a command glyph. It is the one place in the
    // UI that should show what the app *is*, and it is deliberately not routed
    // through the tinting path: that flattens an asset to a single palette
    // colour, and this one is deliberately multi-coloured — its four endpoint
    // dots are the severity ramp (iconography.md §5.4).
    //
    // `QIcon{":/….svg"}` drew NOTHING here, in every build since the mark was
    // added: the qsvgicon icon-engine plugin is not built (client/CMakeLists.txt
    // — qtsvg needs a full Xcode, which is what kept the client from building
    // standalone from its export), and QIcon answers a type it has no engine
    // for with an empty pixmap rather than an error, so the dialog reserved
    // space for a mark it never painted. lunasvg is how every other SVG in this
    // client reaches the screen; this one now takes the same route.
    ui.icon->setPixmap(
        LoadSvgPixmap(":/icons/app-mark.svg", 64, devicePixelRatioF()));

    // Derived from the application font rather than set in pixels, so the
    // heading follows the OS font-size accessibility setting and scales with
    // DPI (the native-look rules in client/CLAUDE.md).
    QFont product_font = QApplication::font();
    product_font.setBold(true);
    product_font.setPointSizeF(product_font.pointSizeF() * 1.25);
    ui.product->setFont(product_font);
    ui.product->setText(ToQString(info_.product));

    ui.version->setText(tr("Version %1").arg(ToQString(info_.version)));
    // Quietened through the palette, never a stylesheet — the breadcrumb
    // de-emphasises the same way.
    QPalette quiet = ui.version->palette();
    quiet.setColor(ui.version->foregroundRole(),
                   quiet.color(QPalette::PlaceholderText));
    ui.version->setPalette(quiet);

    BuildRows();

    // The notice this dialog is one of three homes for — COPYRIGHT's "Scope of
    // this notice" names it alongside that file and README.md, so the holder
    // and the years here are not decoration and have to agree with both.
    //
    // The range this work is claimed over, not the year the dialog was
    // written. It ran as a bare 2018 until 2026-09-20, which understated it at
    // both ends, and then briefly as 2017 -- this repository's first commit --
    // which is not the same question: the work predates the repository. 2012
    // is the earliest FIRST RELEASE the tree can evidence, from a Delphi type
    // library generated out of a shipping client on 17.11.2012. See
    // LICENSE-EXCEPTION.txt note A(ii) for why a floor is the right answer
    // here, and why rounding it earlier would not be.
    // Deliberately NOT derived from the build clock -- an old binary rebuilt
    // next year would then claim a year in which nothing was authored. Widen
    // it by hand when the work is revised, and keep it in step with COPYRIGHT,
    // clause 1 of LICENSE-EXCEPTION.txt and the runtime's own licence.
    //
    // The holder is the REGISTERED ENTITY, not the trading name, because the
    // line this feeds is a copyright notice and a notice naming something that
    // is not a legal person asserts nothing. It is the same string as COPYRIGHT
    // and as clause 1 of LICENSE-EXCEPTION.txt, and the three must agree: an
    // operator reading the About box and a lawyer reading the exception have to
    // be looking at the same holder.
    // If the brand name is wanted here instead, this is the one line to change
    // -- but then the About box stops being one of the places the notice is
    // asserted, and COPYRIGHT's "Scope of this notice" has to stop saying it
    // is.
    //
    // The licence is GPL-3.0 and nothing else. `LICENSE-EXCEPTION.txt` carries
    // an additional permission under section 7, but it is a DRAFT that grants
    // nothing while its marker block is present, and COPYRIGHT says so in as
    // many words: "Until it is adopted, this program is licensed under the GPL
    // alone." Widen this line when the copyright holder adopts it, not before.
    ui.legal->setText(
        QStringLiteral("%1<br>%2")
            .arg(tr("© %1 <a href=\"https://%2\">%3</a>")
                     .arg(QStringLiteral("2012–2026"),
                          QApplication::organizationDomain(),
                          tr("Telecontrol, Ltd.")),
                 tr("GPL-3.0 — see LICENSE beside the program")));

    copy_button_ =
        ui.buttonBox->addButton(tr("Copy details"), QDialogButtonBox::ActionRole);
    connect(copy_button_, &QPushButton::clicked, this,
            &AboutDialog::CopyDetails);

    // An About box states facts and takes no input, so it sizes to its content
    // and is not resizable.
    layout()->setSizeConstraint(QLayout::SetFixedSize);
  }

 private:
  // Adds one label/value row, or nothing at all when the value is empty.
  //
  // **A row nobody can fill is absent, never blank.** Reached from the login
  // window there is no session, and a dialog that drew an empty `Server` would
  // be stating that it has one and cannot name it.
  void AddRow(const QString& label, const QString& value, bool machine_text) {
    if (value.isEmpty())
      return;
    auto* field = new QLabel{value, this};
    // Every value here exists to be read out or copied, so it is selectable;
    // the long ones wrap rather than widening the dialog.
    field->setTextInteractionFlags(Qt::TextSelectableByMouse);
    field->setWordWrap(true);
    if (machine_text) {
      // A commit hash and an endpoint are read character by character.
      field->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    }
    ui.rows->addRow(label, field);
  }

  void BuildRows() {
    // The one row that speaks rather than disappearing: silence would read as
    // "this build has no identity" rather than "this build was made outside a
    // git checkout", and those need different answers from whoever reads it.
    QString build = ToQString(info_.build_commit);
    if (build.isEmpty()) {
      build = tr("not recorded");
    } else if (!info_.build_date.empty()) {
      // Shown as a date in the operator's locale, because it is read by a
      // person; `FormatAboutDetails` keeps the ISO-8601 form, because that is
      // read by whoever receives the ticket. A stamp that will not parse shows
      // the commit alone rather than a date this dialog invented.
      const QDateTime stamp =
          QDateTime::fromString(ToQString(info_.build_date), Qt::ISODate);
      if (stamp.isValid()) {
        build += QStringLiteral(" · ") +
                 QLocale{}.toString(stamp.date(), QLocale::ShortFormat);
      }
    }
    AddRow(tr("Build"), build, !info_.build_commit.empty());
    AddRow(tr("Runtime"), ToQString(info_.runtime), false);

    if (!info_.has_session)
      return;

    AddRow(tr("Server"), ToQString(info_.server), true);
    AddRow(tr("Connection"),
           info_.connected ? tr("connected") : tr("not connected"), false);
    AddRow(tr("Signed in"),
           info_.user.empty() ? tr("anonymous") : ToQString(info_.user), false);
  }

  void CopyDetails() {
    // The report's own timestamp, which the dialog does not draw: the operator
    // knows when they pressed the button and the reader of the ticket does not.
    const std::u16string payload = FormatAboutDetails(
        info_, ToU16String(QDateTime::currentDateTime().toString(Qt::ISODate)));
    QApplication::clipboard()->setText(ToQString(payload));

    copy_button_->setText(tr("Copied"));
    QTimer::singleShot(kCopiedForMs, this, [this] {
      copy_button_->setText(tr("Copy details"));
    });
  }

  const AboutInfo info_;
  Ui::AboutDialog ui;
  QPushButton* copy_button_ = nullptr;
};

#include "about_dialog.moc"

AboutInfo CollectAboutInfo(scada::SessionService& session_service,
                           NodeService& node_service) {
  AboutInfo info;
  info.product = ToU16String(QApplication::applicationDisplayName());
  info.version = ToU16String(QString::fromLatin1(PROJECT_VERSION_DOTTED_STRING));

  // Empty whenever git could not answer at build time — an export, a source
  // tarball, a container with no `.git`. Never filled in from the build clock:
  // a date with no commit behind it identifies nothing while reading exactly
  // like one that does. See modules/about/build_stamp.cmake.
  info.build_commit = ToU16String(QString::fromLatin1(CLIENT_BUILD_COMMIT));
  if (!info.build_commit.empty())
    info.build_date = ToU16String(QString::fromLatin1(CLIENT_BUILD_DATE));

  // `qVersion()` rather than QT_VERSION_STR: what a fault report needs is the
  // Qt that is actually loaded, and the two differing is itself a finding.
  info.runtime =
      ToU16String(QStringLiteral("Qt %1 · %2 · %3")
                      .arg(QString::fromLatin1(qVersion()),
                           QSysInfo::prettyProductName(),
                           QSysInfo::currentCpuArchitecture()));

  // A session exists once the client has a user id to describe — which is what
  // separates "opened from the login window" from "opened while disconnected",
  // two states the three session rows must not conflate.
  const scada::NodeId user_id = session_service.GetUserId();
  info.has_session = !user_id.is_null();
  if (!info.has_session)
    return info;

  info.connected = session_service.IsConnected();
  info.server = ToU16String(
      QString::fromStdString(session_service.GetHostName()));
  if (!session_service.IsAnonymous()) {
    // Whatever the node service already holds. The status strip resolves the
    // same node and keeps it fetched, so this is warm in practice; when it is
    // not, the row reads `anonymous` rather than blocking a modal on a browse.
    info.user = node_service.GetNode(user_id).display_name().text;
  }
  return info;
}

void ShowAboutDialog(DialogService& dialog_service, const AboutInfo& info) {
  auto dialog =
      std::make_unique<AboutDialog>(info, dialog_service.GetParentWidget());
  ShowSelfOwnedModalDialog(std::move(dialog));
}
