/*
 * Copyright 2010 Inalogic Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jay.taoko_AT_gmail_DOT_com>
 *
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/Button.h"


void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
    nux::VLayout* MainVLayout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);

    nux::Button* button = new nux::Button(TEXT("Hello World!"), NUX_TRACKER_LOCATION);

    button->SetMaximumWidth(80);
    button->SetMaximumHeight(60);

    
    MainVLayout->AddView(button, 1, nux::eCenter, nux::eFull);
    MainVLayout->SetContentDistribution(nux::eStackCenter);
    
    nux::GetGraphicsThread()->SetLayout(MainVLayout);
    nux::ColorLayer background(nux::Color(0xFF4D4D4D));
    static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
}

int main(int argc, char **argv)
{
    nux::NuxInitialize(0);
    nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Edit Text Box"), 400, 300, 0, &ThreadWidgetInit, 0);
    wt->Run(NULL);

    delete wt;
    return 0;
}