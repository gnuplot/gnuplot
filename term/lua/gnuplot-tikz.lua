--[[

  This is the execution script for the `Lua generic terminal' driver.

  This script provides an interface to the PGF/TikZ package for LaTeX.


  Copyright 2008    Peter Hedwig <peter@affenbande.org>



  Permission to use, copy, and distribute this software and its
  documentation for any purpose with or without fee is hereby granted,
  provided that the above copyright notice appear in all copies and
  that both that copyright notice and this permission notice appear
  in supporting documentation.

  Permission to modify the software is granted, but not the right to
  distribute the complete modified source code.  Modifications are to
  be distributed as patches to the released version.  Permission to
  distribute binaries produced by compiling modified sources is granted,
  provided you
    1. distribute the corresponding source modifications from the
     released version in the form of a patch file along with the binaries,
    2. add special version identification to distinguish your version
     in addition to the base release version number,
    3. provide your name and address as the primary contact for the
     support of your modified version, and
    4. retain our contact information in regard to use of the base
     software.
  Permission to distribute the released version of the source code along
  with corresponding source modifications in the form of a patch file is
  granted with same provisions 2 through 4 for binary distributions.

  This software is provided "as is" without express or implied warranty
  to the extent permitted by applicable law.



  $Date: 2009/06/05 05:35:12 $
  $Author: sfeam $
  $Rev: 96 $

]]--



--[[
 
 `term'   gnuplot term_api -> local interface
 `gp'     local -> gnuplot interface

  are both initialized by the terminal

]]--


--
-- internal variables
--

local pgf = {}
local gfx = {}

-- the terminal default size in cm
pgf.DEFAULT_CANVAS_SIZE_X = 12.5
pgf.DEFAULT_CANVAS_SIZE_Y = 8.75
-- tic default size in cm
pgf.DEFAULT_TIC_SIZE = 0.18
-- the terminal resolution in "dots" per cm.
pgf.DEFAULT_RESOLUTION = 1000
-- default font size in TeX pt
pgf.DEFAULT_FONT_SIZE = 10

pgf.LATEX_STYLE_FILE = "gnuplot-lua-tikz"  -- \usepackage{gnuplot-lua-tikz}

pgf.REVISION = string.sub("$Rev: 96a $",7,-3)
pgf.REVISION_DATE = string.gsub("$Date: 2009/06/05 05:35:12 $",
                                "$Date: ([0-9]+).([0-9]+).([0-9]+) .*","%1/%2/%3")

pgf.styles = {}

-- the styles are used in conjunction with the 'tikzarrows'
-- and the style number directly corresponds to the used
-- angle in the gnuplot style definition
pgf.styles.arrows = {
   [1] = {"gp arrow 1", ">=latex"},
   [2] = {"gp arrow 2", ">=angle 90"},
   [3] = {"gp arrow 3", ">=angle 60"},
   [4] = {"gp arrow 4", ">=angle 45"},
   [5] = {"gp arrow 5", ">=o"},
   [6] = {"gp arrow 6", ">=*"},
   [7] = {"gp arrow 7", ">=diamond"},
   [8] = {"gp arrow 8", ">=open diamond"},
   [9] = {"gp arrow 9", ">={]}"},
  [10] = {"gp arrow 10", ">={[}"},
  [11] = {"gp arrow 11", ">=)"},
  [12] = {"gp arrow 12", ">=("}
}

-- plot styles are corresponding with linetypes and must have the same number of entries
-- see option 'tikzplot' for usage
pgf.styles.plotstyles_axes = {
  [1] = {"gp plot axes", ""},
  [2] = {"gp plot border", ""},
}

pgf.styles.plotstyles = {
  [1] = {"gp plot 0", "smooth"},
  [2] = {"gp plot 1", "smooth"},
  [3] = {"gp plot 2", "smooth"},
  [4] = {"gp plot 3", "smooth"},
  [5] = {"gp plot 4", "smooth"},
  [6] = {"gp plot 5", "smooth"},
  [7] = {"gp plot 6", "smooth"},
  [8] = {"gp plot 7", "smooth"}
}

pgf.styles.linetypes_axes = {
  [1] = {"gp lt axes", "dashed"},  -- An lt of -1 is used for the X and Y axes.  
  [2] = {"gp lt border", "solid"}, -- An lt of -2 is used for the border of the plot.
}

pgf.styles.linetypes = {
  [1] = {"gp lt plot 0", "solid"},  -- first graph
  [2] = {"gp lt plot 1", "dashed"}, -- second ...
  [3] = {"gp lt plot 2", "dash pattern=on 1.5pt off 2.25pt"},
  [4] = {"gp lt plot 3", "dash pattern=on \\pgflinewidth off 1.125"},
  [5] = {"gp lt plot 4", "dash pattern=on 4.5pt off 1.5pt on \\pgflinewidth off 1.5pt"},
  [6] = {"gp lt plot 5", "dash pattern=on 2.25pt off 2.25pt on \\pgflinewidth off 2.25pt"},
  [7] = {"gp lt plot 6", "dash pattern=on 1.5pt off 1.5pt on 1.5pt off 4.5pt"},
  [8] = {"gp lt plot 7", "dash pattern=on \\pgflinewidth off 1.5pt on 4.5pt off 1.5pt on \\pgflinewidth off 1.5pt"}
}

-- corresponds to pgf.styles.linetypes
pgf.styles.lt_colors_axes = {
  [1] = {"gp lt color axes", "black"},
  [2] = {"gp lt color border", "black"},
}

pgf.styles.lt_colors = {
  [1] = {"gp lt color 0", "red"},
  [2] = {"gp lt color 1", "green!60!black"},
  [3] = {"gp lt color 2", "blue"},
  [4] = {"gp lt color 3", "magenta"},
  [5] = {"gp lt color 4", "cyan"},
  [6] = {"gp lt color 5", "orange"},
  [7] = {"gp lt color 6", "yellow!80!red"},
  [8] = {"gp lt color 7", "blue!80!black"}
}

pgf.styles.patterns = {
  [1] = {"gp pattern 0", "white"},
  [2] = {"gp pattern 1", "pattern=north east lines"},
  [3] = {"gp pattern 2", "pattern=north west lines"},
  [4] = {"gp pattern 3", "pattern=crosshatch"},
  [5] = {"gp pattern 4", "pattern=grid"},
  [6] = {"gp pattern 5", "pattern=vertical lines"},
  [7] = {"gp pattern 6", "pattern=horizontal lines"},
  [8] = {"gp pattern 7", "pattern=dots"},
  [9] = {"gp pattern 8", "pattern=crosshatch dots"},
 [10] = {"gp pattern 9", "pattern=fivepointed stars"},
 [11] = {"gp pattern 10", "pattern=sixpointed stars"},
 [12] = {"gp pattern 11", "pattern=bricks"}
}


pgf.styles.plotmarks = {
  [1] = {"gp mark 0", "mark size=.5\\pgflinewidth,mark=*"}, -- point (-1)
  [2] = {"gp mark 1", "mark=+"},
  [3] = {"gp mark 2", "mark=x"},
  [4] = {"gp mark 3", "mark=star"},
  [5] = {"gp mark 4", "mark=square"},
  [6] = {"gp mark 5", "mark=square*"},
  [7] = {"gp mark 6", "mark=o"},
  [8] = {"gp mark 7", "mark=*"},
  [9] = {"gp mark 8", "mark=triangle"},
 [10] = {"gp mark 9", "mark=triangle*"},
 [11] = {"gp mark 10", "mark=triangle,mark options={rotate=180}"},
 [12] = {"gp mark 11", "mark=triangle*,mark options={rotate=180}"},
 [13] = {"gp mark 12", "mark=diamond"},
 [14] = {"gp mark 13", "mark=diamond*"},
 [15] = {"gp mark 14", "mark=otimes"},
 [16] = {"gp mark 15", "mark=oplus"}
}  

--[[===============================================================================================

    helper functions

]]--===============================================================================================

-- from the Lua wiki
explode = function(div,str)
  if (div=='') then return false end
  local pos,arr = 0,{}
  local trim = function(s) return (string.gsub(s,"^%s*(.-)%s*$", "%1")) end
  -- for each divider found
  for st,sp in function() return string.find(str,div,pos,true) end do
    table.insert(arr, trim(string.sub(str,pos,st-1))) -- Attach chars left of current divider
    pos = sp + 1 -- Jump past current divider
  end
  table.insert(arr, trim(string.sub(str,pos))) -- Attach chars right of last divider
  return arr
end



--[[===============================================================================================

  The PGF/TikZ output routines

]]--===============================================================================================


pgf.transform_xcoord = function(coord)
  return (coord+gfx.origin_xoffset)*gfx.scalex
end

pgf.transform_ycoord = function(coord)
  return (coord+gfx.origin_yoffset)*gfx.scaley
end

pgf.format_coord = function(xc, yc)
  return string.format("%.3f,%.3f", pgf.transform_xcoord(xc), pgf.transform_ycoord(yc))
end

pgf.write_doc_begin = function(preamble)
  gp.write("\\documentclass["..pgf.DEFAULT_FONT_SIZE.."pt]{article}\n"
        .."\\usepackage[T1]{fontenc}\n"
        .."\\usepackage{textcomp}\n\n"
        .."\\usepackage[utf8x]{inputenc}\n\n"
        .."\\usepackage{"..pgf.LATEX_STYLE_FILE.."}\n"
        .."\\pagestyle{empty}\n"
        .."\\usepackage[active,tightpage]{preview}\n"
        .."\\PreviewEnvironment{tikzpicture}\n"
        .."\\setlength\\PreviewBorder{2mm}\n"
        ..preamble.."\n\n"
        .."\\begin{document}\n")
end

pgf.write_doc_end = function()
  gp.write("\\end{document}\n")
end

pgf.write_graph_begin = function (font, noenv)
  local global_opt = "" -- unused
  if noenv then
    gp.write("%% ") -- comment out
  end
  gp.write(string.format("\\begin{tikzpicture}[gnuplot%s]\n",global_opt))
  gp.write(string.format("%%%% generated with GNUPLOT %sp%s (%s; terminal rev. %s, script rev. %s)\n%%%% %s\n",
      term.gp_version, term.gp_patchlevel,
      string.match(term.lua_ident, "Lua [0-9\.]+"),
      string.sub(term.lua_term_revision,7,-3),
      pgf.REVISION,os.date()))
  if font ~= "" then
    gp.write(string.format("\\tikzstyle{every node}+=[font=%s]\n", font))
  end
  if not gfx.opt.lines_dashed then
    gp.write("\\gpsolidlines\n")
  end
  if not gfx.opt.lines_colored then
    gp.write("\\gpmonochromelines\n")
  end
end

pgf.write_graph_end = function(noenv)
  if noenv then
    gp.write("%% ") -- comment out
  end
  gp.write("\\end{tikzpicture}\n")
end

pgf.draw_path = function(t)

  local use_plot = false
  local c_str = '--'

  -- is the current linetype in the list of plots?
  if #gfx.opt.plot_list > 0 then
    for k, v in pairs(gfx.opt.plot_list) do
      if gfx.linetype_idx_set == v  then
        use_plot = true
        c_str = ' '
        break
      end
    end
  end

  gp.write("\\draw[gp path] ")
  if use_plot then
    gp.write("plot["..pgf.styles.plotstyles[((gfx.linetype_idx_set) % #pgf.styles.plotstyles)+1][1].."] coordinates {")
  end
  gp.write("("..pgf.format_coord(t[1][1], t[1][2])..")")
  for i = 2,#t-1 do
    -- pretty printing
    if (i % 6) == 0 then
      gp.write("%\n  ")
    end
    gp.write(c_str.."("..pgf.format_coord(t[i][1], t[i][2])..")")
  end
  if (#t % 6) == 0 then
    gp.write("%\n  ")
  end
  -- check for a cyclic path
  if (t[1][1] == t[#t][1]) and (t[1][2] == t[#t][2]) and (not use_plot) then
    gp.write("--cycle")
  else
    gp.write(c_str.."("..pgf.format_coord(t[#t][1], t[#t][2])..")")
  end
  if use_plot then
    gp.write("}")
  end
  gp.write(";\n")
end


pgf.draw_arrow = function(t, direction, headstyle)
  gp.write("\\draw[gp path")
  if direction ~= '' then
    gp.write(","..direction)
  end
  if headstyle > 0 then
    gp.write(",gp arrow "..headstyle)
  end
  gp.write("]")
  gp.write("("..pgf.format_coord(t[1][1], t[1][2])..")")
  for i = 2,#t do
    if (i % 6) == 0 then
      gp.write("%\n  ")
    end
    gp.write("--("..pgf.format_coord(t[i][1], t[i][2])..")")
  end
  gp.write(";\n")
end


pgf.draw_points = function(t, pm)
  gp.write("\\gppoint{"..pm.."}{")
  for i,v in ipairs(t) do
      gp.write("("..pgf.format_coord(v[1], v[2])..")")
  end
  gp.write("}\n")
end


pgf.set_linetype = function(linetype)
  gp.write("\\gpsetlinetype{"..linetype.."}\n")
end


pgf.set_color = function(color)
  gp.write("\\gpcolor{"..color.."}\n")
end


pgf.set_linewidth = function(width)
  gp.write(string.format("\\gpsetlinewidth{%.2f}\n", width))
end


pgf.set_pointsize = function(size)
  gp.write(string.format("\\gpsetpointsize{%.2f}\n", 4*size))
end


pgf.write_text_node = function(t, text, angle, justification, font)
  local node_options = justification
  if angle ~= 0 then
    node_options = node_options .. ",rotate=" .. angle
  end
  if font ~= '' then
    node_options = node_options .. ",font=" .. font
  end  
  gp.write(string.format("\\node[%s] at (%s) {%s};\n", 
          node_options, pgf.format_coord(t[1], t[2]), text))
end


pgf.draw_fill = function(t, pattern, color, saturation, opacity)
  local fill_path = ''
  local fill_style = ''
  
  if saturation < 100 then
    gp.write("\\begin{colormixin}{"..saturation.."!white}\n")
  end

  fill_path = fill_path .. '('..pgf.format_coord(t[1][1], t[1][2])..')'
  -- draw 2nd to n-1 corners
  for i = 2,#t-1 do
    if (i % 5) == 0 then
      -- pretty printing
      fill_path = fill_path .. "%\n    "
    end
    fill_path = fill_path .. '--('..pgf.format_coord(t[i][1], t[i][2])..')'
  end
  if (#t % 5) == 0 then
    gp.write("%\n  ")
  end
  -- draw last corner
  -- 'cycle' is just for the case that we want to draw a
  -- line around the filled area
  if (t[1][1] == t[#t][1]) and (t[1][2] == t[#t][2]) then -- cyclic
    fill_path = fill_path .. '--cycle'
  else
    fill_path = fill_path
          .. '--('..pgf.format_coord(t[#t][1], t[#t][2])..')--cycle'
  end
  
  if pattern == '' then
    -- solid fills
    fill_style = 'color='..color
    if opacity < 100 then
      fill_style = fill_style..string.format(",opacity=%.2f", opacity/100)
    else
      -- fill_style = "" -- color ?
    end
  else
    -- pattern fills
    fill_style = pattern..',pattern color='..color
  end
  local out = ''
  if (pattern ~= '') and (opacity == 100) then
    -- have to fill bg for opaque patterns
    gp.write("\\def\\gpfillpath{"..fill_path.."}\n"
          .. "\\gpfill{color=gpbgfillcolor} \\gpfillpath;\n"
          .. "\\gpfill{"..fill_style.."} \\gpfillpath;\n")
  else
    gp.write("\\gpfill{"..fill_style.."} "..fill_path..";\n")
  end
  
  if saturation < 100 then
    gp.write("\\end{colormixin}\n")
  end
end

pgf.draw_raw_rgb_image = function(t, m, n, ll, ur)
  local gw = gp.write
  local sf = string.format
  local xs = sf("%.3f", pgf.transform_xcoord(ur[1]) - pgf.transform_xcoord(ll[1]))
  local ys = sf("%.3f", pgf.transform_ycoord(ur[2]) - pgf.transform_ycoord(ll[2]))
  gw("\\def\\gprawrgbimagedata{%\n  ")
  for cnt = 1,#t do
    gw(sf("%02x%02x%02x", 255*t[cnt][1]+0.5, 255*t[cnt][2]+0.5, 255*t[cnt][3]+0.5))
    if (cnt % 16) == 0 then
      gw("%\n  ")
    end
  end
  gw("}%\n")
  gw("\\gprawimage{rgb}{"..sf("%.3f", pgf.transform_xcoord(ll[1])).."}"
      .."{"..sf("%.3f", pgf.transform_ycoord(ll[2])).."}"
      .."{"..m.."}{"..n.."}{"..xs.."}{"..ys.."}{\\gprawrgbimagedata}\n")
end

pgf.draw_raw_cmyk_image = function(t, m, n, ll, ur)
  local gw = gp.write
  local sf = string.format
  local min = math.min
  local max = math.max
  local mf = math.floor
  local UCRBG = {1,1,1,1} -- default corrections
  local rgb2cmyk255 = function(r,g,b)
    local c = 1-r
    local m = 1-g
    local y = 1-b
    local k = min(c,m,y)
    c = mf(255*min(1, max(0, c - UCRBG[1]*k))+0.5)
    m = mf(255*min(1, max(0, m - UCRBG[2]*k))+0.5)
    y = mf(255*min(1, max(0, y - UCRBG[3]*k))+0.5)
    k = mf(255*min(1, max(0,     UCRBG[4]*k))+0.5)
    return c,m,y,k
  end
  local xs = sf("%.3f", pgf.transform_xcoord(ur[1]) - pgf.transform_xcoord(ll[1]))
  local ys = sf("%.3f", pgf.transform_ycoord(ur[2]) - pgf.transform_ycoord(ll[2]))
  gw("\\def\\gprawcmykimagedata{%\n  ")
  for cnt = 1,#t do
    gw(sf("%02x%02x%02x%02x", rgb2cmyk255(t[cnt][1],t[cnt][2],t[cnt][3])))
    if (cnt % 12) == 0 then
      gw("%\n  ")
    end
  end
  gw("}%\n")
  gw("\\gprawimage{cmyk}{"..sf("%.3f", pgf.transform_xcoord(ll[1])).."}"
      .."{"..sf("%.3f", pgf.transform_ycoord(ll[2])).."}"
      .."{"..m.."}{"..n.."}{"..xs.."}{"..ys.."}{\\gprawcmykimagedata}\n")
end

pgf.write_clipbox_begin = function (ll, ur)
  gp.write("\\begin{scope}\n")
  gp.write(string.format("\\clip (%s) rectangle (%s);\n",
      pgf.format_coord(ll[1],ll[2]),pgf.format_coord(ur[1],ur[2])))
end

pgf.write_clipbox_end = function()
  gp.write("\\end{scope}\n")
end

pgf.write_boundingbox = function(t, num)
  gp.write("%% coordinates of the plot area\n")
  gp.write("\\\gpdefrectangularnode{gp plot "..num.."}{"
    ..string.format("\\pgfpoint{%.3fcm}{%.3fcm}", pgf.transform_xcoord(t.xleft), pgf.transform_ycoord(t.ybot)).."}{"
    ..string.format("\\pgfpoint{%.3fcm}{%.3fcm}", pgf.transform_xcoord(t.xright), pgf.transform_ycoord(t.ytop)).."}\n")
end

pgf.write_variables = function(t)
  gp.write("%% gnuplot variables\n")
  for k, v in pairs(t) do
    gp.write(string.format("\\gpsetvar{%s}{%s}\n",k,v))
  end
end

-- write style to seperate file, or whatever...
pgf.create_style = function(f)
f:write([[
%%
%%  This is the style file for the gnuplot PGF/TikZ terminal
%%  
%%  It is associated with the 'gnuplot.lua' script, and usually generated
%%  automatically. So take care whenever you make any changes!
%%
\NeedsTeXFormat{LaTeX2e}
]])
f:write("\\ProvidesPackage{"..pgf.LATEX_STYLE_FILE.."}%\n")
f:write("          ["..pgf.REVISION_DATE.." (rev. "..pgf.REVISION..") GNUPLOT Lua terminal style]\n\n")
f:write([[
\RequirePackage{tikz,xxcolor,ifpdf,ifxetex}

\usetikzlibrary{arrows,patterns,plotmarks}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%
%%  
%%

%
% image related stuff
%
\def\gp@rawimage@pdf#1#2#3#4#5#6{%
  \def\gp@tempa{cmyk}%
  \def\gp@tempb{#1}%
  \ifx\gp@tempa\gp@tempb%
    \def\gp@temp{/CMYK}%
  \else%
    \def\gp@temp{/RGB}%
  \fi%
  \pgf@sys@bp{#4}\pgfsysprotocol@literalbuffered{0 0}\pgf@sys@bp{#5}%
  \pgfsysprotocol@literalbuffered{0 0 cm}%
  \pgfsysprotocol@literalbuffered{BI /W #2 /H #3 /CS \gp@temp}%
  \pgfsysprotocol@literalbuffered{/BPC 8 /F /AHx ID}%
  \pgfsysprotocol@literal{#6 > EI}%
}
\def\gp@rawimage@ps#1#2#3#4#5#6{%
  \def\gp@tempa{cmyk}%
  \def\gp@tempb{#1}%
  \ifx\gp@tempa\gp@tempb%
    \def\gp@temp{4}%
  \else%
    \def\gp@temp{3}%
  \fi%
  \pgfsysprotocol@literalbuffered{0 0 translate}%
  \pgf@sys@bp{#4}\pgf@sys@bp{#5}\pgfsysprotocol@literalbuffered{scale}%
  \pgfsysprotocol@literalbuffered{#2 #3 8 [#2 0 0 -#3 0 #3]}%
  \pgfsysprotocol@literalbuffered{currentfile /ASCIIHexDecode filter}%
  \pgfsysprotocol@literalbuffered{false \gp@temp\space colorimage}%
  \pgfsysprotocol@literal{#6 >}%
}


\ifpdf
  \def\gp@rawimage{\gp@rawimage@pdf}
\else
  \ifxetex
    \def\gp@rawimage{\gp@rawimage@pdf}
  \else
    \def\gp@rawimage{\gp@rawimage@ps}
  \fi
\fi

\def\gp@set@size#1{%
  \def\gp@image@size{#1}%
}
%% \gprawimage{color model}{xcoord}{ycoord}{# of xpixel}{# of ypixel}{xsize}{ysize}{rgb/cmyk hex data RRGGBB/CCMMYYKK ...}
%% color model is 'cmyk' or 'rgb' (default)
\def\gprawimage#1#2#3#4#5#6#7#8{%
  \tikz@scan@one@point\gp@set@size(#6,#7)\relax%
  \tikz@scan@one@point\pgftransformshift(#2,#3)\relax%
  \pgftext {%
    \pgfsys@beginpurepicture%
    \gp@image@size% fill \pgf@x and \pgf@y
    \gp@rawimage{#1}{#4}{#5}{\pgf@x}{\pgf@y}{#8}%
    \pgfsys@endpurepicture%
  }%
}

%
% gnuplottex comapatibility
% (see http://www.ctan.org/tex-archive/help/Catalogue/entries/gnuplottex.html)
%

\def\gnuplottexextension@lua{\string tex}

%
% gnuplot variables getter and setter
%

\def\gpsetvar#1#2{%
  \expandafter\xdef\csname gp@var@#1\endcsname{#2}
}

\def\gpgetvar#1{%
  \csname gp@var@#1\endcsname %
}

%
% some wrapper code
%

% short for the lengthy xcolor rgb definition
\def\gprgb#1#2#3{rgb,1000:red,#1;green,#2;blue,#3}

% short for a filled path
\def\gpfill#1{\path[fill,#1]}

% short for changing the linewidth
\def\gpsetlinewidth#1{\pgfsetlinewidth{#1\gpbaselw}}

\def\gpsetlinetype#1{\tikzstyle{gp path}=[#1,#1 add]}

% short for changing the pointsize
\def\gpsetpointsize#1{\tikzstyle{gp point}=[mark size=#1\gpbasems]}

% wrapper for color settings
\def\gpcolor#1{\pgfsetcolor{#1}}

% prevent plot mark distortions due to changes in the PGF transformation matrix
% use `\gpscalepointstrue' and `\gpscalepointsfalse' for enabling and disabling
% point scaling
%
\newif\ifgpscalepoints
\tikzoption{gp shift only}[]{%
  \ifgpscalepoints%
  \else%
    % this is actually the same definition as used by "shift only" (seen
    % in pgf-1.18 and later)
    \tikz@addtransform{\pgftransformresetnontranslations}%
  \fi%
}
\def\gppoint#1#2{%
  \path[solid] plot[only marks,gp point,#1,mark options={gp shift only}] coordinates {#2};%
}

\def\gpfontsize#1#2{\fontsize{#1}{#2}\selectfont}

%
% char size calculation, that might be used with gnuplottex
%
% Example code (needs gnuplottex.sty):
%
%    % calculate the char size when the "gnuplot" style is used
%    \tikzset{gnuplot/.append style={execute at begin picture=\gpcalccharsize}}
%
%    \tikzset{gnuplot/.append style={font=\ttfamily\footnotesize}}
%
%    \begin{tikzpicture}[gnuplot]
%      \begin{gnuplot}[terminal=lua,%
%          terminaloptions={tikz solid nopic charsize \the\gphcharsize,\the\gpvcharsize}]
%        test
%      \end{gnuplot}
%    \end{tikzpicture}
%
%%%
% The `\gpcalccharsize' command fills the lengths \gpvcharsize and \gphcharsize with
% the values of the current default font used within nodes and is meant to be called
% within a tikzpicture environment.
% 
\newdimen\gpvcharsize
\newdimen\gphcharsize
\def\gpcalccharsize{%
  \pgfinterruptboundingbox%
  \pgfsys@begininvisible%
  \node at (0,0) {%
    \global\gphcharsize=1.05\fontcharwd\font`0%
    \global\gpvcharsize=1.05\fontcharht\font`0%
    \global\advance\gpvcharsize by 1.05\fontchardp\font`g%
  };%
  \pgfsys@endinvisible%
  \endpgfinterruptboundingbox%
}

