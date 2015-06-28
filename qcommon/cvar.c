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
// cvar.c -- dynamic variable tracking

#include "qcommon.h"
#include "wildcard.h"


stable_t cvarNames = {0, 102400};
cvar_t	*cvar_vars[CVAR_HASHMAP_WIDTH];

qboolean	cvar_allowCheats = true;

/*
============
Cvar_InfoValidate
============
*/
static qboolean Cvar_InfoValidate (char *s)
{
	if (strstr (s, "\\"))
		return false;
	if (strstr (s, "\""))
		return false;
	if (strstr (s, ";"))
		return false;
	return true;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t *Cvar_FindVar (char *var_name)
{
	cvar_t	*var;
    char buffer[MAX_TOKEN_CHARS];
    
    Q_strlcpy_lower(buffer, var_name, sizeof(buffer));
    int32_t index = Q_STLookup(cvarNames,buffer);
    
    if (index >= 0) {
        for (var=cvar_vars[index&CVAR_HASHMAP_MASK] ; var ; var=var->next)
            if (index == var->index)
                return var;
    }
	return NULL;
}


/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return atof (var->string);
}


/*
=================
Cvar_VariableInteger
=================
*/
int32_t Cvar_VariableInteger (char *var_name)
{
	cvar_t	*var;

	var = Cvar_FindVar(var_name);
	if (!var)
		return 0;
	return atoi (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}

/*
============
Cvar_DefaultValue
Knightmare added
============
*/
float Cvar_DefaultValue (char *var_name)
{
	cvar_t	*var;
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
#ifdef NEW_CVAR_MEMBERS
	return atof (var->default_string);
#else
	return var->value;
#endif
}


/*
============
Cvar_DefaultString
Knightmare added
============
*/
char *Cvar_DefaultString (char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
#ifdef NEW_CVAR_MEMBERS
	return var->default_string;
#else
	return var->string;
#endif
}

/*
============
Cvar_CompleteVariable
============
*/
const char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int32_t			len;
    int32_t index;
    int i;
    char buffer[MAX_TOKEN_CHARS];

	len = strlen(partial);

	if (!len)
		return NULL;
    Q_strlcpy_lower(buffer, partial, sizeof(buffer));
    index = Q_STLookup(cvarNames,buffer);
    if (index >= 0) {
        return Q_STGetString(cvarNames, index);
    }
    // check partial match
    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (cvar=cvar_vars[i] ; cvar ; cvar=cvar->next) {
            const char *name = Q_STGetString(cvarNames,cvar->index);
            if (!strncmp(buffer, name, len))
                return name;
        }
    }
    return NULL;
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get (char *var_name, char *var_value, int32_t flags)
{
	cvar_t	*var;
    int index;
    char buffer[MAX_TOKEN_CHARS];
    
	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_name))
		{
			Com_Printf("invalid info cvar name\n");
			return NULL;
		}
	}

    var = Cvar_FindVar (var_name);
	if (var)
	{
		var->flags |= flags;
		// Knightmare- added cvar defaults
#ifdef NEW_CVAR_MEMBERS
		if (var_value)
		{
			Z_Free(var->default_string);
			var->default_string = (char*)Z_TagStrdup (var_value, TAG_SYSTEM);
		}
#endif
		return var;
	}

	if (!var_value)
		return NULL;

	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_value))
		{
			Com_Printf("invalid info cvar value\n");
			return NULL;
		}
	}
    Q_strlcpy_lower(buffer, var_name, sizeof(buffer));

	var = (cvar_t*)Z_TagMalloc (sizeof(*var), TAG_SYSTEM);
	var->string = (char*)Z_TagStrdup (var_value, TAG_SYSTEM);
	// Knightmare- added cvar defaults
#ifdef NEW_CVAR_MEMBERS
	var->default_string = (char*)Z_TagStrdup (var_value, TAG_SYSTEM);
	var->integer = atoi(var->string);
#endif
	var->modified = true;
	var->value = atof (var->string);
    var->index = Q_STRegister(&cvarNames, buffer);
    index = var->index & CVAR_HASHMAP_MASK;
	// link the variable in
	var->next = cvar_vars[index];
    cvar_vars[index] = var;

	var->flags = flags;

	return var;
}

/*
 ============
 Cvar_Set_Internal
 ============
 */
