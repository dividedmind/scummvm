/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "interspective/innocent.h"

#include "audio/mididrv.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/error.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/events.h"
#include "engines/util.h"

#include "interspective/debug.h"
#include "interspective/debugger.h"
#include "interspective/eventmanager.h"
#include "interspective/graphics.h"
#include "interspective/logic.h"
#include "interspective/musicparser.h"
#include "interspective/resources.h"

#define GFX_TRANSACTION for (int ___i = 1; ___i && (_system->beginGFXTransaction(), true); ___i--, _system->endGFXTransaction() )

using namespace Common;

namespace Interspective {

Engine *Engine::me;

Engine::Engine(OSystem *syst) :
		::Engine(syst),
		_rnd("Interspective Engine")
		{
	_resources = &Res;
	_resources->setEngine(this);
	_graphics = &Graphics::instance();
	_graphics->setEngine(this);
	_logic = &Logic::instance();
	_logic->setEngine(this);
	_copyProtection = false;
	me = this;
	_lastTicks = 0;

	DebugMan.addDebugChannel(kDebugLevelScript, "script", "bytecode scripts");
	DebugMan.addDebugChannel(kDebugLevelGraphics, "graphics", "graphics handling");
	DebugMan.addDebugChannel(kDebugLevelFlow, "flow", "game code flow status");
	DebugMan.addDebugChannel(kDebugLevelAnimation, "animation", "animations");
	DebugMan.addDebugChannel(kDebugLevelValues, "values", "really low-level debugging of value manipulation");
	DebugMan.addDebugChannel(kDebugLevelFiles, "files", "file input and output");
	DebugMan.addDebugChannel(kDebugLevelEvents, "events", "event handling");
	DebugMan.addDebugChannel(kDebugLevelMusic, "music", "music loading and playing");
	DebugMan.addDebugChannel(kDebugLevelActor, "actor", "actor animation and behaviour");

	/* XXX how to integrate this with EventRecorder? */
//	syst->getEventManager()->registerRandomSource(_rnd, "interspective");
}

Engine::~Engine() {
	DebugMan.clearAllDebugChannels();
	MusicParser::destroy();
}

Common::Error Engine::run() {
	initGraphics(320, 200, false);
	
	_copyProtection = ConfMan.getBool("copy_protection");
	_startRoom = ConfMan.getInt("boot_param");
	_debugger = &Debug;
	Debug.setEngine(this);
	_resources->init();
	_graphics->init();

	_logic->init();

	_resources->loadActors();
	_logic->initCode();
	_graphics->showCursor();
	while(!shouldQuit()) {
		_logic->callAnimations();
		_graphics->paint();
		_logic->tick();
//		_graphics->paintAnimations();
		_graphics->updateScreen();
		_debugger->onFrame();
		delay(40);
		handleEvents();
	}

	return kNoError;
}

void Engine::handleEvents() {
	Common::Event event;
	while (_eventMan->pollEvent(event)) {
		switch(event.type) {

		case Common::EVENT_KEYUP:
			if (event.kbd.keycode == Common::KEYCODE_BACKQUOTE)
				_debugger->attach();
			break;

		case Common::EVENT_LBUTTONUP:
			EventManager::instance().clicked(event.mouse);

		default:
			break;
		}
	}
}

bool Engine::escapePressed() const {
	Common::Event event;
	while (_eventMan->pollEvent(event)) {
		switch(event.type) {

		case Common::EVENT_KEYUP:
			if (event.kbd.keycode == Common::KEYCODE_BACKQUOTE)
				_debugger->attach();
			if (event.kbd.keycode == Common::KEYCODE_ESCAPE)
				return true;
			break;
		default:
			break;
		}
	}
	return false;
}

uint16 Engine::getRandom(uint16 max) const {
	return _rnd.getRandomNumber(max);
}

void Engine::delay(int millis) const {
	int target = _lastTicks + millis;
	while ((_lastTicks = _system->getMillis()) < target) {
		_system->delayMillis(target - _lastTicks);
	}
}

} // End of namespace Interspective