%
%  define a rectangular node in tikz e.g. for the plot area
%  FIXME: this is done globally to work with gnuplottex.sty
%
%  #1 node name
%  #2 coordinate of "south west"
%  #3 coordinate of "north east"
%
\def\gpdefrectangularnode#1#2#3{%
  \expandafter\gdef\csname pgf@sh@ns@#1\endcsname{rectangle}
  \expandafter\gdef\csname pgf@sh@np@#1\endcsname{%
    \def\southwest{#2}%
    \def\northeast{#3}%
  }
  \pgfgettransform\pgf@temp%
  % once it is defined, no more transformations will be applied, I hope
  \expandafter\xdef\csname pgf@sh@nt@#1\endcsname{\pgf@temp}%
  \expandafter\xdef\csname pgf@sh@pi@#1\endcsname{\pgfpictureid}%
}


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%
%%  You may want to adapt the following to fit your needs (in your 
%%  individual style file and/or within your document).
%%

%
% style for every plot
%
\tikzstyle{gnuplot}=[%
  >=stealth',%
  cap=round,%
  join=round,%
]

\tikzstyle{gp node left}=[anchor=mid west,yshift=-.12ex]
\tikzstyle{gp node center}=[anchor=mid,yshift=-.12ex]
\tikzstyle{gp node right}=[anchor=mid east,yshift=-.12ex]

