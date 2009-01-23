/*
 * $Id: gnuplot_mouse.js,v 1.1 2009/01/14 22:23:22 sfeam Exp $
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
var last_x = 0;
var lasy_y = 0;

// These will be initialized by the gnuplot canvas-drawing function
var plot_xmin = 0;
var plot_xmax = 0;
var plot_ymin = 0;
var plot_ymax = 0;
var plot_term_ymax = 0;
var plot_axis_xmin = 0;
var plot_axis_xmax = 0;
var plot_axis_ymin = 0;
var plot_axis_ymax = 0;
var plot_logaxis_x = 0;
var plot_logaxis_y = 0;
var grid_lines = 1;

function gnuplot_init()
{
  document.getElementById("gnuplot_canvas").onmousemove = mouse_update;
  document.getElementById("gnuplot_canvas").onmouseup = saveclick;
  document.getElementById("gnuplot_grid_icon").onmouseup = toggle_grid;
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

  var plotx = mousex - plot_xmin;
  var ploty = plot_term_ymax - mousey - plot_ymin;

  // Limit tracking to the interior of the plot
  if (plotx < 0 || ploty < 0) return;
  if (mousex > plot_xmax || mousey < plot_term_ymax-plot_ymax) return;

  if (plot_logaxis_x != 0) {
	x = Math.log(plot_axis_xmax) - Math.log(plot_axis_xmin);
	x = x * (plotx / (plot_xmax-plot_xmin)) + Math.log(plot_axis_xmin);
	x = Math.exp(x);
  } else {
	x =  plot_axis_xmin + (plotx / (plot_xmax-plot_xmin)) * (plot_axis_xmax - plot_axis_xmin);
  }

  if (plot_logaxis_y != 0) {
	y = Math.log(plot_axis_ymax) - Math.log(plot_axis_ymin);
	y = y * (ploty / (plot_ymax-plot_ymin)) + Math.log(plot_axis_ymin);
	y = Math.exp(y);
  } else {
	y =  plot_axis_ymin + (ploty / (plot_ymax-plot_ymin)) * (plot_axis_ymax - plot_axis_ymin);
  }

  document.getElementById('span_scaled_x').innerHTML = x.toPrecision(4);
  document.getElementById('span_scaled_y').innerHTML = y.toPrecision(4);
}

function saveclick(e)
{
  mouse_update(e);
  
  // Limit tracking to the interior of the plot
  if (plotx < 0 || ploty < 0) return;
  if (mousex > plot_xmax || mousey < plot_term_ymax-plot_ymax) return;

  document.getElementById('span_last_x').innerHTML = document.getElementById('span_scaled_x').innerHTML;
  document.getElementById('span_last_y').innerHTML = document.getElementById('span_scaled_y').innerHTML;
}

function toggle_grid(e)
{
  if (grid_lines == 0) grid_lines = 1;
  else grid_lines = 0;

  ctx.clearRect(0,0,plot_term_xmax,plot_term_ymax);
  gnuplot_canvas();
}
