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
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/TextEntry.h"
 
void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
    nux::VLayout* MainVLayout = new nux::VLayout(NUX_TRACKER_LOCATION);

    nux::TextEntry* text_entry_0 = new nux::TextEntry(TEXT("0123456789 abcdefghijklmnopqrstuvwxyz"), NUX_TRACKER_LOCATION);

    nux::TextEntry* text_entry_1 = new nux::TextEntry(TEXT("0123456789 abcdefghijklmnopqrstuvwxyz"), NUX_TRACKER_LOCATION);

    nux::TextEntry* text_entry_2 = new nux::TextEntry(TEXT("0123456789 abcdefghijklmnopqrstuvwxyz"), NUX_TRACKER_LOCATION);

    text_entry_0->SetMaximumWidth(300);
    text_entry_0->SetMinimumHeight (20);

    text_entry_1->SetMaximumWidth(300);
    text_entry_1->SetMinimumHeight (20);

    text_entry_2->SetMaximumWidth(300);
    text_entry_2->SetMinimumHeight (20);

    MainVLayout->AddView(text_entry_0, 0, nux::eCenter, nux::eFull);
    MainVLayout->AddView(text_entry_1, 0, nux::eCenter, nux::eFull);
    MainVLayout->AddView(text_entry_2, 0, nux::eCenter, nux::eFull, 1.0, nux::NUX_LAYOUT_BEGIN);

    MainVLayout->SetVerticalInternalMargin(10);

    MainVLayout->SetContentDistribution(nux::eStackCenter);
		MainVLayout->SetFocused (true);    
    nux::GetWindowThread ()->SetLayout(MainVLayout);
    nux::ColorLayer background(nux::Color(0xFF4D4D4D));
    static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
}

int main(int argc, char **argv)
{
    nux::NuxInitialize(0);
    nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Text Entry"), 400, 300, 0, &ThreadWidgetInit, 0);
    wt->Run(NULL);

    delete wt;
    return 0;
}
