#pragma once

#include <SkColor.h>
#include <QColor>

QColor ColorToQt(SkColor color);
SkColor ColorFromQt(QColor qcolor);
