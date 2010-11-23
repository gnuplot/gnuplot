/*
 * $Id: gnuplot_mouse.js,v 1.9 2010/11/24 21:54:48 sfeam Exp $
 */
// Mousing code for use with gnuplot's 'canvas' terminal driver.
// The functions defined here assume that the javascript plot produced by
// gnuplot initializes the plot boundary and scaling parameters.

var mousex = 0;
var mousey = 0;
var plotx = 0;
var ploty = 0;
var scaled_x = 0;
var scaled_y = 0;

// These will be initialized by the gnuplot canvas-drawing function
var plot_xmin = 0;
var plot_xmax = 0;
var plot_ybot = 0;
var plot_ytop = 0;
var plot_width  = 0
var plot_height = 0
var plot_term_ymax = 0;
var plot_axis_xmin = 0;
var plot_axis_xmax = 0;
var plot_axis_width  = 0;
var plot_axis_height = 0;
var plot_axis_ymin = 0;
var plot_axis_ymax = 0;
var plot_axis_x2min = "none";
var plot_axis_y2min = "none";
var plot_logaxis_x = 0;
var plot_logaxis_y = 0;
var grid_lines = true;
var zoom_text = false;

// These are the equivalent parameters while zooming
var zoom_axis_xmin = 0;
var zoom_axis_xmax = 0;
var zoom_axis_ymin = 0;
var zoom_axis_ymax = 0;
var zoom_axis_x2min = 0;
var zoom_axis_x2max = 0;
var zoom_axis_y2min = 0;
var zoom_axis_y2max = 0;
var zoom_axis_width = 0;
var zoom_axis_height = 0;
var zoom_temp_xmin = 0;
var zoom_temp_ymin = 0;
var zoom_temp_x2min = 0;
var zoom_temp_y2min = 0;
var zoom_in_progress = false;

var full_canvas_image = null;

function gnuplot_init()
{
  if (document.getElementById("gnuplot_canvas"))
      document.getElementById("gnuplot_canvas").onmousemove = mouse_update;
  if (document.getElementById("gnuplot_canvas"))
      document.getElementById("gnuplot_canvas").onmouseup = zoom_in;
  if (document.getElementById("gnuplot_canvas"))
      document.getElementById("gnuplot_canvas").onmousedown = saveclick;
  if (document.getElementById("gnuplot_canvas"))
      document.getElementById("gnuplot_canvas").onkeypress = do_hotkey;
  if (document.getElementById("gnuplot_grid_icon"))
      document.getElementById("gnuplot_grid_icon").onmouseup = toggle_grid;
  if (document.getElementById("gnuplot_textzoom_icon"))
      document.getElementById("gnuplot_textzoom_icon").onmouseup = toggle_zoom_text;
  if (document.getElementById("gnuplot_rezoom_icon"))
      document.getElementById("gnuplot_rezoom_icon").onmouseup = rezoom;
  if (document.getElementById("gnuplot_unzoom_icon"))
      document.getElementById("gnuplot_unzoom_icon").onmouseup = unzoom;
  mouse_update();
}

function getMouseCoordsWithinTarget(event)
{
	var coords = { x: 0, y: 0};

	if(!event) // then we're in a non-DOM (probably IE) browser
	{
		event = window.event;
		if (event) {
			coords.x = event.offsetX;
			coords.y = event.offsetY;
		}
	}
	else		// we assume DOM modeled javascript
	{
		var Element = event.target ;
		var CalculatedTotalOffsetLeft = 0;
		var CalculatedTotalOffsetTop = 0 ;

		while (Element.offsetParent)
 		{
 			CalculatedTotalOffsetLeft += Element.offsetLeft ;     
			CalculatedTotalOffsetTop += Element.offsetTop ;
 			Element = Element.offsetParent ;
 		}

		coords.x = event.pageX - CalculatedTotalOffsetLeft ;
		coords.y = event.pageY - CalculatedTotalOffsetTop ;
	}

	mousex = coords.x;
	mousey = coords.y;
}


