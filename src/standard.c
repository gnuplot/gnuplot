#ifndef lint
static char *RCSid() { return RCSid("$Id: standard.c,v 1.9.2.1 2000/10/22 14:02:04 joze Exp $"); }
#endif

/* GNUPLOT - standard.c */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

#include "standard.h"

#include "gp_time.h"
#include "internal.h"
#include "setshow.h"
#include "util.h"


static double jzero __PROTO((double x));
static double pzero __PROTO((double x));
static double qzero __PROTO((double x));
static double yzero __PROTO((double x));
static double rj0 __PROTO((double x));
static double ry0 __PROTO((double x));
static double jone __PROTO((double x));
static double pone __PROTO((double x));
static double qone __PROTO((double x));
static double yone __PROTO((double x));
static double rj1 __PROTO((double x));
static double ry1 __PROTO((double x));

/* The bessel function approximations here are from
 * "Computer Approximations"
 * by Hart, Cheney et al.
 * John Wiley & Sons, 1968
 */

/* There appears to be a mistake in Hart, Cheney et al. on page 149.
 * Where it list Qn(x)/x ~ P(z*z)/Q(z*z), z = 8/x, it should read
 *               Qn(x)/z ~ P(z*z)/Q(z*z), z = 8/x
 * In the functions below, Qn(x) is implementated using the later
 * equation.
 * These bessel functions are accurate to about 1e-13
 */

#if (defined (ATARI) || defined (MTOS)) && defined(__PUREC__)
/* Sorry. But PUREC bugs here.
 * These bessel functions are NOT accurate to about 1e-13
 */

#define PI_ON_FOUR	 0.785398163397448309615661
#define PI_ON_TWO	 1.570796326794896619231313
#define THREE_PI_ON_FOUR 2.356194490192344928846982
#define TWO_ON_PI	 0.636619772367581343075535

static double dzero = 0.0;

/* jzero for x in [0,8]
 * Index 5849, 19.22 digits precision
 */
static double pjzero[] = {
	 0.493378725179413356181681e+21,
	-0.117915762910761053603844e+21,
	 0.638205934107235656228943e+19,
	-0.136762035308817138686542e+18,
	 0.143435493914034611166432e+16,
	-0.808522203485379387119947e+13,
	 0.250715828553688194555516e+11,
	-0.405041237183313270636066e+8,
	 0.268578685698001498141585e+5
};

static double qjzero[] = {
	 0.493378725179413356211328e+21,
	 0.542891838409228516020019e+19,
	 0.302463561670946269862733e+17,
	 0.112775673967979850705603e+15,
	 0.312304311494121317257247e+12,
	 0.669998767298223967181403e+9,
	 0.111463609846298537818240e+7,
	 0.136306365232897060444281e+4,
	 0.1e+1
};

/* pzero for x in [8,inf]
 * Index 6548, 18.16 digits precision
 */
static double ppzero[] = {
	 0.227790901973046843022700e+5,
	 0.413453866395807657967802e+5,
	 0.211705233808649443219340e+5,
	 0.348064864432492703474453e+4,
	 0.153762019090083542957717e+3,
	 0.889615484242104552360748e+0
};

static double qpzero[] = {
	 0.227790901973046843176842e+5,
	 0.413704124955104166398920e+5,
	 0.212153505618801157304226e+5,
	 0.350287351382356082073561e+4,
	 0.157111598580808936490685e+3,
	 0.1e+1
};

/* qzero for x in [8,inf]
 * Index 6948, 18.33 digits precision
 */
static double pqzero[] = {
	-0.892266002008000940984692e+2,
	-0.185919536443429938002522e+3,
	-0.111834299204827376112621e+3,
	-0.223002616662141984716992e+2,
	-0.124410267458356384591379e+1,
	-0.8803330304868075181663e-2,
};

static double qqzero[] = {
	 0.571050241285120619052476e+4,
	 0.119511315434346136469526e+5,
	 0.726427801692110188369134e+4,
	 0.148872312322837565816135e+4,
	 0.905937695949931258588188e+2,
	 0.1e+1
};


