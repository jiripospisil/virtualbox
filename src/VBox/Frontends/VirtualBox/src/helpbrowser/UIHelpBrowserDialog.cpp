/* $Id$ */
/** @file
 * VBox Qt GUI - UIHelpBrowserDialog class implementation.
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#if defined(RT_OS_SOLARIS)
# include <QFontDatabase>
#endif
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>

/* GUI includes: */
#include "UIDesktopWidgetWatchdog.h"
#include "UIExtraDataManager.h"
#include "UIIconPool.h"
#include "UIHelpBrowserDialog.h"
#include "UIHelpBrowserWidget.h"
#ifdef VBOX_WS_MAC
# include "VBoxUtils-darwin.h"
#endif

#include <iprt/assert.h>


/*********************************************************************************************************************************
*   Class UIHelpBrowserDialog implementation.                                                                                    *
*********************************************************************************************************************************/

UIHelpBrowserDialog::UIHelpBrowserDialog(QWidget *pParent, QWidget *pCenterWidget, const QString &strHelpFilePath)
    : QIWithRetranslateUI<QIWithRestorableGeometry<QMainWindow> >(pParent)
    , m_strHelpFilePath(strHelpFilePath)
    , m_pWidget(0)
    , m_pCenterWidget(pCenterWidget)
    , m_iGeometrySaveTimerId(-1)
    , m_pZoomLabel(0)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(UIIconPool::iconSetFull(":/vm_show_logs_32px.png", ":/vm_show_logs_16px.png"));
    statusBar()->show();
    m_pZoomLabel = new QLabel;
    statusBar()->addPermanentWidget(m_pZoomLabel);

    prepareCentralWidget();
    loadSettings();
    retranslateUi();
}

void UIHelpBrowserDialog::showHelpForKeyword(const QString &strKeyword)
{
#ifdef VBOX_WITH_QHELP_VIEWER
    if (m_pWidget)
        m_pWidget->showHelpForKeyword(strKeyword);
#else
    Q_UNUSED(strKeyword);
#endif
}

void UIHelpBrowserDialog::retranslateUi()
{
#ifdef VBOX_WITH_QHELP_VIEWER
    setWindowTitle(UIHelpBrowserWidget::tr("Oracle VM VirtualBox User Manual"));
#endif
}

bool UIHelpBrowserDialog::event(QEvent *pEvent)
{
    switch (pEvent->type())
    {
        case QEvent::Resize:
        case QEvent::Move:
        {
            if (m_iGeometrySaveTimerId != -1)
                killTimer(m_iGeometrySaveTimerId);
            m_iGeometrySaveTimerId = startTimer(300);
            break;
        }
        case QEvent::Timer:
        {
            QTimerEvent *pTimerEvent = static_cast<QTimerEvent*>(pEvent);
            if (pTimerEvent->timerId() == m_iGeometrySaveTimerId)
            {
                killTimer(m_iGeometrySaveTimerId);
                m_iGeometrySaveTimerId = -1;
                saveDialogGeometry();
            }
            break;
        }
        default:
            break;
    }
    return QIWithRetranslateUI<QIWithRestorableGeometry<QMainWindow> >::event(pEvent);
}


void UIHelpBrowserDialog::prepareCentralWidget()
{
#ifdef VBOX_WITH_QHELP_VIEWER
    m_pWidget = new UIHelpBrowserWidget(EmbedTo_Dialog, m_strHelpFilePath);
    AssertPtrReturnVoid(m_pWidget);
    setCentralWidget((m_pWidget));
    sltZoomPercentageChanged(m_pWidget->zoomPercentage());
    connect(m_pWidget, &UIHelpBrowserWidget::sigCloseDialog,
            this, &UIHelpBrowserDialog::close);
    connect(m_pWidget, &UIHelpBrowserWidget::sigLinkHighlighted,
            this, &UIHelpBrowserDialog::sltLinkHighlighted);
    connect(m_pWidget, &UIHelpBrowserWidget::sigStatusBarVisible,
            this, &UIHelpBrowserDialog::sltStatusBarVisibilityChange);
    connect(m_pWidget, &UIHelpBrowserWidget::sigZoomPercentageChanged,
            this, &UIHelpBrowserDialog::sltZoomPercentageChanged);

    const QList<QMenu*> menuList = m_pWidget->menus();
    foreach (QMenu *pMenu, menuList)
        menuBar()->addMenu(pMenu);
#endif
}

void UIHelpBrowserDialog::loadSettings()
{
    const QRect availableGeo = gpDesktop->availableGeometry(this);
    int iDefaultWidth = availableGeo.width() / 2;
    int iDefaultHeight = availableGeo.height() * 3 / 4;
    QRect defaultGeo(0, 0, iDefaultWidth, iDefaultHeight);

    const QRect geo = gEDataManager->helpBrowserDialogGeometry(this, m_pCenterWidget, defaultGeo);
    restoreGeometry(geo);
}

void UIHelpBrowserDialog::saveDialogGeometry()
{
    const QRect geo = currentGeometry();
    gEDataManager->setHelpBrowserDialogGeometry(geo, isCurrentlyMaximized());
}

bool UIHelpBrowserDialog::shouldBeMaximized() const
{
    return gEDataManager->helpBrowserDialogShouldBeMaximized();
}

void UIHelpBrowserDialog::sltLinkHighlighted(const QString& strLink)
{
    statusBar()->showMessage(strLink);
}

void UIHelpBrowserDialog::sltStatusBarVisibilityChange(bool fVisible)
{
    statusBar()->setVisible(fVisible);
}

void UIHelpBrowserDialog::sltZoomPercentageChanged(int iPercentage)
{
    if (m_pZoomLabel)
        m_pZoomLabel->setText(QString("%1%").arg(QString::number(iPercentage)));
}