% basic plot mark size (points)
\newdimen\gpbasems
\gpbasems=.4pt

% basic linewidth
\newdimen\gpbaselw
\gpbaselw=.4pt

% this is the default color for pattern backgrounds
\colorlet{gpbgfillcolor}{white}


% this should reverse the normal text node presets, for the
% later referencing as described below
\tikzstyle{gp refnode}=[coordinate,yshift=.12ex]

% to add an empty label with the referenceable name "my node"
% to the plot, just add the following line to your gnuplot
% file:
%
% set label "" at 1,1 font ",gp refnode,name=my node"
%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%
%%  The following TikZ-styles are derived from the 'pgf.styles.*' tables
%%  in the Lua script.
%%  To change the number of used styles you should change them there and
%%  regenerate this style file.
%%

]])
  f:write("% arrow styles settings\n")
  for i = 1, #pgf.styles.arrows do
    f:write("\\tikzstyle{"..pgf.styles.arrows[i][1].."} = ["..pgf.styles.arrows[i][2].."]\n")
  end
  f:write("\n% plotmark settings\n")
  for i = 1, #pgf.styles.plotmarks do
    f:write("\\tikzstyle{"..pgf.styles.plotmarks[i][1].."} = ["..pgf.styles.plotmarks[i][2].."]\n")
  end
  f:write("\n% pattern settings\n")
  for i = 1, #pgf.styles.patterns do
    f:write("\\tikzstyle{"..pgf.styles.patterns[i][1].."} = ["..pgf.styles.patterns[i][2].."]\n")
  end
  f:write("\n% if the 'tikzplot' option is used the corresponding lines will be smoothed by default\n")
  for i = 1, #pgf.styles.plotstyles_axes do
    f:write("\\tikzstyle{"..pgf.styles.plotstyles_axes[i][1].."} = ["..pgf.styles.plotstyles_axes[i][2].."]%\n")
  end
  for i = 1, #pgf.styles.plotstyles do
    f:write("\\tikzstyle{"..pgf.styles.plotstyles[i][1].."} = ["..pgf.styles.plotstyles[i][2].."]%\n")
  end
  -- line styles for borders etc ...
  f:write("\n% linestyle settings\n")
  for i = 1, #pgf.styles.linetypes_axes do
    f:write("\\tikzstyle{"..pgf.styles.linetypes_axes[i][1].."} = ["..pgf.styles.linetypes_axes[i][2].."]\n")
  end
  f:write("\n% linestyle \"addon\" settings for overwriting a default linestyle within the\n")
  f:write("% TeX document via eg. \\tikzstyle{gp lt plot 1 add}=[fill=black,draw=none] etc.\n")
  for i = 1, #pgf.styles.linetypes_axes do
    f:write("\\tikzstyle{"..pgf.styles.linetypes_axes[i][1].." add} = []\n")
  end
  for i = 1, #pgf.styles.linetypes do
    f:write("\\tikzstyle{"..pgf.styles.linetypes[i][1].." add} = []\n")
  end
  f:write("\n% linestyle color settings\n")
  for i = 1, #pgf.styles.lt_colors_axes do
    f:write("\\colorlet{"..pgf.styles.lt_colors_axes[i][1].."}{"..pgf.styles.lt_colors_axes[i][2].."}\n")
  end
  -- line styles for the plots
  f:write("\n% command for switching to dashed lines\n")
  f:write("\\def\\gpdashedlines{%\n")
  for i = 1, #pgf.styles.linetypes do
    f:write("  \\tikzstyle{"..pgf.styles.linetypes[i][1].."} = ["..pgf.styles.linetypes[i][2].."]%\n")
  end
  f:write("}\n")
  f:write("\n% command for switching to colored lines\n")
  f:write("\\def\\gpcoloredlines{%\n")
  for i = 1, #pgf.styles.lt_colors do
    f:write("  \\colorlet{"..pgf.styles.lt_colors[i][1].."}{"..pgf.styles.lt_colors[i][2].."}%\n")
  end
  f:write("}\n")
  f:write("\n% command for switching to solid lines\n")
  f:write("\\def\\gpsolidlines{%\n")
  for i = 1, #pgf.styles.linetypes do
    f:write("  \\tikzstyle{"..pgf.styles.linetypes[i][1].."} = [solid]%\n")
  end
  f:write("}\n")
  f:write("\n% command for switching to monochrome (black) lines\n")
  f:write("\\def\\gpmonochromelines{%\n")
  for i = 1, #pgf.styles.lt_colors do
    f:write("  \\colorlet{"..pgf.styles.lt_colors[i][1].."}{black}%\n")
  end
  f:write("}\n\n")
  f:write([[
%
% some initialisations
%
% by default all lines will be colored and dashed
\gpcoloredlines
\gpdashedlines
\gpsetpointsize{4}
\gpsetlinetype{gp lt solid}
\gpscalepointsfalse
\endinput
]])
  f:close()
