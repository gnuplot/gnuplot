// Javascript routines for mouse and keyboard interaction with
// SVG documents produced by gnuplot SVG terminal driver.

// Find your root SVG element
var svg = document.querySelector('svg');

// Create an SVGPoint for future math
var pt = svg.createSVGPoint();

// Get point in global SVG space
function cursorPoint(evt) {
    pt.x = evt.clientX;
    pt.y = evt.clientY;
    return pt.matrixTransform(svg.getScreenCTM().inverse());
}

var gnuplot_svg = {};

gnuplot_svg.version = "04 March 2018";

gnuplot_svg.SVGDoc = null;
gnuplot_svg.SVGRoot = null;

gnuplot_svg.Init = function (e) {
    gnuplot_svg.SVGDoc = e.target.ownerDocument;
    // unused
    gnuplot_svg.SVGRoot = gnuplot_svg.SVGDoc.documentElement;
    gnuplot_svg.axisdate = new Date();

    // zoom and pan with mouse
    gnuplot_svg.interactive = gnuplot_svg.interactiveInit(e.target);
};

gnuplot_svg.toggleVisibility = function (evt, targetId) {
    var newTarget = evt.target;
    if (targetId) {
        newTarget = gnuplot_svg.SVGDoc.getElementById(targetId);
    }

    var newValue = newTarget.getAttributeNS(null, 'visibility');

    if ('hidden' != newValue) {
        newValue = 'hidden';
    }
    else {
        newValue = 'visible';
    }

    newTarget.setAttributeNS(null, 'visibility', newValue);

    if (targetId) {
        newTarget = gnuplot_svg.SVGDoc.getElementById(targetId.concat("_keyentry"));
        if (newTarget) {
            newTarget.setAttributeNS(null, 'style',
                newValue == 'hidden' ? 'filter:url(#greybox)' : 'none');
        }
    }

    evt.preventDefault();
    evt.stopPropagation();
};

// Mouse tracking echos coordinates to a floating text box

gnuplot_svg.getText = function () {
    return (document.getElementById("coord_text"));
};

gnuplot_svg.updateCoordBox = function (t, evt) {
    // Apply screen CTM transformation to the evt screenX and screenY to get
    // coordinates in SVG coordinate space. Use scaling parameters stored in
    // the plot document by gnuplot to convert further into plot coordinates.
    // Then position the floating text box using the SVG coordinates.

    var m = svg.getScreenCTM();
    var p = svg.createSVGPoint();
    var loc = cursorPoint(evt);

    p.x = loc.x;
    p.y = loc.y;
    var label_x, label_y;

    t.setAttribute("x", p.x);
    t.setAttribute("y", p.y);

    var plotcoord = gnuplot_svg.mouse2plot(p.x, p.y);

    if (isNaN(plotcoord.x) || isNaN(plotcoord.y)) {
	// nonlinear axis or custom function mapping [x,y] to plot coordinates
	label_x = "not ";
	label_y = "supported";

    } else if (gnuplot_svg.plot_timeaxis_x == "DMS" || gnuplot_svg.plot_timeaxis_y == "DMS") {
        if (gnuplot_svg.plot_timeaxis_x == "DMS") {
            label_x = gnuplot_svg.convert_to_DMS(x);
        }
        else {
            label_x = plotcoord.x.toFixed(2);
        }
        if (gnuplot_svg.plot_timeaxis_y == "DMS") {
            label_y = gnuplot_svg.convert_to_DMS(y);
        }
        else {
            label_y = plotcoord.y.toFixed(2);
        }

    } else if (gnuplot_svg.polar_mode) {
        polar = gnuplot_svg.convert_to_polar(plotcoord.x, plotcoord.y);
        label_x = "ang= " + polar.ang.toPrecision(4);
        label_y = "R= " + polar.r.toPrecision(4);

    } else if (gnuplot_svg.plot_timeaxis_x == "Date") {
        gnuplot_svg.axisdate.setTime(1000 * plotcoord.x);
        var year = gnuplot_svg.axisdate.getUTCFullYear();
        var month = gnuplot_svg.axisdate.getUTCMonth();
        var date = gnuplot_svg.axisdate.getUTCDate();
        label_x = (" " + date).slice(-2) + "/"
            + ("0" + (month + 1)).slice(-2) + "/"
            + year;
        label_y = plotcoord.y.toFixed(2);
    } else if (gnuplot_svg.plot_timeaxis_x == "Time") {
        gnuplot_svg.axisdate.setTime(1000 * plotcoord.x);
        var hour = gnuplot_svg.axisdate.getUTCHours();
        var minute = gnuplot_svg.axisdate.getUTCMinutes();
        var second = gnuplot_svg.axisdate.getUTCSeconds();
        label_x = ("0" + hour).slice(-2) + ":"
            + ("0" + minute).slice(-2) + ":"
            + ("0" + second).slice(-2);
        label_y = plotcoord.y.toFixed(2);
    } else if (gnuplot_svg.plot_timeaxis_x == "DateTime") {
        gnuplot_svg.axisdate.setTime(1000 * plotcoord.x);
        label_x = gnuplot_svg.axisdate.toUTCString();
        label_y = plotcoord.y.toFixed(2);
    } else {
        label_x = plotcoord.x.toFixed(2);
        label_y = plotcoord.y.toFixed(2);
    }

    while (null !== t.firstChild) {
        t.removeChild(t.firstChild);
    }
    var textNode = gnuplot_svg.SVGDoc.createTextNode(label_x + " " + label_y);
    t.appendChild(textNode);
};

