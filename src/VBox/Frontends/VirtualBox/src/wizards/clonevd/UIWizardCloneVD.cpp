/* $Id$ */
/** @file
 * VBox Qt GUI - UIWizardCloneVD class implementation.
 */

/*
 * Copyright (C) 2006-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* GUI includes: */
#include "UICommon.h"
#include "UIMedium.h"
#include "UIMessageCenter.h"
#include "UINotificationCenter.h"
#include "UIWizardCloneVD.h"
#include "UIWizardCloneVDPageBasic1.h"
#include "UIWizardCloneVDPageBasic2.h"
#include "UIWizardCloneVDPageBasic3.h"
#include "UIWizardCloneVDPageExpert.h"

/* COM includes: */
#include "CMediumFormat.h"


UIWizardCloneVD::UIWizardCloneVD(QWidget *pParent, const CMedium &comSourceVirtualDisk)
    : UINativeWizard(pParent, WizardType_CloneVD)
    , m_comSourceVirtualDisk(comSourceVirtualDisk)
    , m_enmSourceVirtualDiskDeviceType(m_comSourceVirtualDisk.GetDeviceType())
{
#ifndef VBOX_WS_MAC
    /* Assign watermark: */
    setPixmapName(":/wizard_new_harddisk.png");
#else /* VBOX_WS_MAC */
    /* Assign background image: */
    setPixmapName(":/wizard_new_harddisk_bg.png");
#endif /* VBOX_WS_MAC */
}

bool UIWizardCloneVD::copyVirtualDisk()
{
    /* Gather attributes: */
    // const CMediumFormat comMediumFormat = field("mediumFormat").value<CMediumFormat>();
    // const qulonglong uVariant = field("mediumVariant").toULongLong();
    // const QString strMediumPath = field("mediumPath").toString();
    // const qulonglong uSize = field("mediumSize").toULongLong();
    // /* Check attributes: */
    // AssertReturn(!strMediumPath.isNull(), false);
    // AssertReturn(uSize > 0, false);

    // /* Get VBox object: */
    // CVirtualBox comVBox = uiCommon().virtualBox();

    // /* Create new virtual disk image: */
    // CMedium comVirtualDisk = comVBox.CreateMedium(comMediumFormat.GetName(), strMediumPath, KAccessMode_ReadWrite, m_enmSourceVirtualDiskDeviceType);
    // if (!comVBox.isOk())
    // {
    //     msgCenter().cannotCreateMediumStorage(comVBox, strMediumPath, this);
    //     return false;
    // }

    // /* Compose medium-variant: */
    // QVector<KMediumVariant> variants(sizeof(qulonglong) * 8);
    // for (int i = 0; i < variants.size(); ++i)
    // {
    //     qulonglong temp = uVariant;
    //     temp &= Q_UINT64_C(1) << i;
    //     variants[i] = (KMediumVariant)temp;
    // }

    // /* Copy medium: */
    // UINotificationProgressMediumCopy *pNotification = new UINotificationProgressMediumCopy(m_comSourceVirtualDisk,
    //                                                                                        comVirtualDisk,
    //                                                                                        variants);
    // connect(pNotification, &UINotificationProgressMediumCopy::sigMediumCopied,
    //         &uiCommon(), &UICommon::sltHandleMediumCreated);
    // gpNotificationCenter->append(pNotification);

    /* Positive: */
    return true;
}

void UIWizardCloneVD::retranslateUi()
{
    /* Translate wizard: */
    setWindowTitle(tr("Copy Virtual Disk Image"));
    UINativeWizard::retranslateUi();
}

void UIWizardCloneVD::populatePages()
{
    /* Create corresponding pages: */
    switch (mode())
    {
        case WizardMode_Basic:
        case WizardMode_Expert:
        {
            addPage(new UIWizardCloneVDPageBasic1(m_enmSourceVirtualDiskDeviceType));
            addPage(new UIWizardCloneVDPageBasic2(m_enmSourceVirtualDiskDeviceType));
            addPage(new UIWizardCloneVDPageBasic3);
            break;
        }
        // case WizardMode_Expert:
        // {
        //     setPage(PageExpert, new UIWizardCloneVDPageExpert(m_enmSourceVirtualDiskDeviceType));
        //     break;
        // }
        default:
        {
            AssertMsgFailed(("Invalid mode: %d", mode()));
            break;
        }
    }
}
