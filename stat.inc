#
# $Id: stat.inc 3.38.2.6 1992/11/14 02:25:21 woo Exp $
#
# Library of Statistical Functions version 3.0
#
# Permission granted to distribute freely for non-commercial purposes only
#
# Copyright (c) 1991, 1992 Jos van der Woude, jvdwoude@hut.nl

# If you don't have the gamma() and/or lgamma() functions in your library,
# you can use the following recursive definitions. They are correct for all
# values i / 2 with i = 1, 2, 3, ... This is sufficient for most statistical
# needs.
#logsqrtpi = log(sqrt(pi))
#lgamma(x) = (x<=0.5)?logsqrtpi:((x==1)?0:log(x-1)+lgamma(x-1))
#gamma(x) = exp(lgamma(x))

# If you have the lgamma() function compiled into gnuplot, you can use
# alternate definitions for some PDFs. For larger arguments this will result
# in more efficient evalution. Just uncomment the definitions containing the
# string `lgamma', while at the same time commenting out the originals.
# NOTE: In these cases the recursive definition for lgamma() is NOT sufficient!

# Some PDFs have alternate definitions of a recursive nature. I suppose these
# are not really very efficient, but I find them aesthetically pleasing to the
# brain.

# Define useful constants
fourinvsqrtpi=4.0/sqrt(pi)
invpi=1.0/pi
invsqrt2pi=1.0/sqrt(2.0*pi)
log2=log(2.0)
sqrt2=sqrt(2.0)
sqrt2invpi=sqrt(2.0/pi)
twopi=2.0*pi

# define variables plus default values used as parameters in PDFs
# some are integers, others MUST be reals
a=1.0
alpha=0.5
b=2.0
df1=1
df2=1
g=1.0
lambda=1.0
m=0.0
mm=1
mu=0.0
nn=2
n=2
p=0.5
q=0.5
r=1
rho=1.0
sigma=1.0
u=1.0

#
#define 1.0/Beta function
#
Binv(p,q)=exp(lgamma(p+q)-lgamma(p)-lgamma(q))

#
#define Probability Density Functions (PDFs)
#

# NOTE:
# The discrete PDFs are calulated for all real values, using the int()
# function to truncate to integers. This is a monumental waste of processing
# power, but I see no other easy solution. If anyone has any smart ideas
# about this, I would like to know. Setting the sample size to a larger value
# makes the discrete PDFs look better, but takes even more time.

# Arcsin PDF
arcsin(x)=invpi/sqrt(r*r-x*x)

# Beta PDF
beta(x)=Binv(p,q)*x**(p-1.0)*(1.0-x)**(q-1.0)

# Binomial PDF
#binom(x)=n!/(n-int(x))!/int(x)!*p**int(x)*(1.0-p)**(n-int(x))

bin_s(x)=n!/(n-int(x))!/int(x)!*p**int(x)*(1.0-p)**(n-int(x))
bin_l(x)=exp(lgamma(n+1)-lgamma(n-int(x)+1)-lgamma(int(x)+1)\
+int(x)*log(p)+(n-int(x))*log(1.0-p))
binom(x)=(n<50)?bin_s(x):bin_l(x)

# Cauchy PDF
cauchy(x)=b/(pi*(b*b+(x-a)**2))

# Chi-square PDF
#chi(x)=x**(0.5*df1-1.0)*exp(-0.5*x)/gamma(0.5*df1)/2**(0.5*df1)
chi(x)=exp((0.5*df1-1.0)*log(x)-0.5*x-lgamma(0.5*df1)-df1*0.5*log2)

# Erlang PDF
erlang(x)=lambda**n/(n-1)!*x**(n-1)*exp(-lambda*x)

# Extreme (Gumbel extreme value) PDF
extreme(x)=alpha*(exp(-alpha*(x-u)-exp(-alpha*(x-u))))

# F PDF
f(x)=Binv(0.5*df1,0.5*df2)*(df1/df2)**(0.5*df1)*x**(0.5*df1-1.0)/\
(1.0+df1/df2*x)**(0.5*(df1+df2))

# Gamma PDF
#g(x)=lambda**rho*x**(rho-1.0)*exp(-lambda*x)/gamma(rho)
g(x)=exp(rho*log(lambda)+(rho-1.0)*log(x)-lgamma(rho)-lambda*x)

# Geometric PDF
#geometric(x)=p*(1.0-p)**int(x)
geometric(x)=exp(log(p)+int(x)*log(1.0-p))

# Half normal PDF
halfnormal(x)=sqrt2invpi/sigma*exp(-0.5*(x/sigma)**2)

# Hypergeometric PDF
hypgeo(x)=(int(x)>mm||int(x)<mm+n-nn)?0:\
mm!/(mm-int(x))!/int(x)!*(nn-mm)!/(n-int(x))!/(nn-mm-n+int(x))!*(nn-n)!*n!/nn!

# Laplace PDF
laplace(x)=0.5/b*exp(-abs(x-a)/b)

# Logistic PDF
logistic(x)=lambda*exp(-lambda*(x-a))/(1.0+exp(-lambda*(x-a)))**2

