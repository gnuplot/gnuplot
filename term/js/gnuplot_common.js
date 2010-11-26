/*
 * $Id: gnuplot_common.js,v 1.4 2010/11/25 04:29:35 sfeam Exp $
 */
// Shared routines for gnuplot's HTML5 canvas terminal driver.

var gnuplot = { };

gnuplot.L = function (x,y) {
  if (gnuplot.zoomed) {
    zoom = gnuplot.zoomXY(x/10.0,y/10.0);
    ctx.lineTo(zoom.x,zoom.y);
  } else
    ctx.lineTo(x/10.0,y/10.0);
}
gnuplot.M = function (x,y) {
  if (gnuplot.zoomed) {
    zoom = gnuplot.zoomXY(x/10.0,y/10.0);
    ctx.moveTo(zoom.x,zoom.y);
  } else
    ctx.moveTo(x/10.0,y/10.0);
}
gnuplot.R = function (x,y,w,h) {
  if (gnuplot.zoomed) {
    zoom = gnuplot.zoomXY(x/10.0,y/10.0);
    ctx.fillRect(zoom.x, zoom.y, gnuplot.zoomW(w/10.0), gnuplot.zoomH(h/10.0));
  } else
    ctx.fillRect(x/10.0, y/10.0, w/10.0, h/10.0);
}
gnuplot.T = function (x,y,fontsize,justify,string) {
  xx = x/10.0; yy = y/10.0;
  if (gnuplot.zoomed) {
    zoom = gnuplot.zoomXY(xx,yy);
    if (zoom.clip) return;
    xx = zoom.x; yy = zoom.y;
    if (gnuplot.plot_xmin < xx && xx < gnuplot.plot_xmax && gnuplot.plot_ybot > yy && yy > gnuplot.plot_ytop)
      if ((typeof(gnuplot.zoom_text) != "undefined") && (gnuplot.zoom_text == true))
	fontsize = Math.sqrt(gnuplot.zoomW(fontsize)*gnuplot.zoomH(fontsize));
  }
  if (justify=="") ctx.drawText("sans", fontsize, xx, yy, string);
  else if (justify=="Right") ctx.drawTextRight("sans", fontsize, xx, yy, string);
  else if (justify=="Center") ctx.drawTextCenter("sans", fontsize, xx, yy, string);
}
gnuplot.TR = function (x,y,angle,fontsize,justify,string) {
  xx = x/10.0; yy = y/10.0;
  if (gnuplot.zoomed) {
    zoom = gnuplot.zoomXY(xx,yy);
    if (zoom.clip) return;
    xx = zoom.x; yy = zoom.y;
    if (gnuplot.plot_xmin < xx && xx < gnuplot.plot_xmax && gnuplot.plot_ybot > yy && yy > gnuplot.plot_ytop)
      if ((typeof(gnuplot.zoom_text) != "undefined") && (gnuplot.zoom_text == true))
	fontsize = Math.sqrt(gnuplot.zoomW(fontsize)*gnuplot.zoomH(fontsize));
  }
  ctx.save();
  ctx.translate(xx,yy);
  ctx.rotate(angle * Math.PI / 180);
  if (justify=="") ctx.drawText("sans", fontsize, 0, 0, string);
  else if (justify=="Right") ctx.drawTextRight("sans", fontsize, 0, 0, string);
  else if (justify=="Center") ctx.drawTextCenter("sans", fontsize, 0, 0, string);
  ctx.restore();
}
gnuplot.bp = function (x,y) // begin polygon
    { ctx.beginPath(); gnuplot.M(x,y); }
gnuplot.cfp = function () // close and fill polygon
    { ctx.closePath(); ctx.fill(); }
gnuplot.cfsp = function () // close and fill polygon with stroke color
    { ctx.closePath(); ctx.fillStyle = ctx.strokeStyle; ctx.fill(); }
gnuplot.Dot = function (x,y) {
    xx = x; yy = y;
    if (gnuplot.zoomed) {zoom = gnuplot.zoomXY(xx,yy); xx = zoom.x; yy = zoom.y; if (zoom.clip) return;}
    ctx.strokeRect(xx,yy,0.5,0.5);
}
gnuplot.Pt = function (N,x,y,w) {
    xx = x; yy = y;
    if (gnuplot.zoomed) {zoom = gnuplot.zoomXY(xx,yy); xx = zoom.x; yy = zoom.y; if (zoom.clip) return;}
    if (w==0) return;
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
	gnuplot.Pt(0,x,y,w); gnuplot.Pt(1,x,y,w);
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

// These methods are place holders that are loaded by gnuplot_dashedlines.js

gnuplot.dashtype  = function (dt) {} ;
gnuplot.dashstart = function (x,y) {gnuplot.M(x,y);} ;
gnuplot.dashstep  = function (x,y) {gnuplot.L(x,y);} ;
gnuplot.pattern   = [];
