/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty."
 *
 */

#define		DEFAULT		      (-1)
#define		SOLID_LINE		0
#define		DASH_LINE		1
#define		DOTTED_LINE		2
#define		RUBBER_LINE		3
/* #define		PANEL_LINE		4  ** not needed for gnuplot */

#define		Color			long

#define		BLACK			0
#define		WHITE			7

typedef struct f_pattern {
    int		    w, h;
    int		   *p;
}
		F_pattern;

typedef struct f_point {
    int		    x, y;
    struct f_point *next;
}
		F_point;

typedef struct f_pos {
    int		    x, y;
}
		F_pos;

typedef struct f_arrow {
    int		    type;
    int		    style;
    float	    thickness;
    float	    wid;
    float	    ht;
}
		F_arrow;

typedef struct f_ellipse {
    int		    tagged;
    int		    type;
#define					T_ELLIPSE_BY_RAD	1
#define					T_ELLIPSE_BY_DIA	2
#define					T_CIRCLE_BY_RAD		3
#define					T_CIRCLE_BY_DIA		4
    int		    style;
    int		    thickness;
    Color	    color;
    int		    depth;
    int		    direction;
    float	    style_val;
    float	    angle;
    int		    pen;
    int		    fill_style;
#define					UNFILLED	0
#define					WHITE_FILL	1
#define					BLACK_FILL	21
    struct f_pos    center;
    struct f_pos    radiuses;
    struct f_pos    start;
    struct f_pos    end;
    struct f_ellipse *next;
}
		F_ellipse;

typedef struct f_arc {
    int		    tagged;
    int		    type;
#define					T_3_POINTS_ARC		1
    int		    style;
    int		    thickness;
    Color	    color;
    int		    depth;
    int		    pen;
    int		    fill_style;
    float	    style_val;
    int		    direction;
    struct f_arrow *for_arrow;
    struct f_arrow *back_arrow;
    struct {
	float		x, y;
    }		    center;
    struct f_pos    point[3];
    struct f_arc   *next;
}
		F_arc;

#define		CLOSED_PATH		0
#define		OPEN_PATH		1
#define		DEF_BOXRADIUS		7
#define		DEF_DASHLENGTH		4
#define		DEF_DOTGAP		3

typedef struct f_line {
    int		    tagged;
    int		    type;
#define					T_POLYLINE	1
#define					T_BOX		2
#define					T_POLYGON	3
#define					T_ARC_BOX	4
#define					T_EPS_BOX	5
    int		    style;
    int		    thickness;
    Color	    color;
    int		    depth;
    float	    style_val;
    int		    pen;
    int		    fill_style;
    int		    radius;	/* corner radius for T_ARC_BOX */
    struct f_arrow *for_arrow;
    struct f_arrow *back_arrow;
    struct f_point *points;
    struct f_eps   *eps;
    struct f_line  *next;
}
		F_line;

typedef struct f_text {
    int		    tagged;
    int		    type;
#define					T_LEFT_JUSTIFIED	0
#define					T_CENTER_JUSTIFIED	1
#define					T_RIGHT_JUSTIFIED	2
    int		    font;
    int		    size;	/* point size */
    Color	    color;
    int		    depth;
    float	    angle;	/* in radian */

    int		    flags;
#define					RIGID_TEXT		1
#define					SPECIAL_TEXT		2
#define					PSFONT_TEXT		4
#define					HIDDEN_TEXT		8

    int		    height;	/* pixels */
    int		    length;	/* pixels */
    int		    base_x;
    int		    base_y;
    int		    pen;
    char	   *cstring;
    struct f_text  *next;
}
		F_text;

#define MAXFONT(T) (psfont_text(T) ? NUM_PS_FONTS : NUM_LATEX_FONTS)

#define		rigid_text(t) \
			(t->flags == DEFAULT \
				|| (t->flags & RIGID_TEXT))

#define		special_text(t) \
			((t->flags != DEFAULT \
				&& (t->flags & SPECIAL_TEXT)))

#define		psfont_text(t) \
			(t->flags != DEFAULT \
				&& (t->flags & PSFONT_TEXT))

#define		hidden_text(t) \
			(t->flags != DEFAULT \
				&& (t->flags & HIDDEN_TEXT))

#define		text_length(t) \
			(hidden_text(t) ? hidden_text_length : t->length)

#define		using_ps	(cur_textflags & PSFONT_TEXT)

typedef struct f_control {
    float	    lx, ly, rx, ry;
    struct f_control *next;
}
		F_control;

#define		int_spline(s)		(s->type & 0x2)
#define		normal_spline(s)	(!(s->type & 0x2))
#define		closed_spline(s)	(s->type & 0x1)
#define		open_spline(s)		(!(s->type & 0x1))

