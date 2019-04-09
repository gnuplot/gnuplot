// From:	Marko Karjalainen <markokarjalainen@kolumbus.fi>
// Date:	27 Aug 2018
// Experimental gnuplot plugin for svg
//
// All svg elements on page get own gnuplot plugin attached by js, so no conflict with global variables.
//
// Javascript variables are read from second script tag and converted to json for import to plugin.
// Inline events are removed from xml and new ones are attached with addEventListener function.
// Inline events should be removed from xml and xml should have better id/class names to attach events from js.
//
// Improved mouseover text and image handling
//   content changed to xml only if it really changed and bouncing is calculated once.
//
// Convert functions are same as before, maybe renamed better.
//
// Javascript routines for mouse and keyboard interaction with
// SVG documents produced by gnuplot SVG terminal driver.

// TODO do not create inline events to svg and give id or classes for getting elements
// TODO make own svg layer x/y range sized for coordinates?

if (window) {
    window.addEventListener('load', function () {
        // Find svg elements
        var svg = document.querySelectorAll('svg');
        for (var i = 0; i < svg.length; i++) {
            // Init plugin
            if (!svg[i].gnuplot) {
                // Check if gnuplot generated svg
                if(svg[i].getElementById('gnuplot_canvas')){
                    svg[i].gnuplot = new gnuplot_svg(svg[i]);
                }
            }
        }
    });
}