cvar_t *Cvar_Set_Internal (cvar_t *var, char *value, qboolean force)
{
    assert(var);
    
    if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO))
    {
        if (!Cvar_InfoValidate (value))
        {
            Com_Printf("invalid info cvar value\n");
            return var;
        }
    }
    
    if (!force)
    {
        if (var->flags & CVAR_NOSET)
        {
            const char* var_name = Q_STGetString(cvarNames, var->index);
            Com_Printf ("%s is write protected.\n", var_name);
            return var;
        }
        
        if ((var->flags & CVAR_CHEAT) && !cvar_allowCheats)
        {
            const char* var_name = Q_STGetString(cvarNames, var->index);
            Com_Printf ("%s is cheat protected.\n", var_name);
            return var;
        }
        
        if (var->flags & CVAR_LATCH)
        {
            if (var->latched_string)
            {
                if (strcmp(value, var->latched_string) == 0)
                    return var;
                Z_Free (var->latched_string);
            }
            else
            {
                if (strcmp(value, var->string) == 0)
                    return var;
            }
            
            if (Com_ServerState())
            {
                const char* var_name = Q_STGetString(cvarNames, var->index);
                Com_Printf ("%s will be changed for next game.\n", var_name);
                var->latched_string = (char*)Z_TagStrdup(value, TAG_SYSTEM);
            }
            else
            {
                const char* var_name = Q_STGetString(cvarNames, var->index);
                var->string = (char*)Z_TagStrdup(value, TAG_SYSTEM);
                var->value = atof (var->string);
                
                if (!strcmp(var_name, "game"))
                {
                    FS_SetGamedir (var->string);
                    FS_ExecAutoexec ();
                }
            }
            return var;
        }
    }
    else
    {
        if (var->latched_string)
        {
            Z_Free (var->latched_string);
            var->latched_string = NULL;
        }
    }
    
    if (!strcmp(value, var->string))
        return var;		// not changed
    
    var->modified = true;
    
    if (var->flags & CVAR_USERINFO)
        userinfo_modified = true;	// transmit at next oportunity
    
    Z_Free (var->string);	// free the old value string
    
    var->string = (char*)Z_TagStrdup(value, TAG_SYSTEM);
    var->value = atof (var->string);
#ifdef NEW_CVAR_MEMBERS
    var->integer = atoi(var->string);
#endif
    
    return var;
}


/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2 (char *var_name, char *value, qboolean force)
{
	cvar_t		*var;

	var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, 0);
	}

    return Cvar_Set_Internal(var, value, force);
}

/*
============
Cvar_ForceSet
============
*/
cvar_t *Cvar_ForceSet (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, true);
}

/*
============
Cvar_Set
============
*/
cvar_t *Cvar_Set (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, false);
}


/*
============
Cvar_SetToDefault
Knightmare added
============
*/
cvar_t *Cvar_SetToDefault (char *var_name)
{
	return Cvar_Set2 (var_name, Cvar_DefaultString(var_name), false);
}


/*
============
Cvar_FullSet
============
*/
cvar_t *Cvar_FullSet (char *var_name, char *value, int32_t flags)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, flags);
	}

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity
	
	Z_Free (var->string);	// free the old value string
	
	var->string = (char*)Z_TagStrdup(value, TAG_SYSTEM);
	var->value = atof (var->string);
	var->flags = flags;

	return var;
}


/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (char *var_name, float value)
{
	char	val[32];

	if (value == (int32_t)value)
		Com_sprintf (val, sizeof(val), "%i",(int32_t)value);
	else
		Com_sprintf (val, sizeof(val), "%f",value);
	Cvar_Set (var_name, val);
}


/*
=================
Cvar_SetInteger
=================
*/
void Cvar_SetInteger (char *var_name, int32_t integer)
{
	char	val[32];

	Com_sprintf (val, sizeof(val), "%i",integer);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (void)
{
	cvar_t	*var;
    int i;
    for (i = 0; i< CVAR_HASHMAP_WIDTH; i++) {
        for (var = cvar_vars[i] ; var ; var = var->next)
        {
            if (!var->latched_string)
                continue;
            Z_Free (var->string);
            var->string = var->latched_string;
            var->latched_string = NULL;
            var->value = atof(var->string);
            if (!strcmp( Q_STGetString(cvarNames, var->index), "game"))
            {
                FS_SetGamedir (var->string);
                FS_ExecAutoexec ();
            }
        }
    }
}


/*
=================
Cvar_FixCheatVars

Resets cvars that could be used for multiplayer cheating
Borrowed from Q2E
=================
*/
void Cvar_FixCheatVars (qboolean allowCheats)
{

#ifdef NEW_CVAR_MEMBERS
	cvar_t	*var;
    int i;
    
	if (cvar_allowCheats == allowCheats)
		return;
	cvar_allowCheats = allowCheats;

	if (cvar_allowCheats)
		return;

    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (var = cvar_vars[i]; var; var = var->next)
        {
            if (!(var->flags & CVAR_CHEAT))
                continue;
            
            if (!Q_strcasecmp(var->string, var->default_string))
                continue;
            Cvar_Set_Internal(var, var->default_string, true);
        }
    }
#endif
}


