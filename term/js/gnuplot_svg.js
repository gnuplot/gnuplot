/*
 * $Id: gnuplot_svg.js,v 1.2 2011/04/07 21:48:25 sfeam Exp $
 */
// Shared routines for interaction with SVG fragments produced by 
// gnuplot's SVG terminal driver.

var gnuplot_svg = { };

gnuplot_svg.version = "07 April 2011";

gnuplot_svg.SVGDoc = null;
gnuplot_svg.SVGRoot = null;

gnuplot_svg.Init = function(e)
{
   gnuplot_svg.SVGDoc = e.target.ownerDocument;
   gnuplot_svg.SVGRoot = gnuplot_svg.SVGDoc.documentElement;
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
}
