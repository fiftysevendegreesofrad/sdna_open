# (c) Crispin Cooper on behalf of Cardiff University 2015
# This file is released under MIT license

# options(echo=TRUE) # if you want see commands in output file

suppressPackageStartupMessages(require(optparse,quietly=TRUE))
suppressPackageStartupMessages(require(glmnet,quietly=TRUE))
suppressPackageStartupMessages(require(methods,quietly=TRUE))

op = OptionParser()
op = add_option(op,"--calibrationfile",type="character")
op = add_option(op,"--weightfile",type="character")
op = add_option(op,"--predictinput",type="character")
op = add_option(op,"--predictoutput",type="character")
op = add_option(op,"--xs",type="character")
op = add_option(op,"--target",type="character")
op = add_option(op,"--nfolds",type="integer")
op = add_option(op,"--reps",type="integer")
op = add_option(op,"--alpha",type="double") # 0 for ridge 1 for lasso
op = add_option(op,"--intercept",type="integer") # store_true doesn't seem to work as expected
op = add_option(op,"--reglambdamin",type="double")
op = add_option(op,"--reglambdamax",type="double")
args = parse_args(op)

standardize=TRUE # not ideal for models where all vars are on the same scale e.g. betweenness but better for general use

use_intercept=TRUE
if (args$intercept==0) use_intercept=FALSE

reglambdas = NULL
if (!is.null(args$reglambdamin) && !is.null(args$reglambdamax))
    reglambdas = seq(args$reglambdamin,args$reglambdamax,length.out=50)

calibdata = read.csv(args$calibrationfile)
weights = read.table(args$weightfile)

mymodel = as.formula(paste(args$target,"~",args$xs))
xs = model.matrix(mymodel,calibdata)[,-1] # remove intercept (glmnet adds it again)
numvars = dim(xs)[2]
ys = as.numeric(unlist(calibdata[args$target]))

stopifnot(numvars>1)

lambdas = NULL
for (i in 1:args$reps)
{
    fit <- cv.glmnet(xs,ys,weights=weights,lambda=reglambdas,nfolds=args$nfolds,alpha=args$alpha,family="gaussian",standardize=standardize,intercept=use_intercept,lower.limits=0)
    errors = data.frame(fit$lambda,fit$cvm)
    lambdas <- rbind(lambdas,errors)
}
# this doesn't account for weights - assumed to be handled correctly in glmnet; each repetition has identical weight in total (adding up folds) so it is valid to take an average
lambdas <- aggregate(lambdas[, 2], list(lambdas$fit.lambda), mean)
bestindex = which(lambdas[2]==min(lambdas[2]))
bestlambda = lambdas[bestindex,1]
bestcvm = lambdas[bestindex,2]
cat("begin regcurve\n")
write.table(lambdas,col.names=F,quote=F,sep=",")
cat("end regcurve\n")

fit <- glmnet(xs,ys,lambda=bestlambda,weights=weights,alpha=args$alpha,family="gaussian",standardize=standardize,intercept=use_intercept,lower.limits=0)

r2=1-bestcvm/var(ys)
cat(sprintf("Regularized r2 out of sample,%f\n",r2))
#cat(sprintf("Regularized r2 in sample,%f\n",fit$dev.ratio)) #invalid when not using intercept

badfit <- summary(lm(mymodel,data=calibdata,weights=as.numeric(unlist(weights))))
cat(sprintf("OLS r2 (with intercept)     ,%f\n",badfit$r.squared))
cat(sprintf("OLS adj-r2 (with intercept) ,%f\n",badfit$adj.r.squared))

cat("Coefficients\n")
cat(sprintf("(Intercept) %f\n",fit$a0))
write.table(format(as.matrix(fit$beta)),col.names=F,quote=F)

