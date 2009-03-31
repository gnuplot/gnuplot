/*
 * $Id: gnuplot_common.js,v 1.0 2009/03/20 04:07:54 sfeam Exp $
 */
// Shared helper routines for gnuplot's canvas terminal driver.
// Link to this file by reference rather than including the function definitions
// in every *.js file produced by the canvas terminal.
//
function L(x,y) {
  if (zoomed) {
    zoom = zoomXY(x/10.0,y/10.0);
    ctx.lineTo(zoom.x,zoom.y);
  } else
    ctx.lineTo(x/10.0,y/10.0);
}
function M(x,y) {
  if (zoomed) {
    zoom = zoomXY(x/10.0,y/10.0);
    ctx.moveTo(zoom.x,zoom.y);
  } else
    ctx.moveTo(x/10.0,y/10.0);
}
function R(x,y,w,h) {
  if (zoomed) {
    zoom = zoomXY(x/10.0,y/10.0);
    ctx.fillRect(zoom.x, zoom.y, zoomW(w/10.0), zoomH(h/10.0));
  } else
    ctx.fillRect(x/10.0, y/10.0, w/10.0, h/10.0);
}
function T(x,y,fontsize,justify,string) {
  xx = x/10.0; yy = y/10.0;
  if (zoomed) {
    zoom = zoomXY(xx,yy);
    if (zoom.clip) return;
    xx = zoom.x; yy = zoom.y;
    if (plot_xmin < xx && xx < plot_xmax && plot_ybot > yy && yy > plot_ytop)
      if ((typeof(zoom_text) != "undefined") && (zoom_text == true))
	fontsize = Math.sqrt(zoomW(fontsize)*zoomH(fontsize));
  }
  if (justify=="") ctx.drawText("sans", fontsize, xx, yy, string);
  else if (justify=="Right") ctx.drawTextRight("sans", fontsize, xx, yy, string);
  else if (justify=="Center") ctx.drawTextCenter("sans", fontsize, xx, yy, string);
}
function TR(x,y,angle,fontsize,justify,string) {
  ctx.save(); xx = x/10.0; yy = y/10.0;
  if (zoomed) {zoom = zoomXY(xx,yy); ctx.translate(zoom.x,zoom.y);}
  else ctx.translate(xx,yy);
  ctx.rotate(angle * Math.PI / 180);
  if (justify=="") ctx.drawText("sans", fontsize, 0, 0, string);
  else if (justify=="Right") ctx.drawTextRight("sans", fontsize, 0, 0, string);
  else if (justify=="Center") ctx.drawTextCenter("sans", fontsize, 0, 0, string);
  ctx.restore();
}
function bp(x,y) // begin polygon
    { ctx.beginPath(); M(x,y); }
function cfp() // close and fill polygon
    { ctx.closePath(); ctx.fill(); }
function cfsp() // close and fill polygon with stroke color
    { ctx.closePath(); ctx.fillStyle = ctx.strokeStyle; ctx.fill(); }
function Pt(N,x,y,w) {
    xx = x; yy = y;
    if (zoomed) {zoom = zoomXY(xx,yy); xx = zoom.x; yy = zoom.y; if (zoom.clip) return;}
    switch (N)
    {
    case 0:
	ctx.beginPath();
	ctx.moveTo(xx-w,yy); ctx.lineTo(xx+w,yy);
	ctx.moveTo(xx,yy-w); ctx.lineTo(xx,yy+w);
	ctx.stroke();
	break;
    case 1:
	ww = w * 3/4;
	ctx.beginPath();
	ctx.moveTo(xx-ww,yy-ww); ctx.lineTo(xx+ww,yy+ww);
	ctx.moveTo(xx+ww,yy-ww); ctx.lineTo(xx-ww,yy+ww);
	ctx.stroke();
	break;
    case 2:
	Pt(0,x,y,w); Pt(1,x,y,w);
	break;
    case 3:
	ctx.strokeRect(xx-w/2,yy-w/2,w,w);
	break;
    case 4:
	ctx.save(); ctx.strokeRect(xx-w/2,yy-w/2,w,w); ctx.restore();
	ctx.fillRect(xx-w/2,yy-w/2,w,w);
	break;
    case 5:
	ctx.beginPath(); ctx.arc(xx,yy,w/2,0,Math.PI*2,true); ctx.stroke();
	break;
    default:
    case 6:
	ctx.beginPath(); ctx.arc(xx,yy,w/2,0,Math.PI*2,true); ctx.fill();
	break;
    case 7:
	ctx.beginPath();
	ctx.moveTo(xx,yy-w); ctx.lineTo(xx-w,yy+w/2); ctx.lineTo(xx+w,yy+w/2);
	ctx.closePath();
	ctx.stroke();
	break;
    case 8:
	ctx.beginPath();
	ctx.moveTo(xx,yy-w); ctx.lineTo(xx-w,yy+w/2); ctx.lineTo(xx+w,yy+w/2);
	ctx.closePath();
	ctx.fill();
	break;
    }
}
