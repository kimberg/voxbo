function [peak_threshold, extent_threshold, ...
          peak_threshold_1, extent_threshold_1] = ...
   stat_threshold(search_volume, num_voxels, fwhm,  ... 
   df, p_val_peak, cluster_threshold, p_val_extent,k)
%STAT_THRESHOLD 
%
% [PEAK_THRESHOLD, EXTENT_THRESHOLD, PEAK_THRESHOLD_1 EXTENT_THRESHOLD_1]
%   = STAT_THRESHOLD([ SEARCH_VOLUME [, NUM_VOXELS [, FWHM  [, DF 
%   [, P_VAL_PEAK [, CLUSTER_THRESHOLD [, P_VAL_EXTENT [, K ]]]]]]]] )
%
% If P-values are supplied, returns the thresholds for local maxima 
% or peaks (PEAK_THRESHOLD) and spatial extent of contiguous voxels 
% above CLUSTER_THRESHOLD (EXTENT_THRESHOLD) for t or F statistic  
% maps or conjunctions of K of these. If thresholds are supplied then it
% returns P-values. For peaks (local maxima), two methods are used:  
% random field theory (Worsley et al. 1996, Human Brain Mapping, 4:58-73),
% and a simple Bonferroni correction based on the number of voxels
% in the search region. The final threshold is the minimum of the two.
% PEAK_THRESHOLD_1 is the height of a single peak chosen in advance, and 
% EXTENT_THRESHOLD_1 is the extent of a single cluster chosen in advance
% of looking at the data, e.g. the nearest peak or cluster to a pre-chosen 
% voxel or ROI - see Friston KJ. Testing for anatomically specified 
% regional effects. Human Brain Mapping. 5(2):133-6, 1997.
%
% For clusters, the method of Cao, 1999, Advances in Applied Probability,
% 31:577-593 is used. The cluster size is only accurate for large 
% CLUSTER_THRESHOLD (say >3) and large resels = SEARCH_VOLUME / FWHM^D. 
%
% SEARCH_VOLUME is the volume of the search region in mm^3. Default is
% 1000000, i.e. 1000 cc. The method for finding PEAK_THRESHOLD works well 
% for any value of SEARCH_VOLUME, even SEARCH_VOLUME = 0, which gives the   
% threshold for the image at a single voxel. If SEARCH_VOLUME is a vector  
% of D+1 components, these are the intrinsic volumes of the search region
% = [Euler characteristic, 2 * caliper diameter, 1/2 surface area, volume],
% e.g. [1, 4*r, 2*pi*r^2, 4/3*pi*r^3] for a sphere of radius r in 3D. For 
% a 2D search region, use [1, 1/2 perimeter length, area]. The random field  
% theory threshold is based on the assumption that the search region is a  
% sphere, which is a very tight lower bound for any non-spherical region.
%
% The number of dimensions D is 3 if SEARCH_VOLUME is a scalar, and the 
% (maximum index of the positive components)-1 if SEARCH_VOLUME is a vector, 
% e.g. if 1000000 or [1 300 30000 1000000], D=3 (a 100 x 100 x 100 cube); 
%      if [1 200 10000] or [1 200 10000 0], D=2 (a 100 x 100 square). 
%
% The random estimate of the FWHM adds variability to the summed resels of 
% the cluster. The distribution of the local resels summed over a region 
% depends on the unknown spatial correlation structure of the resels, so we 
% assume that the resels are constant over the cluster, reasonable if the
% clusters are small. The cluster resels are then multiplied by the resel 
% distribution at a point, to give an upper bound to p-values and thresholds.
%
% NUM_VOXELS is the number of voxels (3D) or pixels (2D) in the search volume.
% Default is 1000000.
%
% FWHM is the fwhm in mm of a smoothing kernel applied to the data. Default
% is 0.0, i.e. no smoothing, which is roughly the case for raw fMRI data.
% For motion corrected fMRI data, use at least 6mm; for PET data, this would 
% be the scanner fwhm of about 6mm. Using the geometric mean of the three 
% x,y,z FWHM's is a very good approximation if they are different. 
%
% DF=[DF1 DF2; DFW1 DFW2] is a 2 x 2 matrix of degrees of freedom.
% If DF2 is 0, then DF1 is the df of the T statistic image.
% If DF1=Inf then it calculates thresholds for the Gaussian image. 
% If DF2>0 then DF1 and DF2 are the df's of the F statistic image.
% If DF2<0 then DF1 and -DF2 are the df's of the Hotelling's T^2 statistic.
% DFW1 and DFW2 are the numerator and denominator df of the FWHM image. 
% They are only used for calculating cluster resel p-values and thresholds.
% The random estimate of the local resels adds variability to the summed
% resels of the cluster. The distribution of the local resels summed over a 
% region depends on the unknown spatial correlation structure of the resels,
% so we assume that the resels are constant over the cluster, reasonable if the
% clusters are small. The cluster resels are then multiplied by the random resel 
% distribution at a point. This gives an upper bound to p-values and thresholds.
% If DF=[DF1 DF2] (row) then DFW1=DFW2=Inf, i.e. FWHM is fixed.
% If DF=[DF1; DFW1] (column) then DF2=0 and DFW2=DFW1, i.e. t statistic. 
% If DF=DF1 (scalar) then DF2=0 and DFW1=DFW2=Inf, i.e. t stat, fixed FWHM.
% If any component of DF >= 1000 then it is set to Inf. Default is Inf. 
%
% P_VAL_PEAK is the row vector of desired P-values for peaks. 
% If the first element is greater than 1, then they are
% treated as peak values and P-values are returned. Default is 0.05.
% To get P-values for peak values < 1, set the first element > 1,
% set the second to the deisired peak value, then discard the first result. 
%
% CLUSTER_THRESHOLD is the scalar threshold of the image for clusters.
% If it is <= 1, it is taken as a probability p, and the 
% cluter_thresh is chosen so that P( T > cluster_thresh ) = p. 
% Default is 0.001, i.e. P( T > cluster_thresh ) = 0.001. 
%
% P_VAL_EXTENT is the row vector of desired P-values for spatial extents
% of clusters of contiguous voxels above CLUSTER_THRESHOLD. 
% If the first element is greater than 1, then they are treated as 
% extent values and P-values are returned. Default is 0.05.
%
% K is the number of conjunctions. If K > 1, calculates P-values and 
% thresholds for peaks (but not clusters) of the minimum of K independent 
% t or F maps - see Friston, K.J., Holmes, A.P., Price, C.J., Buchel, C.,
% Worsley, K.J. (1999). Multi-subject fMRI studies and conjunction analyses.
% NeuroImage, 10:385-396. Default is K = 1 (no conjunctions). 
%
% Examples:
%
% T statistic, 1000000mm^3 search volume, 1000000 voxels (1mm voxel volume), 
% 20mm smoothing, 30 degrees of freedom, P=0.05 and 0.01 for peaks, cluster 
% threshold chosen so that P=0.001 at a single voxel, P=0.05 and 0.01 for extent:
%
%   stat_threshold(1000000,1000000,20,30,[0.05 0.01],0.001,[0.05 0.01]);
%
% peak_threshold = 5.0820    5.7847
% Cluster_threshold = 3.3852
% peak_threshold_1 = 4.8780    5.5949
% extent_threshold = 3340.2    6315.5
% extent_threshold_1 = 2911.5    5719.4
%
% Check: Suppose we are given peak values of 5.0589, 5.7803 and
% spatial extents of 3841.9, 6944.4 above a threshold of 3.3852,
% then we should get back our original P-values:
% 
%   stat_threshold(1000000,1000000,20,30,[5.0820 5.7847],3.3852,[3340.2 6315.5]);
%
% P_val_peak = 0.0500    0.0100
% P_val_peak_1 = 0.0318    0.0065
% P_val_extent = 0.0500    0.0100
% P_val_extent_1 = 0.0379    0.0074
%
% Another way of doing this is to use the fact that T^2 has an F 
% distribution with 1 and 30 degrees of freedom, and double the P values: 
% 
%   stat_threshold(1000000,1000000,20,[1 30],[0.1 0.02],0.002,[0.1 0.02]);
%
% peak_threshold = 25.8271   33.4623
% Cluster_threshold = 11.4596
% peak_threshold_1 = 20.7840   27.9735
% extent_threshold = 3297.8    6305.1
% extent_threshold_1 = 1936.4    4420.2
%
% Note that sqrt([25.8271 33.4623 11.4596])=[5.0820 5.7847 3.3852]
% in agreement with the T statistic thresholds, but the spatial extent
% thresholds are very close but not exactly the same.
%
% If the shape of the search region is known, then the 'intrinisic
% volume' must be supplied. E.g. for a spherical search region with
% a volume of 1000000 mm^3, radius r:
%
%   r=(1000000/(4/3*pi))^(1/3);
%   stat_threshold([1 4*r 2*pi*r^2 4/3*pi*r^3], ...
%                 1000000,20,30,[0.05 0.01],0.001,[0.05 0.01]);
% 
% gives the same results as our first example. A 100 x 100 x 100 cube 
%
%   stat_threshold([1 300 30000 1000000], ...
%                 1000000,20,30,[0.05 0.01],0.001,[0.05 0.01]);
% 
% gives slightly higher thresholds (5.0965, 5.7972) for peaks, but the cluster 
% thresholds are the same since they depend (so far) only on the volume and 
% not the shape. For a 2D circular search region of the same radius r, use
%
%   stat_threshold([1 pi*r pi*r^2],1000000,20,30,[0.05 0.01],0.001,[0.05 0.01]);
%
% A volume of 0 returns the usual uncorrected threshold for peaks,
% which can also be found by setting NUM_VOXELS=1, FWHM = 0:
%
%   stat_threshold(0,1,20,30,[0.05 0.01]);
%   stat_threshold(1,1, 0,30,[0.05 0.01]);
%
% For non-isotropic fields, replace the SEARCH_VOLUME by the vector of resels 
% (see Worsley et al. 1996, Human Brain Mapping, 4:58-73), and set FWHM=1, 
% so that EXTENT_THRESHOLD's are measured in resels, not mm^3. If the 
% resels are estimated from residuals, add an extra row to DF (see above).
%
% For the conjunction of 2 and 3 t fields as above:
%
%   stat_threshold(1000000,1000000,20,30,[0.05 0.01],0.001,[0.05 0.01],2);
%   stat_threshold(1000000,1000000,20,30,[0.05 0.01],0.001,[0.05 0.01],3);
%
% returns lower thresholds of [3.0250 3.3978] and [2.2331 2.5134] resp. Note
% that there are as yet no theoretical results for extents so they are NaN.
% 
% For Hotelling's T^2, used in mulivariate linear models from deformations,
%
%   stat_threshold(1000000,1000000,20,[3 -30]);
%
% There are as yet no theoretical results for extents so they are NaN.

