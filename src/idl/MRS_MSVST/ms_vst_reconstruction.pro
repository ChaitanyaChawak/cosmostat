;+
; NAME:
;        ms_vst_reconstruction
;
; PURPOSE:
;	Denoises a Poisson image with the MS-VST method.
;
;
; CALLING:
;
;    ms_vst_reconstruction,image,image_reconstruite,NbrScale=NbrScale,niter=niter,HSD=HSD,coef_seuil=coef_seuil,
;    coef_pos=coef_pos,First_Scale=First_Scale,mask=mask,filter=filter,pyr=pyr,background=background,expo=expo
;       
;
; INPUTS:
;     image -- IDL array of healpix map: Input image to be denoised
;     optional : background : if set, substracts a background to the image
;     optional : supentree : if set, use a given multi-resolution support instead of computing it with the procedure ms_vst_tests_hypotheses
;    
; OUTPUTS:
;     image_reconstruite -- IDL array of heapix denoised image
;     optional : support -- multi-resolution support of the image
;     optional : back_reconstruit : if set, returns the reconstructed background (need the keyword separation)
;     
;
; KEYWORDS:
;      NbrScale  : Number of scales (default is 4)
;      niter  : Number of iterations
;      HSD  : if set, the denoised image will be recontructed using the Hybrid Steepest Descent Method (soft thresholding at each 
;     iteration of the reconstruction)
;      coef_seuil  : determines the threshold for the detection of significant coefficients.
;     For each scale i, the threshold is set to coef_seuil*sigma[i] (default is 5)
;      coef_pos  : if set, negative wavelets coefficients are set to 0.
;      First_Scale  : if > 2, finer wavelet scales are set to 0. (default is 1)
;      mask  : if set, enables impainting with the given mask
;      filter  : if set, the inverse wavelet transform will be computed using filters. Else, it will be obtained by a simple addition ;     of all wavelet scales.
;      pyr  : if set, use pyramidal wavelet transform for the soft thresholding
;      expo  : if set, decreases the thresold exponentially at each step of the HSD. Else, decreases the threshold linearly
;      alm : if set, thresholding is made on alm coefficients instead of wavelet coefficients
;      curvelets : if set, thresholding is made on curvelets coefficients instead of wavelet coefficients
;      separation : if set, compute separately the sources and the background
;      update_support : if set, update the multi-resoluation support at each iteration
;      split_support : if set, splits the multi-resolution support
;      
;
;
; EXTERNAL CALLS:
;       ms_vst_test_hypotheses 
;       mrs_wttrans
;       mrs_wtrec
;       mrs_pwttrans
;       mrs_pwtrec
;
; EXAMPLE:
;
;       Compute the denoising of an image I with default options
;        The result is stored in Output
;               ms_vst_recontruction, Imag, Output, NbrScale=6, /hsd
;
;       Compute the denoising of an image I with background extraction and update of multi-resolution_support
;        The result is stored in Output
;               ms_vst_recontruction, Imag, Output, NbrScale=6, /hsd, background=background, /update_support
;
;       Compute the denoising + inpainting of an image I with missing data
;        The result is stored in Output
;               ms_vst_recontruction, Imag, Output, NbrScale=6, /hsd, niter=50, mask=mask
;         
; HISTORY:
;	Written: J�r�my Schmitt & Jean-Luc Starck, 2009
;	February, 2009 File creation
;--------------------------------------------------------------------------------------------------------

function partie_positive,y
taille=size(y)
taille=taille[1]

pos=fltarr(taille)
for j=ulong(0),taille-1 do begin
 if (y[j] LE 0) then pos[j]=0 else pos[j]=y[j]
endfor
return,pos

end


