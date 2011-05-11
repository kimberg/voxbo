

// SincRezize re-implements the old IDL sincinterpo function

ColumnVector
SincResize(ColumnVector vec,double factor)
{
  if (factor == 1.0)
    return vec;
  ColumnVector newcv((int)(vec.length()*factor));
  if (factor < 1) {
    for (int i=0; i<newcv.length(); i++)
      newcv(i)=InterpolatedElement(vec,(double)i/factor);
    return newcv;
  }

  float timeshift;
  int length,i,j,k;
  int lastfront,firstback,midpoint;
  double twopi=atan(1.0)*8.0;

  length=vec.length();
  if (vec.length()%2) {
    midpoint=lastfront=length/2;
    firstback=length/2+1;
  }
  else {
    lastfront=length/2-1;
    midpoint=length/2;
    firstback=length/2+1;
  }

  for (i=0; i<factor; i++) {
    timeshift=(float)i/factor;
    ColumnVector phi(length);
    phi(0)=0.0;
    for (int f=1;f<=midpoint; f++)
      phi(f)=timeshift*twopi/((float)length/float(f));
    for (j=lastfront,k=firstback; k<length; j--,k++)
      phi(k)=0.0-phi(j);
    ComplexColumnVector shifter(phi.length());
    for (j=0; j<phi.length(); j++)
      shifter(j)=Complex(cos(phi(j)),sin(phi(j)));
    ComplexMatrix fftsig=((ComplexMatrix)vec).fourier();
    fftsig = product(fftsig,(ComplexMatrix)shifter);
    ComplexMatrix shiftedsignal=fftsig.ifourier();
    for (j=0; j<length; j++)
      newcv((int)(j*factor+i))=shiftedsignal(j,0).real();
  }
  return newcv;
}

double
InterpolatedElement(const ColumnVector &vec,double ind)
{
  double proportion=ind-floor(ind);
  int low=(int)floor(ind);
  if (low < 0)
    return vec(0);
  if (low > vec.length()-2)
    return vec(vec.length()-1);
  return (vec(low)*(1-proportion))+(vec(low+1)*proportion);
}