%############################################################################
% COPYRIGHT:   Copyright 2000 K.J. Worsley and C. Liao, 
%              Department of Mathematics and Statistics,
%              McConnell Brain Imaging Center, 
%              Montreal Neurological Institute,
%              McGill University, Montreal, Quebec, Canada. 
%              worsley@math.mcgill.ca, liao@math.mcgill.ca
%
%              Permission to use, copy, modify, and distribute this
%              software and its documentation for any purpose and without
%              fee is hereby granted, provided that this copyright
%              notice appears in all copies. The author and McGill University
%              make no representations about the suitability of this
%              software for any purpose.  It is provided "as is" without
%              express or implied warranty.
%############################################################################

% Defaults:

if nargin < 1
   search_volume=1000000
end
if nargin < 2
   num_voxels=1000000
end
if nargin < 3
   fwhm=0.0
end
if nargin < 4
   df = Inf
end
if nargin < 5
   p_val_peak = 0.05
end
if nargin < 6
   cluster_threshold = 0.001
end
if nargin < 7
   p_val_extent = 0.05
end
if nargin < 8
   k = 1
end

% Warning:
if round(num_voxels)~=num_voxels
    num_voxels
    fprintf(['Error: non-integer number of voxels.\n']);
    fprintf(['Note that the second parameter is now the number of voxels\n']);
    fprintf(['in the search volume, not the voxel volume - see the help.']);
    return