pro ms_vst_reconstruction,image,image_reconstruite,NbrScale=NbrScale,niter=niter,HSD=HSD,coef_seuil=coef_seuil,coef_pos=coef_pos,First_Scale=First_Scale,mask=mask,filter=filter,pyr=pyr,background=background,expo=expo,alm=alm,curv=curv,separation=separation,back_reconstruit=back_reconstruit,update_support=update_support,supentree=supentree,split_support=split_support,support

if not keyword_set(niter) then niter=5
if not keyword_set(NbrScale) then NbrScale=4
if not keyword_set(coef_seuil) then coef_seuil=5
if not keyword_set(coef_pos) then coef_pos=0
if not keyword_set(First_Scale) then First_Scale=1
if keyword_set(mask) then expo=1
if keyword_set(mask) then hsd=1
if keyword_set(pyr) then hsd=1
;if keyword_set(update_support) then separation=1

SizeImage=size(image)
SizeImage=SizeImage[1]

niter=double(niter)

if keyword_set(update_support) then print,'updatesup' else print,'not updatesup'

if keyword_set(background) then mrs_msvsts_iuwt_transform,background,background_vst,NbrScale=NbrScale

if not keyword_set(supentree) then begin
ms_vst_tests_hypotheses,image,image_vst,support,image_rec,NbrScale=NbrScale,coef_seuil=coef_seuil,First_Scale=First_Scale,background=background
;print,'supentree off'
endif else begin
support=supentree
;print,'supentree on'
endelse

calcul_vst,NbrScale,c,b,h,tau1,tau2,tau3,sigma

;if keyword_set(mask) then begin
; mrs_wttrans,image,imtrans,NbrScale=NbrScale
; seuil_max = max(imtrans.coef)
;endif

mrs_wttrans,image,imtrans,NbrScale=NbrScale
seuil_max = max(imtrans.coef)

;reconstruction it�rative

;initialisation
x=fltarr(sizeimage,niter+1)
x[*]=0

xx=fltarr(sizeimage,niter+1)
xx[*]=0
image_reconstruite=fltarr(sizeimage)
if keyword_set(separation) then begin
 b=fltarr(sizeimage,niter+1)
 b[*]=0
 if keyword_set(background) then b[*,0]=background
endif

support_original=support

;it�ration
for i=0.,niter-1 do begin
 print,'n=',i
 
 if keyword_set(split_support) then begin
  supportx=support
  supportb=support
  for i=0,nbrscale-5 do begin
   supportb[*,i]=0
  endfor
  for i=nbrscale-4,nbrscale-2 do begin
   supportx[*,i]=0
  endfor
 ;supportx[*,nbrscale-2]=0
 endif
 
 if keyword_set(separation) then begin
  if keyword_set(split_support) then begin
   residu=image-x[*,i]-b[*,i]
   mrs_wttrans,residu,residu_trans,NbrScale=NbrScale
   residu_trans_signif=residu_trans.coef
   residu_trans_signif[*,0:NbrScale-2]=residu_trans_signif[*,0:NbrScale-2]*supportx[*,0:NbrScale-2]
   residu_trans_signif[*,NbrScale-1]=0
   residu_trans.coef=residu_trans_signif
   if keyword_set(filter) then mrs_wtrec,residu_trans,terme,/filter else mrs_wtrec,residu_trans,terme
   x[*,i+1]=x[*,i]+terme
   
   residub=image-x[*,i+1]-b[*,i]
   mrs_wttrans,residub,residub_trans,NbrScale=NbrScale
   residu_trans_back=residub_trans.coef
   residu_trans_back[*,0:NbrScale-2]=residu_trans_signif[*,0:NbrScale-2]*supportb[*,0:NbrScale-2]
   residub_trans.coef=residu_trans_back
   if keyword_set(filter) then mrs_wtrec,residub_trans,termeback,/filter else mrs_wtrec,residub_trans,termeback
   b[*,i+1]=b[*,i]+termeback
   
  endif else begin
   residu=image-x[*,i]-b[*,i]
   mrs_wttrans,residu,residu_trans,NbrScale=NbrScale
   residu_trans_signif=residu_trans.coef
   residu_trans_signif[*,0:NbrScale-2]=residu_trans_signif[*,0:NbrScale-2]*support[*,0:NbrScale-2]
   residu_trans_signif[*,NbrScale-1]=0
 ; residu_trans_back=residu_trans.coef
 ; residu_trans_back[*,0:NbrScale-2]=0
  
   residu_trans.coef=residu_trans_signif
   if keyword_set(filter) then mrs_wtrec,residu_trans,terme,/filter else mrs_wtrec,residu_trans,terme
   x[*,i+1]=x[*,i]+terme
   xx[*,i+1]=x[*,i+1]
  
   residub=image-x[*,i+1]-b[*,i]
   mrs_wttrans,residub,residub_trans,NbrScale=NbrScale
   residu_trans_back=residub_trans.coef
   residu_trans_back[*,0:NbrScale-2]=0
   residub_trans.coef=residu_trans_back
   if keyword_set(filter) then mrs_wtrec,residub_trans,termeback,/filter else mrs_wtrec,residub_trans,termeback
   b[*,i+1]=b[*,i]+termeback
  endelse

 endif else begin
  if keyword_set(background) then residu=image-background-x[*,i] else residu=image-x[*,i]
  if keyword_set(mask) then residu=residu*mask
  ;if keyword_set(mask) then residu=residu*mask+x[*,i]
  mrs_wttrans,residu,residu_trans,NbrScale=NbrScale
  residu_trans_signif=residu_trans.coef
  residu_trans_signif[*,0:NbrScale-2]=residu_trans_signif[*,0:NbrScale-2]*support[*,0:NbrScale-2]
  if (coef_pos NE 0) then begin
   for k=0,NbrScale-1 do residu_trans_signif[*,k]=partie_positive(residu_trans_signif[*,k])
  end
 
