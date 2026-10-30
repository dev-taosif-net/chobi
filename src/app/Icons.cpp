// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/Icons.h"

#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>
#include <QTransform>

namespace chobi {
namespace {

constexpr qreal kGrid = 24;  // glyphs are drawn on a 24x24 grid and scaled to fit
constexpr qreal kStroke = 2;

void usePen(QPainter* p, const QColor& color, qreal width = kStroke) {
  p->setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);
}

void useFill(QPainter* p, const QColor& color) {
  // A thin outline in the fill colour rounds off the corners of filled shapes.
  p->setPen(QPen(color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(color);
}

void drawSpeaker(QPainter* p, const QColor& color) {
  useFill(p, color);
  p->drawPolygon(QPolygonF{{3, 9}, {7, 9}, {12, 5}, {12, 19}, {7, 15}, {3, 15}});
}

void drawCorners(QPainter* p, const QColor& color, bool inward) {
  usePen(p, color);
  // Each corner as (corner point, direction along x, direction along y) pointing inwards.
  const struct {
    QPointF corner;
    qreal dx, dy;
  } corners[] = {{{4, 4}, 1, 1}, {{20, 4}, -1, 1}, {{20, 20}, -1, -1}, {{4, 20}, 1, -1}};
  for (const auto& c : corners) {
    const QPointF tip = inward ? c.corner + QPointF(5 * c.dx, 5 * c.dy) : c.corner;
    QPainterPath path;
    if (inward) {
      path.moveTo(tip.x(), c.corner.y());
      path.lineTo(tip);
      path.lineTo(c.corner.x(), tip.y());
    } else {
      path.moveTo(tip.x() + 5 * c.dx, tip.y());
      path.lineTo(tip);
      path.lineTo(tip.x(), tip.y() + 5 * c.dy);
    }
    p->drawPath(path);
  }
}

void drawGear(QPainter* p, const QColor& color) {
  QPainterPath gear;
  gear.addEllipse(QPointF(12, 12), 6.5, 6.5);
  QPainterPath tooth;
  tooth.addRoundedRect(QRectF(10.25, 2.5, 3.5, 5), 1, 1);
  for (int i = 0; i < 8; ++i) {
    QTransform turn;
    turn.translate(12, 12).rotate(45.0 * i).translate(-12, -12);
    gear = gear.united(turn.map(tooth));
  }
  QPainterPath hole;
  hole.addEllipse(QPointF(12, 12), 3.25, 3.25);
  p->setPen(Qt::NoPen);
  p->setBrush(color);
  p->drawPath(gear.subtracted(hole));
}

void drawGlyph(QPainter* p, Glyph glyph, const QColor& color) {
  switch (glyph) {
    case Glyph::Play:
      useFill(p, color);
      p->drawPolygon(QPolygonF{{7, 4.5}, {19.5, 12}, {7, 19.5}});
      break;
    case Glyph::Pause:
      p->setPen(Qt::NoPen);
      p->setBrush(color);
      p->drawRoundedRect(QRectF(6, 4.5, 4, 15), 1, 1);
      p->drawRoundedRect(QRectF(14, 4.5, 4, 15), 1, 1);
      break;
    case Glyph::Volume:
      drawSpeaker(p, color);
      usePen(p, color);
      p->drawArc(QRectF(9, 8, 8, 8), -50 * 16, 100 * 16);
      p->drawArc(QRectF(6, 4.5, 15, 15), -50 * 16, 100 * 16);
      break;
    case Glyph::Muted:
      drawSpeaker(p, color);
      usePen(p, color);
      p->drawLine(QPointF(15.5, 9.5), QPointF(20.5, 14.5));
      p->drawLine(QPointF(20.5, 9.5), QPointF(15.5, 14.5));
      break;
    case Glyph::AudioTrack: {  // headphones: a band over the top and two ear cups
      usePen(p, color, 2.2);
      QPainterPath band;
      band.moveTo(4, 16);
      band.lineTo(4, 13);
      band.arcTo(QRectF(4, 5, 16, 16), 180, -180);
      band.lineTo(20, 16);
      p->drawPath(band);
      p->setPen(Qt::NoPen);
      p->setBrush(color);
      p->drawRoundedRect(QRectF(2.5, 13.5, 5.5, 7.5), 1.8, 1.8);
      p->drawRoundedRect(QRectF(16, 13.5, 5.5, 7.5), 1.8, 1.8);
      break;
    }
    case Glyph::Subtitles:  // a screen with two lines of caption text
      usePen(p, color);
      p->drawRoundedRect(QRectF(2.5, 4.5, 19, 15), 3, 3);
      p->drawLine(QPointF(6.5, 11.5), QPointF(8.5, 11.5));
      p->drawLine(QPointF(11.5, 11.5), QPointF(17.5, 11.5));
      p->drawLine(QPointF(6.5, 15.5), QPointF(13.5, 15.5));
      p->drawLine(QPointF(16.5, 15.5), QPointF(17.5, 15.5));
      break;
    case Glyph::Fullscreen: drawCorners(p, color, false); break;
    case Glyph::ExitFullscreen: drawCorners(p, color, true); break;
    case Glyph::Settings: drawGear(p, color); break;
    case Glyph::Minimize:
      usePen(p, color);
      p->drawLine(QPointF(5, 12), QPointF(19, 12));
      break;
    case Glyph::Close:
      usePen(p, color);
      p->drawLine(QPointF(6, 6), QPointF(18, 18));
      p->drawLine(QPointF(18, 6), QPointF(6, 18));
      break;
  }
}

class GlyphIconEngine final : public QIconEngine {
 public:
  GlyphIconEngine(Glyph glyph, const QColor& color) : glyph_(glyph), color_(color) {}

  void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State) override {
    QColor color = color_;
    if (mode == QIcon::Disabled) color.setAlphaF(color.alphaF() * 0.4);
    const qreal side = qMin(rect.width(), rect.height());
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->translate(rect.x() + (rect.width() - side) / 2, rect.y() + (rect.height() - side) / 2);
    painter->scale(side / kGrid, side / kGrid);
    drawGlyph(painter, glyph_, color);
    painter->restore();
  }

  QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
    return scaledPixmap(size, mode, state, 1.0);
  }

  QPixmap scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state,
                       qreal scale) override {
    QPixmap pixmap(size * scale);
    pixmap.fill(Qt::transparent);
    {
      QPainter painter(&pixmap);
      paint(&painter, pixmap.rect(), mode, state);
    }
    pixmap.setDevicePixelRatio(scale);
    return pixmap;
  }

  QIconEngine* clone() const override { return new GlyphIconEngine(glyph_, color_); }

 private:
  Glyph glyph_;
  QColor color_;
};

}  // namespace

QIcon glyphIcon(Glyph glyph, const QColor& color) {
  return QIcon(new GlyphIconEngine(glyph, color));
}

}  // namespace chobi
