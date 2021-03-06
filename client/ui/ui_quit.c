/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// ui_quit.c -- the quit menu

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client.h"
#include "include/ui_local.h"

//#define QUITMENU_NOKEY

/*
=======================================================================

QUIT MENU

=======================================================================
*/

#ifdef QUITMENU_NOKEY

static menuframework_s	s_quit_menu;
static menuseparator_s	s_quit_header;
static menuaction_s		s_quit_yes_action;
static menuaction_s		s_quit_no_action;

static void QuitYesFunc( void *unused )
{
	cls.key_dest = key_console;
	CL_Quit_f ();
}


void Quit_MenuInit ( void )
{
	s_quit_menu.x = SCREEN_WIDTH*0.5 - 24;
	s_quit_menu.y = SCREEN_HEIGHT*0.5 - 58;
	s_quit_menu.nitems = 0;

	s_quit_header.generic.type	= MTYPE_SEPARATOR;
	s_quit_header.generic.name	= "Quit game?";
	s_quit_header.generic.x	= MENU_FONT_SIZE*0.7 * strlen(s_quit_header.generic.name);
	s_quit_header.generic.y	= 20;

	s_quit_yes_action.generic.type	= MTYPE_ACTION;
	s_quit_yes_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_quit_yes_action.generic.x		= MENU_FONT_SIZE*3;
	s_quit_yes_action.generic.y		= 60;
	s_quit_yes_action.generic.name	= "yes";
	s_quit_yes_action.generic.callback = QuitYesFunc;
	s_quit_yes_action.generic.cursor_offset = -MENU_FONT_SIZE;

	s_quit_no_action.generic.type	= MTYPE_ACTION;
	s_quit_no_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_quit_no_action.generic.x		= MENU_FONT_SIZE*3;
	s_quit_no_action.generic.y		= 80;
	s_quit_no_action.generic.name	= "no";
	s_quit_no_action.generic.callback = UI_BackMenu;
	s_quit_no_action.generic.cursor_offset = -MENU_FONT_SIZE;

	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_header );
	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_yes_action );
	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_no_action );
}

#endif // QUITMENU_NOKEY


const char *M_Quit_Key (int32_t key)
{
#ifdef QUITMENU_NOKEY
	return Default_MenuKey( &s_quit_menu, key );
#else // QUITMENU_NOKEY
	switch (key)
	{
	case K_GAMEPAD_BACK:
	case K_GAMEPAD_START:
	case K_GAMEPAD_B:
	case K_ESCAPE:
	case 'n':
	case 'N':
		UI_PopMenu ();
		break;

	case K_GAMEPAD_A:
	case 'Y':
	case 'y':
		cls.key_dest = key_console;
		CL_Quit_f ();
		break;

	default:
		break;
	}
	return NULL;
#endif // QUITMENU_NOKEY
}


void M_Quit_Draw (void)
{
#ifdef QUITMENU_NOKEY
	Menu_AdjustCursor( &s_quit_menu, 1 );
	Menu_Draw( &s_quit_menu );
#else // QUITMENU_NOKEY
	int32_t		w, h;
    struct image_s *img = R_DrawFindPic("quit");
    if (img) {
        R_DrawGetImageSize (&w, &h, img);
        SCR_DrawImage (SCREEN_WIDTH/2-w/2, SCREEN_HEIGHT/2-h/2, w, h, ALIGN_CENTER, img, 1.0);
    }
#endif // QUITMENU_NOKEY
}


void M_Menu_Quit_f (void)
{
#ifdef QUITMENU_NOKEY
	Quit_MenuInit();
#endif
	UI_PushMenu (M_Quit_Draw, M_Quit_Key, NULL);
}