end


pgf.print_help = function(fwrite)

  fwrite([[
      {help}
      {monochrome}
      {solid}
      {originreset}
      {gparrows}
      {gppoints}
      {nopicenvironment}
      {size <x>{unit},<y>{unit}}
      {scale <x>,<y>}
      {plotsize <x>{unit},<y>{unit}}
      {charsize <x>{unit},<y>{unit}}
      {font "<fontdesc>"}
      {createstyle}
      {fulldoc|standalone}
      {{preamble|header} "<preamble_string>"}
      {tikzplot <ltn>,...}
      {tikzarrows}
      {cmykimages}
      {nobitmap}
      {providevars <var name>,...}

 For all options that expect lengths as their arguments they
 will default to 'cm' if no unit is specified. For all lengths
 the following units may be used: 'cm', 'mm', 'in' or 'inch',
 'pt', 'pc', 'bp', 'dd', 'cc'. Blanks between numbers and units
 are not allowed.

 'monochrome' disables line coloring and switches to grayscaled
 fills.

 'solid' use only solid lines.

 'originreset' moves the origin of the TikZ picture to the lower
 left corner of the plot. It may be used to align several plots
 within one tikzpicture environment. This is not tested with
 multiplots and pm3d plots!

 'gparrows' use gnuplot's internal arrow drawing function
 instead of the ones provided by TikZ.

 'gppoints' use gnuplot's internal plotmark drawing function
 instead of the ones provided by TikZ.

 'nopicenvironment' omits the declaration of the 'tikzpicture'
 environment in order to set it manually. This permits putting
 some PGF/TikZ code directly before or after the plot.

 The 'size' option expects two lenghts <x> and <y> as the canvas size.
 The default size of the canvas is ]]..pgf.DEFAULT_CANVAS_SIZE_X..[[cm x ]]..pgf.DEFAULT_CANVAS_SIZE_Y..[[cm.

 The 'scale' option works similar to the 'size' option but expects
 scaling factors <x> and <y> instead of lengths.

 The 'plotsize' option permits setting the size of the plot area
 instead of the canvas size, which is the usual gnuplot behaviour.
 Using this option may lead to slightly asymmetric tic lengths.
 Like 'originreset' this option may not lead to convenient results
 if used with multiplots or pm3d plots.

 The 'charsize' option expects the average horizontal and vertical
 size of the used font. Look at the generated style file for an
 example of how to use it from within your TeX document.

 'createstyle' derives the LaTeX style file from the script and
 writes it to the file ']]..pgf.LATEX_STYLE_FILE..'.sty'..[['.

 'fulldoc' or 'standalone' produces a full LaTeX document for direct
 compilation.

 'preamble' or 'header' may be used to put any additional LaTeX code
 into the document preamble in standalone mode.

 With the 'tikzplot' option the '\path plot' command will be used
 instead of only '\path'. The following list of numbers of linetypes
 (<ltn>,...) defines the affected plotlines. There exists a plotstyle
 for every linetype. The default plotstyle is 'smooth' for every
 linetype >= 1.

 By using the 'tikzarrows' option the gnuplot arrow styles defined by
 the user will be mapped to TikZ arrow styles. This is done by 'misusing'
 the angle value of the arrow definition. E.g. an arrow style with the
 angle '7' will be mapped to the TikZ style 'gp arrow 7' ignoring all the
 other given values. By default the TikZ terminal uses the stealth' arrow
 tips for all arrows. To obtain the default gnuplot behaviour please use
 the 'gparrows' option.

 With 'cmykimages' the CMYK color model will be used for image data
 instead of the RGB model. All other colors (like line colors etc.) are
 not affected by this option, since they are handled by the xcolors
 package. So take care to change the color model also there if needed.

 The 'nobitmap' option let images be rendered as filled rectangles
 instead of the nativ PS or PDF image format. This option has to be
 enabled if you intend to use other output formats.

 The 'providevars' options makes gnuplot's internal and user variables
 available by using the '\gpgetvar{<var name>}' commmand within the TeX
 script. Use gnuplot's 'show variables all' command to see the list
 of valid variables.

 The <fontdesc> string may contain any valid LaTeX font commands like
 e.g. '\small'. It is passed directly as a node parameter in form of
 "font=<fontdesc>". This can be 'misused' to add further code to a node,
 e.g. '\small,yshift=1ex' or ',yshift=1ex' are also valid while the
 latter does not change the current font settings. One exception is
 the second argument of the list. If it is a number of the form
 <number>{unit} it will be interpreted as a fontsize like in other
 terminals and will be appended to the first argument. If the unit is
 omitted the value is interpreted as 'pt'. As an example the string
 '\sffamily,12,fill=red' sets the font to LaTeX's sans serif font at
 a size of 12pt and red background color.

 Strings have to be put in single or double quotes. Double quoted
 strings may contain special characters like newlines '\n' etc.
]])
end