gnuplot_svg.showCoordBox = function (evt) {
    var t = gnuplot_svg.getText();
    if (null !== t) {
        t.setAttribute("visibility", "visible");
        gnuplot_svg.updateCoordBox(t, evt);
    }
};

gnuplot_svg.moveCoordBox = function (evt) {
    var t = gnuplot_svg.getText();
    if (null !== t) {
        gnuplot_svg.updateCoordBox(t, evt);
    }
};

gnuplot_svg.hideCoordBox = function (evt) {
    var t = gnuplot_svg.getText();
    if (null !== t) {
        t.setAttribute("visibility", "hidden");
    }
};

gnuplot_svg.toggleCoordBox = function (evt) {
    var t = gnuplot_svg.getText();
    if (null !== t) {
        var state = t.getAttribute('visibility');
        if ('hidden' != state) {
            state = 'hidden';
        }
        else {
            state = 'visible';
        }
        t.setAttribute('visibility', state);
    }
};

gnuplot_svg.toggleGrid = function () {
    if (!gnuplot_svg.SVGDoc.getElementsByClassName) { // Old browsers
        return;
    }
    var grid = gnuplot_svg.SVGDoc.getElementsByClassName('gridline');
    for (var i = 0; i < grid.length; i++) {
        var state = grid[i].getAttribute('visibility');
        grid[i].setAttribute('visibility', (state == 'hidden') ? 'visible' : 'hidden');
    }
};

gnuplot_svg.showHypertext = function (evt, mouseovertext) {
    var lines = mouseovertext.split('\n');

    // If text starts with "image:" process it as an xlinked bitmap
    if (lines[0].substring(0, 5) == "image") {
        var nameindex = lines[0].indexOf(":");
        if (nameindex > 0) {
            gnuplot_svg.showHyperimage(evt, lines[0]);
            lines[0] = lines[0].slice(nameindex + 1);
        }
    }

    var loc = cursorPoint(evt);
    var anchor_x = loc.x;
    var anchor_y = loc.y;

    var hypertextbox = document.getElementById("hypertextbox");
    hypertextbox.setAttributeNS(null, "x", anchor_x + 10);
    hypertextbox.setAttributeNS(null, "y", anchor_y + 4);
    hypertextbox.setAttributeNS(null, "visibility", "visible");

    var hypertext = document.getElementById("hypertext");
    hypertext.setAttributeNS(null, "x", anchor_x + 14);
    hypertext.setAttributeNS(null, "y", anchor_y + 18);
    hypertext.setAttributeNS(null, "visibility", "visible");

    var height = 2 + 16 * lines.length;
    hypertextbox.setAttributeNS(null, "height", height);
    var length = hypertext.getComputedTextLength();
    hypertextbox.setAttributeNS(null, "width", length + 8);

    // bounce off frame bottom
    if (anchor_y > gnuplot_svg.plot_ybot + 16 - height) {
        anchor_y -= height;
        hypertextbox.setAttributeNS(null, "y", anchor_y + 4);
        hypertext.setAttributeNS(null, "y", anchor_y + 18);
    }

    while (null !== hypertext.firstChild) {
        hypertext.removeChild(hypertext.firstChild);
    }

    var textNode = document.createTextNode(lines[0]);

    var tspan_element;

    if (lines.length <= 1) {
        hypertext.appendChild(textNode);
    } else {
        xmlns = "http://www.w3.org/2000/svg";
        tspan_element = document.createElementNS(xmlns, "tspan");
        tspan_element.appendChild(textNode);
        hypertext.appendChild(tspan_element);
        length = tspan_element.getComputedTextLength();
        var ll = length;

        for (var l = 1; l < lines.length; l++) {
            tspan_element = document.createElementNS(xmlns, "tspan");
            tspan_element.setAttributeNS(null, "dy", 16);
            textNode = document.createTextNode(lines[l]);
            tspan_element.appendChild(textNode);
            hypertext.appendChild(tspan_element);

            ll = tspan_element.getComputedTextLength();
            if (length < ll) { length = ll; }
        }
        hypertextbox.setAttributeNS(null, "width", length + 8);
    }

    // bounce off right edge
    if (anchor_x > gnuplot_svg.plot_xmax + 14 - length) {
        anchor_x -= length;
        hypertextbox.setAttributeNS(null, "x", anchor_x + 10);
        hypertext.setAttributeNS(null, "x", anchor_x + 14);
    }

    // left-justify multiline text
    tspan_element = hypertext.firstChild;
    while (tspan_element) {
        if (tspan_element.setAttributeNS) {
            tspan_element.setAttributeNS(null, "x", anchor_x + 14);
        }
        tspan_element = tspan_element.nextElementSibling;
    }

};

