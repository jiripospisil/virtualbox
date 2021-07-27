/* $Id$ */
/** @file
 * VBox Qt GUI - UIWizardNewVD class declaration.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVD_h
#define FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVD_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"

/* New Virtual Hard Drive wizard: */
class SHARED_LIBRARY_STUFF UIWizardNewVD : public UINativeWizard
{
    Q_OBJECT;

public:

    /* Page IDs: */
    enum
    {
        Page1,
        Page2,
        Page3
    };

    /* Page IDs: */
    enum
    {
        PageExpert
    };

    /* Constructor: */
    UIWizardNewVD(QWidget *pParent,
                  const QString &strDefaultName, const QString &strDefaultPath,
                  qulonglong uDefaultSize,
                  WizardMode mode = WizardMode_Auto);

    /* Pages related stuff: */
    void prepare();

protected:

    /* Creates virtual-disk: */
    bool createVirtualDisk();

    virtual void populatePages() /* final override */;

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Variables: */
    QString     m_strDefaultName;
    QString     m_strDefaultPath;
    qulonglong  m_uDefaultSize;
};

typedef QPointer<UIWizardNewVD> UISafePointerWizardNewVD;

#endif /* !FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVD_h */