; if keyword_set(HSD) then begin
;  seuil=(niter-(i+1))/(niter-1)
;  for l=0,NbrScale-2 do begin
;   for m=ulong(0),sizeimage-1 do begin
;    if (abs(residu_trans_signif[m,l]) LE sigma(residu_trans_signif[*,l])*seuil) then residu_trans_signif[m,l]=0 else residu_trans_signif[m,l]=residu_trans_signif[m,l]-sigma(residu_trans_signif[*,l])*seuil*sgn(residu_trans_signif[m,l])
;   endfor
;  endfor
; end
 
 ;terme=total(residu_trans_signif,2)
  residu_trans.coef=residu_trans_signif
  if keyword_set(filter) then mrs_wtrec,residu_trans,terme,/filter else mrs_wtrec,residu_trans,terme
; x[*,i+1]=partie_positive(x[*,i]+terme)
  x[*,i+1]=x[*,i]+terme
; x[*,i+1]=terme

;print,'x calcul�'
 
; if keyword_set(HSD) then begin
;  seuil=mean(image)*(niter-(i+1))/(niter-1)
;  for j=ulong(0),sizeimage-1 do begin
;   if (abs(x[j,i+1]) LE seuil) then x[j,i+1]=0 else x[j,i+1]=x[j,i+1]-seuil*sgn(x[j,i+1])
;  endfor
; end
 endelse
 
 ;x[*,i+1]=partie_positive(x[*,i+1])
 ;if keyword_set(separation) then b[*,i+1]=partie_positive(b[*,i+1])
 
 if keyword_set(HSD) then begin
  ;print,'debut calcul seuil'
  ;if keyword_set(expo) then seuil=seuil_max*exp(-i) else seuil=(niter-(i+1.))/(niter-1.)*5*sqrt(mean(image))
  if keyword_set(expo) then seuil=seuil_max*(exp((niter-(i+1.))/(niter-1.))-1.) else seuil=0.1*(niter-(i+1.))/(niter-1.)
  ;if keyword_set(expo) then seuil=seuil_max*exp(-i) else seuil=1*(niter-(i+1.))/(niter-1.)
  ;if keyword_set(expo) then seuil=seuil_max*(niter-(i+1.))/(niter-1.) else seuil=1.*(niter-(i+1.))/(niter-1.)
  ;if keyword_set(expo) then seuil=seuil_max*(niter-(i+1.))/(niter-1.) else seuil=0.5*(niter-(i+1.))/(niter-1.)
  print,'seuil=',seuil
  ;seuil=seuil_max*exp(-i)
  ;print,'seuil calcul�'
  
  if keyword_set(pyr) then begin
  ima=x[*,i+1]
   mrs_pwttrans,ima,w,NbrScale=NbrScale
   for l=0,NbrScale-2 do begin
    my_command='echelle=w.scale'+strcompress(string(l+1), /remove_all)
    ACK = execute(my_command)
    sig=sigma(echelle)
    sizeech=size(echelle)
    sizeech=sizeech[1]
    ;for m=ulong(0),sizeech-1 do begin
    ; if (abs(echelle[m]) le sig*seuil) then echelle[m]=0 else echelle[m]=echelle[m]-sig*seuil*sgn(echelle[m])
    ;endfor
    thresh=sig*seuil
    softthreshold,echelle,thresh
    my_command='w.scale'+strcompress(string(l+1), /remove_all)+'=echelle'
    ACK=execute(my_command)
   endfor
   mrs_pwtrec,w,wrec
   x[*,i+1]=wrec
  
  endif else begin
   if keyword_set(mask) then begin
    if keyword_set(alm) then begin
     ima=x[*,i+1]
     mca_proj_alm,ima,seuil,sigmanoise=1.
     x[*,i+1]=ima
    endif else begin
     if keyword_set(curv) then begin
      ima=x[*,i+1]
      mca_proj_cur,ima,seuil,sigmanoise=1.
      x[*,i+1]=ima
     endif else begin
      ima=x[*,i+1]
      mca_proj_wave,ima,seuil,sigmanoise=1.,nbrscale=nbrscale
      x[*,i+1]=ima
     endelse
    endelse
   endif else begin
    mrs_wttrans,x[*,i+1],w,NbrScale=NbrScale
    if keyword_set(split_support) then mrs_wttrans,b[*,i+1],wb,NbrScale=NbrScale
    for l=0,NbrScale-2 do begin
    ;for l=0,NbrScale-1 do begin
    sig=sigma(w.coef[*,l])
    if keyword_set(split_support) then sigb=sigma(wb.coef[*,l])
    ;print,'echelle',l
     ;for m=ulong(0),sizeimage-1 do begin
     ;print,'point',m
     ; if (abs(w.coef[m,l]) LE sig*seuil) then w.coef[m,l]=0 else w.coef[m,l]=w.coef[m,l]-sig*seuil*sgn(w.coef[m,l])
     ;endfor
     thresh=sig*seuil
     if keyword_set(split_support) then threshb=sigb*seuil
     ;thresh=seuil
     coeff=w.coef[*,l]
     softthreshold,coeff,thresh
     w.coef[*,l]=coeff
     if keyword_set(split_support) then begin
      coeffb=wb.coef[*,l]
      softthreshold,coeffb,threshb
      wb.coef[*,l]=coeffb
     endif
    endfor
    ;x[*,i+1]=total(w.coef,2)
    ;if keyword_set(filter) then mrs_wtrec,w,wrec,/filter else mrs_wtrec,w,wrec
    mrs_wtrec,w,wrec
    x[*,i+1]=wrec
    if keyword_set(split_support) then begin
     mrs_wtrec,wb,wbrec
     b[*,i+1]=wbrec
    endif
   endelse
  endelse
 end
 
 x[*,i+1]=partie_positive(x[*,i+1])
 ;if keyword_set(separation) then b[*,i+1]=partie_positive(b[*,i+1])
 
 if keyword_set(update_support) then begin
  xi=x[*,i+1]
  ;br=image-x[*,i+1]
  ;ms_vst_tests_hypotheses,br,brvst,sup,tjaj,tot,brrec,NbrScale=NbrScale,coef_seuil=coef_seuil,First_Scale=First_Scale,background=background
  ;ms_vst_tests_hypotheses,br,brvst,sup,tjaj,tot,brrec,NbrScale=NbrScale,coef_seuil=coef_seuil,First_Scale=First_Scale
  ;ms_vst_tests_hypotheses,br,brvst,sup,tjaj,tot,brrec,NbrScale=NbrScale,coef_seuil=coef_seuil,First_Scale=2
  ;ms_vst_tests_hypotheses,xi,xivst,sup,tjaj,tot,xirec,nbrscale=nbrscale,coef_seuil=coef_seuil,First_Scale=First_Scale
  ms_vst_tests_hypotheses,xi,xivst,sup,xirec,nbrscale=nbrscale,coef_seuil=coef_seuil,First_Scale=First_Scale
  rvst=image_vst
  rvst.coef[*]=image_vst.coef-xivst.coef
  ;support=support+sup
  ;ind=where(support GT 1,count)
  ;if count NE 0 then support[ind]=1
  for scale=1,NbrScale-2 do begin
   if keyword_set(background) then begin
    ind=where(abs(rvst.coef[*,scale]) - abs(background_vst.coef[*,scale]) GT coef_seuil*sigma[scale],taillesup)
    if taillesup GT 0 then Support[ind,scale]=1
   endif else begin ind=where(abs(rvst.coef[*,scale]) GT coef_seuil*sigma[scale],taillesup)
    if taillesup GT 0 then Support[ind,scale]=1
   endelse
  endfor
 end
 
 if keyword_set(separation) then begin
  save,filename='resultat_reconstruction.xdr',image,x,xx,b,image_vst,support,NbrScale,niter,support_original
 endif else begin
  save,filename='resultat_reconstruction.xdr',image,x,image_vst,support,NbrScale,niter
 endelse
