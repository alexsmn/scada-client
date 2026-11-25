#pragma once

#include <QString>

class QFontMetrics;

namespace scada::aui {

// Fits `path` — segments joined by `separator`, root first, leaf last — into
// `width` pixels, giving up the segments in the order a reader misses them
// least, so the leaf survives longest:
//
//  1. the whole path, when it fits;
//  2. the root and the leaf, the middle replaced by one ellipsis
//     («ЭСТРА-ПС : … : Коэффициент мощности»);
//  3. the leaf alone behind an ellipsis («… : Коэффициент мощности»);
//  4. the leaf alone, elided at its end if even that does not fit
//     («Коэффициент мо…»). A leaf that fits bare carries no ellipsis; the
//     tooltip is where the dropped prefix is read.
//
// A path with no separator is a plain name and elides at its end.
//
// Qt's own Qt::ElideMiddle cuts at a pixel, not at a segment: on the event
// journal's 170px Signal column it gave «ЭСТРА-ПС :…т мощности», and the two
// rows «Активная мощность» and «Реактивная мощность» both read «…я мощность»
// (backlog 1126).
QString ElidePath(const QString& path,
                  const QString& separator,
                  const QFontMetrics& metrics,
                  int width);

}  // namespace scada::aui