gnuplot_svg.hideHypertext = function () {
    var hypertextbox = document.getElementById("hypertextbox");
    var hypertext = document.getElementById("hypertext");
    var hyperimage = document.getElementById("hyperimage");
    hypertextbox.setAttributeNS(null, "visibility", "hidden");
    hypertext.setAttributeNS(null, "visibility", "hidden");
    hyperimage.setAttributeNS(null, "visibility", "hidden");
};

gnuplot_svg.showHyperimage = function (evt, linktext) {
    var loc = cursorPoint(evt);
    var anchor_x = loc.x;
    var anchor_y = loc.y;

    var hyperimage = document.getElementById("hyperimage");
    hyperimage.setAttributeNS(null, "x", anchor_x);
    hyperimage.setAttributeNS(null, "y", anchor_y);
    hyperimage.setAttributeNS(null, "visibility", "visible");

    // Pick up height and width from "image(width,height):name"
    var width = hyperimage.getAttributeNS(null, "width");
    var height = hyperimage.getAttributeNS(null, "height");
    if (linktext.charAt(5) == "(") {
        width = parseInt(linktext.slice(6));
        height = parseInt(linktext.slice(linktext.indexOf(",") + 1));
        hyperimage.setAttributeNS(null, "width", width);
        hyperimage.setAttributeNS(null, "height", height);
        hyperimage.setAttributeNS(null, "preserveAspectRatio", "none");
    }

    // bounce off frame bottom and right
    if (anchor_y > gnuplot_svg.plot_ybot + 50 - height) {
        hyperimage.setAttributeNS(null, "y", 20 + anchor_y - height);
    }
    if (anchor_x > gnuplot_svg.plot_xmax + 150 - width) {
        hyperimage.setAttributeNS(null, "x", 10 + anchor_x - width);
    }

    // attach image URL as a link
    linktext = linktext.slice(linktext.indexOf(":") + 1);
    var xlinkns = "http://www.w3.org/1999/xlink";
    hyperimage.setAttributeNS(xlinkns, "xlink:href", linktext);
};

