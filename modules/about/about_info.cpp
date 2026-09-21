#include "modules/about/about_info.h"

namespace {

// The label column the web client's payload uses, so the two blocks line up
// character for character when a reader has one of each in front of them.
constexpr size_t kLabelWidth = 10;

void AppendRow(std::u16string& out,
               std::u16string_view label,
               std::u16string_view value) {
  // An empty value is a row nobody could fill, and the payload drops it for
  // the same reason the dialog does not draw it.
  if (value.empty())
    return;
  out += label;
  for (size_t written = label.size(); written < kLabelWidth; ++written)
    out += u' ';
  out += u' ';
  out += value;
  out += u'\n';
}

}  // namespace

std::u16string FormatAboutDetails(const AboutInfo& info,
                                  std::u16string_view reported_at) {
  std::u16string out;
  out += info.product.empty() ? std::u16string{u"Telecontrol SCADA"}
                              : info.product;
  out += u" — desktop client\n";

  AppendRow(out, u"Version", info.version);
  // The one row that speaks rather than disappearing; see the header.
  AppendRow(out, u"Build",
            info.build_commit.empty()
                ? std::u16string{u"not recorded"}
                : info.build_date.empty()
                      ? info.build_commit
                      : info.build_commit + u" · " + info.build_date);
  AppendRow(out, u"Runtime", info.runtime);
  // The three session rows stand or fall together: a report taken before
  // sign-in must not be readable as one taken against a server.
  if (info.has_session) {
    AppendRow(out, u"Server", info.server);
    AppendRow(out, u"Connection",
              info.connected ? u"connected" : u"not connected");
    AppendRow(out, u"Signed in",
              info.user.empty() ? std::u16string{u"anonymous"} : info.user);
  }
  AppendRow(out, u"Reported", reported_at);
  return out;
}
