/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_svg_import.h"

#include <kpluginfactory.h>
#include <QFileInfo>
#include "kis_config.h"

#include <QInputDialog>
#include <KisDocument.h>
#include <kis_image.h>

#include <SvgParser.h>
#include <KoColorSpaceRegistry.h>
#include "kis_shape_layer.h"
#include <KoShapeControllerBase.h>

K_PLUGIN_FACTORY_WITH_JSON(SVGImportFactory, "krita_svg_import.json", registerPlugin<KisSVGImport>();)

KisSVGImport::KisSVGImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisSVGImport::~KisSVGImport()
{
}

KisImportExportErrorCode KisSVGImport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP configuration)
{
    Q_UNUSED(configuration);

    KisDocument * doc = document;

    const QString baseXmlDir = QFileInfo(filename()).canonicalPath();

    KisConfig cfg(false);

    qreal resolutionPPI = cfg.preferredVectorImportResolutionPPI(true);

    if (!batchMode()) {
        bool okay = false;
        const QString name = QFileInfo(filename()).fileName();
        resolutionPPI = QInputDialog::getInt(0,
                                             i18n("Import SVG"),
                                             i18n("Enter preferred resolution (PPI) for \"%1\"", name),
                                             cfg.preferredVectorImportResolutionPPI(),
                                             0, 100000, 1, &okay);

        if (!okay) {
            return ImportExportCodes::Cancelled;
        }

        cfg.setPreferredVectorImportResolutionPPI(resolutionPPI);
    }

    const qreal resolution = resolutionPPI;

    QStringList warnings;
    QStringList errors;

    QSizeF fragmentSize;
    QList<KoShape*> shapes =
            KisShapeLayer::createShapesFromSvg(io, baseXmlDir,
                                               QRectF(0,0,1200,800), resolutionPPI,
                                               doc->shapeController()->resourceManager(),
                                               false,
                                               &fragmentSize,
                                               &warnings, &errors);

    if (!warnings.isEmpty()) {
        doc->setWarningMessage(warnings.join('\n'));
    }
    if (!errors.isEmpty()) {
        doc->setErrorMessage(errors.join('\n'));
        return ImportExportCodes::FileFormatIncorrect;
    }


    QRectF rawImageRect(QPointF(), fragmentSize);
    QRect imageRect(rawImageRect.toAlignedRect());

    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(doc->createUndoStore(), imageRect.width(), imageRect.height(), cs, "svg image");
    image->setResolution(resolution / 72.0, resolution / 72.0);
    doc->setCurrentImage(image);

    KisShapeLayerSP shapeLayer =
            new KisShapeLayer(doc->shapeController(), image,
                              i18n("Vector Layer"),
                              OPACITY_OPAQUE_U8);

    Q_FOREACH (KoShape *shape, shapes) {
        shapeLayer->addShape(shape);
    }

    image->addNode(shapeLayer);
    return ImportExportCodes::OK;
}

#include <kis_svg_import.moc>
