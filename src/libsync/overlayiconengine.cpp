#include "overlayiconengine.h"

namespace OCC {

OverlayIconEngine::OverlayIconEngine(const QIcon &base, const QIcon &overlay)
    : _base(base)
    , _overlay(overlay)
{
}

void OverlayIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    // TODO: copied from qsvgiconengine
    QSize pixmapSize = rect.size();
    if (painter->device())
        pixmapSize *= painter->device()->devicePixelRatio();
    painter->drawPixmap(rect, pixmap(pixmapSize, mode, state));
}

QPixmap OverlayIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QImage img(size, QImage::Format_RGBA64_Premultiplied);
    // QImage's pixels are not initialized, therefore first things first: fill image with transparency
    img.fill(0);

    QPainter p(&img);

    // these settings improve the quality greatly
    p.setRenderHint(QPainter::RenderHint::Antialiasing);
    p.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform);
    p.setRenderHint(QPainter::RenderHint::LosslessImageRendering);

    // draw the overlay on top of the base
    // note: ideally, both icons have the same size (only in case of pixel graphics) and aspect ratio
    p.drawPixmap(0, 0, _base.pixmap(size, mode, state).scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    p.drawPixmap(0, 0, _overlay.pixmap(size, mode, state).scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    return QPixmap::fromImage(img);
}

QIconEngine *OverlayIconEngine::clone() const
{
    return new OverlayIconEngine(_base, _overlay);
}

}
