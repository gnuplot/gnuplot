/*
 * $Id: epsviewe.h%v 3.50 1993/07/09 05:35:24 woo Exp $
 *
 */

#import <appkit/Application.h>
#import <appkit/graphics.h>
#import <dpsclient/dpsclient.h>
#import <appkit/Window.h>

@interface EpsViewer:Application
{
	id theNewWin;
}

- windowCreate:(NXCoord)width Height:(NXCoord)height;
- (NXRect *)nextRectForWidth:(NXCoord)width Height:(NXCoord)height;

@end


