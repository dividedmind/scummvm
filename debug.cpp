/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Change Log:
 * $Log$
 * Revision 1.7.2.1  2001/11/12 16:27:25  yazoo
 * The dig and Full Throttle support
 *
 * Revision 1.3  2001/10/26 17:34:50  strigeus
 * bug fixes, code cleanup
 *
 * Revision 1.2  2001/10/23 19:56:57  strigeus
 * fixed spelling error
 *
 * Revision 1.1  2001/10/23 19:51:50  strigeus
 * recompile not needed when switching games
 * debugger skeleton implemented
 *
 *
 */

#include "stdafx.h"
#include "scumm.h"


enum {
	CMD_INVALID,
	CMD_HELP,
	CMD_QUIT,
	CMD_GO,
	CMD_ACTOR,
	CMD_SCRIPTS,
};

void ScummDebugger::attach(Scumm *s) {
	if (_s)
		detach();

	_welcome = true;
	_s = s;
	s->_debugger = this;

	_go_amount = 1;
}

bool ScummDebugger::do_command() {
	int cmd;

	switch(get_command()) {
	case CMD_HELP:
		printf("Debugger commands:\n"
		       "help -> display this help text\n"
			   "quit -> quit the debugger\n"
			   "go [numframes] -> increase frame\n"
			   );
		return true;

	case CMD_QUIT:
		detach();
		return false;

	case CMD_GO:
		if (!_parameters[0])
			_go_amount = 1;
		else
			_go_amount = atoi(_parameters);
		return false;
	case CMD_ACTOR:
		if (!_parameters[0])
			printActors(-1);
		else
			printActors(atoi(_parameters));
		return true;
	case CMD_SCRIPTS:
		printScripts();
		return true;
	}

	return true;
}

void ScummDebugger::enter() {
	if (_welcome) {
		_welcome = false;
		printf("Debugging Mode entered!, please switch to this console for input.\n"
		       "Enter h to list all the debug commands\n");
	}
	while(do_command()) {}
}


void ScummDebugger::on_frame() {
	if (_go_amount==0)
		return;
	if (!--_go_amount)
		enter();
}


void ScummDebugger::detach() {
	_s->_debugger = NULL;
	_s = NULL;
}

struct DebuggerCommands {
	char text[8];
	byte len;
	byte id;
};

static const DebuggerCommands debugger_commands[] = {
	{ "h", 1, CMD_HELP },
	{ "q", 1, CMD_QUIT },
	{ "g", 1, CMD_GO },
	{ "a", 1, CMD_ACTOR },
	{ "s", 1, CMD_SCRIPTS },
	{ 0, 0, 0 },
};

int ScummDebugger::get_command() {
	const DebuggerCommands *dc;
	char *s;
	int i;

	do {
		printf("debug> ");
		if (!fgets(_cmd_buffer, sizeof(_cmd_buffer), stdin))
			return CMD_QUIT;

		i = strlen(_cmd_buffer);
		while (i>0 && _cmd_buffer[i-1]==10)
			_cmd_buffer[--i] = 0;

		if (i==0)
			continue;
						
		dc = debugger_commands;
		do {
			if (!strncmp(_cmd_buffer, dc->text, dc->len)) {
				for(s=_cmd_buffer;*s;s++) {
					if (*s==32) { s++; break; }
				}
				_parameters = s;
				return _command = dc->id;
			}
		} while ((++dc)->text[0]);
		
		for(s=_cmd_buffer;*s;s++)
			if (*s==32) { *s=0; break; }
		printf("Invalid command '%s'. Type 'help' for a list of available commands.\n", _cmd_buffer);
	} while (1);
}

void ScummDebugger::printActors(int act) {
	int i;
	Actor *a;

	if (act==-1) {
		printf("+--------------------------------------------------------------+\n");
		printf("|# |room|  x y   |elev|cos|width|box|mov|zp|frame|scale|spd|dir|\n");
		printf("+--+----+--------+----+---+-----+---+---+--+-----+-----+---+---+\n");
		for(i=1; i<_s->_maxActors; i++) {
			a = &_s->actor[i];
			if (a->visible)
				printf("|%2d|%4d|%3d  %3d|%4d|%3d|%5d|%3d|%3d|%2d|%5d|%5d|%3d|%3d|\n",
					i,a->room,a->x,a->y,a->elevation,a->costume,a->width,a->walkbox,a->moving,a->neverZClip,a->animIndex,a->scalex,a->speedx,a->facing);
		}
		printf("+--------------------------------------------------------------+\n");
	}
}

void ScummDebugger::printScripts() {
	int i;
	ScriptSlot *ss;

	printf("+----------------------------------+\n");
	printf("|# |num |sta|typ|un1|un2|fc|cut|un5|\n");
	printf("+--+----+---+---+---+---+--+---+---+\n");
	for(i=0; i<25; i++) {
		ss = &_s->vm.slot[i];
		if (ss->number) {
			printf("|%2d|%4d|%3d|%3d|%3d|%3d|%2d|%3d|%3d|\n",
				i, ss->number, ss->status, ss->type, ss->unk1, ss->unk2, ss->freezeCount, ss->cutsceneOverride, ss->unk5);
		}
	}
	printf("+----------------------------------+\n");
}