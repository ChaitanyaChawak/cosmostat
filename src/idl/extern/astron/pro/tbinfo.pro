pro tbinfo,h,tb_str
;+
; NAME:
;       TBINFO
; PURPOSE:
;       Return informational structure from a FITS binary table header.
;
; CALLING SEQUENCE:
;       tbinfo, h, tb_str
; INPUTS:
;       h - FITS binary table header, e.g. as returned by READFITS()
;
; OUTPUTS:
;       tb_str - IDL structure with extracted info from the FITS binary table
;               header.   Tags include
;       .tbcol - starting column position in bytes, integer vector
;       .width - width of the field in bytes, integer vector
;       .idltype - idltype of field, byte vector
;               7 - string, 4- real*4, 3-integer*4, 5-real*8
;       .numval - repeat count, longword vector
;       .tunit - string unit numbers, string vector
;       .tnull - null value for the field, string vector
;       .tform - format for the field, string vector
;       .ttype - field name, string vector
;       .maxval- maximum number of elements in a variable length array, long
;               vector
;       .tscale - scale factor for converting to physical values, default 1.0
;       .tzero - additive offset for converting to physical values, default 0.0
;       .tdisp - recommended output display format
;
;       All of the output vectors will have same number of elements, equal
;       to the number of columns in the binary table
; SIDE EFFECTS:
;       If there are difficulties interpreting the table then !ERR is set 
;       to -1
; PROCEDURES USED:
;       SXPAR()
; NOTES:
;       For variable length ('P' format) column, TBINFO returns values for
;       reading the 2 element longward array of pointers (numval=2, 
;       idltype = 3, width=4)
; HISTORY:
;       Major rewrite to return a structure      W. Landsman   August 1997
;       Release for IDL V5.0   August 1997
;       Converted to IDL V5.0   W. Landsman   September 1997
;-
;----------------------------------------------------------------------------
 On_error,2
 if N_params() LT 2 then begin
        print,'Syntax - TBINFO, h, tb_str'
        return
 endif

; get number of fields

 tfields = sxpar( h, 'TFIELDS', COUNT = N_TFields)
 if N_TFields LT 0 then $
        message,'Invalid FITS header. keyword TFIELDS is missing'

 if tfields EQ 0 then begin     ;ANY fields in table?
                !ERR = -1
                return
 endif

; Create output arrays with default values

 idltype = intarr(tfields) & tnull = idltype
 numval = lonarr(tfields) & tbcol = numval & width = numval & maxval = numval
 tunit = replicate('',tfields) & ttype = tunit & tdisp = tunit & tnull = tunit
 tscal = replicate(1.0, tfields) 
 tzero = replicate(0.0, tfields)

 type = sxpar(h,'TTYPE*', COUNT = N_ttype)
 if N_ttype GT 0 then ttype[0] = strtrim(type,2) 

 tform = strtrim( sxpar(h,'tform*', COUNT = N_tform), 2)     ; column format
 if N_tform EQ 0 then $
        message,'Invalid FITS table header -- keyword TFORM not present'

 tform =  strupcase(strtrim(tform,2))
                                                
 unit = sxpar(h, 'TUNIT*', COUNT = N_tunit)     ;physical units
 if N_tunit GT 0 then tunit[0] = unit

 null = sxpar(h, 'TNULL*', COUNT = N_tnull)      ;null data value
 if N_tnull GT 0 then tnull[0] = null

 scale = sxpar(h,'TSCAL*', COUNT = N_tscal)     ;Scale factor
 if N_tscal GT 0 then begin                     ;Special check needed because
        bad = where(scale EQ 0.0, Nbad)         ;TSCAL default is 1.0 not 0.0
        if Nbad GT 0 then scale[bad] = 1.0
        tscal[0] = scale
 endif

 zero = sxpar(h, 'TZERO*', COUNT = N_tzero)      ;Offset scale value
 if N_tzero GT 0 then tzero[0] = zero

 disp = sxpar(h,'TDISP*', COUNT = N_tdisp)       ;Display format string
 if N_tdisp GT 0 then tdisp[0] = disp

; determine idl data type from format

 len = strlen(tform)

 for i = 0, N_elements(tform)-1 do begin

; Step through each character in the format, until a non-numerical character
; is encountered

        ichar = 0
NEXT_CHAR:
        if ichar GE len[i] then message, $
           'Invalid format specification for keyword TFORM ' + strtrim(i+1)
        char = strupcase( strmid(tform[i],ichar,1) )
        if ( (char GE '0') and ( char LE '9')) then begin
                ichar = ichar + 1
                goto, NEXT_CHAR
        endif

        if ichar EQ 0 then numval[i] = 1 else $
        numval[i] = strmid( tform[i], 0, ichar )

        if char EQ "P" then begin            ;Variable length array?
                char = strupcase( strmid(tform[i],ichar+1,1) )
                maxval[i] = long( strmid(tform[i],ichar+3, len[i]-ichar-4) )
                width[i] = 4  & numval[i] = 2  & idltype[i] = 3
        endif else begin

        tform[i] =  char

        case strupcase( tform[i] ) of

        'A' : begin & idltype[i] = 7 &  width[i] = 1 &  end
        'I' : begin & idltype[i] = 2 &  width[i] = 2 &  end
        'J' : begin & idltype[i] = 3 &  width[i] = 4 &  end
        'E' : begin & idltype[i] = 4 &  width[i] = 4 &  end
        'D' : begin & idltype[i] = 5 &  width[i] = 8 &  end
        'L' : begin & idltype[i] = 1 &  width[i] = 1 &  end
        'B' : begin & idltype[i] = 1 &  width[i] = 1 &  end
        'C' : begin & idltype[i] = 6 &  width[i] = 8 &  end
        'M' : begin & idltype[i] = 9 &  width[i] =16 &  end
;  Treat bit arrays as byte arrays with 1/8 the number of elements.

        'X' : begin
              idltype[i] = 1
              numval[i] = long((numval[i]+7)/8)
              width[i] = 1
              end

        else : message,'Invalid format specification for keyword ' + $
                        'TFORM'+ strtrim(i+1,2)
 endcase
 endelse

 if i ge 1 then tbcol[i] = tbcol[i-1] + width[i-1]*numval[i-1]

 endfor
 tb_str = {TBCOL:tbcol,WIDTH:width,IDLTYPE:idltype,NUMVAL:numval,TUNIT:tunit,$
           TNULL:tnull,TFORM:tform,TTYPE:ttype,MAXVAL:maxval, TSCAL:tscal, $
           TZERO:tzero, TDISP:tdisp}
 return
 end
