/*
 *  SPDX-FileCopyrightText: 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kis_pressure_sharpness_option.h"

#include <klocalizedstring.h>

#include <widgets/kis_curve_widget.h>

#include <KoColor.h>
#include <KoColorSpace.h>
#include <kis_fixed_paint_device.h>
#include <brushengine/kis_paintop.h>

KisPressureSharpnessOption::KisPressureSharpnessOption()
    : KisCurveOption(KoID("Sharpness", i18n("Sharpness")), KisPaintOpOption::GENERAL, false)
    , m_alignOutlinePixels(false)
    , m_softness(0)
{
}

void KisPressureSharpnessOption::writeOptionSetting(KisPropertiesConfigurationSP setting) const
{
    KisCurveOption::writeOptionSetting(setting);
    setting->setProperty(SHARPNESS_ALIGN_OUTLINE_PIXELS, m_alignOutlinePixels);
    setting->setProperty(SHARPNESS_SOFTNESS, m_softness);
}

void KisPressureSharpnessOption::readOptionSetting(const KisPropertiesConfigurationSP setting)
{
    KisCurveOption::readOptionSetting(setting);
    m_alignOutlinePixels = setting->getBool(SHARPNESS_ALIGN_OUTLINE_PIXELS);
    m_softness = quint32(setting->getInt(SHARPNESS_SOFTNESS));

    // backward compatibility: test for a "sharpness factor" property
    //                         and use this value if it does exist
    if (setting->hasProperty(SHARPNESS_FACTOR) && !setting->hasProperty("SharpnessValue")) {
        KisCurveOption::setValue(setting->getDouble(SHARPNESS_FACTOR));
        m_softness = quint32(setting->getDouble(SHARPNESS_FACTOR) * 100);
    }

}

void KisPressureSharpnessOption::apply(const KisPaintInformation &info, const QPointF &pt, qint32 &x, qint32 &y, qreal &xFraction, qreal &yFraction) const
{
    bool sharpness=isChecked() && KisCurveOption::value() != 0.0;
    applyInner(info,pt,x,y,xFraction,yFraction,sharpness);
}

void KisPressureSharpnessOption::applyOutline(const KisPaintInformation &info, const QPointF &pt, qint32 &x, qint32 &y, qreal &xFraction, qreal &yFraction) const
{
    bool sharpness=isChecked() && alignOutlineToPixels() && KisCurveOption::value() != 0.0;
    applyInner(info,pt,x,y,xFraction,yFraction,sharpness);
}

void KisPressureSharpnessOption::applyInner(const KisPaintInformation &info, const QPointF &pt, qint32 &x, qint32 &y, qreal &xFraction, qreal &yFraction, bool sharpness) const
{
    if (sharpness) {
        qreal processedSharpness = computeSizeLikeValue(info);

        if (processedSharpness == 1.0) {
            // pen
            xFraction = 0.0;
            yFraction = 0.0;
            x = qRound(pt.x());
            y = qRound(pt.y());
        }
        else {
            // something in between
            qint32 xi = qRound(pt.x());
            qint32 yi = qRound(pt.y());

            qreal xf = processedSharpness * xi + (1.0 - processedSharpness) * pt.x();
            qreal yf = processedSharpness * yi + (1.0 - processedSharpness) * pt.y();

            KisPaintOp::splitCoordinate(xf, &x, &xFraction);
            KisPaintOp::splitCoordinate(yf, &y, &yFraction);
        }
    } else {
        // brush
        KisPaintOp::splitCoordinate(pt.x(), &x, &xFraction);
        KisPaintOp::splitCoordinate(pt.y(), &y, &yFraction);
    }
}

void KisPressureSharpnessOption::applyThreshold(KisFixedPaintDeviceSP dab, const KisPaintInformation & info)
{
    if (!isChecked()) return;
    const KoColorSpace * cs = dab->colorSpace();

    // Set all alpha > opaque/2 to opaque, the rest to transparent.
    // XXX: Using 4/10 as the 1x1 circle brush paints nothing with 0.5.
    quint8* dabPointer = dab->data();
    QRect rc = dab->bounds();

    qreal threshold = computeSizeLikeValue(info);

    quint32 pixelSize = dab->pixelSize();
    int pixelCount = rc.width() * rc.height();

    quint32 tolerance = quint32(OPACITY_OPAQUE_U8 - (threshold * OPACITY_OPAQUE_U8));

    for (int i = 0; i < pixelCount; i++) {
        quint8 opacity = cs->opacityU8(dabPointer);

        // Check what pixel goes sharp
        if (opacity > (tolerance) ) {
            cs->setOpacity(dabPointer, OPACITY_OPAQUE_U8, 1);
        } else {
            // keep original value if in soft range
            if (opacity <= (100 - m_softness) * tolerance / 100) {
                cs->setOpacity(dabPointer, OPACITY_TRANSPARENT_U8, 1);
            }
        }
        dabPointer += pixelSize;
    }
}