# Lognormal PDF
lognormal(x)=invsqrt2pi/sigma/x*exp(-0.5*((log(x)-mu)/sigma)**2)

# Maxwell PDF
maxwell(x)=fourinvsqrtpi*a**3*x*x*exp(-a*a*x*x)

# Negative binomial PDF
#negbin(x)=(r+int(x)-1)!/int(x)!/(r-1)!*p**r*(1.0-p)**int(x)
negbin(x)=exp(lgamma(r+int(x))-lgamma(r)-lgamma(int(x)+1)+\
r*log(p)+int(x)*log(1.0-p))

# Negative exponential PDF
nexp(x)=lambda*exp(-lambda*x)

# Normal PDF
normal(x)=invsqrt2pi/sigma*exp(-0.5*((x-mu)/sigma)**2)

# Pareto PDF
pareto(x)=x<a?0:b/x*(a/x)**b

# Poisson PDF
poisson(x)=mu**int(x)/int(x)!*exp(-mu)
#poisson(x)=exp(int(x)*log(mu)-lgamma(int(x)+1)-mu)
#poisson(x)=(x<1)?exp(-mu):mu/int(x)*poisson(x-1)
#lpoisson(x)=(x<1)?-mu:log(mu)-log(int(x))+lpoisson(x-1)

# Rayleigh PDF
rayleigh(x)=lambda*2.0*x*exp(-lambda*x*x)

# Sine PDF
sine(x)=2.0/a*sin(n*pi*x/a)**2

# t (Student's t) PDF
t(x)=Binv(0.5*df1,0.5)/sqrt(df1)*(1.0+(x*x)/df1)**(-0.5*(df1+1.0))

# Triangular PDF
triangular(x)=1.0/g-abs(x-m)/(g*g)

# Uniform PDF
uniform(x)=1.0/(b-a)

# Weibull PDF
weibull(x)=lambda*n*x**(n-1)*exp(-lambda*x**n)

#
#define Cumulative Distribution Functions (CDFs)
#

# Arcsin CDF
carcsin(x)=0.5+invpi*asin(x/r)

# incomplete Beta CDF
cbeta(x)=ibeta(p,q,x)

# Binomial CDF
#cbinom(x)=(x<1)?binom(0):binom(x)+cbinom(x-1)
cbinom(x)=ibeta(n-x,x+1.0,1.0-p)

# Cauchy CDF
ccauchy(x)=0.5+invpi*atan((x-a)/b)

# Chi-square CDF
cchi(x)=igamma(0.5*df1,0.5*x)

# Erlang CDF
# approximation, using first three terms of expansion
cerlang(x)=1.0-exp(-lambda*x)*(1.0+lambda*x+0.5*(lambda*x)**2)

# Extreme (Gumbel extreme value) CDF
cextreme(x)=exp(-exp(-alpha*(x-u)))

# F CDF
cf(x)=1.0-ibeta(0.5*df2,0.5*df1,df2/(df2+df1*x))

# incomplete Gamma CDF
cgamma(x)=igamma(rho,x)

# Geometric CDF
cgeometric(x)=(x<1)?geometric(0):geometric(x)+cgeometric(x-1)

# Half normal CDF
chalfnormal(x)=erf(x/sigma/sqrt2)

# Hypergeometric CDF
chypgeo(x)=(x<1)?hypgeo(0):hypgeo(x)+chypgeo(x-1)

# Laplace CDF
claplace(x)=(x<a)?0.5*exp((x-a)/b):1.0-0.5*exp(-(x-a)/b)

# Logistic CDF
clogistic(x)=1.0/(1.0+exp(-lambda*(x-a)))

# Lognormal CDF
clognormal(x)=cnormal(log(x))

# Maxwell CDF
cmaxwell(x)=igamma(1.5,a*a*x*x)

# Negative binomial CDF
cnegbin(x)=(x<1)?negbin(0):negbin(x)+cnegbin(x-1)

# Negative exponential CDF
cnexp(x)=1.0-exp(-lambda*x)

# Normal CDF
cnormal(x)=0.5+0.5*erf((x-mu)/sigma/sqrt2)
#cnormal(x)=0.5+((x>mu)?0.5:-0.5)*igamma(0.5,0.5*((x-mu)/sigma)**2)

# Pareto CDF
cpareto(x)=x<a?0:1.0-(a/x)**b

# Poisson CDF
#cpoisson(x)=(x<1)?poisson(0):poisson(x)+cpoisson(x-1)
cpoisson(x)=1.0-igamma(x+1.0,mu)

# Rayleigh CDF
crayleigh(x)=1.0-exp(-lambda*x*x)

# Sine CDF
csine(x)=x/a-sin(n*twopi*x/a)/(n*twopi)

# t (Student's t) CDF
ct(x)=(x<0.0)?0.5*ibeta(0.5*df1,0.5,df1/(df1+x*x)):\
1.0-0.5*ibeta(0.5*df1,0.5,df1/(df1+x*x))

# Triangular PDF
ctriangular(x)=0.5+(x-m)/g-(x-m)*abs(x-m)/(2.0*g*g)

# Uniform CDF
cuniform(x)=(x-a)/(b-a)

# Weibull CDF
cweibull(x)=1.0-exp(-lambda*x**n)