// Convert from svg panel mouse coordinates to the coordinate
// system of the gnuplot figure
gnuplot_svg.mouse2plot = function (mousex, mousey) {
    var plotcoord = {};
    var plotx = mousex - gnuplot_svg.plot_xmin;
    var ploty = mousey - gnuplot_svg.plot_ybot;
    var x, y;

    if (gnuplot_svg.plot_logaxis_x < 0) {
	// nonlinear axis or custom mapping function
	x = NaN;
    } else if (gnuplot_svg.plot_logaxis_x !== 0) {
        x = Math.log(gnuplot_svg.plot_axis_xmax)
            - Math.log(gnuplot_svg.plot_axis_xmin);
        x = x * (plotx / (gnuplot_svg.plot_xmax - gnuplot_svg.plot_xmin))
            + Math.log(gnuplot_svg.plot_axis_xmin);
        x = Math.exp(x);
    } else {
        x = gnuplot_svg.plot_axis_xmin + (plotx / (gnuplot_svg.plot_xmax - gnuplot_svg.plot_xmin)) * (gnuplot_svg.plot_axis_xmax - gnuplot_svg.plot_axis_xmin);
    }

    if (gnuplot_svg.plot_logaxis_y < 0) {
	y = NaN;
    } else if (gnuplot_svg.plot_logaxis_y !== 0) {
        y = Math.log(gnuplot_svg.plot_axis_ymax)
            - Math.log(gnuplot_svg.plot_axis_ymin);
        y = y * (ploty / (gnuplot_svg.plot_ytop - gnuplot_svg.plot_ybot))
            + Math.log(gnuplot_svg.plot_axis_ymin);
        y = Math.exp(y);
    } else {
        y = gnuplot_svg.plot_axis_ymin + (ploty / (gnuplot_svg.plot_ytop - gnuplot_svg.plot_ybot)) * (gnuplot_svg.plot_axis_ymax - gnuplot_svg.plot_axis_ymin);
    }

    plotcoord.x = x;
    plotcoord.y = y;
    return plotcoord;
};

gnuplot_svg.convert_to_polar = function (x, y) {
    polar = {};
    var phi, r;
    phi = Math.atan2(y, x);
    if (gnuplot_svg.plot_logaxis_r) {
        r = Math.exp((x / Math.cos(phi) + Math.log(gnuplot_svg.plot_axis_rmin) / Math.LN10) * Math.LN10);
    }
    else if (gnuplot_svg.plot_axis_rmin > gnuplot_svg.plot_axis_rmax) {
        r = gnuplot_svg.plot_axis_rmin - x / Math.cos(phi);
    } else {
        r = gnuplot_svg.plot_axis_rmin + x / Math.cos(phi);
    }
    phi = phi * (180 / Math.PI);
    if (gnuplot_svg.polar_sense < 0) {
        phi = -phi;
    }
    if (gnuplot_svg.polar_theta0 !== undefined) {
        phi = phi + gnuplot_svg.polar_theta0;
    }
    if (phi > 180) { phi = phi - 360; }
    polar.r = r;
    polar.ang = phi;
    return polar;
};

gnuplot_svg.convert_to_DMS = function (x) {
    var dms = { d: 0, m: 0, s: 0 };
    var deg = Math.abs(x);
    dms.d = Math.floor(deg);
    dms.m = Math.floor((deg - dms.d) * 60);
    dms.s = Math.floor((deg - dms.d) * 3600 - dms.m * 60);
    fmt = ((x < 0) ? "-" : " ")
        + dms.d.toFixed(0) + "°"
        + dms.m.toFixed(0) + "\""
        + dms.s.toFixed(0) + "'";
    return fmt;
};