/* yzero for x in [0,8]
 * Index 6245, 18.78 digits precision
 */
static double pyzero[] = {
	-0.275028667862910958370193e+20,
	 0.658747327571955492599940e+20,
	-0.524706558111276494129735e+19,
	 0.137562431639934407857134e+18,
	-0.164860581718572947312208e+16,
	 0.102552085968639428450917e+14,
	-0.343637122297904037817103e+11,
	 0.591521346568688965427383e+8,
	-0.413703549793314855412524e+5
};

static double qyzero[] = {
	 0.372645883898616588198998e+21,
	 0.419241704341083997390477e+19,
	 0.239288304349978185743936e+17,
	 0.916203803407518526248915e+14,
	 0.261306575504108124956848e+12,
	 0.579512264070072953738009e+9,
	 0.100170264128890626566665e+7,
	 0.128245277247899380417633e+4,
	 0.1e+1
};


/* jone for x in [0,8]
 * Index 6050, 20.98 digits precision
 */
static double pjone[] = {
	 0.581199354001606143928051e+21,
	-0.667210656892491629802094e+20,
	 0.231643358063400229793182e+19,
	-0.358881756991010605074364e+17,
	 0.290879526383477540973760e+15,
	-0.132298348033212645312547e+13,
	 0.341323418230170053909129e+10,
	-0.469575353064299585976716e+7,
	 0.270112271089232341485679e+4
};

static double qjone[] = {
	 0.116239870800321228785853e+22,
	 0.118577071219032099983711e+20,
	 0.609206139891752174610520e+17,
	 0.208166122130760735124018e+15,
	 0.524371026216764971540673e+12,
	 0.101386351435867398996705e+10,
	 0.150179359499858550592110e+7,
	 0.160693157348148780197092e+4,
	 0.1e+1
};


/* pone for x in [8,inf]
 * Index 6749, 18.11 digits precision
 */
static double ppone[] = {
	 0.352246649133679798341724e+5,
	 0.627588452471612812690057e+5,
	 0.313539631109159574238670e+5,
	 0.498548320605943384345005e+4,
	 0.211152918285396238210572e+3,
	 0.12571716929145341558495e+1
};

static double qpone[] = {
	 0.352246649133679798068390e+5,
	 0.626943469593560511888834e+5,
	 0.312404063819041039923016e+5,
	 0.493039649018108897938610e+4,
	 0.203077518913475932229357e+3,
	 0.1e+1
};

/* qone for x in [8,inf]
 * Index 7149, 18.28 digits precision
 */
static double pqone[] = {
	 0.351175191430355282253332e+3,
	 0.721039180490447503928086e+3,
	 0.425987301165444238988699e+3,
	 0.831898957673850827325226e+2,
	 0.45681716295512267064405e+1,
	 0.3532840052740123642735e-1
};

static double qqone[] = {
	 0.749173741718091277145195e+4,
	 0.154141773392650970499848e+5,
	 0.915223170151699227059047e+4,
	 0.181118670055235135067242e+4,
	 0.103818758546213372877664e+3,
	 0.1e+1
};


/* yone for x in [0,8]
 * Index 6444, 18.24 digits precision
 */
static double pyone[] = {
	-0.292382196153296254310105e+20,
	 0.774852068218683964508809e+19,
	-0.344104806308411444618546e+18,
	 0.591516076049007061849632e+16,
	-0.486331694256717507482813e+14,
	 0.204969667374566218261980e+12,
	-0.428947196885524880182182e+9,
	 0.355692400983052605669132e+6
};

static double qyone[] = {
	 0.149131151130292035017408e+21,
	 0.181866284170613498688507e+19,
	 0.113163938269888452690508e+17,
	 0.475517358888813771309277e+14,
	 0.150022169915670898716637e+12,
	 0.371666079862193028559693e+9,
	 0.726914730719888456980191e+6,
	 0.107269614377892552332213e+4,
	 0.1e+1
};

#else

#define PI_ON_FOUR       0.78539816339744830961566084581987572
#define PI_ON_TWO        1.57079632679489661923131269163975144
#define THREE_PI_ON_FOUR 2.35619449019234492884698253745962716
#define TWO_ON_PI        0.63661977236758134307553505349005744

