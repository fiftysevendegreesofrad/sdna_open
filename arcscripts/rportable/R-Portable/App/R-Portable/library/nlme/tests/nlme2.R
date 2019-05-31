library(nlme)

## https://stat.ethz.ch/pipermail/r-help/2014-September/422123.html
nlme( circumference ~ SSlogis(age, Asym, xmid, scal), data = Orange,
                      fixed = Asym + xmid + scal ~ 1)