typedef struct f_spline {
    int		    tagged;
    int		    type;
#define					T_OPEN_NORMAL	0
#define					T_CLOSED_NORMAL 1
#define					T_OPEN_INTERP	2
#define					T_CLOSED_INTERP 3
    int		    style;
    int		    thickness;
    Color	    color;
    int		    depth;
    float	    style_val;
    int		    pen;
    int		    fill_style;
    struct f_arrow *for_arrow;
    struct f_arrow *back_arrow;
    /*
     * For T_OPEN_NORMAL and T_CLOSED_NORMAL points are control points while
     * they are knots for T_OPEN_INTERP and T_CLOSED_INTERP whose control
     * points are stored in controls.
     */
    struct f_point *points;
    struct f_control *controls;
    struct f_spline *next;
}
		F_spline;

typedef struct f_compound {
    int		    tagged;
    struct f_pos    nwcorner;
    struct f_pos    secorner;
    struct f_line  *lines;
    struct f_ellipse *ellipses;
    struct f_spline *splines;
    struct f_text  *texts;
    struct f_arc   *arcs;
    struct f_compound *compounds;
    struct f_compound *next;
}
		F_compound;

typedef struct f_linkinfo {
    struct f_line  *line;
    struct f_point *endpt;
    struct f_point *prevpt;
    int		    two_pts;
    struct f_linkinfo *next;
}
		F_linkinfo;

#define		ARROW_SIZE		sizeof(struct f_arrow)
#define		POINT_SIZE		sizeof(struct f_point)
#define		CONTROL_SIZE		sizeof(struct f_control)
#define		ELLOBJ_SIZE		sizeof(struct f_ellipse)
#define		ARCOBJ_SIZE		sizeof(struct f_arc)
#define		LINOBJ_SIZE		sizeof(struct f_line)
#define		TEXOBJ_SIZE		sizeof(struct f_text)
#define		SPLOBJ_SIZE		sizeof(struct f_spline)
#define		COMOBJ_SIZE		sizeof(struct f_compound)
#define		EPS_SIZE		sizeof(struct f_eps)
#define		LINKINFO_SIZE		sizeof(struct f_linkinfo)

/**********************	 object codes  **********************/

#define		O_ELLIPSE		1
#define		O_POLYLINE		2
#define		O_SPLINE		3
#define		O_TEXT			4
#define		O_ARC			5
#define		O_COMPOUND		6
#define		O_END_COMPOUND		-O_COMPOUND
#define		O_ALL_OBJECT		99

/*********************	object masks  ************************/

#define M_NONE			0x000
#define M_POLYLINE_POLYGON	0x001
#define M_POLYLINE_LINE		0x002
#define M_POLYLINE_BOX		0x004	/* includes ARCBOX */
#define M_SPLINE_O_NORMAL	0x008
#define M_SPLINE_C_NORMAL	0x010
#define M_SPLINE_O_INTERP	0x020
#define M_SPLINE_C_INTERP	0x040
#define M_TEXT_NORMAL		0x080
#define M_TEXT_HIDDEN		0x100
#define M_ARC			0x200
#define M_ELLIPSE		0x400
#define M_COMPOUND		0x800

#define M_TEXT		(M_TEXT_HIDDEN | M_TEXT_NORMAL)
#define M_SPLINE_O	(M_SPLINE_O_NORMAL | M_SPLINE_O_INTERP)
#define M_SPLINE_C	(M_SPLINE_C_NORMAL | M_SPLINE_C_INTERP)
#define M_SPLINE_NORMAL (M_SPLINE_O_NORMAL | M_SPLINE_C_NORMAL)
#define M_SPLINE_INTERP (M_SPLINE_O_INTERP | M_SPLINE_C_INTERP)
#define M_SPLINE	(M_SPLINE_NORMAL | M_SPLINE_INTERP)
#define M_POLYLINE	(M_POLYLINE_LINE | M_POLYLINE_POLYGON | M_POLYLINE_BOX)
#define M_VARPTS_OBJECT (M_POLYLINE_LINE | M_POLYLINE_POLYGON | M_SPLINE)
#define M_OPEN_OBJECT	(M_POLYLINE_LINE | M_SPLINE_O | M_ARC)
#define M_ROTATE_ANGLE	(M_VARPTS_OBJECT | M_ARC | M_TEXT | M_COMPOUND)
#define M_OBJECT	(M_ELLIPSE | M_POLYLINE | M_SPLINE | M_TEXT | M_ARC)
#define M_NO_TEXT	(M_ELLIPSE | M_POLYLINE | M_SPLINE | M_COMPOUND | M_ARC)
#define M_ALL		(M_OBJECT | M_COMPOUND)