gnuplot_svg = function (svgElement) {

    var version = '09 April 2019';

    var settings = {};

    var viewBoxResetValue = [];

    var drag = {
        'enabled': false,
        'offset': { 'x': 0, 'y': 0 },
        'change': svgElement.createSVGPoint(),
        'timeout': null
    };

    var coordinateText = {
        'enabled': false,
        'element': svgElement.getElementById('coord_text')
    };

    var popoverContainer = {
        'element': null,
        'content': null,
    };

    var popoverImage = {
        'element': null,
        'content': null,
        'width': 300,
        'height': 200,
        'defaultWidth': 300,
        'defaultHeight': 200,
    };

    var popoverText = {
        'element': null,
        'content': null,
        'width': 11,
        'height': 16,
        'defaultWidth': 11,
        'defaultHeight': 16,
    };

    var point = svgElement.createSVGPoint();

    var axisDate = new Date();

    var gridEnabled = false;

    // Get plot boundaries and axis scaling information for mousing from current object script tag
    // TODO add these to svg xml custom attribute for reading(json format)
    var parseSettings = function () {
        var script = svgElement.querySelectorAll('script');
        if (script && script[1]) {
            var scriptText = script[1].firstChild.nodeValue;
            // Remove inline comments
            scriptText = scriptText.replace(/^\s*\/\/.*\n/g, '');
            // Change prefix to "
            scriptText = scriptText.replace(/gnuplot_svg\./g, '"');
            // Change = to " :
            scriptText = scriptText.replace(/ = /g, '" : ');
            // Change line endings to comma
            scriptText = scriptText.replace(/;\n|\n/g, ',');
            // Remove last comma
            scriptText = scriptText.replace(/,+$/, '');
            // Parse as json string
            settings = JSON.parse("{\n" + scriptText + "\n}");
        }
    };

    // Add interactive events
    var addEvents = function () {
        var i;

        // Get keyentry elements
        var toggleVisibility = svgElement.querySelectorAll('g[id$="_keyentry"]');
        for (i = 0; i < toggleVisibility.length; i++) {
            // ------- Remove inline events
            toggleVisibility[i].removeAttribute('onclick');
            // -------

            // Add keyentry event to toggle visibility
            toggleVisibility[i].addEventListener('click', key.bind(null, toggleVisibility[i].getAttribute('id'), null));
        }

        // ------- Remove inline events from bounding box
        var boundingBox = svgElement.querySelector('rect[onclick^="gnuplot_svg.toggleCoordBox"]');
        if (boundingBox) {
            boundingBox.removeAttribute('onclick');
            boundingBox.removeAttribute('onmousemove');
        }
        // ------- Remove inline events from canvas
        var canvas = svgElement.getElementById('gnuplot_canvas');
        if (canvas) {
            canvas.removeAttribute('onclick');
            canvas.removeAttribute('onmousemove');
        }
        // -------

        // Get grid image
        var toggleGrid = svgElement.querySelector('image[onclick^="gnuplot_svg.toggleGrid"]');
        if (toggleGrid) {
            // ------- Remove inline events
            toggleGrid.removeAttribute('onclick');
            // -------

            // Add Toggle grid image event
            toggleGrid.addEventListener('click', function (evt) {
                grid();
                evt.preventDefault();
                evt.stopPropagation();
            });
        }

        // Get hypertexts
        var hyperText = svgElement.querySelectorAll('g[onmousemove^="gnuplot_svg.showHypertext"]');

        // Set view element variables
        if (hyperText.length) {
            popoverContainer.element = svgElement.getElementById('hypertextbox');
            popoverText.element = svgElement.getElementById('hypertext');
            popoverImage.element = svgElement.getElementById('hyperimage');
            popoverImage.defaultWidth = popoverImage.element.getAttribute('width');
            popoverImage.defaultHeight = popoverImage.element.getAttribute('height');
        }

        for (i = 0; i < hyperText.length; i++) {
            // Get text from attr uggly way, svg has empty title element
            var text = hyperText[i].getAttribute('onmousemove').substr(31).slice(0, -2);

            // ------- Remove inline events
            hyperText[i].removeAttribute('onmousemove');
            hyperText[i].removeAttribute('onmouseout');
            // -------

            // Add event
            hyperText[i].addEventListener('mousemove', popover.bind(null, text, true));
            hyperText[i].addEventListener('mouseout', popover.bind(null, null, false));
        }

        // Toggle coordinates visibility on left click on boundingBox element
        svgElement.addEventListener('click', function (evt) {
            if (!drag.enabled) {
                // TODO check if inside data area, own layer for this is needed?
                coordinate();
                setCoordinateLabel(evt);
            }
        });

        // Save move start position, enable drag after delay
        svgElement.addEventListener('mousedown', function (evt) {

            drag.offset = { 'x': evt.clientX, 'y': evt.clientY };

            // Delay for moving, so not move accidentally if only click
            drag.timeout = setTimeout(function () {
                drag.enabled = true;
            }, 250);

            // Cancel draggable
            evt.stopPropagation();
            evt.preventDefault();
            return false;
        });

        // Disable drag
        svgElement.addEventListener('mouseup', function (evt) {
            drag.enabled = false;
            clearTimeout(drag.timeout);
        });

        // Mouse move
        svgElement.addEventListener('mousemove', function (evt) {

            // Drag svg element
            if (evt.buttons == 1 && drag.enabled) {

                // Position change
                drag.change.x = evt.clientX - drag.offset.x;
                drag.change.y = evt.clientY - drag.offset.y;

                // Set current mouse position
                drag.offset.x = evt.clientX;
                drag.offset.y = evt.clientY;

                // Convert to svg position
                drag.change.matrixTransform(svgElement.getScreenCTM().inverse());

                var viewBoxValues = getViewBox();

                viewBoxValues[0] -= drag.change.x;
                viewBoxValues[1] -= drag.change.y;

                setViewBox(viewBoxValues);
            }

            // View coordinates on mousemove over svg element
            if (coordinateText.enabled) {
                // TODO check if inside data area, own layer for this is needed?
                setCoordinateLabel(evt);
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

            setViewBox(viewBoxResetValue);

            // Disable native context menu
            evt.stopPropagation();
            evt.preventDefault();
            return false;
        });

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
                    viewBoxValues = viewBoxResetValue;
                    break;
                // Toggle grid
                case '#':
                    grid();
                    break;
            }

            if (viewBoxValues.length) {
                setViewBox(viewBoxValues);
            }
        });
    };

    // Get svg viewbox details
    var getViewBox = function () {
        var viewBoxValues = svgElement.getAttribute('viewBox').split(' ');
        viewBoxValues[0] = parseFloat(viewBoxValues[0]);
        viewBoxValues[1] = parseFloat(viewBoxValues[1]);
        viewBoxValues[2] = parseFloat(viewBoxValues[2]);
        viewBoxValues[3] = parseFloat(viewBoxValues[3]);
        return viewBoxValues;
    };

    // Set svg viewbox details
    var setViewBox = function (viewBoxValues) {
        svgElement.setAttribute('viewBox', viewBoxValues.join(' '));
    };

    // Set coordinate label position and text
    var setCoordinateLabel = function (evt) {
        var position = convertDOMToSVG({ 'x': evt.clientX, 'y': evt.clientY });

        // Set coordinate label position
        coordinateText.element.setAttribute('x', position.x);
        coordinateText.element.setAttribute('y', position.y);

        // Convert svg position to plot coordinates
        var plotcoord = convertSVGToPlot(position);

        // Parse label to view
        var label = parseCoordinateLabel(plotcoord);

        // Set coordinate label text
        coordinateText.element.textContent = label.x + ' ' + label.y;
    };

    // Convert position DOM to SVG
    var convertDOMToSVG = function (position) {
        point.x = position.x;
        point.y = position.y;
        return point.matrixTransform(svgElement.getScreenCTM().inverse());
    };

    // Convert position SVG to Plot
    var convertSVGToPlot = function (position) {
        var plotcoord = {};
        var plotx = position.x - settings.plot_xmin;
        var ploty = position.y - settings.plot_ybot;
        var x, y;

        if (settings.plot_logaxis_x !== 0) {
            x = Math.log(settings.plot_axis_xmax)
                - Math.log(settings.plot_axis_xmin);
            x = x * (plotx / (settings.plot_xmax - settings.plot_xmin))
                + Math.log(settings.plot_axis_xmin);
            x = Math.exp(x);
        } else {
            x = settings.plot_axis_xmin + (plotx / (settings.plot_xmax - settings.plot_xmin)) * (settings.plot_axis_xmax - settings.plot_axis_xmin);
        }

        if (settings.plot_logaxis_y !== 0) {
            y = Math.log(settings.plot_axis_ymax)
                - Math.log(settings.plot_axis_ymin);
            y = y * (ploty / (settings.plot_ytop - settings.plot_ybot))
                + Math.log(settings.plot_axis_ymin);
            y = Math.exp(y);
        } else {
            y = settings.plot_axis_ymin + (ploty / (settings.plot_ytop - settings.plot_ybot)) * (settings.plot_axis_ymax - settings.plot_axis_ymin);
        }

        plotcoord.x = x;
        plotcoord.y = y;
        return plotcoord;
    };

    // Parse plot x/y values to label
    var parseCoordinateLabel = function (plotcoord) {
        var label = { 'x': 0, 'y': 0 };

        if (settings.plot_timeaxis_x == 'DMS' || settings.plot_timeaxis_y == 'DMS') {
            if (settings.plot_timeaxis_x == 'DMS') {
                label.x = convertToDMS(plotcoord.x);
            }
            else {
                label.x = plotcoord.x.toFixed(2);
            }

            if (settings.plot_timeaxis_y == 'DMS') {
                label.y = convertToDMS(plotcoord.y);
            }
            else {
                label.y = plotcoord.y.toFixed(2);
            }

        } else if (settings.polar_mode) {
            polar = convertToPolar(plotcoord.x, plotcoord.y);
            label.x = 'ang= ' + polar.ang.toPrecision(4);
            label.y = 'R= ' + polar.r.toPrecision(4);

        } else if (settings.plot_timeaxis_x == 'Date') {
            axisDate.setTime(1000 * plotcoord.x);
            var year = axisDate.getUTCFullYear();
            var month = axisDate.getUTCMonth();
            var date = axisDate.getUTCDate();
            label.x = (' ' + date).slice(-2) + '/' + ('0' + (month + 1)).slice(-2) + '/' + year;
            label.y = plotcoord.y.toFixed(2);
        } else if (settings.plot_timeaxis_x == 'Time') {
            axisDate.setTime(1000 * plotcoord.x);
            var hour = axisDate.getUTCHours();
            var minute = axisDate.getUTCMinutes();
            var second = axisDate.getUTCSeconds();
            label.x = ('0' + hour).slice(-2) + ':' + ('0' + minute).slice(-2) + ':' + ('0' + second).slice(-2);
            label.y = plotcoord.y.toFixed(2);
        } else if (settings.plot_timeaxis_x == 'DateTime') {
            axisDate.setTime(1000 * plotcoord.x);
            label.x = axisDate.toUTCString();
            label.y = plotcoord.y.toFixed(2);
        } else {
            label.x = plotcoord.x.toFixed(2);
            label.y = plotcoord.y.toFixed(2);
        }

        return label;
    };

    // Convert position to Polar
    var convertToPolar = function (x, y) {
        polar = {};
        var phi, r;
        phi = Math.atan2(y, x);
        if (settings.plot_logaxis_r) {
            r = Math.exp((x / Math.cos(phi) + Math.log(settings.plot_axis_rmin) / Math.LN10) * Math.LN10);
        }
        else if (settings.plot_axis_rmin > settings.plot_axis_rmax) {
            r = settings.plot_axis_rmin - x / Math.cos(phi);
        } else {
            r = settings.plot_axis_rmin + x / Math.cos(phi);
        }
        phi = phi * (180 / Math.PI);
        if (settings.polar_sense < 0) {
            phi = -phi;
        }
        if (settings.polar_theta0 !== undefined) {
            phi = phi + settings.polar_theta0;
        }
        if (phi > 180) { phi = phi - 360; }
        polar.r = r;
        polar.ang = phi;
        return polar;
    };

    // Convert position to DMS
    var convertToDMS = function (x) {
        var dms = { d: 0, m: 0, s: 0 };
        var deg = Math.abs(x);
        dms.d = Math.floor(deg);
        dms.m = Math.floor((deg - dms.d) * 60);
        dms.s = Math.floor((deg - dms.d) * 3600 - dms.m * 60);
        fmt = ((x < 0) ? '-' : ' ') + dms.d.toFixed(0) + '°' + dms.m.toFixed(0) + '"' + dms.s.toFixed(0) + "'";
        return fmt;
    };

    // Set popover text to show
    var setPopoverText = function (content) {

        // Minimum length
        popoverText.width = popoverText.defaultWidth;

        // Remove old texts
        while (null !== popoverText.element.firstChild) {
            popoverText.element.removeChild(popoverText.element.firstChild);
        }

        var lines = content.split(/\n|\\n/g);

        // Single line
        if (lines.length <= 1) {
            popoverText.element.textContent = content;
            popoverText.width = popoverText.element.getComputedTextLength() + 8;
        }
        // Multiple lines
        else {
            var lineWidth = 0;
            var tspanElement;

            for (var l = 0; l < lines.length; l++) {
                tspanElement = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
                // Y relative position
                if (l > 0) {
                    tspanElement.setAttribute('dy', popoverText.defaultHeight);
                }
                // Append text
                tspanElement.appendChild(document.createTextNode(lines[l]));
                popoverText.element.appendChild(tspanElement);

                // Max line width
                lineWidth = tspanElement.getComputedTextLength() + 8;
                if (popoverText.width < lineWidth) {
                    popoverText.width = lineWidth;
                }
            }
        }

        // Box Height
        popoverText.height = 2 + popoverText.defaultHeight * lines.length;
        popoverContainer.element.setAttribute('height', popoverText.height);

        // Box Width
        popoverContainer.element.setAttribute('width', popoverText.width);
    };

    // Set popover image to show
    var setPopoverImage = function (content) {

        // Set default image size
        popoverImage.width = popoverImage.defaultWidth;
        popoverImage.height = popoverImage.defaultHeight;

        // Pick up height and width from image(width,height):name
        if (content.charAt(5) == '(') {
            popoverImage.width = parseInt(content.slice(6));
            popoverImage.height = parseInt(content.slice(content.indexOf(',') + 1));
        }

        popoverImage.element.setAttribute('width', popoverImage.width);
        popoverImage.element.setAttribute('height', popoverImage.height);
        popoverImage.element.setAttribute('preserveAspectRatio', 'none');

        // attach image URL as a link
        content = content.slice(content.indexOf(':') + 1);
        popoverImage.element.setAttributeNS('http://www.w3.org/1999/xlink', 'xlink:href', content);
    };

    // Show popover text in given position
    var showPopoverText = function (position) {
        var domRect = svgElement.getBoundingClientRect();
        domRect = convertDOMToSVG({'x': domRect.right, 'y': domRect.bottom });

        // bounce off frame bottom
        if (position.y + popoverText.height + 16 > domRect.y) {
            position.y = domRect.y - popoverText.height - 16;
        }

        // bounce off right edge
        if (position.x + popoverText.width + 14 > domRect.x) {
            position.x = domRect.x - popoverText.width - 14;
        }

        // Change Box position
        popoverContainer.element.setAttribute('x', position.x + 10);
        popoverContainer.element.setAttribute('y', position.y + 4);
        popoverContainer.element.setAttribute('visibility', 'visible');

        // Change Text position
        popoverText.element.setAttribute('x', position.x + 14);
        popoverText.element.setAttribute('y', position.y + 18);
        popoverText.element.setAttribute('visibility', 'visible');

        // Change multiline text position
        var tspan = popoverText.element.querySelectorAll('tspan');
        for (var i = 0; i < tspan.length; i++) {
            tspan[i].setAttribute('x', position.x + 14);
        }

        // Font properties
        if (settings.hypertext_fontFamily != null)
            popoverText.element.setAttribute('font-family', settings.hypertext_fontFamily);
        if (settings.hypertext_fontStyle != null)
            popoverText.element.setAttribute('font-style', settings.hypertext_fontStyle);
        if (settings.hypertext_fontWeight != null)
            popoverText.element.setAttribute('font-weight', settings.hypertext_fontWeight);
        if (settings.hypertext_fontSize > 0)
            popoverText.element.setAttribute('font-size', settings.hypertext_fontSize);
    };

    // Show popover image in given position
    var showPopoverImage = function (position) {
        var domRect = svgElement.getBoundingClientRect();
        domRect = convertDOMToSVG({'x': domRect.right, 'y': domRect.bottom });

        // bounce off frame bottom
        if (position.y + popoverImage.height + 16 > domRect.y) {
            position.y = domRect.y - popoverImage.height - 16;
        }

        // bounce off right edge
        if (position.x + popoverImage.width + 14 > domRect.x) {
            position.x = domRect.x - popoverImage.width - 14;
        }

        popoverImage.element.setAttribute('x', position.x);
        popoverImage.element.setAttribute('y', position.y);
        popoverImage.element.setAttribute('visibility', 'visible');
    };

    // Hide all popovers
    var hidePopover = function () {
        popoverContainer.element.setAttribute('visibility', 'hidden');
        popoverText.element.setAttribute('visibility', 'hidden');
        popoverImage.element.setAttribute('visibility', 'hidden');
    };

    // Zoom svg inside viewbox
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

    // Pan svg inside viewbox
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

    // Toggle key and chart on/off or set manually to wanted
    var key = function (id, set, evt) {
        var visibility = null;

        // Chart element
        var chartElement = svgElement.getElementById(id.replace('_keyentry', ''));
        if (chartElement) {
            // Set on/off
            if (set === true || set === false) {
                visibility = set ? 'visible' : 'hidden';
            }
            // Toggle
            else {
                visibility = chartElement.getAttribute('visibility') == 'hidden' ? 'visible' : 'hidden';
            }
            chartElement.setAttribute('visibility', visibility);
        }

        // Key element
        var keyElement = svgElement.getElementById(id);
        if (keyElement && visibility) {
            keyElement.setAttribute('style', visibility == 'hidden' ? 'filter:url(#greybox)' : 'none');
        }

        if (evt !== undefined) {
            evt.stopPropagation();
            evt.preventDefault();
        }
    };

    // Toggle coordinates on/off or set manually to wanted
    var coordinate = function (set) {
        if (coordinateText.element) {
            // Set on/off
            if (set === true || set === false) {
                coordinateText.enabled = set;
            }
            // Toggle
            else {
                coordinateText.enabled = coordinateText.element.getAttribute('visibility') == 'hidden' ? true : false;
            }
            coordinateText.element.setAttribute('visibility', coordinateText.enabled ? 'visible' : 'hidden');
        }
    };

    // Toggle grid on/off or set manually to wanted
    var grid = function (set) {
        var grid = svgElement.getElementsByClassName('gridline');

        // Set on/off
        if (set === true || set === false) {
            gridEnabled = set;
        }
        // Toggle, get state from first element
        else if (grid.length) {
            gridEnabled = grid[0].getAttribute('visibility') == 'hidden' ? true : false;
        }

        for (i = 0; i < grid.length; i++) {
            grid[i].setAttribute('visibility', gridEnabled ? 'visible' : 'hidden');
        }
    };

    // Show popover text or image
    var popover = function (content, set, evt) {

        // Hide popover
        if (set === false) {
            hidePopover();

            if (evt !== undefined) {
                evt.stopPropagation();
                evt.preventDefault();
            }

            return;
        }

        var position = null;

        // Change content only if changed
        if (popoverContainer.content != content) {

            // Set current text
            popoverContainer.content = content;

            popoverImage.content = '';
            popoverText.content = content;

            // If text starts with image: process it as an xlinked bitmap
            if (content.substring(0, 5) == 'image') {
                var lines = content.split(/\n|\\n/g);
                var nameindex = lines[0].indexOf(':');
                if (nameindex > 0) {
                    popoverImage.content = lines.shift();
                    popoverText.content = '';

                    // Additional text lines
                    if (lines !== undefined && lines.length > 0) {
                        popoverText.content = lines.join('\n');
                    }
                }
            }

            // Set image content
            if(popoverImage.content){
                setPopoverImage(popoverImage.content);
            }

            // Set text content
            if(popoverText.content){
                setPopoverText(popoverText.content);
            }
        }

        if(popoverImage.content || popoverText.content){
            position = convertDOMToSVG({'x': evt.clientX, 'y': evt.clientY });
        }

        // Show popover image on mouse position
        if(popoverImage.content){
            showPopoverImage(position);
        }

        // Show popover on mouse position
        if(popoverText.content){
            showPopoverText(position);
        }

        if (evt !== undefined) {
            evt.stopPropagation();
            evt.preventDefault();
        }
    };

    // Parse plot settings
    parseSettings();

    // viewBox initial position and size
    viewBoxResetValue = getViewBox();

    // Set focusable for event focusing, not work on old svg version
    svgElement.setAttribute('focusable', true);

    // Disable native draggable
    svgElement.setAttribute('draggable', false);

    // Add events
    addEvents();

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
            setViewBox(viewBoxResetValue);
            return this;
        },
        key: function (id, set) {
            key(id, set);
            return this;
        },
        coordinate: function (set) {
            coordinate(set);
            return this;
        },
        grid: function (set) {
            grid(set);
            return this;
        }
    };
};

// Old init function, remove when svg inline events removed
gnuplot_svg.Init = function() { };
