FUNCTION CONS_RA,RA,Y,ASTR, DELTA      ;Find line of constant RA
;+
; NAME:
;       CONS_RA
; PURPOSE:
;       Obtain the X and Y coordinates of a line of constant right ascension
; EXPLANATION:
;       Return a set of X pixel values given an image with astrometry, 
;       and either
;       (1) a set of Y pixel values, and a scalar right ascension, or
;       (2) a set of right ascension values, and a scalar Y value.
;
;       In usage (1), CONS_RA can be used to determine the (X,Y) values
;       of a line of constant right ascension.  In usage (2), CONS_RA can
;       used to determine the X positions of specified RA values, along a
;       line of constant Y.
;
; CALLING SEQUENCE:
;       X = CONS_RA( RA, Y, ASTR, [ DEC] )
;
; INPUTS:         
;       RA -  Right Ascension value in DEGREES (0 < RA < 360.).  If Y is a
;               vector, then RA must be a scalar
;       Y -   Specified Y pixel value(s) for line of constant right ascension
;               If RA is a vector, then Y must be a scalar
;       ASTR - Astrometry structure as extracted from a FITS header by the 
;               procedure EXTAST
; OUTPUTS
;       X   - Computed set of X pixel values.   The number of elements of X
;               is the maximum of the number of elements of RA and Y.
; OPTIONAL OUTPUT:
;       DEC - Computed set of declinations (in DEGREES) for X,Y, coordinates
; NOTES:
;       The algorithm (and notation) is based on AIPS Memo 27 by Eric Greisen,
;       with modifications for a coordinate description (CD) matrix as 
;       described in Paper II of Greisen & Calabretta (2000, A&A, in press).
;       These documents are available from 
;       http://fits.cv.nrao.edu/documents/wcs/wcs.html
;
; RESTRICTIONS:
;       Implemented only for the TANgent and SIN projections 
;
; REVISION HISTORY:
;       Written, Wayne Landsman  STX Co.        April, 1988
;       Algorithm adapted from AIPS memo No. 27 by Eric Griessen
;       New astrometry structure
;       Converted to IDL V5.0   W. Landsman   September 1997
;       Added SIN projection    W. Landsman   January 2000
;       Fix possible sign error introduced Jan. 2000   W. Landsman  May 2000
;-
  On_error,2

  if ( N_params() LT 3 ) then begin
        print,'Syntax - X = CONS_RA( RA, Y, ASTR, [ Dec ])'
        return, 0
  endif

  radeg = 180.0/!DPI
  n = N_elements(y)
  crpix = astr.crpix - 1.
  crval = astr.crval/RADEG
  cdelt = astr.cdelt
  cdelta = [ [ cdelt[0], 0.],[0., cdelt[1] ] ]
  cd = astr.cd/RADEG
  cdel0 = cos( crval[1] )  &    sdel0 = sin( crval[1] )
  delra = ra/RADEG - crval[0]
  cdelra = cos( delra )    &    sdelra = sin( delra )

  ctype = strupcase( strmid(astr.ctype[0], 5,3))
  case ctype of 
  
  'TAN': begin
 
  cdi = invert( cdelta # cd )     ;Griesen uses invert of CD matrix
  yy = y - ( crpix[1])    ;New coordinate origin, Unit pixel offset in CRPIX
  delta = atan( sdel0*cdelra*cdi[1,1] - sin(delra)*cdi[1,0] + yy*cdelra*cdel0, $
              cdel0*cdi[1,1] - yy*sdel0)
  end
  'SIN': begin

  A = -cd[0,0]*cdelt[0] 
  B = -cd[0,1]*cdelt[0] 
  C =  cd[1,0]*cdelt[1]
  D =  cd[1,1]*cdelt[1] 
  yy = (y - crpix[1])*(b*c - a*d)   ;New coordinate origin
  aa = cdel0*d
  bb = sdel0*cdelra*d + sdelra*b
  denom = sqrt(aa^2 + bb^2)
  delta = atan(bb/aa)  + asin(yy/denom)

  end

  ELSE: message,'ERROR - Program only works for TAN and SIN projections'
  endcase

  delta = delta*RADEG
  if (N_elements(ra) EQ 1) and (n GT 1) then $
  ad2xy, replicate(ra,n), delta, astr, x else $
  ad2xy, ra, delta, astr, x

  return, x
  end