/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command (void)
{
	cvar_t			*v;
    const char *name;
// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;
    
    name = Q_STGetString(cvarNames, v->index);
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{	// Knightmare- show latched value if applicable
#ifdef NEW_CVAR_MEMBERS
		if ((v->flags & CVAR_LATCH) && v->latched_string)
			Com_Printf ("\"%s\" is \"%s\" : default is \"%s\" : latched to \"%s\"\n", name, v->string, v->default_string, v->latched_string);
		else if (v->flags & CVAR_NOSET)
			Com_Printf ("\"%s\" is \"%s\"\n", name, v->string, v->default_string);
		else
			Com_Printf ("\"%s\" is \"%s\" : default is \"%s\"\n", name, v->string, v->default_string);
#else
		if ((v->flags & CVAR_LATCH) && v->latched_string)
			Com_Printf ("\"%s\" is \"%s\" : latched to \"%s\"\n", name, v->string, v->latched_string);
		else
			Com_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
#endif
		return true;
	}

	Cvar_Set_Internal(v, Cmd_Argv(1), false);
	return true;
}


/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console
============
*/
void Cvar_Set_f (void)
{
	int32_t		c;
	int32_t		flags;

	c = Cmd_Argc();
	if (c != 3 && c != 4)
	{
		Com_Printf ("usage: set <variable> <value> [u / s]\n");
		return;
	}

	if (c == 4)
	{
		if (!strcmp(Cmd_Argv(3), "u"))
			flags = CVAR_USERINFO;
		else if (!strcmp(Cmd_Argv(3), "s"))
			flags = CVAR_SERVERINFO;
		else
		{
			Com_Printf ("flags can only be 'u' or 's'\n");
			return;
		}
		Cvar_FullSet (Cmd_Argv(1), Cmd_Argv(2), flags);
	}
	else
		Cvar_Set (Cmd_Argv(1), Cmd_Argv(2));
}


/*
=================
Cvar_Toggle_f

Allows toggling of arbitrary cvars from console
=================
*/	
void Cvar_Toggle_f (void)
{
	cvar_t	*var;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: toggle <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}
#ifdef NEW_CVAR_MEMBERS
	Cvar_Set_Internal(var, va("%i", !var->integer), false);
#else
	Cvar_Set_Internal(var, va("%i", (int) !var->value), false);
#endif
}


