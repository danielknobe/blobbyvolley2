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

#pragma once

#include "PlayerInput.h"
#include "BlobbyDebug.h"

class DuelMatch;

/*! \class InputSource
	\brief abstracts several possible input sources.
	\details This class abstracts several possible input sources, like local input
		from InputDevices, input from a scripted player or input over network
		which in turn can all be recorded by a recorder.
		The updateInput method has to be called (and is called)
		once per frame in DuelMatch in local mode, so input written with
		setInput will be overwritten in these cases.*/
class InputSource : public ObjectCounter<InputSource>
{
	public:
		InputSource();

		virtual ~InputSource();

		/// forces a recalculation of the input
		PlayerInput updateInput();

		/// gets the current input
		PlayerInput getInput() const;

		/// get original input data
		PlayerInputAbs getRealInput() const;

		/// set input which is returned on next call
		/// of getInput
		void setInput(PlayerInput ip);

		/// set input which is returned on next call
		/// of getInput
		void setInput(PlayerInputAbs ip);

		/// gets  match associated with this InputSource
		const DuelMatch* getMatch() const;
		/// sets match associated with this InputSource
		/// should only be called once!
		void setMatch(const DuelMatch* match);

	private:
		/// method that actually calculates the new input
		virtual PlayerInputAbs getNextInput();

		/// cached input
		PlayerInputAbs mInput;

		/// match connected with this source
		const DuelMatch* mMatch;
};
