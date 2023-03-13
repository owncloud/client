#pragma once

#include <QIcon>
#include <QIconEngine>
#include <QPainter>

namespace OCC {

/**
 * A QIconEngine that paints one picture above another one.
 * It works like layers in a graphics program:
 * Transparency is supported, i.e., transparent areas in the overlay do not affect the base image, and transparencies in the base image work as expected, too.
 * Ideally, both icons have the same resolution/size (in case of pixel graphics) and
 */
class OverlayIconEngine : public QIconEngine
{
public:
    OverlayIconEngine(const QIcon &base, const QIcon &overlay);

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;

    QIconEngine *clone() const override;

private:
    QIcon _base;
    QIcon _overlay;
};

}