static double dzero = 0.0;

/* jzero for x in [0,8]
 * Index 5849, 19.22 digits precision
 */
static double GPFAR pjzero[] = {
	 0.4933787251794133561816813446e+21,
	-0.11791576291076105360384408e+21,
	 0.6382059341072356562289432465e+19,
	-0.1367620353088171386865416609e+18,
	 0.1434354939140346111664316553e+16,
	-0.8085222034853793871199468171e+13,
	 0.2507158285536881945555156435e+11,
	-0.4050412371833132706360663322e+8,
	 0.2685786856980014981415848441e+5
};

static double GPFAR qjzero[] = {
	0.4933787251794133562113278438e+21,
	0.5428918384092285160200195092e+19,
	0.3024635616709462698627330784e+17,
	0.1127756739679798507056031594e+15,
	0.3123043114941213172572469442e+12,
	0.669998767298223967181402866e+9,
	0.1114636098462985378182402543e+7,
	0.1363063652328970604442810507e+4,
	0.1e+1
};

/* pzero for x in [8,inf]
 * Index 6548, 18.16 digits precision
 */
static double GPFAR ppzero[] = {
	0.2277909019730468430227002627e+5,
	0.4134538663958076579678016384e+5,
	0.2117052338086494432193395727e+5,
	0.348064864432492703474453111e+4,
	0.15376201909008354295771715e+3,
	0.889615484242104552360748e+0
};

static double GPFAR qpzero[] = {
	0.2277909019730468431768423768e+5,
	0.4137041249551041663989198384e+5,
	0.2121535056188011573042256764e+5,
	0.350287351382356082073561423e+4,
	0.15711159858080893649068482e+3,
	0.1e+1
};

/* qzero for x in [8,inf]
 * Index 6948, 18.33 digits precision
 */
static double GPFAR pqzero[] = {
	-0.8922660020080009409846916e+2,
	-0.18591953644342993800252169e+3,
	-0.11183429920482737611262123e+3,
	-0.2230026166621419847169915e+2,
	-0.124410267458356384591379e+1,
	-0.8803330304868075181663e-2,
};

static double GPFAR qqzero[] = {
	0.571050241285120619052476459e+4,
	0.1195113154343461364695265329e+5,
	0.726427801692110188369134506e+4,
	0.148872312322837565816134698e+4,
	0.9059376959499312585881878e+2,
	0.1e+1
};


/* yzero for x in [0,8]
 * Index 6245, 18.78 digits precision
 */
static double GPFAR pyzero[] = {
	-0.2750286678629109583701933175e+20,
	 0.6587473275719554925999402049e+20,
	-0.5247065581112764941297350814e+19,
	 0.1375624316399344078571335453e+18,
	-0.1648605817185729473122082537e+16,
	 0.1025520859686394284509167421e+14,
	-0.3436371222979040378171030138e+11,
	 0.5915213465686889654273830069e+8,
	-0.4137035497933148554125235152e+5
};

static double GPFAR qyzero[] = {
	0.3726458838986165881989980739e+21,
	0.4192417043410839973904769661e+19,
	0.2392883043499781857439356652e+17,
	0.9162038034075185262489147968e+14,
	0.2613065755041081249568482092e+12,
	0.5795122640700729537380087915e+9,
	0.1001702641288906265666651753e+7,
	0.1282452772478993804176329391e+4,
	0.1e+1
};


/* jone for x in [0,8]
 * Index 6050, 20.98 digits precision
 */
static double GPFAR pjone[] = {
	 0.581199354001606143928050809e+21,
	-0.6672106568924916298020941484e+20,
	 0.2316433580634002297931815435e+19,
	-0.3588817569910106050743641413e+17,
	 0.2908795263834775409737601689e+15,
	-0.1322983480332126453125473247e+13,
	 0.3413234182301700539091292655e+10,
	-0.4695753530642995859767162166e+7,
	 0.270112271089232341485679099e+4
};