endfor

image_reconstruite=x[*,niter]
if keyword_set(separation) then back_reconstruit=b[*,niter]

;if keyword_set(supupdate) then supupdate=support

if keyword_set(update_support) then begin
 ms_vst_tests_hypotheses,image_reconstruite,xivst,sup,xirec,nbrscale=nbrscale,coef_seuil=coef_seuil,First_Scale=First_Scale
  rvst=image_vst
  rvst.coef[*]=image_vst.coef-xivst.coef
  ;support=support+sup
  ;ind=where(support GT 1,count)
  ;if count NE 0 then support[ind]=1
  for scale=1,NbrScale-2 do begin
   if keyword_set(background) then begin
    ind=where(abs(rvst.coef[*,scale]) - abs(background_vst.coef[*,scale]) GT coef_seuil*sigma[scale],taillesup)
    if taillesup GT 0 then Support[ind,scale]=1
   endif else begin ind=where(abs(rvst.coef[*,scale]) GT coef_seuil*sigma[scale],taillesup)
    if taillesup GT 0 then Support[ind,scale]=1
   endelse
  endfor
  ms_vst_reconstruction,image,image_reconstruite,NbrScale=NbrScale,niter=niter,HSD=HSD,coef_seuil=coef_seuil,coef_pos=coef_pos,First_Scale=First_Scale,mask=mask,filter=filter,pyr=pyr,background=background,expo=expo,alm=alm,curv=curv,supentree=support
end
 

if keyword_set(separation) then begin 
 save,filename='resultat_reconstruction.xdr',image,x,xx,b,image_reconstruite,back_reconstruit,image_vst,support,NbrScale,niter,support_original
endif else begin
 save,filename='resultat_reconstruction.xdr',image,x,image_reconstruite,image_vst,support,NbrScale,niter
endelse

end