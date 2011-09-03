/*
 * $Id: gnuplot_svg.js,v 1.5 2011/04/09 20:56:43 sfeam Exp $
 */
// Javascript routines for interaction with SVG documents produced by 
// gnuplot's SVG terminal driver.

var gnuplot_svg = { };

gnuplot_svg.version = "03 September 2011";

gnuplot_svg.SVGDoc = null;
gnuplot_svg.SVGRoot = null;

gnuplot_svg.Init = function(e)
{
   gnuplot_svg.SVGDoc = e.target.ownerDocument;
   gnuplot_svg.SVGRoot = gnuplot_svg.SVGDoc.documentElement;
   gnuplot_svg.axisdate = new Date();
}

gnuplot_svg.toggleVisibility = function(evt, targetId)
{
   var newTarget = evt.target;
   if (targetId)
      newTarget = gnuplot_svg.SVGDoc.getElementById(targetId);

   var newValue = newTarget.getAttributeNS(null, 'visibility')

   if ('hidden' != newValue)
      newValue = 'hidden';
   else
      newValue = 'visible';

   newTarget.setAttributeNS(null, 'visibility', newValue);
   evt.preventDefault();
   evt.stopPropagation();
}

// Mouse tracking echos coordinates to a floating text box

gnuplot_svg.getText = function() {
	return(document.getElementById("coord_text"));
}

gnuplot_svg.updateCoordBox = function(t, evt) {
    /* 
     * Apply screen CTM transformation to the evt screenX and screenY to get 
     * coordinates in SVG coordinate space.  Use scaling parameters stored in
     * the plot document by gnuplot to convert further into plot coordinates.
     * Then position the floating text box using the SVG coordinates.
     */
    var m = document.documentElement.getScreenCTM();
    var p = document.documentElement.createSVGPoint(); 
    p.x = evt.clientX;
    p.y = evt.clientY; 
    p = p.matrixTransform(m.inverse()); 
    t.setAttribute("x", p.x);
    t.setAttribute("y", p.y);
   
    plotcoord = gnuplot_svg.mouse2plot(p.x,p.y);

    if (gnuplot_svg.polar_mode) {
	polar = gnuplot_svg.convert_to_polar(plotcoord.x,plotcoord.y);
	label_x = "ang= " + polar.ang.toPrecision(4);
	label_y = "R= " + polar.r.toPrecision(4);
    } else if (gnuplot_svg.plot_timeaxis_x == "Date") {
	gnuplot_svg.axisdate.setTime(1000. * (plotcoord.x + 946684800));
	year = gnuplot.axisdate.getUTCFullYear();
	month = gnuplot.axisdate.getUTCMonth();
	date = gnuplot.axisdate.getUTCDate();
	label_x = (" " + date).slice (-2) + "/"
		+ ("0" + (month+1)).slice (-2) + "/"
		+ year;
	label_y = plotcoord.y.toFixed(2);
    } else if (gnuplot_svg.plot_timeaxis_x == "Time") {
	gnuplot_svg.axisdate.setTime(1000. * (plotcoord.x + 946684800));
	hour = gnuplot_svg.axisdate.getUTCHours();
	minute = gnuplot_svg.axisdate.getUTCMinutes();
	second = gnuplot_svg.axisdate.getUTCSeconds();
	label_x = ("0" + hour).slice (-2) + ":" 
		+ ("0" + minute).slice (-2) + ":"
		+ ("0" + second).slice (-2);
	label_y = plotcoord.y.toFixed(2);
    } else if (gnuplot_svg.plot_timeaxis_x == "DateTime") {
	gnuplot_svg.axisdate.setTime(1000. * (plotcoord.x + 946684800));
	label_x = gnuplot_svg.axisdate.toUTCString();
	label_y = plotcoord.y.toFixed(2);
    } else {
	label_x = plotcoord.x.toFixed(2);
	label_y = plotcoord.y.toFixed(2);
    }

    while (null != t.firstChild) {
    	t.removeChild(t.firstChild);
    }
    var textNode = document.createTextNode(".  "+label_x+" "+label_y);
    t.appendChild(textNode);
}