static double GPFAR qjone[] = {
	0.11623987080032122878585294e+22,
	0.1185770712190320999837113348e+20,
	0.6092061398917521746105196863e+17,
	0.2081661221307607351240184229e+15,
	0.5243710262167649715406728642e+12,
	0.1013863514358673989967045588e+10,
	0.1501793594998585505921097578e+7,
	0.1606931573481487801970916749e+4,
	0.1e+1
};


/* pone for x in [8,inf]
 * Index 6749, 18.11 digits precision
 */
static double GPFAR ppone[] = {
	0.352246649133679798341724373e+5,
	0.62758845247161281269005675e+5,
	0.313539631109159574238669888e+5,
	0.49854832060594338434500455e+4,
	0.2111529182853962382105718e+3,
	0.12571716929145341558495e+1
};

static double GPFAR qpone[] = {
	0.352246649133679798068390431e+5,
	0.626943469593560511888833731e+5,
	0.312404063819041039923015703e+5,
	0.4930396490181088979386097e+4,
	0.2030775189134759322293574e+3,
	0.1e+1
};

/* qone for x in [8,inf]
 * Index 7149, 18.28 digits precision
 */
static double GPFAR pqone[] = {
	0.3511751914303552822533318e+3,
	0.7210391804904475039280863e+3,
	0.4259873011654442389886993e+3,
	0.831898957673850827325226e+2,
	0.45681716295512267064405e+1,
	0.3532840052740123642735e-1
};

static double GPFAR qqone[] = {
	0.74917374171809127714519505e+4,
	0.154141773392650970499848051e+5,
	0.91522317015169922705904727e+4,
	0.18111867005523513506724158e+4,
	0.1038187585462133728776636e+3,
	0.1e+1
};


/* yone for x in [0,8]
 * Index 6444, 18.24 digits precision
 */
static double GPFAR pyone[] = {
	-0.2923821961532962543101048748e+20,
	 0.7748520682186839645088094202e+19,
	-0.3441048063084114446185461344e+18,
	 0.5915160760490070618496315281e+16,
	-0.4863316942567175074828129117e+14,
	 0.2049696673745662182619800495e+12,
	-0.4289471968855248801821819588e+9,
	 0.3556924009830526056691325215e+6
};

static double GPFAR qyone[] = {
	0.1491311511302920350174081355e+21,
	0.1818662841706134986885065935e+19,
	0.113163938269888452690508283e+17,
	0.4755173588888137713092774006e+14,
	0.1500221699156708987166369115e+12,
	0.3716660798621930285596927703e+9,
	0.726914730719888456980191315e+6,
	0.10726961437789255233221267e+4,
	0.1e+1
};

#endif /* (ATARI || MTOS) && __PUREC__ */

void
f_real()
{
    struct value a;
    push(Gcomplex(&a, real(pop(&a)), 0.0));
}

void
f_imag()
{
    struct value a;
    push(Gcomplex(&a, imag(pop(&a)), 0.0));
}


/* ang2rad is 1 if we are in radians, or pi/180 if we are in degrees */

void
f_arg()
{
    struct value a;
    push(Gcomplex(&a, angle(pop(&a)) / ang2rad, 0.0));
}

void
f_conjg()
{
    struct value a;
    (void) pop(&a);
    push(Gcomplex(&a, real(&a), -imag(&a)));
}

/* ang2rad is 1 if we are in radians, or pi/180 if we are in degrees */

void
f_sin()
{
    struct value a;
    (void) pop(&a);
    push(Gcomplex(&a, sin(ang2rad * real(&a)) * cosh(ang2rad * imag(&a)), cos(ang2rad * real(&a)) * sinh(ang2rad * imag(&a))));
}

void
f_cos()
{
    struct value a;
    (void) pop(&a);
    push(Gcomplex(&a, cos(ang2rad * real(&a)) * cosh(ang2rad * imag(&a)), -sin(ang2rad * real(&a)) * sinh(ang2rad * imag(&a))));
}