end

% D=number of dimensions:
lsv=length(search_volume);
if lsv==1
   D=3
else
   D=max(find(search_volume))-1
end

% determines which method was used to estimate fwhm (see fmrilm or multistat): 
df_limit=4;

% max number of pvalues or thresholds to print:
nprint=5;

if length(df)==1
    df=[df 0];
end
if size(df,1)==1
    df=[df; Inf Inf]
end
if size(df,2)==1
    df=[df [0; df(2,1)]]
end

% is_tstat=1 if it is a t statistic, is_hotel=1 if it is
% Hotelling's T^2, otherwise it is an F statistic.
is_tstat=(df(1,2)==0);
is_hotel=(df(1,2)<0);

if is_tstat
   df1=1;
   df2=df(1,1);
else
   df1=df(1,1);
   df2=abs(df(1,2));
end
if df2 >= 1000
   df2=Inf;
end
dfw1=df(2,1);
dfw2=df(2,2);
if dfw1 >= 1000
    dfw1=Inf;
end
if dfw2 >= 1000
    dfw2=Inf;
end

% Values of the F statistic or T^2/df1 based on squares of t values:
t=[100:-0.1:10.1 10:-0.01:0].^2;

% Find the upper tail probs of the F distribution by 
% cumulating the F density using the mid-point rule:
n=length(t);
n1=1:(n-1);
tt=(t(n1)+t(n1+1))/2;
if df2==Inf
   u=df1*tt;
   b=exp(-u/2-gammaln(df1/2)-df1/2*log(2)+(df1/2-1)*log(u));
