/*
 * $Id: EpsViewer.h,v 3.24 1992/02/29 16:27:40 woo Exp woo $
 *
 * $Log: EpsViewer.h,v $
 * Revision 3.24  1992/02/29  16:27:40  woo
 * gnuplot3.2, beta 4
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


