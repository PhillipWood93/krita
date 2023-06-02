#include "kisresourceitemviwer.h"
#include "ui_kisresourceitemviwer.h"

#include "ResourceListViewModes.h"
#include "KisPopupButton.h"
#include <KoIcon.h>
#include <kis_config.h>
#include <QMenu>

KisResourceItemViwer::KisResourceItemViwer(int type, QWidget *parent) :
    KisPopupButton(parent),
    m_ui(new Ui::KisResourceItemViwer),
    m_mode(ListViewMode::IconGrid),
    m_type(type)
{
    m_ui->setupUi(this);

    QMenu* viewModeMenu = new QMenu(this);
    KisConfig cfg(true);

    viewModeMenu->setStyleSheet("margin: 6px");
    setArrowVisible(false);
    setAutoRaise(true);

    // View Modes Btns
    viewModeMenu->addSection(i18nc("@title Which elements to display (e.g., thumbnails or details)", "Display"));

    if (m_type == 0) {
        m_mode = (cfg.readEntry<quint32>("ResourceItemsBCSearch.viewMode", 1) == 1)? ListViewMode::IconGrid : ListViewMode::Detail;
    } else if (m_type == 1) {
        m_mode = (cfg.readEntry<quint32>("ResourceItemsRM.viewMode", 1) == 1)? ListViewMode::IconGrid : ListViewMode::Detail;
    } else {
        m_mode = (cfg.readEntry<quint32>("ResourceItemsBCSelected.viewMode", 1) == 1)? ListViewMode::IconGrid : ListViewMode::Detail;
    }

    QActionGroup *actionGroup = new QActionGroup(viewModeMenu);

    QAction* action = viewModeMenu->addAction(KisIconUtils::loadIcon("view-preview"), i18n("Thumbnails"));
    action->setCheckable(true);
    action->setChecked(m_mode == ListViewMode::IconGrid);
    action->setActionGroup(actionGroup);
    connect(action, SIGNAL(triggered()), this, SLOT(slotViewThumbnail()));

    action = viewModeMenu->addAction(KisIconUtils::loadIcon("view-list-details"), i18n("Details"));
    action->setCheckable(true);
    action->setChecked(m_mode == ListViewMode::Detail);
    action->setActionGroup(actionGroup);
    connect(action, SIGNAL(triggered()), this, SLOT(slotViewDetails()));

    setPopupWidget(viewModeMenu);
    setPopupMode(QToolButton::InstantPopup);
    setIcon(KisIconUtils::loadIcon("view-choose"));

    if (m_mode == ListViewMode::IconGrid) {
        slotViewThumbnail();
    } else {
        slotViewDetails();
    }

}

KisResourceItemViwer::~KisResourceItemViwer()
{
    delete m_ui;
}

void KisResourceItemViwer::slotViewThumbnail()
{
    KisConfig cfg(false);
    if (m_type == 0) {
        cfg.writeEntry("ResourceItemsBCSearch.viewMode", qint32(1));
    } else if (m_type == 1) {
        cfg.writeEntry("ResourceItemsRM.viewMode", qint32(1));
    } else {
        cfg.writeEntry("ResourceItemsBCSelected.viewMode", qint32(1));
    }
    emit onViewThumbnail();
}

void KisResourceItemViwer::slotViewDetails()
{
    KisConfig cfg(false);
    if (m_type == 0) {
        cfg.writeEntry("ResourceItemsBCSearch.viewMode", qint32(0));
    } else if (m_type == 1) {
        cfg.writeEntry("ResourceItemsRM.viewMode", qint32(0));
    } else {
        cfg.writeEntry("ResourceItemsBCSelected.viewMode", qint32(0));
    }
    emit onViewDetails();
}