else  
   if is_hotel
      df22=df2-df1+1;
   else
      df22=df2;
   end
   u=df1*tt/df2;
   b=(1+u).^(-(df1+df22)/2).*u.^(df1/2-1)*df1/df2/beta(df1/2,df22/2);
end
pt=[0 -cumsum(b.*diff(t))];

if fwhm>0 
   % Random field theory
   if lsv==1
      radius=(search_volume/(4/3*pi))^(1/3);
      resels=[1 4*radius/fwhm 2*pi*(radius/fwhm)^2 search_volume/fwhm^3];
   else
      resels=[search_volume./fwhm.^(0:lsv-1) zeros(1,3-D)];
   end
   rho=zeros(3,n);
   t1=t(n1);
   f1=sqrt(2*log(2)/pi);
   if df2==Inf
      u=df1*t1;
      logu=log(u);
      b1=exp(-(df1-2)/2*log(2)-gammaln(df1/2));
      
      c1=exp((df1-1)/2*logu-u/2);
      c2=exp((df1-2)/2*logu-u/2);
      c3=exp((df1-3)/2*logu-u/2);
      
      rho(1,:)=f1  *b1*[c1                 (df1==1)];
      rho(2,:)=f1^2*b1*[c2.*(u-(df1-1)) -1*(df1==2)];
      rho(3,:)=f1^3*b1*[c3.*(u.^2-(2*df1-1)*u+(df1-1)*(df1-2)) ...
            -1*(df1==1)+2*(df1==3)];
   else   
      u=df1*t1/df2;
      logu=log(u);
      log1pu=log(1+u);
      if ~is_hotel
         b1=exp(gammaln((df1+df2-1)/2)-gammaln(df1/2)-gammaln(df2/2));
         b2=exp(gammaln((df1+df2-2)/2)-gammaln(df1/2)-gammaln(df2/2));
         b3=exp(gammaln((df1+df2-3)/2)-gammaln(df1/2)-gammaln(df2/2));
         
         c1=exp((df1-1)/2*logu-(df1+df2-2)/2*log1pu);
         c2=exp((df1-2)/2*logu-(df1+df2-2)/2*log1pu);
         c3=exp((df1-3)/2*logu-(df1+df2-2)/2*log1pu);
         
         p0=(df1-1)*(df1-2);
         p1=-(2*df1*df2-df1-df2-1);
         p2=(df2-1)*(df2-2);
         
         rho(1,:)=f1  *b1*sqrt(2)*[c1                         (df1==1)];
         rho(2,:)=f1^2*b2        *[c2.*((df2-1)*u-(df1-1)) -1*(df1==2)];
         rho(3,:)=f1^3*b3/sqrt(2)*[c3.*(p2*u.^2+p1*u+p0) ... 
               p1*(df1==1)+p0*(df1==3)];
      else
         b1=exp(gammaln((df2+1)/2)-gammaln(df1/2)-gammaln((df2-df1+2)/2));
         b2=exp(gammaln((df2+1)/2)-gammaln(df1/2)-gammaln((df2-df1+3)/2));
         b3=exp(gammaln((df2+1)/2)-gammaln(df1/2)-gammaln((df2-df1+4)/2));
         
         c1=exp((df1-1)/2*logu-(df2-1)/2*log1pu);
         c2=exp((df1-2)/2*logu-(df2-1)/2*log1pu);
         c3=exp((df1-3)/2*logu-(df2-1)/2*log1pu);
         
         p0=(df1-1)*(df1-2);
         p1=-(2*df1-1)*(df2-df1+2);
         p2= (df2-df1)*(df2-df1+2);
         
         rho(1,:)=f1  *b1*sqrt(2)*[c1                             (df1==1)];
         rho(2,:)=f1^2*b2        *[c2.*((df2-df1+1)*u-(df1-1)) -1*(df1==2)];
         rho(3,:)=f1^3*b3/sqrt(2)*[c3.*(p2*u.^2+p1*u+p0) ... 
               p1*(df1==1)+p0*(df1==3)];
      end
   end
   if is_tstat
      t=[sqrt(t(n1)) -fliplr(sqrt(t))];
      pt=[pt(n1)/2 1-fliplr(pt)/2];
      rho=[rho(:,n1) diag([1 -1 1])*fliplr(rho)]/2;
      n=2*n-1;
   end
   if is_hotel
      t=t*df1;
   end
   if k>1
      % Conjunctions:
      b=gamma((2:4)/2)/gamma(1/2).*(4*log(2)).^((1:3)/2);
      for i=1:n
         R=toeplitz([pt(i) rho(:,i)'./b], [pt(i) 0 0 0])';
         RR=R^k;
         rho(:,i)=RR(1,2:4)';
         pt(i)=RR(1,1);
      end
      rho=diag(b)*rho;
   end
   pval_rf=resels(1)*pt+resels(2:4)*rho;
else
   if is_tstat
      t=[sqrt(t(n1)) -fliplr(sqrt(t))];
      pt=[pt(n1)/2 1-fliplr(pt)/2];
   end
   if is_hotel
      t=t*df1;
   end
   pval_rf=Inf;
   pt=pt.^k;
end

% Bonferroni 
pval_bon=num_voxels*pt;

% Minimum of the two:
pval=min(pval_rf,pval_bon);

tlim=1;
if p_val_peak(1) <= tlim
   peak_threshold=minterp1(pval,t,p_val_peak);
   if length(p_val_peak)<=nprint
      peak_threshold
   end
else
   % p_val_peak is treated as a peak value:
   P_val_peak=interp1(t,pval,p_val_peak);
   peak_threshold=P_val_peak;
   if length(p_val_peak)<=nprint
      P_val_peak
   end
end

if fwhm<=0 | is_hotel
   extent_threshold=peak_threshold+NaN;
   extent_threshold_1=extent_threshold;
   return
end

% Cluster_threshold:

if cluster_threshold > tlim
   tt=cluster_threshold;
else
   % cluster_threshold is treated as a probability:
   tt=minterp1(pt,t,cluster_threshold);
   Cluster_threshold=tt
end

rhoD=interp1(t,rho(D,:),tt);
p=interp1(t,pt,tt);

% Pre-selected peak:

pval=rho(D,:)./rhoD;

if p_val_peak(1) <= tlim 
   peak_threshold_1=minterp1(pval,t, p_val_peak);
   if length(p_val_peak)<=nprint
      peak_threshold_1
   end
else
   % p_val_peak is treated as a peak value:
   P_val_peak_1=interp1(t,pval,p_val_peak);
   peak_threshold_1=P_val_peak_1;
   if length(p_val_peak)<=nprint
      P_val_peak_1
   end
end

if  k>1 | search_volume==0
    extent_threshold=p_val_extent+NaN;
    extent_threshold_1=extent_threshold;
    if length(p_val_extent)<=nprint
       extent_threshold
       extent_threshold_1
    end
    return
end

% Expected number of clusters:

EL=resels(D+1)*rhoD;

if df2==Inf & dfw1==Inf
   if p_val_extent(1) <= tlim 
      pS=-log(1-p_val_extent)/EL;
      extent_threshold=p/rhoD/gamma(D/2+1)*fwhm^D*(-log(pS)).^(D/2);
      pS=-log(1-p_val_extent);
      extent_threshold_1=p/rhoD/gamma(D/2+1)*fwhm^D*(-log(pS)).^(D/2);
      if length(p_val_extent)<=nprint
         extent_threshold
         extent_threshold_1
      end
   else
      % p_val_extent is now treated as a spatial extent:
      pS=exp(-(p_val_extent/p*rhoD*gamma(D/2+1)/fwhm^D).^(2/D));
      P_val_extent=1-exp(-pS*EL);
      extent_threshold=P_val_extent;
      P_val_extent_1=1-exp(-pS);
      extent_threshold_1=P_val_extent_1;
      if length(p_val_extent)<=nprint
         P_val_extent
         P_val_extent_1
      end
   end
else
   % Find dbn of S by taking logs then using fft for convolution:
   ny=2^12;
   a=D/2;
   b2=a*10*max(sqrt(2/(min(df1+df2,dfw1))),1);
   if df2<Inf
      b1=a*log((1-(1-0.000001)^(2/(df2-D)))*df2/2);
   else
      b1=a*log(-log(1-0.000001));
   end
   dy=(b2-b1)/ny;
   b1=round(b1/dy)*dy;
   y=((1:ny)'-1)*dy+b1;
   numrv=1+(D+1)*(df2<Inf)+D*(dfw1<Inf)+(dfw2<Inf);
   f=zeros(ny,numrv);
   mu=zeros(1,numrv);
   if df2<Inf
      % Density of log(Beta(1,(df2-D)/2)^(D/2)):
      yy=exp(y./a)/df2*2;  
      yy=yy.*(yy<1);
      f(:,1)=(1-yy).^((df2-D)/2-1).*((df2-D)/2).*yy/a;
      mu(1)=exp(gammaln(a+1)+gammaln((df2-D+2)/2)-gammaln((df2+2)/2)+a*log(df2/2));
   else
      % Density of log(exp(1)^(D/2)):
      yy=exp(y./a);   
      f(:,1)=exp(-yy).*yy/a;
      mu(1)=exp(gammaln(a+1));
   end
   
   nuv=[];
   aav=[];
   if df2<Inf
      nuv=[df1+df2-D  df2+2-(1:D)];
      aav=[a repmat(-1/2,1,D)]; 
   end
   if dfw1<Inf
      if dfw1>df_limit
         nuv=[nuv dfw1-dfw1/dfw2-(0:(D-1))];
      else
         nuv=[nuv repmat(dfw1-dfw1/dfw2,1,D)];
      end
      aav=[aav repmat(1/2,1,D)];
   end   
   if dfw2<Inf
      nuv=[nuv dfw2];
      aav=[aav -a];
   end   
   
   for i=1:(numrv-1)
      nu=nuv(i);
      aa=aav(i);
      yy=y/aa+log(nu);
      % Density of log((chi^2_nu/nu)^aa):
      f(:,i+1)=exp(nu/2*yy-exp(yy)/2-(nu/2)*log(2)-gammaln(nu/2))/abs(aa);
      mu(i+1)=exp(gammaln(nu/2+aa)-gammaln(nu/2)-aa*log(nu/2));
   end
   % Check: plot(y,f); sum(f*dy,1) should be 1
      
   omega=2*pi*((1:ny)'-1)/ny/dy;
   shift=complex(cos(-b1*omega),sin(-b1*omega))*dy;
   prodfft=prod(fft(f),2).*shift.^(numrv-1);
   % Density of Y=log(B^(D/2)*U^(D/2)/sqrt(det(Q))):
   ff=real(ifft(prodfft));
   % Check: plot(y,ff); sum(ff*dy) should be 1
   mu0=prod(mu);
   % Check: plot(y,ff.*exp(y)); sum(ff.*exp(y)*dy.*(y<10)) should equal mu0   
   
   alpha=p/rhoD/mu0*fwhm^D;
   
   % Integrate the density to get the p-value for one cluster: 
   pS=cumsum(ff(ny:-1:1))*dy;
   pS=pS(ny:-1:1);
   % The number of clusters is Poisson with mean EL:
   pSmax=1-exp(-pS*EL);
   
   if p_val_extent(1) <= tlim 
      yval=minterp1(-pSmax,y,-p_val_extent);
      % Spatial extent is alpha*exp(Y) -dy/2 correction for mid-point rule:
      extent_threshold=alpha*exp(yval-dy/2);
      % For a single cluster:
      yval=minterp1(-pS,y,-p_val_extent);
      extent_threshold_1=alpha*exp(yval-dy/2);
      if length(p_val_extent)<=nprint
         extent_threshold
         extent_threshold_1
      end
   else
      % p_val_extent is now treated as a spatial extent:
      P_val_extent=interp1(y,pSmax,log(p_val_extent/alpha)+dy/2);
      extent_threshold=P_val_extent;
      % For a single cluster:
      P_val_extent_1=interp1(y,pS,log(p_val_extent/alpha)+dy/2);
      extent_threshold_1=P_val_extent_1;
      if length(p_val_extent)<=nprint
         P_val_extent
         P_val_extent_1
      end
   end
   
end
   
return

function iy=minterp1(x,y,ix);
% interpolates only the monotonically increasing values of x at ix
n=length(x);
mx=x(1);
my=y(1);
xx=x(1);
for i=2:n
   if x(i)>xx
      xx=x(i);
      mx=[mx xx];
      my=[my y(i)];
   end
end
iy=interp1(mx,my,ix);
return