gnuplot_svg.interactiveInit = function (svgElement) {

    var dragOffset = {'x' : 0, 'y' : 0};

    var dragChange = svgElement.createSVGPoint();
    
    var dragTimeout = null;

    var dragEnabled = false;

    var getViewBox = function () {
        var viewBoxValues = svgElement.getAttribute('viewBox').split(' ');
        viewBoxValues[0] = parseFloat(viewBoxValues[0]);
        viewBoxValues[1] = parseFloat(viewBoxValues[1]);
        viewBoxValues[2] = parseFloat(viewBoxValues[2]);
        viewBoxValues[3] = parseFloat(viewBoxValues[3]);
        return viewBoxValues;
    };

    var setViewBox = function (viewBoxValues) {
        svgElement.setAttribute('viewBox', viewBoxValues.join(' '));
    };

    var zoom = function (direction) {
        var zoomRate = 1.1;
        var viewBoxValues = getViewBox();

        var widthBefore = viewBoxValues[2];
        var heightBefore = viewBoxValues[3];

        if (direction == 'in') {
            viewBoxValues[2] /= zoomRate;
            viewBoxValues[3] /= zoomRate;
            // Pan to center
            viewBoxValues[0] -= (viewBoxValues[2] - widthBefore) / 2;
            viewBoxValues[1] -= (viewBoxValues[3] - heightBefore) / 2;
        }
        else if (direction == 'out') {
            viewBoxValues[2] *= zoomRate;
            viewBoxValues[3] *= zoomRate;
            // Pan to center
            viewBoxValues[0] += (widthBefore - viewBoxValues[2]) / 2;
            viewBoxValues[1] += (heightBefore - viewBoxValues[3]) / 2;
        }

        return viewBoxValues;
    };

    var pan = function (direction) {
        var panRate = 10;
        var viewBoxValues = getViewBox();

        switch (direction) {
            case 'left':
                viewBoxValues[0] += panRate;
                break;
            case 'right':
                viewBoxValues[0] -= panRate;
                break;
            case 'up':
                viewBoxValues[1] += panRate;
                break;
            case 'down':
                viewBoxValues[1] -= panRate;
                break;
        }

        return viewBoxValues;
    };

    // viewBox initial position and size
    var resetValue = getViewBox();

    // Keyboard actions, old svg version not support key events so must listen window
    window.addEventListener('keydown', function (evt) {

        // Not capture event from inputs
        // body = svg inline in page, svg = plain svg file, window = delegated events to object
        if (evt.target.nodeName != 'BODY' && evt.target.nodeName != 'svg' && evt.target != window) {
            return true;
        }

        var viewBoxValues = [];

        switch (evt.key) {
            // Move, Edge sends without Arrow word
            case 'ArrowLeft':
            case 'Left':
            case 'ArrowRight':
            case 'Right':
            case 'ArrowUp':
            case 'Up':
            case 'ArrowDown':
            case 'Down':
                viewBoxValues = pan(evt.key.replace('Arrow', '').toLowerCase());
                break;
            // Zoom in
            case '+':
            case 'Add':
                viewBoxValues = zoom('in');
                break;
            // Zoom out
            case '-':
            case 'Subtract':
                viewBoxValues = zoom('out');
                break;
            // Reset
            case 'Home':
                viewBoxValues = resetValue;
                break;
            // Toggle grid
            case '#':
                gnuplot_svg.toggleGrid();
                break;
        }

        if (viewBoxValues.length) {
            setViewBox(viewBoxValues);
        }
    });

    // Set focusable for event focusing, not work on old svg version
    svgElement.setAttribute('focusable', true);

    // Disable native draggable
    svgElement.setAttribute('draggable', false);

    // Save move relative start position
    svgElement.addEventListener('mousedown', function (evt) {

        dragOffset = {'x' : evt.clientX,  'y' : evt.clientY};

        // Delay for moving, so not move accidentally if only click
        dragTimeout = setTimeout(function () {
            dragEnabled = true;
        }, 250);

        // Cancel draggable
        evt.stopPropagation();
        evt.preventDefault();
        return false;
    });

    svgElement.addEventListener('mouseup', function (evt) {
        dragEnabled = false;
        clearTimeout(dragTimeout);
    });

    // Mouse move
    svgElement.addEventListener('mousemove', function (evt) {
        if (evt.buttons == 1 && dragEnabled) {

            // Position change
            dragChange.x = evt.clientX - dragOffset.x;
            dragChange.y = evt.clientY - dragOffset.y;

            // Set current mouse position
            dragOffset.x = evt.clientX;
            dragOffset.y = evt.clientY;

            dragChange.matrixTransform(svgElement.getScreenCTM().inverse());

            var viewBoxValues = getViewBox();

            viewBoxValues[0] -= dragChange.x;
            viewBoxValues[1] -= dragChange.y;

            setViewBox(viewBoxValues);
        }
    });

    // Zoom with wheel
    svgElement.addEventListener('wheel', function (evt) {
        // x or y scroll zoom both axels
        var delta = Math.max(-1, Math.min(1, (evt.deltaY || evt.deltaX)));

        if (delta > 0) {
            setViewBox(zoom('in'));
        }
        else {
            setViewBox(zoom('out'));
        }

        // Disable scroll the entire webpage
        evt.stopPropagation();
        evt.preventDefault();
        return false;
    });

    // Reset on right click or hold tap
    svgElement.addEventListener('contextmenu', function (evt) {

        setViewBox(resetValue);

        // Disable native context menu
        evt.stopPropagation();
        evt.preventDefault();
        return false;
    });

    // Return functions to outside use
    return {
        zoom: function (direction) {
            setViewBox(zoom(direction));
            return this;
        },
        pan: function (direction) {
            setViewBox(pan(direction));
            return this;
        },
        reset: function () {
        	setViewBox(resetValue);
        	return this;
        }
    };
};