void
f_tan()
{
    struct value a;
    register double den;
    (void) pop(&a);
    if (imag(&a) == 0.0)
	push(Gcomplex(&a, tan(ang2rad * real(&a)), 0.0));
    else {
	den = cos(2 * ang2rad * real(&a)) + cosh(2 * ang2rad * imag(&a));
	if (den == 0.0) {
	    undefined = TRUE;
	    push(&a);
	} else
	    push(Gcomplex(&a, sin(2 * ang2rad * real(&a)) / den, sinh(2 * ang2rad * imag(&a)) / den));
    }
}

void
f_asin()
{
    struct value a;
    register double alpha, beta, x, y;
    (void) pop(&a);
    x = real(&a);
    y = imag(&a);
    if (y == 0.0 && fabs(x) <= 1.0) {
	push(Gcomplex(&a, asin(x) / ang2rad, 0.0));
    } else if (x == 0.0) {
	push(Gcomplex(&a, 0.0, -log(-y + sqrt(y * y + 1)) / ang2rad));
    } else {
	beta = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
	if (beta > 1)
	    beta = 1;		/* Avoid rounding error problems */
	alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
	push(Gcomplex(&a, asin(beta) / ang2rad, -log(alpha + sqrt(alpha * alpha - 1)) / ang2rad));
    }
}

void
f_acos()
{
    struct value a;
    register double x, y;
    (void) pop(&a);
    x = real(&a);
    y = imag(&a);
    if (y == 0.0 && fabs(x) <= 1.0) {
	/* real result */
	push(Gcomplex(&a, acos(x) / ang2rad, 0.0));
    } else {
	double alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 
	               + sqrt((x - 1) * (x - 1) + y * y) / 2;
	double beta = sqrt((x + 1) * (x + 1) + y * y) / 2 
	              - sqrt((x - 1) * (x - 1) + y * y) / 2;
	if (beta > 1)
	    beta = 1;		/* Avoid rounding error problems */
	else if (beta < -1)
	    beta = -1;
	push(Gcomplex(&a, (y > 0? -1: 1)*acos(beta) / ang2rad, 
	                  log(alpha + sqrt(alpha * alpha - 1)) / ang2rad));
    }
}

void
f_atan()
{
    struct value a;
    register double x, y, u, v, w, z;
    (void) pop(&a);
    x = real(&a);
    y = imag(&a);
    if (y == 0.0)
	push(Gcomplex(&a, atan(x) / ang2rad, 0.0));
    else if (x == 0.0 && fabs(y) >= 1.0) {
	undefined = TRUE;
	push(Gcomplex(&a, 0.0, 0.0));
    } else {
	if (x >= 0) {
	    u = x;
	    v = y;
	} else {
	    u = -x;
	    v = -y;
	}

	z = atan(2 * u / (1 - u * u - v * v));
	w = log((u * u + (v + 1) * (v + 1)) / (u * u + (v - 1) * (v - 1))) / 4;
	if (z < 0)
	    z = z + 2 * PI_ON_TWO;
	if (x < 0) {
	    z = -z;
	    w = -w;
	}
	push(Gcomplex(&a, 0.5 * z / ang2rad, w));
    }
}

/* real parts only */
void
f_atan2()
{
    struct value a;
    double x;
    double y;

    x = real(pop(&a));
    y = real(pop(&a));

    if (x == 0.0 && y == 0.0) {
	undefined = TRUE;
	push(Ginteger(&a, 0));
    }
    push(Gcomplex(&a, atan2(y, x) / ang2rad, 0.0));
}


void
f_sinh()
{
    struct value a;
    (void) pop(&a);
    push(Gcomplex(&a, sinh(real(&a)) * cos(imag(&a)), cosh(real(&a)) * sin(imag(&a))));
}

void
f_cosh()
{
    struct value a;
    (void) pop(&a);
    push(Gcomplex(&a, cosh(real(&a)) * cos(imag(&a)), sinh(real(&a)) * sin(imag(&a))));
}

void
f_tanh()
{
    struct value a;
    register double den;
    (void) pop(&a);
    den = cosh(2 * real(&a)) + cos(2 * imag(&a));
    push(Gcomplex(&a, sinh(2 * real(&a)) / den, sin(2 * imag(&a)) / den));
}