function mouse_update(e)
{
  getMouseCoordsWithinTarget(e);

  plotx = mousex - plot_xmin;
  ploty = -(mousey - plot_ybot);

  // Limit tracking to the interior of the plot
  if (plotx < 0 || ploty < 0) return;
  if (mousex > plot_xmax || mousey < plot_ytop) return;

  var axis_xmin = (zoomed) ? zoom_axis_xmin : plot_axis_xmin;
  var axis_xmax = (zoomed) ? zoom_axis_xmax : plot_axis_xmax;
  var axis_ymin = (zoomed) ? zoom_axis_ymin : plot_axis_ymin;
  var axis_ymax = (zoomed) ? zoom_axis_ymax : plot_axis_ymax;

    if (plot_logaxis_x != 0) {
	x = Math.log(axis_xmax) - Math.log(axis_xmin);
	x = x * (plotx / (plot_xmax-plot_xmin)) + Math.log(axis_xmin);
	x = Math.exp(x);
    } else {
	x =  axis_xmin + (plotx / (plot_xmax-plot_xmin)) * (axis_xmax - axis_xmin);
    }

    if (plot_logaxis_y != 0) {
	y = Math.log(axis_ymax) - Math.log(axis_ymin);
	y = y * (-ploty / (plot_ytop-plot_ybot)) + Math.log(axis_ymin);
	y = Math.exp(y);
    } else {
	y =  axis_ymin - (ploty / (plot_ytop-plot_ybot)) * (axis_ymax - axis_ymin);
    }

    if (plot_axis_x2min != "none") {
	axis_x2min = (zoomed) ? zoom_axis_x2min : plot_axis_x2min;
	axis_x2max = (zoomed) ? zoom_axis_x2max : plot_axis_x2max;
	x2 =  axis_x2min + (plotx / (plot_xmax-plot_xmin)) * (axis_x2max - axis_x2min);
	if (document.getElementById(active_plot_name + "_x2"))
	    document.getElementById(active_plot_name + "_x2").innerHTML = x2.toPrecision(4);
    }
    if (plot_axis_y2min != "none") {
	axis_y2min = (zoomed) ? zoom_axis_y2min : plot_axis_y2min;
	axis_y2max = (zoomed) ? zoom_axis_y2max : plot_axis_y2max;
	y2 = axis_y2min - (ploty / (plot_ytop-plot_ybot)) * (axis_y2max - axis_y2min);
	if (document.getElementById(active_plot_name + "_y2"))
	    document.getElementById(active_plot_name + "_y2").innerHTML = y2.toPrecision(4);
    }

  if (document.getElementById(active_plot_name + "_x"))
      document.getElementById(active_plot_name + "_x").innerHTML = x.toPrecision(4);
  if (document.getElementById(active_plot_name + "_y"))
      document.getElementById(active_plot_name + "_y").innerHTML = y.toPrecision(4);

  // Echo the zoom box interactively
  if (zoom_in_progress) {
    // Clear previous box before drawing a new one
    if (full_canvas_image == null) {
      full_canvas_image = ctx.getImageData(0,0,canvas.width,canvas.height);
    } else {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      ctx.putImageData(full_canvas_image,0,0);
    }
    ctx.strokeStyle="rgba(128,128,128,0.60)";
    var x0 = plot_xmin + zoom_temp_plotx;
    var y0 = plot_ybot - zoom_temp_ploty;
    var w = plotx - zoom_temp_plotx;
    var h = -(ploty - zoom_temp_ploty);
    if (w<0) {x0 = x0 + w; w = -w;}
    if (h<0) {y0 = y0 + h; h = -h;}
    ctx.strokeRect(x0,y0,w,h);
  }
}