--[[===============================================================================================

  gfx.* helper functions
  
  Main intention is to prevent redundancies in the drawing
  operations and keep the pgf.* API as consistent as possible.
  
]]--===============================================================================================


gfx.in_path = false

gfx.path = {}
gfx.posx = nil
gfx.posy = nil


-- gfx.DEFAULT_LINE_TYPE = -2
-- gfx.linetype_idx = gfx.DEFAULT_LINE_TYPE -- current linetype intended for the plot
gfx.linetype_idx = nil       -- current linetype intended for the plot
gfx.linetype_idx_set = nil   -- current linetype set in the plot
gfx.linewidth = nil
gfx.linewidth_set = nil

-- internal calculated scaling factors
gfx.scalex = 1
gfx.scaley = 1

-- recalculate the origin of the plot
-- used for moving the origin to the lower left
-- corner...
gfx.origin_xoffset = 0
gfx.origin_yoffset = 0



-- color set in the document
gfx.color = ''
gfx.color_set = ''

gfx.pointsize = nil
gfx.pointsize_set = nil

gfx.text_font = ''
gfx.text_justify = "center"
gfx.text_angle = 0

-- option vars
gfx.opt = {
  latex_preamble = '',
  default_font = '',
  lines_dashed = true,
  lines_colored = true,
  -- use gnuplot arrows or points instead of TikZ?
  gp_arrows = false,
  gp_points = false,
  -- don't put graphic commands into a tikzpicture environment
  nopicenv = false,
  -- produce full LaTeX document?
  full_doc = false,
  -- in gnuplot all sizes refer to the size of the canvas
  -- and not the size of plot itself
  plotsize_x = nil,
  plotsize_y = nil,
  set_plotsize = false,
  -- recalculate the origin of the plot
  -- used for moving the origin to the lower left
  -- corner...
  set_origin = false,
  -- list of _linetypes_ of plots that should be drawn as with the \plot
  -- command instead of \path
  plot_list = {},
  -- uses some pdf/ps specials with image function that will only work
  -- with pdf/ps generation!
  direct_image = true,
  -- list of gnuplot variables that should be made available via
  -- \gpsetvar{name}{val}
  gnuplot_vars = {},
  -- if true, the gnuplot arrow will be mapped to TikZ arrow styles by the
  -- given angle. E.g. an arrow with the angle `7' will be mapped to `gp arrow 7'
  -- style.
  tikzarrows = false,
  -- if true, cmyk image model will be used for bitmap images
  cmykimage = false
}

-- within tikzpicture environment or not
gfx.in_picture = false

-- have not determined the plotbox, see the 'plotsize' option
gfx.have_plotbox = false

gfx.current_boundingbox = {
  xleft = nil, xright = nil, ytop = nil, ybot = nil
}
-- plot bounding boxes counter
gfx.boundingbox_cnt = 0

gfx.TEXT_ANCHOR = {
  ["left"]   = "gp node left",
  ["center"] = "gp node center",
  ["right"]  = "gp node right"
}

gfx.HEAD_STR = {"", "->", "<-", "<->"}


-- conversion factors in `cm'
gfx.units = {
  ['']    = 1,        -- default
  ['cm']  = 1,
  ['mm']  = 0.1,
  ['in']  = 2.54,
  ['inch']= 2.54,
  ['pt']  = 0.035146, -- Pica Point   (72.27pt = 1in)
  ['pc']  = 0.42176,  -- Pica         (1 Pica = 1/6 inch)
  ['bp']  = 0.035278, -- Big Point    (72bp = 1in)
  ['dd']  = 0.0376,   -- Didot Point  (1cm = 26.6dd)
  ['cc']  = 0.45113   -- Cicero       (1cc = 12 dd)
}


gfx.parse_number_unit = function (str, from, to)
  to = to or 'cm'
  from = from or 'cm'
  local num, unit = string.match(str, '([%d%.]+)([a-z]*)')
  if unit and (string.len(unit) > 0) then
    from = unit
  else
    unit = false
  end
  local factor_from = gfx.units[from]
  local factor_to   = gfx.units[to]
  num = tonumber(num)
  if num and factor_from then
    -- to cm and then to our target unit
    return num*(factor_from/factor_to), unit
  else
    return false, false
  end
end


gfx.parse_font_string = function (str)
  local size,rets,toks = nil, str, explode(',', str)
  -- if at least two tokens
  if #toks > 1 then
    -- add first element to font string
    rets = table.remove(toks,1)
    -- no unit means 'pt'
    size, _ = gfx.parse_number_unit(toks[1],'pt','pt')
    if (size) then
      table.remove(toks,1)
      rets = rets .. string.format('\\gpfontsize{%.2fpt}{%.2fpt}',size,size*1.2)
    end
    -- add remaining parts
    for k,v in ipairs(toks) do
      rets = rets .. ',' .. v
    end
  end
  return rets, size
end


gfx.write_boundingbox = function()
  local t = gp.get_boundingbox()
  for k, v in pairs (t) do
    if v ~= gfx.current_boundingbox[k] then
      gfx.boundingbox_cnt = gfx.boundingbox_cnt + 1
      gfx.current_boundingbox = t
      pgf.write_boundingbox(t, gfx.boundingbox_cnt)
      break
    end  
  end
end

gfx.adjust_plotbox = function()
  local t = gp.get_boundingbox()
  if gfx.opt.set_origin then
    -- move origin to the lower left corner of the plot
    gfx.origin_xoffset = - t.xleft
    gfx.origin_yoffset = - t.ybot
  end
  if gfx.opt.set_plotsize then
    if (t.xright - t.xleft) > 0 then
      gfx.scalex = gfx.scalex*gfx.opt.plotsize_x * pgf.DEFAULT_RESOLUTION/(t.xright - t.xleft)
      gfx.scaley = gfx.scaley*gfx.opt.plotsize_y * pgf.DEFAULT_RESOLUTION/(t.ytop - t.ybot)
    else
      -- could not determin a valid bounding box, so keep using the
      -- plotsize as the canvas size
      gp.term_out("WARNING: PGF/TikZ Terminal: `plotsize' option used, but I could not determin the plot area!\n")
    end
  end