void
f_asinh()
{
    struct value a;		/* asinh(z) = -I*asin(I*z) */
    register double alpha, beta, x, y;
    (void) pop(&a);
    x = -imag(&a);
    y = real(&a);
    if (y == 0.0 && fabs(x) <= 1.0) {
	push(Gcomplex(&a, 0.0, -asin(x) / ang2rad));
    } else if (y == 0.0) {
	push(Gcomplex(&a, 0.0, 0.0));
	undefined = TRUE;
    } else if (x == 0.0) {
	push(Gcomplex(&a, log(y + sqrt(y * y + 1)) / ang2rad, 0.0));
    } else {
	beta = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
	alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
	push(Gcomplex(&a, log(alpha + sqrt(alpha * alpha - 1)) / ang2rad, -asin(beta) / ang2rad));
    }
}

void
f_acosh()
{
    struct value a;
    register double alpha, beta, x, y;	/* acosh(z) = I*acos(z) */
    (void) pop(&a);
    x = real(&a);
    y = imag(&a);
    if (y == 0.0 && fabs(x) <= 1.0) {
	push(Gcomplex(&a, 0.0, acos(x) / ang2rad));
    } else if (y == 0) {
	push(Gcomplex(&a, log(x + sqrt(x * x - 1)) / ang2rad, 0.0));
    } else {
	alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 
	        + sqrt((x - 1) * (x - 1) + y * y) / 2;
	beta = sqrt((x + 1) * (x + 1) + y * y) / 2 
	       - sqrt((x - 1) * (x - 1) + y * y) / 2;
	push(Gcomplex(&a, log(alpha + sqrt(alpha * alpha - 1)) / ang2rad, 
	                  (y<0 ? -1 : 1) * acos(beta) / ang2rad));
    }
}

void
f_atanh()
{
    struct value a;
    register double x, y, u, v, w, z;	/* atanh(z) = -I*atan(I*z) */
    (void) pop(&a);
    x = -imag(&a);
    y = real(&a);
    if (y == 0.0)
	push(Gcomplex(&a, 0.0, -atan(x) / ang2rad));
    else if (x == 0.0 && fabs(y) >= 1.0) {
	undefined = TRUE;
	push(Gcomplex(&a, 0.0, 0.0));
    } else {
	if (x >= 0) {
	    u = x;
	    v = y;
	} else {
	    u = -x;
	    v = -y;
	}

	z = atan(2 * u / (1 - u * u - v * v));
	w = log((u * u + (v + 1) * (v + 1)) / (u * u + (v - 1) * (v - 1))) / 4;
	if (z < 0)
	    z = z + 2 * PI_ON_TWO;
	if (x < 0) {
	    z = -z;
	    w = -w;
	}
	push(Gcomplex(&a, w, -0.5 * z / ang2rad));
    }
}

void
f_int()
{
    struct value a;
    push(Ginteger(&a, (int) real(pop(&a))));
}


void
f_abs()
{
    struct value a;
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	push(Ginteger(&a, abs(a.v.int_val)));
	break;
    case CMPLX:
	push(Gcomplex(&a, magnitude(&a), 0.0));
    }
}

void
f_sgn()
{
    struct value a;
    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	push(Ginteger(&a, (a.v.int_val > 0) ? 1 :
		      (a.v.int_val < 0) ? -1 : 0));
	break;
    case CMPLX:
	push(Ginteger(&a, (a.v.cmplx_val.real > 0.0) ? 1 :
		      (a.v.cmplx_val.real < 0.0) ? -1 : 0));
	break;
    }
}


void
f_sqrt()
{
    struct value a;
    register double mag;
    (void) pop(&a);
    mag = sqrt(magnitude(&a));
    if (imag(&a) == 0.0) {
	if (real(&a) < 0.0)
	    push(Gcomplex(&a, 0.0, mag));
	else
	    push(Gcomplex(&a, mag, 0.0));
    } else {
	/* -pi < ang < pi, so real(sqrt(z)) >= 0 */
	double ang = angle(&a) / 2.0;
	push(Gcomplex(&a, mag * cos(ang), mag * sin(ang)));
    }
}


