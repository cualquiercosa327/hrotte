/* Routines from winrott needed for the highres support for the SDL port */

#include <string.h>
#include "WinRott.h"
#include "modexlib.h"

int iGLOBAL_SCREENWIDTH = 640;//bna val 800
int iGLOBAL_SCREENHEIGHT = 480;//bna val 600
int iGLOBAL_SCREENBWIDE;
int iG_SCREENWIDTH;// default screen width in bytes

int iGLOBAL_HEALTH_X;
int iGLOBAL_HEALTH_Y;
int iGLOBAL_AMMO_X;
int iGLOBAL_AMMO_Y;

int iGLOBAL_FOCALWIDTH;
double dGLOBAL_FPFOCALWIDTH;

double dTopYZANGLELIMIT;

int iG_X_center;
int iG_Y_center;

boolean iG_aimCross = 0;

extern int viewheight;
extern int viewwidth;

//----------------------------------------------------------------------
#define FINEANGLES                        2048
void SetRottScreenRes( int Width, int Height ) {
	iGLOBAL_SCREENWIDTH = Width;
	iGLOBAL_SCREENHEIGHT = Height;

	iGLOBAL_SCREENBWIDE = iGLOBAL_SCREENWIDTH * ( 96 / 320 );
	iG_SCREENWIDTH = iGLOBAL_SCREENWIDTH * ( 96 / 320 ); // default screen width in bytes

	iGLOBAL_HEALTH_X = 20;
	iGLOBAL_HEALTH_Y = iGLOBAL_SCREENHEIGHT - 15;
	iGLOBAL_AMMO_X = iGLOBAL_SCREENWIDTH - 20;
	iGLOBAL_AMMO_Y = iGLOBAL_SCREENHEIGHT - 15;    // Was -16, but this was probably in error...

	if ( iGLOBAL_SCREENWIDTH == 320 ) {
		iGLOBAL_FOCALWIDTH = 160;
		dGLOBAL_FPFOCALWIDTH = 160.0;

		dTopYZANGLELIMIT = ( 44 * FINEANGLES / 360 );
	}
	if ( iGLOBAL_SCREENWIDTH == 640 ) {
		iGLOBAL_FOCALWIDTH = 180;
		dGLOBAL_FPFOCALWIDTH = 180.0;

		dTopYZANGLELIMIT = ( 42 * FINEANGLES / 360 );
	}
	if ( iGLOBAL_SCREENWIDTH == 800 ) {
		iGLOBAL_FOCALWIDTH = 200;
		dGLOBAL_FPFOCALWIDTH = 200.0;

		dTopYZANGLELIMIT = ( 90 * FINEANGLES / 360 );
	}

//	GetCurrentDirectory(sizeof(ApogeePath),ApogeePath);// curent directory name
}

//----------------------------------------------------------------------
//luckey for me that I am not programmin a 386 or the next
//4 function would never have worked. bna++
extern int viewsize;
void MoveScreenUpLeft( void ) {
	int startX, startY, startoffset;
	byte * Ycnt, * b;
//   SetTextMode (  );
	b = ( byte * ) bufferofs;
	b += ((( iGLOBAL_SCREENHEIGHT - viewheight ) / 2 ) * iGLOBAL_SCREENWIDTH )
		+ (( iGLOBAL_SCREENWIDTH - viewwidth ) / 2 );
	if ( viewsize == 8 ) {
		b += 8 * iGLOBAL_SCREENWIDTH;
	}
	startX = 3; //take 3 pixels to the right
	startY = 3; //take 3 lines down
	startoffset = ( startY * iGLOBAL_SCREENWIDTH ) + startX;

	for ( Ycnt = b; Ycnt < b + (( viewheight - startY ) * iGLOBAL_SCREENWIDTH ); Ycnt += iGLOBAL_SCREENWIDTH ) {
		memcpy( Ycnt, Ycnt + startoffset, viewwidth - startX );
	}
}
//----------------------------------------------------------------------
void MoveScreenDownLeft( void ) {
	int startX, startY, startoffset;
	byte * Ycnt, * b;
//   SetTextMode (  );
	b = ( byte * ) bufferofs;
	b += ((( iGLOBAL_SCREENHEIGHT - viewheight ) / 2 ) * iGLOBAL_SCREENWIDTH )
		+ (( iGLOBAL_SCREENWIDTH - viewwidth ) / 2 );
	if ( viewsize == 8 ) {
		b += 8 * iGLOBAL_SCREENWIDTH;
	}
	startX = 3; //take 3 pixels to the right
	startY = 3; //take 3 lines down
	startoffset = ( startY * iGLOBAL_SCREENWIDTH );//+startX;

	//Ycnt starts in botton of screen and copys lines upwards
	for ( Ycnt = b + (( viewheight - startY - 1 ) * iGLOBAL_SCREENWIDTH ); Ycnt > b; Ycnt -= iGLOBAL_SCREENWIDTH ) {
		memcpy( Ycnt + startoffset, Ycnt + startX, viewwidth - startX );
	}
}
//----------------------------------------------------------------------
void MoveScreenUpRight( void ) {
	int startX, startY, startoffset;
	byte * Ycnt, * b;
//   SetTextMode (  );
	b = ( byte * ) bufferofs;

	b += ((( iGLOBAL_SCREENHEIGHT - viewheight ) / 2 ) * iGLOBAL_SCREENWIDTH )
		+ (( iGLOBAL_SCREENWIDTH - viewwidth ) / 2 );
	if ( viewsize == 8 ) {
		b += 8 * iGLOBAL_SCREENWIDTH;
	}
	startX = 3; //take 3 pixels to the right
	startY = 3; //take 3 lines down
	startoffset = ( startY * iGLOBAL_SCREENWIDTH );//+startX;

	for ( Ycnt = b; Ycnt < b + (( viewheight - startY ) * iGLOBAL_SCREENWIDTH ); Ycnt += iGLOBAL_SCREENWIDTH ) {
		memcpy( Ycnt + startX, Ycnt + startoffset, viewwidth - startX );
	}
}
//----------------------------------------------------------------------
void MoveScreenDownRight( void ) {
	int startX, startY, startoffset;
	byte * Ycnt, * b;
//   SetTextMode (  );
	b = ( byte * ) bufferofs;

	b += ((( iGLOBAL_SCREENHEIGHT - viewheight ) / 2 ) * iGLOBAL_SCREENWIDTH )
		+ (( iGLOBAL_SCREENWIDTH - viewwidth ) / 2 );
	if ( viewsize == 8 ) {
		b += 8 * iGLOBAL_SCREENWIDTH;
	}
	startX = 3; //take 3 pixels to the right
	startY = 3; //take 3 lines down
	startoffset = (startY * iGLOBAL_SCREENWIDTH) + startX;

	//Ycnt starts in botton of screen and copys lines upwards
	for ( Ycnt = b + ((viewheight - startY - 1) * iGLOBAL_SCREENWIDTH); Ycnt > b; Ycnt -= iGLOBAL_SCREENWIDTH ) {
		memcpy( Ycnt + startoffset, Ycnt, viewwidth - startX );
	}
}


