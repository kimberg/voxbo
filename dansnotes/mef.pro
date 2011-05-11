
;**********
;
; MakeExoFilt
;
;   Produces the time domain representation of the filter used to
;     smooth the data in time. Can include an IRF representation
;     and/or a notch filter. Note that only the magnitude component
;     is returned -- all phase information is discarded as this is
;     not to be included in the K matrix (any delay component is
;     modeled in the G matrix)
;
;   Inputs:
;
;     OrderG - the length of the vector to be returned
;
;     IRFName - name of time-domain filter to be added
;
;     Notch - array of two integer values indicating the number
;         of low and high frequencies to remove, respectively.
;
;   Outputs:
;
;     An array of floating values that contains the time-domain
;         representation of the exogenous filter
;

FUNCTION MakeExoFilt,OrderG,TRIn, $
      IRFName=IRFName, Notch=Notch, $
      ErrorFlag=ErrorFlag, NOGUI=NOGUI, QUIET=QUIET

  if n_elements(TRIn) le 0 then TRIn=2000
  TR=TRIn/1000.   ; Now in seconds

  ExoFilt=fltarr(OrderG)
  ExoFilt(0)=1.

;***
; if the Notch keyWord is set, zero the frequencies below and above
;   the identified points
;***

  if KeyWord_Set(Notch) then begin
  
    NotchFilt=fltarr(OrderG)+1.
    NotchFilt(0)=0.
  
    if Notch(0) gt 0 then begin
      NotchFilt(1:Notch(0))=0
      NotchFilt(OrderG-1-Notch(0):OrderG-1)=0
    endif
    
    if Notch(1) gt 0 then begin
      if fix(OrderG) mod 2 eq 0 then begin
        NotchFilt(OrderG/2-(Notch(1)-1):OrderG/2+(Notch(1)-1))=0
      endif else begin
        NotchFilt(fix(OrderG/2)-Notch(1):fix(OrderG/2)+(Notch(1)-1))=0
      endelse
    endif
    ExoFilt=float(fft((fft(ExoFilt,-1)*NotchFilt),1))
  endif

;***
; Mean center the response
;***
  ExoFilt=ExoFilt-total(ExoFilt)/OrderG

;***
; Remove any phase component
;***

  ExoFilt= float(fft(sqrt(returnps(ExoFilt)),1))

;***
; Normalize the vector so that the maximum magnitude component is unity
;***

  ExoFilt=NormMag(ExoFilt)
  
  return,ExoFilt
  
end



FUNCTION ReturnPS, Signal
  return,float(fft(Signal,-1)*conj(fft(Signal,-1)))
end


;*******************
;
; NormMag
;
; Image is an array of floating values
;
; The routine normalizes the magnitude component of a signal to unity
;   while preserving the phase component.
;

FUNCTION NormMag, Image
  TwoPi = (!PI) * 2.0
  FTImage=fft(Image,-1)

;***
; Note that the magnitude of an complex number is the
; sqrt of ( the number times its complex conjugate)
;***

  MagImage=sqrt(float(FTImage*conj(FTImage)))

;***
; Note that the phase angle will be undefined
;   where the magnitude image equals zero
; 
; We get around this by temporarily setting
;   these zero values to unitary values.
;***

  ZeroPoints=where(MagImage eq 0,ZeroCount)
  if ZeroCount gt 0 then MagImage(ZeroPoints)=1.

;***
; Because the domain of the arcsine and arccosine
; "functions" is limited, we must check the signs of the
; imaginary and real components to compensate
;***
print,"real"
print,float(FTImage)
print,magimage

  PhaseImage = acos(float(FTImage)/MagImage)
  
  NegVals = where(imaginary(FTImage) lt 0,NegCount)
  if NegCount gt 0 then PhaseImage(NegVals) = TwoPi-PhaseImage(NegVals)

  if ZeroCount gt 0 then PhaseImage(ZeroPoints)=0
  if ZeroCount gt 0 then MagImage(ZeroPoints)=0

print,magimage
print,phaseimage

;***
; Normalize the Magnitude Image and convert back to image space
;***

  MagImage=MagImage/max(MagImage)
  
  RealPart=MagImage*cos(PhaseImage)
  ImaginaryPart=MagImage*sin(PhaseImage)
  FTImage=complex(RealPart,ImaginaryPart)
  Image=float(fft(FTImage,1))

  return,Image

end