/*
=================
Cvar_Reset_f

Allows resetting of arbitrary cvars from console
=================
*/
void Cvar_Reset_f (void)
{
	cvar_t *var;
#ifdef NEW_CVAR_MEMBERS
	if (Cmd_Argc() != 2){
		Com_Printf("Usage: reset <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}


	Cvar_Set_Internal(var, var->default_string, false);
#else
	Com_Printf("Error: unsupported command\n");
#endif
}


/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (char *path)
{
	cvar_t	*var;
	char	buffer[1024];
	FILE	*f;
    int i = 0;
    
	f = fopen (path, "a");
    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (var = cvar_vars[i] ; var ; var = var->next)
        {
            if (var->flags & CVAR_ARCHIVE)
            {
                char const *name = Q_STGetString(cvarNames, var->index);
                Com_sprintf (buffer, sizeof(buffer), "set %s \"%s\"\n", name, var->string);
                fprintf (f, "%s", buffer);
            }
        }
    }
	fclose (f);
}


/*
============
Cvar_List_f
============
*/
void Cvar_List_f (void)
{
	cvar_t	*var;
	int32_t		i, j, k, c;
	char	*wc;

	// RIOT's Quake3-sytle cvarlist
	c = Cmd_Argc();

	if (c != 1 && c!= 2)
	{
		Com_Printf ("usage: cvarlist [wildcard]\n");
		return;
	}

	if (c == 2)
		wc = Cmd_Argv(1);
	else
		wc = "*";

	i = 0;
	j = 0;
    for (k = 0; k < CVAR_HASHMAP_WIDTH; k++) {
        for (var = cvar_vars[k]; var; var = var->next, i++)
        {
            char const *name = Q_STGetString(cvarNames, var->index);
            if (wildcardfit (wc, name))
                //if (strstr (var->name, Cmd_Argv(1)))
            {
                j++;
                if (var->flags & CVAR_ARCHIVE)
                    Com_Printf ("A");
                else
                    Com_Printf (" ");
                
                if (var->flags & CVAR_USERINFO)
                    Com_Printf ("U");
                else
                    Com_Printf (" ");
                
                if (var->flags & CVAR_SERVERINFO)
                    Com_Printf ("S");
                else
                    Com_Printf (" ");
                
                if (var->flags & CVAR_NOSET)
                    Com_Printf ("-");
                else if (var->flags & CVAR_LATCH)
                    Com_Printf ("L");
                else
                    Com_Printf (" ");
                
                if (var->flags & CVAR_CHEAT)
                    Com_Printf("C");
                else
                    Com_Printf(" ");
                
                // show latched value if applicable
#ifdef NEW_CVAR_MEMBERS
                if ((var->flags & CVAR_LATCH) && var->latched_string)
                    Com_Printf (" %s \"%s\" - default: \"%s\" - latched: \"%s\"\n", name, var->string, var->default_string, var->latched_string);
                else
                    Com_Printf (" %s \"%s\" - default: \"%s\"\n", name, var->string, var->default_string);
#else
                if ((var->flags & CVAR_LATCH) && var->latched_string)
                    Com_Printf (" %s \"%s\" - latched: \"%s\"\n", name, var->string, var->latched_string);
                else
                    Com_Printf (" %s \"%s\"\n", var->name, var->string);
                
#endif
            }
        }
    }
	Com_Printf (" %i cvars, %i matching\n", i, j);
}


qboolean userinfo_modified;


char	*Cvar_BitInfo (int32_t bit)
{
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;
    int i;
    
	info[0] = 0;
    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (var = cvar_vars[i] ; var ; var = var->next)
        {
            if (var->flags & bit) {
                char const *name = Q_STGetString(cvarNames, var->index);
                Info_SetValueForKey (info, name, var->string);
            }
        }
    }
	return info;
}

// returns an info string containing all the CVAR_USERINFO cvars
char	*Cvar_Userinfo (void)
{
	return Cvar_BitInfo (CVAR_USERINFO);
}

// returns an info string containing all the CVAR_SERVERINFO cvars
char	*Cvar_Serverinfo (void)
{
	return Cvar_BitInfo (CVAR_SERVERINFO);
}



void Cvar_AssertRange (cvar_t *var, float min, float max, qboolean isInteger)
{
    if (!var)
        return;
#ifdef NEW_CVAR_MEMBERS
    if (isInteger && (var->value != (float) var->integer))
    {
        char const *name = Q_STGetString(cvarNames, var->index);
        Com_Printf (S_COLOR_YELLOW"Warning: cvar '%s' must be an integer (%f)\n", name, var->value);
        Cvar_Set_Internal(var, va("%d", var->integer),false);
    }
#else
    if (isInteger && (var->value != (float)((int) var->value)))
    {
        char const *name = Q_STGetString(cvarNames, var->index);
        Com_Printf (S_COLOR_YELLOW"Warning: cvar '%s' must be an integer (%f)\n", name, var->value);
        Cvar_Set_Internal(var, va("%d", var->integer),false);
    }
#endif
    if (var->value < min)
    {
        char const *name = Q_STGetString(cvarNames, var->index);
        Com_Printf (S_COLOR_YELLOW"Warning: cvar '%s' is out of range (%f < %f)\n", name, var->value, min);
        Cvar_Set_Internal(var, va("%d", var->integer),false);
    }
    else if (var->value > max)
    {
        char const *name = Q_STGetString(cvarNames, var->index);
        Com_Printf (S_COLOR_YELLOW"Warning: cvar '%s' is out of range (%f > %f)\n", name, var->value, max);
        Cvar_Set_Internal(var, va("%d", var->integer),false);
    }
}

const char *Cvar_GetName(cvar_t *var) {
    return var ? Q_STGetString(cvarNames, var->index) : NULL;
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init (void)
{
    memset(cvar_vars, 0, sizeof(cvar_vars));
	Cmd_AddCommand ("set", Cvar_Set_f);
	Cmd_AddCommand ("toggle", Cvar_Toggle_f);
	Cmd_AddCommand ("reset", Cvar_Reset_f);
	Cmd_AddCommand ("cvarlist", Cvar_List_f);

}
