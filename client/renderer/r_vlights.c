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

// Vic did core of this - THANK YOU KING VIC!!! - psychospaz

#include "include/r_local.h"
#include "include/vlights.h"

#define VLIGHT_CLAMP_MIN	50.0f
#define VLIGHT_CLAMP_MAX	256.0f

#define VLIGHT_GRIDSIZE_X	256
#define VLIGHT_GRIDSIZE_Y	256

vec3_t	vlightgrid[VLIGHT_GRIDSIZE_X][VLIGHT_GRIDSIZE_Y];

void VLight_InitAnormTable (void)
{
	int32_t x, y;
	float angle;
	float sp, sy, cp, cy;
	
	for ( x = 0; x < VLIGHT_GRIDSIZE_X; x++ )
	{
		angle = (x * 360 / VLIGHT_GRIDSIZE_X) * ( M_PI / 180.0f );
		sy = sin(angle);
		cy = cos(angle);

		for ( y = 0; y < VLIGHT_GRIDSIZE_Y; y++ )
		{
			angle = (y * 360 / VLIGHT_GRIDSIZE_X) * ( M_PI / 180.0f );
			sp = sin(angle);
			cp = cos(angle);
			
			vlightgrid[x][y][0] = cp*cy;
			vlightgrid[x][y][1] = cp*sy;
			vlightgrid[x][y][2] = -sp;
		}
	}
}

float VLight_GetLightValue ( vec3_t normal, vec3_t dir, float apitch, float ayaw)
{
    int32_t pitchofs, yawofs;
    float angle1, angle2, light;
    
    if ( normal[1] == 0 && normal[0] == 0 )
    {
        angle2 = 0;
        if ( normal[2] > 0 )
            angle1 = 90;
        else
            angle1 = 270;
    }
    else
    {
        float forward;
        
        angle2 = atan2( normal[1], normal[0] ) * ( 180.0f / M_PI );
        if (angle2 < 0)
            angle2 += 360;
        
        forward = sqrt ( normal[0]*normal[0] + normal[1]*normal[1] );
        angle1 = atan2( normal[2], forward ) * ( 180.0f / M_PI );
        if (angle1 < 0)
            angle1 += 360;
    }
    
    pitchofs = ( angle1 + apitch ) * VLIGHT_GRIDSIZE_X / 360;
    yawofs = ( angle2 + ayaw ) * VLIGHT_GRIDSIZE_Y / 360;
    
    pitchofs = fmodf(pitchofs, VLIGHT_GRIDSIZE_X);
    if (pitchofs < 0)
        pitchofs += VLIGHT_GRIDSIZE_X;
    
    yawofs = fmodf(yawofs, VLIGHT_GRIDSIZE_Y);
    if (yawofs < 0)
        yawofs += VLIGHT_GRIDSIZE_Y;
    
    
    light = ( DotProduct( vlightgrid[pitchofs][yawofs], dir ) + 2.0f ) * 63.5f;
    light = clamp(light, VLIGHT_CLAMP_MIN, VLIGHT_CLAMP_MAX);
    
    return light * ( 1.0f / 256.0f );
    
}

float VLight_GetDLightValue ( vec3_t normal, vec3_t dir, float apitch, float ayaw)
{
    int32_t pitchofs, yawofs;
    float angle1, angle2, light;
    
    if ( normal[1] == 0 && normal[0] == 0 )
    {
        angle2 = 0;
        if ( normal[2] > 0 )
            angle1 = 90;
        else
            angle1 = 270;
    }
    else
    {
        float forward;
        
        angle2 = atan2( normal[1], normal[0] ) * ( 180.0f / M_PI );
        if (angle2 < 0)
            angle2 += 360;
        
        forward = sqrt ( normal[0]*normal[0] + normal[1]*normal[1] );
        angle1 = atan2( normal[2], forward ) * ( 180.0f / M_PI );
        if (angle1 < 0)
            angle1 += 360;
    }
    
    pitchofs = ( angle1 + apitch ) * VLIGHT_GRIDSIZE_X / 360;
    yawofs = ( angle2 + ayaw ) * VLIGHT_GRIDSIZE_Y / 360;
    
    pitchofs = fmodf(pitchofs, VLIGHT_GRIDSIZE_X);
    if (pitchofs < 0)
        pitchofs += VLIGHT_GRIDSIZE_X;
    
    yawofs = fmodf(yawofs, VLIGHT_GRIDSIZE_Y);
    if (yawofs < 0)
        yawofs += VLIGHT_GRIDSIZE_Y;
    
    light = DotProduct( vlightgrid[pitchofs][yawofs], dir );
    
    light = clamp(light, 0, 1);
    
    return light;
}

float VLight_LerpLight ( int32_t index1, int32_t index2, float ilerp, vec3_t dir, vec3_t angles)
{
	vec3_t normal;

	normal[0] = r_avertexnormals[index1][0] + ( r_avertexnormals[index2][0] - r_avertexnormals[index1][0] ) * ilerp;
	normal[1] = r_avertexnormals[index1][1] + ( r_avertexnormals[index2][1] - r_avertexnormals[index1][1] ) * ilerp;
	normal[2] = r_avertexnormals[index1][2] + ( r_avertexnormals[index2][2] - r_avertexnormals[index1][2] ) * ilerp;
	VectorNormalize ( normal );

	return VLight_GetLightValue( normal, dir, angles[PITCH], angles[YAW]);
}

float VLight_LerpDLight ( int32_t index1, int32_t index2, float ilerp, vec3_t dir, vec3_t angles)
{
    vec3_t normal;
    
    normal[0] = r_avertexnormals[index1][0] + ( r_avertexnormals[index2][0] - r_avertexnormals[index1][0] ) * ilerp;
    normal[1] = r_avertexnormals[index1][1] + ( r_avertexnormals[index2][1] - r_avertexnormals[index1][1] ) * ilerp;
    normal[2] = r_avertexnormals[index1][2] + ( r_avertexnormals[index2][2] - r_avertexnormals[index1][2] ) * ilerp;
    VectorNormalize ( normal );
    
    return VLight_GetDLightValue( normal, dir, angles[PITCH], angles[YAW]);
}


void VLight_Init (void)
{
	VLight_InitAnormTable ();
}
