/*
 * $Id: object.h,v 3.26 92/03/24 22:35:34 woo Exp Locker: woo $
 */

/* 
 *	FIG : Facility for Interactive Generation of figures
 *
 *	(c) copy right 1985 by Supoj Sutanthavibul (supoj@sally.utexas.edu)
 *      January 1985.
 *	1st revision : Aug 1985.
 *	2nd revision : Feb 1988.
 *
 *	%W%	%G%
*/
#define					DEFAULT			-1

typedef		struct f_pattern {
			int			w, h;
			int			*p;
			}
		F_pattern;

typedef		struct f_pen {
			int			x, y;
			int			*p;
			}
		F_pen;

typedef		struct f_point {
			int			x, y;
			struct f_point		*next;
			}
		F_point;

typedef		struct f_pos {
			int			x, y;
			}
		F_pos;

typedef		struct f_arrow {
			int			type;
			int			style;
			float			thickness;
			float			wid;
			float			ht;
			}
		F_arrow;

typedef		struct f_ellipse {
			int			type;
#define					T_ELLIPSE_BY_RAD	1
#define					T_ELLIPSE_BY_DIA	2
#define					T_CIRCLE_BY_RAD		3
#define					T_CIRCLE_BY_DIA		4
			int			style;
			int			thickness;
			int			color;
#define					BLACK			0
			int			depth;
			int			direction;
			float			style_val;
			float			angle;
			struct f_pen		*pen;
			struct f_pattern	*area_fill;
#define		       			UNFILLED	(F_pattern *)0
#define		       			BLACK_FILL	(F_pattern *)1
#define		       			DARK_GRAY_FILL	(F_pattern *)2
#define		       			MED_GRAY_FILL	(F_pattern *)3
#define		       			LIGHT_GRAY_FILL	(F_pattern *)4
#define		       			WHITE_FILL	(F_pattern *)4
			struct f_pos		center;
			struct f_pos		radiuses;
			struct f_pos		start;
			struct f_pos		end;
			struct f_ellipse	*next;
			}
		F_ellipse;

typedef		struct f_arc {
			int			type;
#define					T_3_POINTS_ARC		1
			int			style;
			int			thickness;
			int			color;
			int			depth;
			struct f_pen		*pen;
			struct f_pattern	*area_fill;
			float			style_val;
			int			direction;
			struct f_arrow		*for_arrow;
			struct f_arrow		*back_arrow;
			struct {float x, y;}	center;
			struct f_pos		point[3];
			struct f_arc		*next;
			}
		F_arc;

typedef		struct f_line {
			int			type;
#define					T_POLYLINE	1
#define					T_BOX		2
#define					T_POLYGON	3
			int			style;
			int			thickness;
			int			color;
			int			depth;
			float			style_val;
			struct f_pen		*pen;
			struct f_pattern	*area_fill;
			struct f_arrow		*for_arrow;
			struct f_arrow		*back_arrow;
			struct f_point		*points;
			struct f_line		*next;
			}
		F_line;

typedef		struct f_text {
			int			type;
#define					T_LEFT_JUSTIFIED	0
#define					T_CENTER_JUSTIFIED	1
#define					T_RIGHT_JUSTIFIED	2
			int			font;
#define					DEFAULT_FONT		0
#define					ROMAN_FONT		1
#define					BOLD_FONT		2
#define					ITALIC_FONT		3
#define					MODERN_FONT		4
#define					TYPEWRITER_FONT		5
			int			size;	/* point size */
			int			color;
			int			depth;
			float			angle;	/* in radian */
			int			style;
#define					PLAIN		1
#define					ITALIC		2
#define					BOLD		4
#define					OUTLINE		8
#define					SHADOW		16
			int			height;	/* pixels */
			int			length;	/* pixels */
			int			base_x;
			int			base_y;
			struct f_pen		*pen;
			char			*cstring;
			struct f_text		*next;
			}
		F_text;

typedef		struct f_control {
			float			lx, ly, rx, ry;
			struct f_control	*next;
			}
		F_control;

#define		int_spline(s)		(s->type & 0x2)
#define		normal_spline(s)	(!(s->type & 0x2))
#define		closed_spline(s)	(s->type & 0x1)
#define		open_spline(s)		(!(s->type & 0x1))

typedef		struct f_spline {
			int			type;
#define					T_OPEN_NORMAL		0
#define					T_CLOSED_NORMAL		1
#define					T_OPEN_INTERPOLATED	2
#define					T_CLOSED_INTERPOLATED	3
			int			style;
			int			thickness;
			int			color;
			int			depth;
			float			style_val;
			struct f_pen		*pen;
			struct f_pattern	*area_fill;
			struct f_arrow		*for_arrow;
			struct f_arrow		*back_arrow;
			/*
			For T_OPEN_NORMAL and T_CLOSED_NORMAL points
			are control points while they are knots for
			T_OPEN_INTERPOLATED and T_CLOSED_INTERPOLTED
			whose control points are stored in controls.
			*/
			struct f_point		*points;
			struct f_control	*controls;
			struct f_spline		*next;
			}
		F_spline;

typedef		struct f_compound {
			struct f_pos		nwcorner;
			struct f_pos		secorner;
			struct f_line		*lines;
			struct f_ellipse	*ellipses;
			struct f_spline		*splines;
			struct f_text		*texts;
			struct f_arc		*arcs;
			struct f_compound	*compounds;
			struct f_compound	*next;
			}
		F_compound;

#define		ARROW_SIZE		sizeof(struct f_arrow)
#define		POINT_SIZE		sizeof(struct f_point)
#define		CONTROL_SIZE		sizeof(struct f_control)
#define		ELLOBJ_SIZE		sizeof(struct f_ellipse)
#define		ARCOBJ_SIZE		sizeof(struct f_arc)
#define		LINOBJ_SIZE		sizeof(struct f_line)
#define		TEXOBJ_SIZE		sizeof(struct f_text)
#define		SPLOBJ_SIZE		sizeof(struct f_spline)
#define		COMOBJ_SIZE		sizeof(struct f_compound)

/**********************  object codes  **********************/

#define		O_ELLIPSE		1
#define		O_POLYLINE		2
#define		O_SPLINE		3
#define		O_TEXT			4
#define		O_ARC			5
#define		O_COMPOUND		6
#define		O_END_COMPOUND		-O_COMPOUND
#define		O_ALL_OBJECT		99

/************  object styles (except for f_text)  ************/

#define		SOLID_LINE		0
#define		DASH_LINE		1
#define		DOTTED_LINE		2

#define		CLOSED_PATH		0
#define		OPEN_PATH		1
