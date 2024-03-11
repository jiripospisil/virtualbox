/* $Id$ */
/** @file
 * VBox Qt GUI - UIParavirtProviderEditor class implementation.
 */

/*
 * Copyright (C) 2019-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>

/* GUI includes: */
#include "UIConverter.h"
#include "UIGlobalSession.h"
#include "UIParavirtProviderEditor.h"

/* COM includes: */
#include "CPlatformProperties.h"


UIParavirtProviderEditor::UIParavirtProviderEditor(QWidget *pParent /* = 0 */)
    : UIEditor(pParent)
    , m_enmValue(KParavirtProvider_Max)
    , m_pLabel(0)
    , m_pCombo(0)
{
    prepare();
}

void UIParavirtProviderEditor::setValue(KParavirtProvider enmValue)
{
    /* Update cached value and
     * combo if value has changed: */
    if (m_enmValue != enmValue)
    {
        m_enmValue = enmValue;
        populateCombo();
    }
}

KParavirtProvider UIParavirtProviderEditor::value() const
{
    return m_pCombo ? m_pCombo->currentData().value<KParavirtProvider>() : m_enmValue;
}

int UIParavirtProviderEditor::minimumLabelHorizontalHint() const
{
    return m_pLabel ? m_pLabel->minimumSizeHint().width() : 0;
}

void UIParavirtProviderEditor::setMinimumLayoutIndent(int iIndent)
{
    if (m_pLayout)
        m_pLayout->setColumnMinimumWidth(0, iIndent);
}

void UIParavirtProviderEditor::retranslateUi()
{
    if (m_pLabel)
        m_pLabel->setText(tr("&Paravirtualization Interface:"));
    if (m_pCombo)
    {
        for (int i = 0; i < m_pCombo->count(); ++i)
        {
            const KParavirtProvider enmType = m_pCombo->itemData(i).value<KParavirtProvider>();
            m_pCombo->setItemText(i, gpConverter->toString(enmType));
        }
        m_pCombo->setToolTip(tr("Selects the paravirtualization guest interface "
                                "provider to be used by this virtual machine."));
    }
}

void UIParavirtProviderEditor::handleFilterChange()
{
    populateCombo();
}

void UIParavirtProviderEditor::prepare()
{
    /* Create main layout: */
    m_pLayout = new QGridLayout(this);
    if (m_pLayout)
    {
        m_pLayout->setContentsMargins(0, 0, 0, 0);

        /* Create label: */
        m_pLabel = new QLabel(this);
        if (m_pLabel)
        {
            m_pLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_pLayout->addWidget(m_pLabel, 0, 0);
        }

        /* Create combo layout: */
        QHBoxLayout *pComboLayout = new QHBoxLayout;
        if (pComboLayout)
        {
            /* Create combo: */
            m_pCombo = new QComboBox(this);
            if (m_pCombo)
            {
                /* This is necessary since contents is dynamical now: */
                m_pCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
                if (m_pLabel)
                    m_pLabel->setBuddy(m_pCombo);
                pComboLayout->addWidget(m_pCombo);
            }

            /* Add stretch: */
            pComboLayout->addStretch();

            /* Add combo-layout into main-layout: */
            m_pLayout->addLayout(pComboLayout, 0, 1);
        }
    }

    /* Populate combo: */
    populateCombo();

    /* Apply language settings: */
    retranslateUi();
}

void UIParavirtProviderEditor::populateCombo()
{
    if (m_pCombo)
    {
        /* Clear combo first of all: */
        m_pCombo->clear();

        /* Load currently supported paravirt provider types: */
        const KPlatformArchitecture enmArch = optionalFlags().contains("arch")
                                            ? optionalFlags().value("arch").value<KPlatformArchitecture>()
                                            : KPlatformArchitecture_x86;
        CPlatformProperties comProperties = gpGlobalSession->virtualBox().GetPlatformProperties(enmArch);
        m_supportedValues = comProperties.GetSupportedParavirtProviders();

        /* Make sure requested value if sane is present as well: */
        if (   m_enmValue != KParavirtProvider_Max
            && !m_supportedValues.contains(m_enmValue))
            m_supportedValues.prepend(m_enmValue);

        /* Update combo with all the supported values: */
        foreach (const KParavirtProvider &enmType, m_supportedValues)
            m_pCombo->addItem(QString(), QVariant::fromValue(enmType));

        /* Look for proper index to choose: */
        const int iIndex = m_pCombo->findData(QVariant::fromValue(m_enmValue));
        if (iIndex != -1)
            m_pCombo->setCurrentIndex(iIndex);

        /* Retranslate finally: */
        retranslateUi();
    }
}
