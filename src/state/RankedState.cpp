/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* header include */
#include "RankedState.h"

#include "IMGUI.h"

/* implementation */
RankedState::RankedState()
{
}

RankedState::~RankedState()
{
}

void RankedState::step_impl()
{
	// TODO
	// This is just a dummy implementation
	IMGUI& imgui = getIMGUI();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doText(GEN_ID, Vector2(400.0, 140.0), "Logged in", TF_ALIGN_CENTER);
	if (imgui.doButton(GEN_ID, Vector2(400.0, 240.0), "back", TF_ALIGN_CENTER)) 
	{
		switchState(new MainMenuState());
	}
}

const char* RankedState::getStateName() const
{
	return "RankedState";
}