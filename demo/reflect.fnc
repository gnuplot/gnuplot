#
#   Model function for Reflectivity evaluation
#

mu = 1.130469005513490E-001                     # (cm-1) @ 17.479 keV
t0 = 0.18                                       # cm
tb = 11.417823202820120 * 0.01745329251994      # thetaB (radians)
A = mu * t0 / cos(tb)
P = (1 + (cos(2.*tb))**2) / 2
Fhkl = sqrt(3.536346308456155**2 + (4.58815426260982e-4)**2) * 0.968
r0 = 2.81794092e-13                             # classical electron radius
lambda = 7.09338062818239e-9                    # Mo K in cm
V = 1.62253546981499e-23
P = (1. + (cos(2.*tb))**2) / 2.

#

W(x) = 1./(sqrt(2.*pi)*eta) * exp( -1. * x**2 / (2.*eta**2) )
Y(tc) = tc/sin(tb) * (Fhkl*r0*lambda) / V
f(tc)= (tanh(Y(tc)) + abs(cos(2.*tb)) * tanh(abs(Y(tc)*cos(2.*tb)))) / (Y(tc)*(1.+(cos(2.*tb))**2))
Q(tc) = (r0*Fhkl/V)**2 * (lambda**3/sin(2.*tb)) * P * f(tc)
a(x) = W(x) * Q(tc) / mu

#

R(x) = sinh(A*a(x)) * exp(-1.*A*(1.+a(x)))