void
f_exp()
{
    struct value a;
    register double mag, ang;
    (void) pop(&a);
    mag = gp_exp(real(&a));
    ang = imag(&a);
    push(Gcomplex(&a, mag * cos(ang), mag * sin(ang)));
}


void
f_log10()
{
    struct value a;
    (void) pop(&a);
    if (magnitude(&a) == 0.0) {
        undefined = TRUE;
	push(&a);
    } else
        push(Gcomplex(&a, log(magnitude(&a)) / M_LN10, angle(&a) / M_LN10));
}


void
f_log()
{
    struct value a;
    (void) pop(&a);
    if (magnitude(&a) == 0.0) {
        undefined = TRUE;
	push(&a);
    } else
        push(Gcomplex(&a, log(magnitude(&a)), angle(&a)));
}


void
f_floor()
{
    struct value a;

    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	push(Ginteger(&a, (int) floor((double) a.v.int_val)));
	break;
    case CMPLX:
	push(Ginteger(&a, (int) floor(a.v.cmplx_val.real)));
    }
}


void
f_ceil()
{
    struct value a;

    (void) pop(&a);
    switch (a.type) {
    case INTGR:
	push(Ginteger(&a, (int) ceil((double) a.v.int_val)));
	break;
    case CMPLX:
	push(Ginteger(&a, (int) ceil(a.v.cmplx_val.real)));
    }
}

/* bessel function approximations */
static double
jzero(x)
double x;
{
    double p, q, x2;
    int n;

    x2 = x * x;
    p = pjzero[8];
    q = qjzero[8];
    for (n = 7; n >= 0; n--) {
	p = p * x2 + pjzero[n];
	q = q * x2 + qjzero[n];
    }
    return (p / q);
}

static double
pzero(x)
double x;
{
    double p, q, z, z2;
    int n;

    z = 8.0 / x;
    z2 = z * z;
    p = ppzero[5];
    q = qpzero[5];
    for (n = 4; n >= 0; n--) {
	p = p * z2 + ppzero[n];
	q = q * z2 + qpzero[n];
    }
    return (p / q);
}

static double
qzero(x)
double x;
{
    double p, q, z, z2;
    int n;

    z = 8.0 / x;
    z2 = z * z;
    p = pqzero[5];
    q = qqzero[5];
    for (n = 4; n >= 0; n--) {
	p = p * z2 + pqzero[n];
	q = q * z2 + qqzero[n];
    }
    return (p / q);
}

static double
yzero(x)
double x;
{
    double p, q, x2;
    int n;

    x2 = x * x;
    p = pyzero[8];
    q = qyzero[8];
    for (n = 7; n >= 0; n--) {
	p = p * x2 + pyzero[n];
	q = q * x2 + qyzero[n];
    }
    return (p / q);
}

static double
rj0(x)
double x;
{
    if (x <= 0.0)
	x = -x;
    if (x < 8.0)
	return (jzero(x));
    else
	return (sqrt(TWO_ON_PI / x) *
		(pzero(x) * cos(x - PI_ON_FOUR) - 8.0 / x * qzero(x) * sin(x - PI_ON_FOUR)));

}

static double
ry0(x)
double x;
{
    if (x < 0.0)
	return (dzero / dzero);	/* error */
    if (x < 8.0)
	return (yzero(x) + TWO_ON_PI * rj0(x) * log(x));
    else
	return (sqrt(TWO_ON_PI / x) *
		(pzero(x) * sin(x - PI_ON_FOUR) +
		 (8.0 / x) * qzero(x) * cos(x - PI_ON_FOUR)));

}


static double
jone(x)
double x;
{
    double p, q, x2;
    int n;

    x2 = x * x;
    p = pjone[8];
    q = qjone[8];
    for (n = 7; n >= 0; n--) {
	p = p * x2 + pjone[n];
	q = q * x2 + qjone[n];
    }
    return (p / q);
}