end


gfx.check_variables = function()
  local vl = gfx.opt.gnuplot_vars
  local t = gp.get_all_variables()
  local sl = {}
  for i=1,#vl do
    if t[vl[i]] then
      sl[vl[i]] = t[vl[i]][3]
      if t[vl[i]][4] then
        sl[vl[i].." Im"] = t[vl[i]][4]
      end
    end
  end
  pgf.write_variables(sl)
end


-- do we have to start a new path?
gfx.check_in_path = function()
  -- boundingbox data is available with the first
  -- drawing command
  if (not gfx.have_plotbox) and gfx.in_picture then
    gfx.adjust_plotbox()
    gfx.have_plotbox = true
  end
  
  if gfx.in_path == true then
    if #gfx.path > 1 then
      -- don't draw zero length paths
      pgf.draw_path(gfx.path)
    end
    gfx.in_path = false
    gfx.path = {}
    gfx.posx = nil
    gfx.posy = nil
  end
end

-- did the linetype change?
gfx.check_linetype = function()
  if gfx.linetype_idx ~= gfx.linetype_idx_set then
    local lt
    if gfx.linetype_idx < 0 then
      lt = pgf.styles.linetypes_axes[math.abs(gfx.linetype_idx)][1]
    else
      lt = pgf.styles.linetypes[(gfx.linetype_idx % #pgf.styles.linetypes)+1][1]
    end
    pgf.set_linetype(lt)
    gfx.linetype_idx_set = gfx.linetype_idx
  end
end

-- did the color change?
gfx.check_color = function()
  if gfx.color_set ~= gfx.color then
    pgf.set_color(gfx.color)
    gfx.color_set = gfx.color
  end
end

-- sanity check if we already are at this position in our path
-- and save this position
gfx.check_coord = function(x, y)
  if (x == gfx.posx) and (y == gfx.posy) then
    return true
  end
  gfx.posx = x
  gfx.posy = y
  return false
end

-- did the linewidth change?
gfx.check_linewidth = function()
  if gfx.linewidth ~= gfx.linewidth_set then
    pgf.set_linewidth(gfx.linewidth)
    gfx.linewidth_set = gfx.linewidth
  end
end

-- did the pointsize change?
gfx.check_pointsize = function()
  if gfx.pointsize ~= gfx.pointsize_set then
    pgf.set_pointsize(gfx.pointsize)
    gfx.pointsize_set = gfx.pointsize
  end
end


gfx.start_path = function(x, y)
  gfx.check_color()
  gfx.check_linetype()
  gfx.check_linewidth()

  --  init path with first coords
  gfx.path = {{x,y}}  
  gfx.in_path = true
  gfx.posx = x
  gfx.posy = y
end

-- ctype  string  LT|RGB|GRAY
-- val   table   {name}|{r,g,b}
gfx.format_color = function(ctype, val)
  local c
  if ctype == 'LT' then
    if val[1] < 0 then
      if val[1] < -2 then --  LT_NODRAW, LT_BACKGROUND, LT_UNDEFINED
        c = 'gpbgfillcolor'
      else
        c = pgf.styles.lt_colors_axes[math.abs(val[1])][1]
      end
    else
      c = pgf.styles.lt_colors[(val[1] % #pgf.styles.lt_colors)+1][1]
    end
    -- c = pgf.styles.lt_colors[((val[1]+3) % #pgf.styles.lt_colors) + 1][1]
  elseif ctype == 'RGB' then
    c = string.format("\\gprgb{%i}{%i}{%i}",
                  1000*val[1]+0.5, 1000*val[2]+0.5, 1000*val[3]+0.5)
  elseif ctype == 'GRAY' then
    c = string.format("black!%i", 100*val[1]+0.5)
  end
  return c
end

gfx.set_color = function(ctype, val)
  gfx.color = gfx.format_color(ctype, val)
end


--[[===============================================================================================

  The terminal layer
  
  The term.* functions are usually called from the gnuplot Lua terminal

]]--===============================================================================================


if arg then
  -- when called from the command line we have
  -- to initialize the table `term' manually
  -- to avoid errors
  term = {}
else
  --
  -- gnuplot terminal default parameters and flags
  --
  term.xmax = pgf.DEFAULT_RESOLUTION * pgf.DEFAULT_CANVAS_SIZE_X
  term.ymax = pgf.DEFAULT_RESOLUTION * pgf.DEFAULT_CANVAS_SIZE_Y
  term.h_tic =  pgf.DEFAULT_RESOLUTION * pgf.DEFAULT_TIC_SIZE
  term.v_tic =  pgf.DEFAULT_RESOLUTION * pgf.DEFAULT_TIC_SIZE
  -- default size for CM@10pt
  term.h_char = 184 * math.floor((pgf.DEFAULT_FONT_SIZE/10) * (pgf.DEFAULT_RESOLUTION/1000) + .5)
  term.v_char = 308 * math.floor((pgf.DEFAULT_FONT_SIZE/10) * (pgf.DEFAULT_RESOLUTION/1000) + .5)
  term.description = "Lua PGF/TikZ terminal for LaTeX2e"
  term.flags = term.TERM_BINARY + term.TERM_CAN_CLIP
                + term.TERM_IS_POSTSCRIPT + term.TERM_CAN_MULTIPLOT
  if term.IS_GNUPLOT_43 then  -- gnuplot 4.3
    term.flags = term.flags + term.TERM_CAN_DASH
  end
end


--
-- initial = 1  for the initial "set term" call
--           0  for subsequent option changes -- currently unused, since the changeable options
--              are hardcoded within gnuplot :-(
--
-- t_count   see e.g. int_error()
--
term.options = function(opt_str, initial, t_count)

  local o_next = ""
  local o_type = nil
  local s_start, s_end = 1, 1

  -- trim spaces
  opt_str = opt_str:gsub("^%s*(.-)%s*$", "%1")
  local opt_len = string.len(opt_str)

  t_count = t_count - 1

  local set_t_count = function(num) 
    -- gnuplot handles commas as regular tokens
    t_count = t_count + 2*num - 2
  end

  local almost_equals = function(param, opt)
    local op1, op2

    local st, _ = string.find(opt, "$", 2, true)
    if st then
      op1 = string.sub(opt, 1, st-1)
      op2 = string.sub(opt, st+1)
      if (string.sub(param, 1, st-1) == op1)
          and (string.find(op1..op2, param, 1, true) == 1) then
        return true
      end
    elseif opt == param then
      return true
    end
    return false
  end

  --
  -- simple parser for options and strings
  --
  local get_next_token = function()

    -- beyond the limit?
    if s_start > opt_len then
      o_next = ""
      o_type = nil
      return
    end

    t_count = t_count + 1

    -- search the start of the next token
    s_start, _ = string.find (opt_str, '[^%s]', s_start)
    if not s_start then
      o_next = ""
      o_type = nil
      return
    end

    -- a new string argument?
    local next_char = string.sub(opt_str, s_start, s_start)
    if next_char == '"' or next_char == "'" then
      -- find the end of the string by searching for
      -- the next not escaped quote
      _ , s_end = string.find (opt_str, '[^\\]'..next_char, s_start+1)
      if s_end then
        o_next = string.sub(opt_str, s_start+1, s_end-1)
        if next_char == '"' then
          -- Wow! this is to resolve all string escapes, kind of "unescape string"
          o_next = assert(loadstring("return(\""..o_next.."\")"))()
        end
        o_type = "string"
      else
        -- FIXME: error: string does not end...
        -- seems that gnuplot adds missing quotes
        -- so this will never happen...
      end
    else
      -- ok, it's not a string...
      -- then find the next white space or end of line
      -- comma separated strings are regarded as one token
      s_end, _ = string.find (opt_str, '[^,][%s]+[^,]', s_start)
      if not s_end then -- reached the end of the string
        s_end = opt_len + 1
      else
        s_end = s_end + 1
      end
      o_next = string.sub(opt_str, s_start, s_end-1)
      o_type = "op"
    end
    s_start = s_end + 1
    return
  end    

  local get_two_sizes = function(str)
    local args = explode(',', str)
    set_t_count(#args)

    local num1, num2, unit
    if #args ~= 2 then
      return false, nil
    else
      num1, unit = gfx.parse_number_unit(args[1])
      if unit then
        t_count = t_count + 1
      end
      num2, unit = gfx.parse_number_unit(args[2])
      if unit then
        t_count = t_count + 1
      end
      if not (num1 and num2) then
        return false, nil
      end
    end
    return num1, num2
  end
  
  local print_help = false

  while true do
    get_next_token()
    if not o_type then break end
    if almost_equals(o_next, "he$lp") then
      print_help = true
    elseif almost_equals(o_next, "mono$chrome") then
      -- no colored lines
      gfx.opt.lines_colored = false
    elseif almost_equals(o_next, "c$olor") or almost_equals(o_next, "c$olour") then
      -- colored lines
      gfx.opt.lines_colored = true
    elseif almost_equals(o_next, "so$lid") then
      -- no dashed and dotted etc. lines
      gfx.opt.lines_dashed = false
    elseif almost_equals(o_next, "da$shed") then
      -- dashed and dotted etc. lines
      gfx.opt.lines_dashed = true
    elseif almost_equals(o_next, "gparr$ows") then
      -- use gnuplot arrows instead of TikZ
      gfx.opt.gp_arrows = true
    elseif almost_equals(o_next, "gppoint$s") then
      -- use gnuplot points instead of TikZ
      gfx.opt.gp_points = true
    elseif almost_equals(o_next, "nopic$environment") then
      -- omit the 'tikzpicture' environment
      gfx.opt.nopicenv = true
    elseif almost_equals(o_next, "origin$reset") then
      -- moves the origin of the TikZ picture to the lower left corner of the plot
      gfx.opt.set_origin = true
    elseif almost_equals(o_next, "plot$size") then
      get_next_token()
      gfx.opt.plotsize_x, gfx.opt.plotsize_y = get_two_sizes(o_next)
      if not gfx.opt.plotsize_x then
        gp.int_error(t_count, string.format("error: two comma seperated lengths expected, got `%s'.", o_next))
      end
      gfx.opt.set_plotsize = true
      -- we set the canvas size to the plotsize to keep the aspect ratio as good as possible
      -- and rescale later once we know the actual plotsize...
      term.xmax = gfx.opt.plotsize_x*pgf.DEFAULT_RESOLUTION
      term.ymax = gfx.opt.plotsize_y*pgf.DEFAULT_RESOLUTION
    elseif almost_equals(o_next, "si$ze") then
      get_next_token()
      local plotsize_x, plotsize_y = get_two_sizes(o_next)
      if not plotsize_x then
        gp.int_error(t_count, string.format("error: two comma seperated lengths expected, got `%s'.", o_next))
      end
      term.xmax = plotsize_x*pgf.DEFAULT_RESOLUTION
      term.ymax = plotsize_y*pgf.DEFAULT_RESOLUTION
    elseif almost_equals(o_next, "char$size") then
      get_next_token()
      local charsize_h, charsize_v = get_two_sizes(o_next)
      if not charsize_h then
        gp.int_error(t_count, string.format("error: two comma seperated lengths expected, got `%s'.", o_next))
      end
      term.h_char = math.floor(charsize_h*pgf.DEFAULT_RESOLUTION + .5)
      term.v_char = math.floor(charsize_v*pgf.DEFAULT_RESOLUTION + .5)
    elseif almost_equals(o_next, "sc$ale") then
      get_next_token()
      local xscale, yscale = get_two_sizes(o_next)
      if not xscale then
        gp.int_error(t_count, string.format("error: two comma seperated numbers expected, got `%s'.", o_next))
      end
      term.xmax = term.xmax * xscale
      term.ymax = term.ymax * yscale
    elseif almost_equals(o_next, "tikzpl$ot") then
      get_next_token()
      local args = explode(',', o_next)
      set_t_count(#args)
      for i = 1,#args do
        args[i] = tonumber(args[i])
        if args[i] == nil then
          gp.int_error(t_count, string.format("error: list of comma seperated numbers expected, got `%s'.", o_next))
        end
        args[i] = args[i] - 1
      end
      gfx.opt.plot_list = args
    elseif almost_equals(o_next, "provide$vars") then
      get_next_token()
      local args = explode(',', o_next)
      set_t_count(#args)
      gfx.opt.gnuplot_vars = args
    elseif almost_equals(o_next, "tikzar$rows") then
      -- map the arrow angles to TikZ arrow styles
      gfx.opt.tikzarrows = true
    elseif almost_equals(o_next, "nobit$map") then
      -- render images as filled rectangles instead of the nativ
      -- PS or PDF image format
      gfx.opt.direct_image = false
    elseif almost_equals(o_next, "cmyk$image") then
      -- use cmyk color model for images
      gfx.opt.cmykimage = true
    elseif almost_equals(o_next, "full$doc") or almost_equals(o_next, "stand$alone") then
      -- produce full tex document
      gfx.opt.full_doc = true
    elseif almost_equals(o_next, "create$style") then
      -- creates the coresponding LaTeX style from the script
      local f = io.open(pgf.LATEX_STYLE_FILE..".sty" , "w+")
      pgf.create_style(f)
    elseif almost_equals(o_next, "fo$nt") then
      local fsize
      get_next_token()
      if o_type == 'string' then
        gfx.opt.default_font, fsize = gfx.parse_font_string(o_next)
      else
        gp.int_error(t_count, string.format("error: string expected, got `%s'.", o_next))
      end
      if fsize then
        term.h_char = math.floor(term.h_char * (fsize/pgf.DEFAULT_FONT_SIZE) + .5)
        term.v_char = math.floor(term.v_char * (fsize/pgf.DEFAULT_FONT_SIZE) + .5)
      end
    elseif almost_equals(o_next, "pre$amble") or almost_equals(o_next, "header") then
      get_next_token()
      if o_type == 'string' then
        gfx.opt.latex_preamble = gfx.opt.latex_preamble .. o_next .. "\n"
      else
        gp.int_error(t_count, string.format("error: string expected, got `%s'.", o_next))
      end
    else
      gp.int_warn(t_count, string.format("unknown option `%s'.", o_next))
    end
  end

  if print_help then
    pgf.print_help(gp.term_out)
  end

  local tf = function(b,y,n)
    if b then 
      return(y)
    else
      return(n)
    end
  end

  local opt_str = string.format("%s %s",
    tf(gfx.opt.lines_colored, 'color', 'monochrome'),
    tf(gfx.opt.lines_dashed, 'dashed', 'solid'))

  gp.term_options(opt_str)

  return 1
end

-- Called once, when the device is first selected.
term.init = function()
  if gfx.opt.full_doc then
    pgf.write_doc_begin(gfx.opt.latex_preamble)
  end
  return 1
end

-- Called just before a plot is going to be displayed.
term.graphics = function()
  -- reset some state variables
  gfx.linetype_idx_set = nil
  gfx.linewidth_set = nil
  gfx.pointsize_set = nil
  gfx.color_set = nil
  gfx.in_picture = true
  gfx.have_plotbox = false
  gfx.boundingbox_cnt = 0
  gfx.scalex = 1/pgf.DEFAULT_RESOLUTION
  gfx.scaley = 1/pgf.DEFAULT_RESOLUTION
  gfx.current_boundingbox = {
    xleft = nil, xright = nil, ytop = nil, ybot = nil
  }

    -- put a newline between subsequent plots in fulldoc mode...
  if gfx.opt.full_doc then
    gp.write("\n")
  end
  pgf.write_graph_begin(gfx.opt.default_font, gfx.opt.nopicenv)
  return 1
end


term.vector = function(x, y)
  if not gfx.in_path then
    gfx.start_path(gfx.posx, gfx.posy)
  elseif not gfx.check_coord(x, y) then
    -- checked for zero path length and add the path coords to gfx.path
    gfx.path[#gfx.path+1] = {x,y}
  end
  return 1
end

term.move = function(x, y)
  -- if we move to our last position we will just continue the path there
  if not gfx.check_coord(x, y) then
    gfx.check_in_path()
    gfx.start_path(x, y)
  end
  return 1
end

term.linetype = function(ltype)
  gfx.check_in_path()

  gfx.set_color('LT', {ltype})
  gfx.linetype_idx = ltype

  return 1
end

term.point = function(x, y, num)
  if gfx.opt.gp_points then
    return 0
  else
    gfx.check_in_path()
    gfx.check_color()
    gfx.check_linewidth()
    gfx.check_pointsize()
  
    local pm
    if num == -1 then
      pm = pgf.styles.plotmarks[1][1]
    else
      pm = pgf.styles.plotmarks[(num % (#pgf.styles.plotmarks-1)) + 2][1]
    end
    pgf.draw_points({{x,y}}, pm)
    
    return 1
  end
end


--[[
  this differs from the original API
  one may use the additional parameters to define own styles
  e.g. "misuse" angle for numbering predefined styles...

  int length        /* head length */
  double angle      /* head angle in degrees */
  double backangle  /* head back angle in degrees */
  int filled        /* arrow head filled or not */
]]
term.arrow = function(sx, sy, ex, ey, head, length, angle, backangle, filled)
  if gfx.opt.gp_arrows then
    return 0
  else
    local headstyle = 0
    if gfx.opt.tikzarrows then
      headstyle = angle
    end
    gfx.check_in_path()
    gfx.check_color()
    gfx.check_linetype()
    gfx.check_linewidth()
    pgf.draw_arrow({{sx,sy},{ex,ey}}, gfx.HEAD_STR[head+1], headstyle)
    return 1
  end
end

-- Called immediately after a plot is displayed.
term.text = function()
  gfx.check_in_path()
  pgf.write_graph_end(gfx.opt.nopicenv)
  gfx.in_picture = false
  return 1
end

term.put_text = function(x, y, txt)
  gfx.check_in_path()
  gfx.check_color()
  
  if (txt ~= '') or (gfx.text_font ~= '')  then -- omit empty nodes
    pgf.write_text_node({x, y}, txt, gfx.text_angle, gfx.TEXT_ANCHOR[gfx.text_justify], gfx.text_font)
  end
  return 1
end

term.justify_text = function(justify)
  gfx.text_justify = justify
  return 1
end

term.text_angle = function(ang)
  gfx.text_angle = ang
  return 1
end

term.linewidth = function(width)
  if gfx.linewidth ~= width then
    gfx.linewidth = width
    gfx.check_in_path()
  end
  return 1
end

term.pointsize = function(size)
  if gfx.pointsize ~= size then
    gfx.pointsize = size
    gfx.check_in_path()
  end
  return 1
end

term.set_font = function(font)
  gfx.text_font = gfx.parse_font_string(font)
  return 1
end

-- at the moment this is only used to check
-- the plot's bounding box as seldom as possible
term.layer = function(l)
  if l == 'end_text' then
    -- called after a plot is finished (also after each "mutiplot")
    gfx.write_boundingbox()
  end
  return 1
end

-- we don't use this, because we are implicitly testing
-- for closed paths
term.path = function(p)
  return 1
end


term.filled_polygon = function(style, fillpar, t)
  local pattern = nil
  local color = nil
  local opacity = 100
  local saturation = 100
  
  gfx.check_in_path()

  if style == 'EMPTY' then
      -- FIXME: should be the "background color" and not gpbgfillcolor
      pattern = ''
      color = 'gpbgfillcolor'
      saturation = 100
      opacity = 100
  elseif style == 'DEFAULT' or style == 'OPAQUE' then -- FIXME: not shure about the opaque style
      pattern = ''
      color = gfx.color
      saturation = 100
      opacity = 100
  elseif style == 'SOLID' then
      pattern = ''
      color = gfx.color
      if fillpar < 100 then
        saturation = fillpar
      else
        saturation = 100
      end
      opacity = 100
  elseif style == 'PATTERN' then
      pattern = pgf.styles.patterns[(fillpar % #pgf.styles.patterns) + 1][1]
      color = gfx.color
      saturation = 100
      opacity = 100
  elseif style == 'TRANSPARENT_SOLID' then
      pattern = ''
      color = gfx.color
      saturation = 100
      opacity = fillpar
  elseif style == 'TRANSPARENT_PATTERN' then
      pattern = pgf.styles.patterns[(fillpar % #pgf.styles.patterns) + 1][1]
      color = gfx.color
      saturation = 100
      opacity = 0
  end
  
  pgf.draw_fill(t, pattern, color, saturation, opacity)  
  
  return 1
end


term.boxfill = function(style, fillpar, x1, y1, width, height)
  local t = {{x1, y1}, {x1+width, y1}, {x1+width, y1+height}, {x1, y1+height}}
  return term.filled_polygon(style, fillpar, t)
end

-- points[row][column]
-- m: #cols, n: #rows
-- corners: clip box and draw box coordinates
-- ctype: "RGB" or "GRAY" (unused since we allways use RGB to keep things simple)
term.image = function(m, n, points, corners, ctype)
  gfx.check_in_path()
  
  pgf.write_clipbox_begin({corners[3][1],corners[3][2]},{corners[4][1],corners[4][2]})
  
  if gfx.opt.direct_image then
    local ll = {corners[1][1],corners[2][2]}
    local ur = {corners[2][1],corners[1][2]}
    if gfx.opt.cmykimage then
      pgf.draw_raw_cmyk_image(points, m, n, ll, ur)
    else
      pgf.draw_raw_rgb_image(points, m, n, ll, ur)
    end
  else
    local w = (corners[2][1] - corners[1][1])/m
    local h = (corners[1][2] - corners[2][2])/n

    local yy,yyy,xx,xxx
    for cnt = 1,#points do
      xx = corners[1][1]+(cnt%m-1)*w
      yy = corners[1][2]-math.floor(cnt/m)*h
      yyy = yy-h
      xxx = xx+w
      pgf.draw_fill({{xx, yy}, {xxx, yy}, {xxx, yyy}, {xx, yyy}}, '', gfx.format_color('RGB', points[cnt]) , 100, 100)
    end
  end
  pgf.write_clipbox_end()
end

term.make_palette = function()
  -- continuous number of colours
  return 0
end

term.previous_palette = function()
  return 1
end

term.set_color = function(ctype, lt, value, r, g, b)
  gfx.check_in_path()
  -- FIXME gryscale on monochrome?? ... or use xcolor?

  if ctype == 'LT' then
    gfx.set_color('LT', {lt})
  elseif ctype == 'FRAC' then
    if gfx.opt.lines_colored then
      gfx.set_color('RGB', {r, g , b})
    else
      gfx.set_color('GRAY', {value})
    end
  elseif ctype == 'RGB' then
    gfx.set_color('RGB', {r, g , b})
  else
    gp.int_error(string.format("set color: unknown type (%s), lt (%i), value (%.3f)\n", ctype, lt, value))
  end
  
  return 1
end

-- Called when gnuplot is exited.
term.reset = function(p)
  gfx.check_in_path()
  gfx.check_variables()
  if gfx.opt.full_doc then
    pgf.write_doc_end()
  end
  return 1
end

--[[===============================================================================================

  command line code

]]--===============================================================================================

term_help = function(helptext)
  local w
  for w in string.gmatch(helptext, "([^\n]*)\n") do
    w = string.gsub(w, "\\", "\\\\")
    w = string.gsub(w, "\"", "\\\"")
    io.write('"'..w.."\",\n")
  end
--[[  
  local out = string.gsub(helptext, "\n", "\",\n\"")
  local out = string.gsub(helptext, "\n", "\",\n\"")
  io.write(out)]]
end

if arg then -- called from the command line!
  if #arg > 0 and arg[1] == 'style' then
    -- write style file
    local f = io.open(pgf.LATEX_STYLE_FILE..".sty" , "w+")
    pgf.create_style(f)
  elseif arg[1] == 'termhelp' then
    io.write([["2 tikz",
"?set terminal lua tikz",
"?set term lua tikz",
"?term lua tikz",
"?tikz",
" The TikZ driver is an output driver for the generic Lua terminal.",
" Please read the Lua terminal section for additional information.",
"",
" Syntax:",
"     set terminal lua tikz",
"",
]])
    pgf.print_help(term_help)
    io.write("\"\"\n")
  else
    io.write([[
 This script is intended to be called from GNUPLOT.

 For generating the associated LaTeX style file
  (']] .. pgf.LATEX_STYLE_FILE..".sty')" .. [[ just call this script
 with the additional option 'style':

   # lua gnuplot.lua style

 The TikZ driver provides the following additional terminal options:

]])
    pgf.print_help(io.write)
  end
end
