function expt_snap,expt_in

;Written by Adam Amara 18 August 2008
;PURPOSE: This routines contains the survey informations for
;the 'generic' experiment. This is to be used as an example.
;OPTIONAL INPUTS: expt_in - structure containing user specified
;                           inputs.
;OUTPUTS: expt structure that is used in set_fiducial
;
; Experiment probe parameter:
; Lensing:
;    sv_n_zbin - The number of redshift slices (lensing tomography).
;    sv_zerror - z error, sigma(z) = gerror*(1+z).
;    sv_z_med - Median redshift of all galaxies.
;    sv_ng - number density of galaxies [per arc min^2] - this can be
;            a scalr or a vector (where the vector version has the
;            same number of elements at there are tomography bins and
;            the entry in each element is the number of galaxies in
;            that slice)
;    sv_a_survey - area of survey [sq. deg.]
;    sv_eff - masking efficiency
;    sv_sig_int - sigma_e1/G from RRG I
;                   <gamma_1^2>^.5=<gamma_2^2>^.5 from intrinsic+noise    
;    sv_probes - probes that the survey can be used to measure
;    sv_dndzp - parameters of lensing dndz if dndz takes an analytic
;               form (e.g. for Smail et al. p(0) = alpha and (1) = beta)
;    sv_dndzz - parameter 3 of lensing dndz
;    sv_dndztype - form of galaxy distribution for lensing
;    dndztype options: 
;        'smail': galaxies distributed according to the analytic
;                 distribution given by Smail et al
;                  - dndzp - [alpha,beta] 
;                  - dndzz - (optional) The boundary of the bins. This
;                    should be a vector where the number of elements
;                    is sv_n_zbin+1.  eg for 3 bins we could have
;                    dndzz=[0.d,0.5d,1.0d,3.0]. If dndzz is not set
;                    (i.e. dndzz=0) then the bins will be made so that
;                    each bin contains (roughly) the same number of
;                    galaxies. 
;                  - ng - should be scalar (i.e. total galaxies in the
;                    survey). The number of galaxies per bin will be
;                    calculated by mk_survey
;                               
;        'plane': galaxies are placed on a planes (or sheets) in redshift
;                  - dndzp - not used
;                  - dndzz - redshift of the (scalar or vector) 
;                  - ng - number of galaxies [# gals/arcmin^2] per
;                    plane.  number of elements should match those of dndzz 
;                  - z_m, z_error and n_zbins should not be set
;
;        'hist': galaxies are distributed in histrograms
;                  - dndzp - not used
;                  - dndzz - The boundary of the bins. This
;                    should be a vector where the number of elements
;                    is sv_n_zbin+1.  eg for 3 bins we could have
;                    dndzz=[0.d,0.5d,1.0d,3.0]
;                  - ng - number of galaxies [# gals/arcmin^2] per
;                    plane.  The number of elements should be = number
;                    of bins
;
;  Not yet implimented:'Gauss'
;
; References for SNAP: astro-ph/0507460 Albert et al. (2005)
;                      astro-ph/0506030 Huterer et al. (2005) 
                    
names=['wide','spectra','deep']  
  
; Wide Survey:
sv1_n_zbin=10                   ;# of z bins
sv1_zerror=0.03d                ;z error  
sv1_z_med=1.d                   ;median z
sv1_ng=100.0d                   ;# galaxies [# gals/arcmin^2]
sv1_a_survey=1000.d             ;area [sq. deg.]
sv1_eff=1.0d                    ;masking eff.
sv1_sig_int=.22                 ;intrinsic+noise
sv1_dndztype='smail'            ;type of gal dist
sv1_dndzp=[2.0d,1.5d]           ;param of gal dist
sv1_dndzz=0                     ;z info for gal dist
sv1_biastype='bias1'            ;bias type
sv1_ns=-1                       ;# Sne [# Sne/arcmin^2]
sv1_sigmam=-1                   ;intrisic scatter in Sne mag [magnitude] 
sv1_delm=-1                     ;magnitude precision [magnitude]
sv1_sne_zran=[-1,-1]            ;Sne redshift range
sv1_probes=['lens']             ;possible probes 

; Deep Survey:
sv3_n_zbin=10                   ;# of z bins
sv3_zerror=0.0d                 ;z error 
sv3_z_med=1.43d                  ;median z
sv3_ng=260.0d                     ;# galaxies [# gals/arcmin^2]
sv3_a_survey=15.d               ;area [sq. deg.]
sv3_eff=1.0d                    ;masking eff.
sv3_sig_int=-1                  ;intrinsic+noise
sv3_dndztype='smail'            ;type of gal dist
sv3_dndzp=[2.0d,1.5d]           ;param of gal dist
sv3_dndzz=0                     ;z info for gal dist
sv3_biastype='bias1'            ;bias type
sv3_ns=3.5d-2                   ;# Sne [# SNe/arcmin^2]
sv3_sigmam=0.15d                ;intrisic scatter in Sne mag [magnitude] 
sv3_delm=0.02d                  ;magnitude precision [magnitude]
sv3_sne_zran=[0.2d,1.7d]        ;Sne redshift range
sv3_probes=['sne']              ;possible probes 

;*** User Customized (sv1): ***
if tag_check(expt_in,'sv1_n_zbin',val=val) then sv1_n_zbin=val 
if tag_check(expt_in,'sv1_zerror',val=val) then sv1_zerror=val      
if tag_check(expt_in,'sv1_z_med',val=val) then sv1_z_m=val
if tag_check(expt_in,'sv1_ng',val=val) then sv1_ng=val
if tag_check(expt_in,'sv1_a_survey',val=val) then sv1_a_survey=val
if tag_check(expt_in,'sv1_eff',val=val) then sv1_eff=val
if tag_check(expt_in,'sv1_sig_int',val=val) then sv1_sig_int=val
if tag_check(expt_in,'sv1_dndztype',val=val) then sv1_dndztype=val
if tag_check(expt_in,'sv1_dndzp',val=val) then sv1_dndzp=val
if tag_check(expt_in,'sv1_dndzz',val=val) then sv1_dndzz=val
if tag_check(expt_in,'sv1_biastype',val=val) then sv1_biastype=val
if tag_check(expt_in,'sv1_probes',val=val) then sv1_probes=val
if tag_check(expt_in,'sv1_ns',val=val) then sv1_ns=val
if tag_check(expt_in,'sv1_sigmam',val=val) then sv1_sigmam=val
if tag_check(expt_in,'sv1_delm',val=val) then sv1_delm=val
if tag_check(expt_in,'sv1_sne_zran',val=val) then sv1_sne_zran=val

;*** User Customized (sv3): ***
if tag_check(expt_in,'sv3_n_zbin',val=val) then sv3_n_zbin=val 
if tag_check(expt_in,'sv3_zerror',val=val) then sv3_zerror=val      
if tag_check(expt_in,'sv3_z_med',val=val) then sv3_z_m=val
if tag_check(expt_in,'sv3_ng',val=val) then sv3_ng=val
if tag_check(expt_in,'sv3_a_survey',val=val) then sv3_a_survey=val
if tag_check(expt_in,'sv3_eff',val=val) then sv3_eff=val
if tag_check(expt_in,'sv3_sig_int',val=val) then sv3_sig_int=val
if tag_check(expt_in,'sv3_dndztype',val=val) then sv3_dndztype=val
if tag_check(expt_in,'sv3_dndzp',val=val) then sv3_dndzp=val
if tag_check(expt_in,'sv3_dndzz',val=val) then sv3_dndzz=val
if tag_check(expt_in,'sv3_probes',val=val) then sv3_probes=val
if tag_check(expt_in,'sv3_ns',val=val) then sv3_ns=val
if tag_check(expt_in,'sv3_sigmam',val=val) then sv3_sigmam=val
if tag_check(expt_in,'sv3_delm',val=val) then sv3_delm=val
if tag_check(expt_in,'sv3_sne_zran',val=val) then sv3_sne_zran=val

; Creating structures:
sv1= {n_zbin:sv1_n_zbin,zerror:double(sv1_zerror),z_med:double(sv1_z_med),ng:double(sv1_ng),a_survey:double(sv1_a_survey),eff:double(sv1_eff),sig_int:double(sv1_sig_int),dndztype:sv1_dndztype,dndzp:double(sv1_dndzp),dndzz:double(sv1_dndzz),biastype:sv1_biastype,ns:double(sv1_ns),sigmam:double(sv1_sigmam),delm:double(sv1_delm),sne_zran:double(sv1_sne_zran),name:names(0),probes:sv1_probes}

sv3= {n_zbin:sv3_n_zbin,zerror:double(sv3_zerror),z_med:double(sv3_z_med),ng:double(sv3_ng),a_survey:double(sv3_a_survey),eff:double(sv3_eff),sig_int:double(sv3_sig_int),dndztype:sv3_dndztype,dndzp:double(sv3_dndzp),dndzz:double(sv3_dndzz),biastype:sv3_biastype,ns:double(sv3_ns),sigmam:double(sv3_sigmam),delm:double(sv3_delm),sne_zran:double(sv3_sne_zran),name:names(2),probes:sv3_probes}

;expt=create_struct('surveys',names,names(0),sv1,names(2),sv3)
expt={sv1:sv1,sv3:sv3}

return,expt

end