static double
pone(x)
double x;
{
    double p, q, z, z2;
    int n;

    z = 8.0 / x;
    z2 = z * z;
    p = ppone[5];
    q = qpone[5];
    for (n = 4; n >= 0; n--) {
	p = p * z2 + ppone[n];
	q = q * z2 + qpone[n];
    }
    return (p / q);
}

static double
qone(x)
double x;
{
    double p, q, z, z2;
    int n;

    z = 8.0 / x;
    z2 = z * z;
    p = pqone[5];
    q = qqone[5];
    for (n = 4; n >= 0; n--) {
	p = p * z2 + pqone[n];
	q = q * z2 + qqone[n];
    }
    return (p / q);
}

static double
yone(x)
double x;
{
    double p, q, x2;
    int n;

    x2 = x * x;
    p = 0.0;
    q = qyone[8];
    for (n = 7; n >= 0; n--) {
	p = p * x2 + pyone[n];
	q = q * x2 + qyone[n];
    }
    return (p / q);
}

static double
rj1(x)
double x;
{
    double v, w;
    v = x;
    if (x < 0.0)
	x = -x;
    if (x < 8.0)
	return (v * jone(x));
    else {
	w = sqrt(TWO_ON_PI / x) *
	    (pone(x) * cos(x - THREE_PI_ON_FOUR) -
	     8.0 / x * qone(x) * sin(x - THREE_PI_ON_FOUR));
	if (v < 0.0)
	    w = -w;
	return (w);
    }
}

static double
ry1(x)
double x;
{
    if (x <= 0.0)
	return (dzero / dzero);	/* error */
    if (x < 8.0)
	return (x * yone(x) + TWO_ON_PI * (rj1(x) * log(x) - 1.0 / x));
    else
	return (sqrt(TWO_ON_PI / x) *
		(pone(x) * sin(x - THREE_PI_ON_FOUR) +
		 (8.0 / x) * qone(x) * cos(x - THREE_PI_ON_FOUR)));
}


void
f_besj0()
{
    struct value a;
    (void) pop(&a);
    if (fabs(imag(&a)) > zero)
	int_error(NO_CARET, "can only do bessel functions of reals");
    push(Gcomplex(&a, rj0(real(&a)), 0.0));
}


void
f_besj1()
{
    struct value a;
    (void) pop(&a);
    if (fabs(imag(&a)) > zero)
	int_error(NO_CARET, "can only do bessel functions of reals");
    push(Gcomplex(&a, rj1(real(&a)), 0.0));
}


void
f_besy0()
{
    struct value a;
    (void) pop(&a);
    if (fabs(imag(&a)) > zero)
	int_error(NO_CARET, "can only do bessel functions of reals");
    if (real(&a) > 0.0)
	push(Gcomplex(&a, ry0(real(&a)), 0.0));
    else {
	push(Gcomplex(&a, 0.0, 0.0));
	undefined = TRUE;
    }
}


void
f_besy1()
{
    struct value a;
    (void) pop(&a);
    if (fabs(imag(&a)) > zero)
	int_error(NO_CARET, "can only do bessel functions of reals");
    if (real(&a) > 0.0)
	push(Gcomplex(&a, ry1(real(&a)), 0.0));
    else {
	push(Gcomplex(&a, 0.0, 0.0));
	undefined = TRUE;
    }
}


/* functions for accessing fields from tm structure, for time series
 * they are all the same, so define a macro
 */

#define TIMEFUNC(name, field) \
void name() { \
  struct value a;  struct tm tm;         \
  (void) pop(&a);  ggmtime(&tm, real(&a)); \
  push(Gcomplex(&a, (double)tm.field, 0.0));        \
}

TIMEFUNC(f_tmsec, tm_sec)
TIMEFUNC(f_tmmin, tm_min)
TIMEFUNC(f_tmhour, tm_hour)
TIMEFUNC(f_tmmday, tm_mday)
TIMEFUNC(f_tmmon, tm_mon)
TIMEFUNC(f_tmyear, tm_year)
TIMEFUNC(f_tmwday, tm_wday)
TIMEFUNC(f_tmyday, tm_yday)