function saveclick(event)
{
  mouse_update(event);
  
  // Limit tracking to the interior of the plot
  if (plotx < 0 || ploty < 0) return;
  if (mousex > plot_xmax || mousey < plot_ytop) return;

  if (event.which == null) 	/* IE case */
    button= (event.button < 2) ? "LEFT" : ((event.button == 4) ? "MIDDLE" : "RIGHT");
  else				/* All others */
    button= (event.which < 2) ? "LEFT" : ((event.which == 2) ? "MIDDLE" : "RIGHT");

  if (button == "LEFT") {
    ctx.strokeStyle="black";
    ctx.strokeRect(mousex, mousey, 1, 1);
    click = " " + x.toPrecision(4) + ", " + y.toPrecision(4);
    ctx.drawText("sans", 9, mousex, mousey, click);
  }

  // Save starting corner of zoom box
  else {
    zoom_temp_xmin = x;
    zoom_temp_ymin = y;
    if (plot_axis_x2min != "none") zoom_temp_x2min = x2;
    if (plot_axis_y2min != "none") zoom_temp_y2min = y2;
    // Only used to echo the zoom box interactively
    zoom_temp_plotx = plotx;
    zoom_temp_ploty = ploty;
    zoom_in_progress = true;
    full_canvas_image = null;
  }
  return false; // Nobody else should respond to this event
}

function zoom_in(event)
{
  if (!zoom_in_progress)
    return false;

  mouse_update(event);
  
  if (event.which == null) 	/* IE case */
    button= (event.button < 2) ? "LEFT" : ((event.button == 4) ? "MIDDLE" : "RIGHT");
  else				/* All others */
    button= (event.which < 2) ? "LEFT" : ((event.which == 2) ? "MIDDLE" : "RIGHT");

  // Save ending corner of zoom box
  if (button != "LEFT") {
    if (x > zoom_temp_xmin) {
        zoom_axis_xmin = zoom_temp_xmin;
	zoom_axis_xmax = x;
	if (plot_axis_x2min != "none") {
            zoom_axis_x2min = zoom_temp_x2min;
	    zoom_axis_x2max = x2;
	}
    } else {
        zoom_axis_xmin = x;
	zoom_axis_xmax = zoom_temp_xmin;
	if (plot_axis_x2min != "none") {
            zoom_axis_x2min = x2;
	    zoom_axis_x2max = zoom_temp_x2min;
	}
    }
    if (y > zoom_temp_ymin) {
        zoom_axis_ymin = zoom_temp_ymin;
	zoom_axis_ymax = y;
	if (plot_axis_y2min != "none") {
            zoom_axis_y2min = zoom_temp_y2min;
	    zoom_axis_y2max = y2;
	}
    } else {
        zoom_axis_ymin = y;
	zoom_axis_ymax = zoom_temp_ymin;
	if (plot_axis_y2min != "none") {
            zoom_axis_y2min = y2;
	    zoom_axis_y2max = zoom_temp_y2min;
	}
    }
    zoom_axis_width = zoom_axis_xmax - zoom_axis_xmin;
    zoom_axis_height = zoom_axis_ymax - zoom_axis_ymin;
    zoom_in_progress = false;
    rezoom(event);
  }
  return false; // Nobody else should respond to this event
}

function toggle_grid(e)
{
  if (!grid_lines) grid_lines = true;
  else grid_lines = false;
  ctx.clearRect(0,0,plot_term_xmax,plot_term_ymax);
  gnuplot_canvas();
}

function toggle_zoom_text(e)
{
  if (!zoom_text) zoom_text = true;
  else zoom_text = false;
  ctx.clearRect(0,0,plot_term_xmax,plot_term_ymax);
  gnuplot_canvas();
}

function rezoom(e)
{
  if (zoom_axis_width > 0)
    zoomed = true;
  ctx.clearRect(0,0,plot_term_xmax,plot_term_ymax);
  gnuplot_canvas();
}

function unzoom(e)
{
  zoomed = false;
  ctx.clearRect(0,0,plot_term_xmax,plot_term_ymax);
  gnuplot_canvas();
}