gnuplot_svg.showCoordBox = function(evt) {
    var t = gnuplot_svg.getText();
    if (null != t) {
    	t.setAttribute("visibility", "visible");
    	gnuplot_svg.updateCoordBox(t, evt);
    }
}

gnuplot_svg.moveCoordBox = function(evt) {
    var t = gnuplot_svg.getText();
    if (null != t)
    	gnuplot_svg.updateCoordBox(t, evt);
}

gnuplot_svg.hideCoordBox = function(evt) {
    var t = gnuplot_svg.getText();
    if (null != t)
    	t.setAttribute("visibility", "hidden");
}

gnuplot_svg.toggleCoordBox = function(evt) {
    var t = gnuplot_svg.getText();
    if (null != t) {
	state = t.getAttribute('visibility');
	if ('hidden' != state)
	    state = 'hidden';
	else
	    state = 'visible';
	t.setAttribute('visibility', state);
    }
}

gnuplot_svg.toggleGrid = function() {
    if (!gnuplot_svg.SVGDoc.getElementsByClassName) // Old browsers
	return;
    var grid = gnuplot_svg.SVGDoc.getElementsByClassName('gridline');
    for (var i=0; i<grid.length; i++) {
	state = grid[i].getAttribute('visibility');
	grid[i].setAttribute('visibility', (state == 'hidden') ? 'visible' : 'hidden');
    }
}

// Convert from svg panel mouse coordinates to the coordinate
// system of the gnuplot figure
gnuplot_svg.mouse2plot = function(mousex,mousey) {
    var plotcoord = new Object;
    var plotx = mousex - gnuplot_svg.plot_xmin;
    var ploty = mousey - gnuplot_svg.plot_ybot;

    if (gnuplot_svg.plot_logaxis_x != 0) {
	x = Math.log(gnuplot_svg.plot_axis_xmax)
	  - Math.log(gnuplot_svg.plot_axis_xmin);
	x = x * (plotx / (gnuplot_svg.plot_xmax - gnuplot_svg.plot_xmin))
	  + Math.log(gnuplot_svg.plot_axis_xmin);
	x = Math.exp(x);
    } else {
	x = gnuplot_svg.plot_axis_xmin + (plotx / (gnuplot_svg.plot_xmax-gnuplot_svg.plot_xmin)) * (gnuplot_svg.plot_axis_xmax - gnuplot_svg.plot_axis_xmin);
    }

    if (gnuplot_svg.plot_logaxis_y != 0) {
	y = Math.log(gnuplot_svg.plot_axis_ymax)
	  - Math.log(gnuplot_svg.plot_axis_ymin);
	y = y * (ploty / (gnuplot_svg.plot_ytop - gnuplot_svg.plot_ybot))
	  + Math.log(gnuplot_svg.plot_axis_ymin);
	y = Math.exp(y);
    } else {
	y = gnuplot_svg.plot_axis_ymin + (ploty / (gnuplot_svg.plot_ytop-gnuplot_svg.plot_ybot)) * (gnuplot_svg.plot_axis_ymax - gnuplot_svg.plot_axis_ymin);
    }

    plotcoord.x = x;
    plotcoord.y = y;
    return plotcoord;
}

gnuplot_svg.convert_to_polar = function (x,y)
{
    polar = new Object;
    var phi, r;
    phi = Math.atan2(y,x);
    if (gnuplot_svg.plot_logaxis_r) 
        r = Math.exp( (x/Math.cos(phi) + Math.log(gnuplot_svg.plot_axis_rmin)/Math.LN10) * Math.LN10);
    else
        r = x/Math.cos(phi) + gnuplot_svg.plot_axis_rmin;
    polar.ang = phi * 180./Math.PI;
    polar.r = r;
    return polar;
}

