pro ftab_print,filename,columns,rows, TEXTOUT = textout, FMT = fmt, $
        EXTEN_NO = exten_no
;+
; NAME:
;       FTAB_PRINT
; PURPOSE:
;       Print the contents of a FITS (binary or ASCII) table extension.
; EXPLANATION:
;       User can specify which rows or columns to print
;
; CALLING SEQUENCE:
;       FTAB_PRINT, filename, columns, rows, [ TEXTOUT=, FMT=, EXTEN_NO=]
;
; INPUTS:
;       filename - scalar string giving name of a FITS file containing a 
;               binary or ASCII table
;       columns - string giving column names, or vector giving
;               column numbers (beginning with 1).  If string 
;               supplied then column names should be separated by comma's.
;       rows - (optional) vector of row numbers to print (beginning with 0).  
;               If not supplied or set to scalar, -1, then all rows
;               are printed.
; OPTIONAL KEYWORD INPUT:
;       EXTEN_NO - Extension number to read.   If not set, then the first 
;               extension is printed (EXTEN_NO=1)
;       TEXTOUT - scalar number (0-7) or string (file name) determining
;               output device (see TEXTOPEN).  Default is TEXTOUT=1, output 
;               to the user's terminal    
;       FMT = Format string for print display (binary tables only).   If not
;               supplied, then any formats in the TDISP keyword fields will be
;               used, otherwise IDL default formats.    For ASCII tables, the
;               format used is always as stored in the FITS table.
; EXAMPLE:
;       Print all rows of the first 5 columns of the first extension of the
;       file 'wfpc.fits'
;               IDL> ftab_print,'wfpc.fits',indgen(5)+1
;       
; SYSTEM VARIABLES:
;       Uses the non-standard system variables !TEXTOUT and !TEXTUNIT
;       which must be defined (e.g. with ASTROLIB) prior to compilation.
; PROCEDURES USED:
;       FITS_OPEN, FITS_READ, FTPRINT, TBPRINT
; HISTORY:
;       version 1  W. Landsman    August 1997
;       Converted to IDL V5.0   W. Landsman   September 1997
;-
;----------------------------------------------------------------------
 if N_params() LT 1 then begin
        print,'Syntax - ftab_print, filename, columns, rows,' 
        print,'              [EXTEN_NO=, FMT= , TEXTOUT=  ]'
        return
 endif

 if not keyword_set(exten_no) then exten_no = 1

 fits_open,filename,fcb
 fits_read,filename,tab,htab,exten_no=exten_no
 ext_type = fcb.xtension[exten_no]

 case ext_type of
 'A3DTABLE': binary = 1b
 'BINTABLE': binary = 1b
 'TABLE': binary = 0b
 else: message,'ERROR - Extension type of ' + $
                ext_type + 'is not a FITS table format'
 endcase

 if binary then tbprint,htab,tab,columns,rows, TEXTOUT = textout,fmt=fmt $
           else ftprint,htab,tab,columns,rows, TEXTOUT = textout
 return
 end