function zoomXY(x,y)
{
  zoom = new Object;
  var xreal, yreal;

  zoom.x = x; zoom.y = y; zoom.clip = false;

  if (plot_logaxis_x != 0) {
	xreal = Math.log(plot_axis_xmax) - Math.log(plot_axis_xmin);
	xreal = Math.log(plot_axis_xmin) + (x - plot_xmin) * xreal/plot_width;
	zoom.x = Math.log(zoom_axis_xmax) - Math.log(zoom_axis_xmin);
	zoom.x = plot_xmin + (xreal - Math.log(zoom_axis_xmin)) * plot_width/zoom.x;
  } else {
	xreal = plot_axis_xmin + (x - plot_xmin) * (plot_axis_width/plot_width);
	zoom.x = plot_xmin + (xreal - zoom_axis_xmin) * (plot_width/zoom_axis_width);
  }
  if (plot_logaxis_y != 0) {
	yreal = Math.log(plot_axis_ymax) - Math.log(plot_axis_ymin);
	yreal = Math.log(plot_axis_ymin) + (plot_ybot - y) * yreal/plot_height;
	zoom.y = Math.log(zoom_axis_ymax) - Math.log(zoom_axis_ymin);
	zoom.y = plot_ybot - (yreal - Math.log(zoom_axis_ymin)) * plot_height/zoom.y;
  } else {
	yreal = plot_axis_ymin + (plot_ybot - y) * (plot_axis_height/plot_height);
	zoom.y = plot_ybot - (yreal - zoom_axis_ymin) * (plot_height/zoom_axis_height);
  }

  // Limit the zoomed plot to the original plot area
  if (x > plot_xmax) {
    zoom.x = x;
    if (plot_axis_y2min == "none") {
      zoom.y = y;
      return zoom;
    }
    if (plot_ybot <= y && y <= plot_ybot + 15)
      zoom.clip = true;
  }

  else if (x < plot_xmin)
    zoom.x = x;
  else if (zoom.x < plot_xmin)
    { zoom.x = plot_xmin; zoom.clip = true; }
  else if (zoom.x > plot_xmax)
    { zoom.x = plot_xmax; zoom.clip = true; }

  if (y < plot_ytop) {
    zoom.y = y;
    if (plot_axis_x2min == "none") {
      zoom.x = x; zoom.clip = false;
      return zoom;
    }
  }

  else if (y > plot_ybot)
    zoom.y = y;
  else if (zoom.y > plot_ybot)
    { zoom.y = plot_ybot; zoom.clip = true; }
  else if (zoom.y < plot_ytop)
    { zoom.y = plot_ytop; zoom.clip = true; }

  return zoom;
}

function zoomW(w) { return (w*plot_axis_width/zoom_axis_width); }
function zoomH(h) { return (h*plot_axis_height/zoom_axis_height); }

function do_hotkey(event) {
    keychar = String.fromCharCode(event.charCode ? event.charCode : event.keyCode);
    switch (keychar) {
    case 'e':	ctx.clearRect(0,0,plot_term_xmax,plot_term_ymax);
		gnuplot_canvas();
		break;
    case 'g':	toggle_grid();
		break;
    case 'n':	rezoom();
		break;
    case 'r':
		ctx.lineWidth = 0.5;
		ctx.strokeStyle="rgba(128,128,128,0.50)";
		ctx.moveTo(plot_xmin, mousey); ctx.lineTo(plot_xmax, mousey);
		ctx.moveTo(mousex, plot_ybot); ctx.lineTo(mousex, plot_ytop);
		ctx.stroke();
		break;
    case 'p':
    case 'u':	unzoom();
		break;
    case '':	zoom_in_progress = false;
		break;

// Arrow keys
    case '%':	// ctx.drawText("sans", 10, mousex, mousey, "<");
		break;
    case '\'':	// ctx.drawText("sans", 10, mousex, mousey, ">");
		break;
    case '&':	// ctx.drawText("sans", 10, mousex, mousey, "^");
		break;
    case '(':	// ctx.drawText("sans", 10, mousex, mousey, "v");
		break;

    default:	// ctx.drawText("sans", 10, mousex, mousey, keychar);
		return true; // Let someone else handle it
		break;
    }
    return false; // Nobody else should respond to this keypress
}
