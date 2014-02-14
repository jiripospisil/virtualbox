/* $Id$ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIDnDDrag class declaration
 */

/*
 * Copyright (C) 2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIDnDDrag_h___
#define ___UIDnDDrag_h___

/* Qt includes: */
#include <QMimeData>

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"
#include "CConsole.h"
#include "CGuest.h"

#include "UIDnDHandler.h"

/** @todo Subclass QWindowsMime / QMacPasteboardMime
 *  to register own/more MIME types. */

/**
 * Implementation wrapper for OS-dependent drag'n drop
 * operations on the host.
 */
class UIDnDDrag : public QObject
{
    Q_OBJECT;

public:

    UIDnDDrag(CSession &session, const QStringList &lstFormats,
              Qt::DropAction defAction,
              Qt::DropActions actions, QWidget *pParent);

public:

    int DoDragDrop(void);

    int RetrieveData(const QString &strMimeType,
                     QVariant::Type vaType, QVariant &vaData);

public slots:

#ifndef RT_OS_WINDOWS
    void sltDataAvailable(const QString &mimetype);
#endif

private:

    QWidget          *m_pParent;
    CSession          m_session;
    QStringList       m_lstFormats;
    Qt::DropAction    m_defAction;
    Qt::DropActions   m_actions;
#ifndef RT_OS_WINDOWS
    UIDnDMimeData    *pMData;
    friend class UIDnDMimeData;
#endif
};

#endif /* ___UIDnDDrag_h___ */

