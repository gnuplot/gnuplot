#ifndef lint
static char *RCSid = "$Id: epsviewe.m%v 3.50.1.6 1993/07/27 03:35:12 woo Exp $";
#endif


#import "epsviewe.h"
#import <appkit/OpenPanel.h>
#import <appkit/View.h>

@implementation EpsViewer

- windowCreate:(NXCoord) width Height:(NXCoord) height
{
	
	/* create the new window, in a good place */
	theNewWin = [Window
		newContent:[self nextRectForWidth:width Height:height]
		style:NX_TITLEDSTYLE
		backing:NX_RETAINED
		buttonMask:(NX_CLOSEBUTTONMASK | NX_MINIATURIZEBUTTONMASK)
		defer:NO];
	/* we need to receive windowDidBecomeMain: and windowDidResignMain: */
	[theNewWin setDelegate:self];
	/*
         * create a new View, make it the contentView of our new window,
	 * and destroy the window's old contentView 
	 */
        [[theNewWin setContentView:[[View alloc] init]] free];
		/* display the window, and bring it forth */
	[theNewWin display];
	[theNewWin makeKeyAndOrderFront:self];	
/*	[theNewWin orderBack:self];			*/
	/* show the frame */
	return self;
}
 
/***************************************************************************/
/* nextRectForWidth:Height: - return the next good content rectangle       */
/*  from Carl F. Sutter's wonderful ViewGif2 'Controller' method...        */
/***************************************************************************/
/* nextTopLeft - return the next good top left window position		   */
/***************************************************************************/

- (NXRect *)nextRectForWidth:(NXCoord)width Height:(NXCoord)height
{
#define OFFSET 10.0
#define MAX_STEPS 20
#define INITIAL_X 356.0
#define INITIAL_Y 241.0
	NXPoint         nxpTopLeft;
	NXRect          nxrTemp;	/* used to find window height	 */
	NXRect          nxrWinHeight;	/* bounds of enclosing window	 */
	NXSize          nxsScreen;	/* size of screen		 */
	static NXRect   nxrResult;	/* the Answer!			 */
	static int      nCurStep = 0;

	/* find a good top-left coord */
	nxpTopLeft.x = INITIAL_X + nCurStep * OFFSET;
	nxpTopLeft.y = INITIAL_Y + nCurStep * OFFSET;
	if (++nCurStep > MAX_STEPS)
		nCurStep = 0;
	/* find window height using nxrTemp */
	nxrTemp.size.width = width;
	nxrTemp.size.height = height;
	nxrTemp.origin.x = nxrTemp.origin.y = 0;
	[Window getFrameRect:&nxrWinHeight forContentRect:&nxrTemp
	 style:NX_TITLEDSTYLE];
	[NXApp getScreenSize:&nxsScreen];
	/* find the lower-left coord */
	nxrResult.origin.x = nxpTopLeft.x;
	nxrResult.origin.y = nxsScreen.height - nxrWinHeight.size.height - nxpTopLeft.y;
	nxrResult.size.width = width;
	nxrResult.size.height = height;
	return (&nxrResult);
}


// Keep compiler quiet
- (const char *) rcsid
{
    return RCSid;
}

@end

