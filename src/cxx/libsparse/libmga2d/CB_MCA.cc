/******************************************************************************
**                   Copyright (C) 2003 by CEA
*******************************************************************************
**
**    UNIT
**
**    Version: 1.0
**
**    Author: Jean-Luc Starck
**
**    Date:  03/02/03
**
**    File:  mbase.cc
**
*******************************************************************************
**
**    DESCRIPTION  Decomposition of an image on multiple bases
**    -----------
**
******************************************************************************/


#include "CB_MCA.h"
#include "MR_SoftRegul.h"
#include "IM_Rot.h"


/***********************************************************************/

inline float MCA::mca_update(float CoefSol, float Threshold, float NoiseLevel)
// L2 norm minimization with L1 norm penalty
{
   float NewCoef = 0;
   if (UseNormL1 == True)
   {
      float T2 = Threshold/2.;
//        float SoftL =  SoftLevel*NoiseLevel;
//        if (SoftL < T2)
//             NewCoef = soft_threshold(CoefSol, SoftL);
//        else NewCoef = soft_threshold(CoefSol, T2);
       NewCoef = soft_threshold(CoefSol, T2);
   }
   else NewCoef = hard_threshold(CoefSol, Threshold);
   return NewCoef;
}

/***********************************************************************/

void MCA::reset()  // Reset all internal variables
{
    Verbose = False;
    WriteAll = False;
		Bounded = False;
		SigmaBounded = False;
    UseZoom = False;
    TV_NbrScale2D = 0;
    Linear = True;
    T_Decreasing = MCA_THRES_LINEAR;

    Bord = I_MIRROR;
    Nl = Nc = NbrBase = 0;
    FirstSoftThreshold = 0; // DEF_MCA_FIRST_SOFT_THRESHOLD; // (50)
    LastSoftThreshold =  DEF_MCA_LAST_SOFT_THRESHOLD;
    Nbr_Iter = DEF_MCA_NBR_ITER;
    PositivRecIma = True;
    DataSigmaNoise = 0.;
    Verbose = False;

    AT_NbrScale2D = 4;
    AT_PositivRecIma = PositivRecIma;
    AT_KillLastScale = True;
    AT_Trans = NULL;
		AT_Trans1 = NULL;
		AT_Trans2 = NULL;
    AT_TransCorrel = NULL;

    RID_BlockSize = -1;
    RID_FirstDetectScale = 0;
    RID_PositivRecIma = PositivRecIma;

    CUR_NbrScale2D = 4;
    CUR_BlockSize = 16;
    CUR_BlockOverlap = False;
    CUR_KillLastScale= False;
    CUR_PositivRecIma = PositivRecIma;
    CUR_Trans = NULL;

    PCUR_NbrScale2D = 4;
    PCUR_BlockSize = 16;
    PCUR_BlockOverlap = False;
    PCUR_KillLastScale= False;
    PCUR_PositivRecIma = PositivRecIma;

    FCUR_NbrScale2D = 4;
    FCUR_NDIR = 32;
    FCUR_KillLastScale= False;
    FCUR_PositivRecIma = PositivRecIma;
    FCur = NULL;

    WT_NbrScale2D = 4;
    WT_SelectFilter = NULL;
    WT_SB1D = NULL;
    WT_TabDec = NULL;
    WT = NULL;
    WT_PositivRecIma = PositivRecIma;
    WT_NbrUndecimatedScale = 1;
    UseNormL1= False;
    LambdaTV = 0;

    COS_PositivRecIma = False;
    COS_BlockSize = 32;
    COS_Overlapping = False;
    COS_Sensibility = 1.;
    COS_Min  = 0.;
    COS_WeightFirst   = False;
    COS_Isotrop = False;

    MCOS_PositivRecIma = False;
    MCOS_FirstBlockSize = 8;
    MCOS_Overlapping = False;

    DWT_TabImaRec=NULL;
    DWT_NbrAngle = 10;
    DWT_NbrScale2Di = 4;
    DWT_NbrScale2Dj = 4;
    DWT_PositivRecIma = PositivRecIma;
    NSigmaSoft = 1.5;

    WP = NULL;
    //WP_SelectFilter = NULL;
    //WP_SB1D = NULL;
    WP_PositivRecIma = False;
    WP_NbrScale2D = 4;
    WP_NbrUndecimatedScale = 2;
    WP_Filter = F_MALLAT_7_9;
    WP_NbrBand = 0;
    //WP_TabTrans= NULL;

    RemoveLastScale=True;
    PIX_TabNoise=0;
    UseMad = False;
    UseMask = False;
    UseCorrelConstraint=False;
    CorrelConstraint=0.1;
}

/***********************************************************************/

void MCA::alloc(int Nli, int Nci, Ifloat * Tab)
{
   int i,s;
   Nl = Nli;
   Nc = Nci;

   for (i=0; i < NbrBase; i++) Tab[i].alloc(Nl,Nc, "alloc");

   //cout << "Image size: Nl = " << Nl << " " << " Nc = " << Nc << endl;
   //StepL = (FirstSoftThreshold - LastSoftThreshold ) / (float) Nbr_Iter;
   //Lambda = FirstSoftThreshold;
   Resi.alloc(Nl,Nc,"resi");

   if (RemoveLastScale == True) LastScale.alloc(Nl,Nc,"resi");

   for (i =0; i < NbrBase; i++)
   {
       TabImaRec[i].alloc(Nl,Nc,StringMCABase(TabSelect[i]));
       if (Verbose == True)
          cout << "Selection: " << StringMCABase(TabSelect[i]) << endl;
        switch (TabSelect[i])
        {
             case MCA_ATROU:
                    AWT.alloc(AT_Trans, Nl, Nc, AT_NbrScale2D);
		    AT_TabNoise.alloc(AT_NbrScale2D);
		    if (UseCorrelConstraint == True)
		          AWT.alloc(AT_TransCorrel, Nl, Nc, AT_NbrScale2D);
                    break;
             case MCA_RID:
                   RID.alloc(Nl, Nc, RID_BlockSize);
                   NlRid = RID.rid_nl();
                   NcRid = RID.rid_nc();
                   RID_Trans.alloc(NlRid, NcRid, "RidTrans");
                   RID.VarianceStab=PoissonNoise;
                   // RID.KillLastScale = False;
                   if (Verbose == True)
                   {
                     cout << " Ridgelet: " << " Nl = " << NlRid  << " Nc = " <<  NcRid << endl;
                     cout << " BlockSize = " << RID_BlockSize << endl;
                   }
		   if (UseCorrelConstraint == True)
		         RID_TransCorrel.alloc(NlRid, NcRid, "RidTrans");
                  break;
             case MCA_WT:
                 if (WT_NbrScale2D  < 2)
                 {
                     cout << "Error: bad number of scales ... " << endl;
                     exit(-1);
                 }
                 WT_SelectFilter = new FilterAnaSynt(F_MALLAT_7_9);
                 WT_SB1D = new SubBandFilter(*WT_SelectFilter, NORM_L2);
                 WT = new HALF_DECIMATED_2D_WT(*WT_SB1D);
                 WT_TabDec = new Bool[WT_NbrScale2D];
                 if ((WT_NbrUndecimatedScale < 0) || (WT_NbrUndecimatedScale > WT_NbrScale2D))
                      WT_NbrUndecimatedScale = WT_NbrScale2D;
  		 for (s=0; s < MIN(WT_NbrScale2D,WT_NbrUndecimatedScale); s++) WT_TabDec[s] = False;
                 for (s=WT_NbrUndecimatedScale; s < WT_NbrScale2D; s++)  WT_TabDec[s] = True;
                 WT_NbrBand = WT->alloc(WT_Trans, Nl, Nc, WT_NbrScale2D,WT_TabDec);
                 if (Verbose == True)
                 {
                    cout << "WT: Nbr scales = " << WT_NbrScale2D;
                    cout << " Nbr band = " << WT_NbrBand << endl;
                    //for (s=0; s <  WT_NbrBand; s++)
                    //    cout << " Band " << s+1 << " Nl = " << WT_Resi[s].nl() << " Nc = " << WT_Resi[s].nc() << endl;
                 }
		 WT_TabNoise.alloc(WT_NbrBand);
		 if (UseCorrelConstraint == True) WT_NbrBand = WT->alloc(WT_TransCorrel, Nl, Nc, WT_NbrScale2D,WT_TabDec);
                 break;
             case MCA_WP: {
	         // cout << "MCA_WP" << endl;
	         if (WP_NbrScale2D  < 2)
                 {
                     cout << "Error: bad number of scales ... " << endl;
                     exit(-1);
                 }
 		 //WP_SelectFilter = new FilterAnaSynt(WP_Filter);
                 //WP_SB1D = new SubBandFilter(*WP_SelectFilter, NORM_L2);
                 //WP = new WPACKETS_2D(*WT_SB1D);
		 cout << Nl << " " << Nc << " " << WP_NbrScale2D << " " << WP_NbrUndecimatedScale << endl;

                 FilterAnaSynt*  FAS = new FilterAnaSynt(WP_Filter);
                 SubBandFilter* WT1D = new SubBandFilter(*FAS, NORM_L2);
                 WP = new WPACKETS_2D(*WT1D);
                 WP->alloc(Nl, Nc, WP_NbrScale2D, WP_NbrUndecimatedScale);
		 if (UseCorrelConstraint == True)
		 {
		    WPC = new WPACKETS_2D(*WT1D);
                    WPC->alloc(Nl, Nc, WP_NbrScale2D, WP_NbrUndecimatedScale);
		  }
 		 }
                 break;
	     case MCA_CUR:
                   CUR.Border = Bord;
                   CUR.NbrScale2D = CUR_NbrScale2D;
                   // CUR.tab_block_size_init(CUR_BlockSize);
		   CUR.RidTrans = RID_PYR_FFT;
		   // CUR.VarNorm =  True;
		   CUR.BlockOverlap = CUR_BlockOverlap;
		   CUR.OddBlockSize=False;

                   if (CUR_NbrScale2D  < 2)
                   {
                       cout << "Error: bad number scales in the curvelet transform ... " << endl;
                       exit(-1);
                   }
		   CUR.alloc(Nl, Nc, CUR_BlockSize);
 		   CUR_TabNoise.alloc(CUR.nbr_band());
   	           break;
	     case MCA_PCUR:
                   PCUR.Border = Bord;
                   PCUR.NbrScale2D = PCUR_NbrScale2D;
 		   PCUR.RidTrans = RID_PYR_FFT;
 		   PCUR.BlockOverlap = PCUR_BlockOverlap;
 		   PCUR.OddBlockSize=False;
                  if (PCUR_NbrScale2D  < 2)
                   {
                       cout << "Error: bad number scales in the curvelet transform ... " << endl;
                       exit(-1);
                   }
		   PCUR.alloc(Nl, Nc, PCUR_BlockSize);
 		   PCUR_TabNoise.alloc(PCUR.nbr_band());
  	           if (UseCorrelConstraint == True)
		   {
		       PCUR_Correl.Border = Bord;
                       PCUR_Correl.NbrScale2D = PCUR_NbrScale2D;
 		       PCUR_Correl.RidTrans = RID_PYR_FFT;
 		       PCUR_Correl.BlockOverlap = PCUR_BlockOverlap;
                       PCUR_Correl.alloc(Nl, Nc, PCUR_BlockSize);
		   }
		   break;
 	     case MCA_FCUR:
	           FCur = new FCUR;
                   FCur->alloc_from_fine(FCUR_NbrScale2D, Nl,Nc,FCUR_NDIR,False,False);
 		   FCur->get_norm_coeff(3.);
		   FCUR_TabNoise.alloc(FCur->nbr_tot_band());
		   if (UseCorrelConstraint == True)
		   {
		      FCUR_Correl = new FCUR;
                      FCUR_Correl->alloc_from_fine(FCUR_NbrScale2D, Nl,Nc,FCUR_NDIR,False,False);
		   }
 	           break;
	     case MCA_FFT2D_WT1D:
	           // Fft2dWt1d.alloc(Nl);
	           break;
             case MCA_COS:
	          LDCT.alloc(Nl, Nc, COS_BlockSize, COS_Overlapping, COS_WeightFirst  );
		  if (UseCorrelConstraint == True)
		      LDCT_Correl.alloc(Nl, Nc, COS_BlockSize, COS_Overlapping, COS_WeightFirst);
                  // COS_Resi.alloc(Nl, Nc, "cos resi");
                  // COS_Trans.alloc(Nl, Nc, "cos trans");
                  if (Verbose == True)
		  {
		     cout << " LDCT transf. size: " << " Nl = " << LDCT.nl()  << " Nc = " <<  LDCT.nc() << endl;
                     cout << " LDCT BlockSize = " << LDCT.block_size() << endl;
		     if (COS_Overlapping == True) cout << " Overlapping Blocks " << endl;
		     else cout << " Non Overlapping Blocks " << endl;
                   }
		 break;
	     case MCA_MCOS:
	          M_DCT.BlockOverlap = MCOS_Overlapping;
	          M_DCT.alloc(Nl, Nc,  MCOS_NbrScale2D, MCOS_FirstBlockSize);
                  // COS_Resi.alloc(Nl, Nc, "cos resi");
                  // COS_Trans.alloc(Nl, Nc, "cos trans");
                 break;
	     case MCA_DDWT:
                  DWT_TabImaRec = new Ifloat [DWT_NbrAngle];
                  DWT_TabAngle.alloc(DWT_NbrAngle);
		  DWT_SelectFilter = new FilterAnaSynt(F_MALLAT_7_9);
                  DWT_SB1D = new SubBandFilter(*DWT_SelectFilter, NORM_L2);
		  LC.alloc(*DWT_SB1D);
                  if (Verbose == True) cout << " NbrAngle = " <<  DWT_NbrAngle  << endl;
		  for (int a = 0; a < DWT_NbrAngle; a++)
		  {
		     DWT_TabAngle(a) = (float) (PI / (float) DWT_NbrAngle * (float) a);
		     if (Verbose == True)
		      cout << " Angle " << DWT_TabAngle(a) / PI * 180. << endl;
		     DWT_TabImaRec[a].alloc(Nl, Nc, "dwt trans");
		  }
  	          break;
	     case MCA_PIXEL:
	          break;
	     default:
               cout << "Error: not implemeted transform ... " << endl;
               exit(-1);
        }
   }
   if (UseCorrelConstraint == True) ImaCorrel.alloc(Nl, Nc, "imacorrel");

   if (NbrBase  < 1)
   {
       cout << "Error: no selected transform ... " << endl;
       exit(-1);
   }

   if (Verbose == True)
            cout << "Number of selected bases = " << NbrBase << endl;
}

/****************************************************************************/

// void MCA::fft2dwt1d_proj(Ifloat & ImaPS,  float Lamba_Sigma)
// {
//    int i,j;
//    float PSNoise = DataSigmaNoise*DataSigmaNoise;
//    float Level = Lamba_Sigma*PSNoise;
//    Fft2dWt1d.transform(ImaPS);
//    for (i=0; i < Fft2dWt1d.nbr_line(); i++)
//    for (j=0; j < Fft2dWt1d.nbr_coeff_in_line(i); j++)
//       Fft2dWt1d(i,j) = mca_update(Fft2dWt1d(i,j), Level);
//
//    Fft2dWt1d.recons(ImaPS);
// }

/****************************************************************************/

void MCA::rid_proj(Ifloat & ImaRid, float Lamba_Sigma)
// Ridgelet base project of the residual
//  RidTrans = Ifloat = IN/OUT decomposition on a ridgelet base
//  ImaRid = IN/OUT: reconstruction from  RidTrans
// SigmaNoise = IN: noise standard deviation
// NSigma = IN: N sigma filtering
// KillLastScale = IN: the last scale is taken into account
{
   int s,i,j;
   int NScale = RID.NbrScale;
   int LastScale = (RID.KillLastScale == True) ? NScale-1: NScale;
   float RidNoise = (PoissonNoise == True) ? 1. : sqrt((float) RID.block_size())*DataSigmaNoise;
   float NSigma = Lamba_Sigma;

   RID.transform(ImaRid,  RID_Trans);
   RID.set_tab_norm(RID.NbrScale);
   if (UseCorrelConstraint == True) RID.transform(ImaCorrel, RID_TransCorrel);
   for (s=RID_FirstDetectScale; s < LastScale; s++)
   {
      float NSig = (s == 0) ? NSigma + 1: NSigma;
      if (NSigma == 0) NSig = 0;
      float Noise = (s == NScale-1) ? RidNoise*RID.rid_norm(s-1) :
                                               RidNoise*RID.rid_norm(s);
      float Level = (s == NScale-1) ? 0: NSig*Noise;
      int Nls = RID.size_scale_nl(s);
      int Ncs = RID.size_scale_nc(s);
      int Depi = RID.ipos(s);
      int Depj = RID.jpos(s);
      if (UseCorrelConstraint == False)
      {
         for (i=Depi; i < Depi+Nls; i++)
         for (j=Depj; j < Depj+Ncs; j++)
         {
          float Coef = RID_Trans(i,j);
          RID_Trans(i,j) =  mca_update(Coef, Level, Noise);
         }
      }
      else
      {
          float CorrelLevel = Lamba_Sigma * CorrelConstraint;
          for (i=Depi; i < Depi+Nls; i++)
          for (j=Depj; j < Depj+Ncs; j++)
          {
	     float AdaptLevel = Level + CorrelLevel * ABS(RID_TransCorrel(i,j));
             RID_Trans(i,j)  = mca_update(RID_Trans(i,j), AdaptLevel, Noise);
          }
      }
   }
   RID.recons(RID_Trans, ImaRid);
}

/****************************************************************************/

float MCA::atrou_max(Ifloat & ImaAtrou)
{
   float MAX_NormCoef=0.;
   AWT.transform(ImaAtrou, AT_Trans, AT_NbrScale2D);
   for (int s=0; s < AT_NbrScale2D-1; s++)
   {
      float NormMaxLevel = ABS( AT_Trans[s].maxfabs()) / AT_TabNoise(s);
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
   if (Verbose == True)
         cout << "AT MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

float MCA::atrou_init(Ifloat & ImaAtrou, Bool SetMAD)
{
   float AT_SigmaNoise = DataSigmaNoise;
   float MAX_NormCoef=0.;

   AWT.transform(ImaAtrou, AT_Trans, AT_NbrScale2D);
   for (int s=0; s < AT_NbrScale2D-1; s++)
   {
      float Norm = (s == AT_NbrScale2D-1) ? AWT.norm_band(s-1): AWT.norm_band(s);
      float Noise =  AT_SigmaNoise*Norm;
      if (SetMAD == True)
      {
         Noise = detect_noise_from_mad(AT_Trans[s]);
      }
     if (Verbose == True)
	   cout << "AT Band " << s + 1 << " Noise Level =  " << Noise  << endl;
      AT_TabNoise(s) = Noise;
      float MaxLevel = ABS( AT_Trans[s].maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
   if (Verbose == True)
         cout << "AT MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

void MCA::atrou_proj(Ifloat & ImaAtrou, float Lamba_Sigma)
// ImaAtrou = IN-OUT: reconstruction from AT_Trans
// AT_NbrScale2D = IN: number of scales
// AT_KillLastScale = IN: the last scale is taken into account
// Bord = IN: type of border management
{
   int i,j;
   float NSigma = Lamba_Sigma;

   // WT of the residual
   // INFO_X(Resi, "Resi");
   AWT.ModifiedAWT = True;
   AWT.transform(ImaAtrou, AT_Trans, AT_NbrScale2D);
   if (UseCorrelConstraint == True) AWT.transform(ImaCorrel, AT_TransCorrel, AT_NbrScale2D);

   // cout << "ATROU Lambda = " << Lamba_Sigma << endl;
   for (int s=0; s < AT_NbrScale2D-1; s++)
   {
      float NSig = (s == 0) ? NSigma + 1: NSigma;
      if (NSigma == 0) NSig = 0;
      float Noise =  AT_TabNoise(s);
      // float Level = (s == AT_NbrScale2D-1) ? 0: NSig*Noise;
      float Level = NSig*Noise;
      if (UseCorrelConstraint == False)
      {
         for (i=0; i < Nl; i++)
         for (j=0; j < Nc; j++)
         {
          float Coef = (AT_Trans[s])(i,j);
         (AT_Trans[s])(i,j) = mca_update(Coef, Level, Noise);
         }
      }
      else
      {
         float CorrelLevel = Lamba_Sigma * CorrelConstraint;
	 for (i=0; i < Nl; i++)
         for (j=0; j < Nc; j++)
         {
	    float AdaptLevel = Level + CorrelLevel * ABS((AT_TransCorrel[s])(i,j));
            float Coef = (AT_Trans[s])(i,j);
            (AT_Trans[s])(i,j) = mca_update(Coef, AdaptLevel, Noise);
         }
      }
   }
   if (AT_KillLastScale == True) (AT_Trans[AT_NbrScale2D-1]).init();
   AWT.recons(AT_Trans, ImaAtrou, AT_NbrScale2D);
}

/****************************************************************************/

float MCA::pixel_max(Ifloat & ImaPix)
{
   float MaxLevel = ABS(ImaPix.maxfabs());
   float MAX_NormCoef = MaxLevel / PIX_TabNoise;
   return MAX_NormCoef;
}

/****************************************************************************/

float MCA::pixel_init(Ifloat & ImaPix, Bool SetMAD)
// ImaAtrou = IN-OUT: reconstruction from AT_Trans
// AT_NbrScale2D = IN: number of scales
// AT_KillLastScale = IN: the last scale is taken into account
// Bord = IN: type of border management
{
   float Noise = DataSigmaNoise;
   // float Mean = ImaPix.mean();
   if (SetMAD == True) Noise = detect_noise_from_mad(ImaPix);
   PIX_TabNoise = Noise;
   float MaxLevel = ABS(ImaPix.maxfabs());
   float MAX_NormCoef = MaxLevel / PIX_TabNoise;

   if (Verbose == True)
         cout << "PIX MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

void MCA::pixel_proj(Ifloat & ImaPix, float Lamba_Sigma)
// ImaAtrou = IN-OUT: reconstruction from AT_Trans
// AT_NbrScale2D = IN: number of scales
// AT_KillLastScale = IN: the last scale is taken into account
// Bord = IN: type of border management
{
   int i,j;
   float PIX_SigmaNoise = PIX_TabNoise;
   float NSigma = Lamba_Sigma;
   float Level = NSigma*PIX_SigmaNoise;
   for (i=0; i < ImaPix.nl(); i++)
   for (j=0; j < ImaPix.nc(); j++)
   {
      float Coef = ImaPix(i,j);
      ImaPix(i,j) = mca_update(Coef, Level, PIX_SigmaNoise);
   }
}

/****************************************************************************/

float MCA::wt_max(Ifloat & ImaWT)
{
   float MAX_NormCoef=0.;
   WT->transform(ImaWT, WT_Trans, WT_NbrScale2D, WT_TabDec);
   for (int s=0; s < WT_NbrBand-1; s++)
   {
      float MaxLevel = ABS( WT_Trans[s].maxfabs());
      float NormMaxLevel = MaxLevel / DataSigmaNoise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
    if (Verbose == True)
         cout << "WT MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

float MCA::wt_init(Ifloat & ImaWT, Bool SetMAD)
{
   float WT_SigmaNoise = DataSigmaNoise;
   float MAX_NormCoef=0.;

    WT->transform(ImaWT, WT_Trans, WT_NbrScale2D, WT_TabDec);
   for (int s=0; s < WT_NbrBand-1; s++)
   {
      float Noise =  WT_SigmaNoise;
      if (SetMAD == True)
      {
         Noise = detect_noise_from_mad(WT_Trans[s]);
         if (Verbose == True)
	   cout << "WT Band " << s + 1 << " Noise Level =  " << Noise  << endl;
      }
      WT_TabNoise(s) = Noise;
      float MaxLevel = ABS( WT_Trans[s].maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
   if (Verbose == True)
         cout << "WT MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

void MCA::wt_proj(Ifloat & Sol, float Lamba_Sigma)
// Resi = IN: residual image
// WT_Resi = Ifloat[0..WT_NbrScale2D-1] = IN: buffer for WT computation
// WT_Trans = Ifloat[0..WT_NbrScale2D-1] = IN/OUT decomposition on WT
// ImaRec = OUT: reconstruction from WT_Trans
// WT_NbrScale2D = IN: number of scales
// WT_SigmaNoise = IN: noise standard deviation
// NSigma = IN: N sigma filtering
{
   int s,i,j;
   float NSigma = Lamba_Sigma;

   WT->transform(Sol, WT_Trans, WT_NbrScale2D, WT_TabDec);
   if (UseCorrelConstraint == True) WT->transform(ImaCorrel, WT_TransCorrel, WT_NbrScale2D, WT_TabDec);
   for (s=0; s < WT_NbrBand-1; s++)
   {
      float NSig = (s < 3) ? NSigma + 1: NSigma;
      float Noise = WT_TabNoise(s);
      float Level = NSig*Noise;
      if (UseCorrelConstraint == False)
      {
         for (i=0; i <  (WT_Trans[s]).nl(); i++)
         for (j=0; j <  (WT_Trans[s]).nc(); j++) (WT_Trans[s])(i,j) = mca_update((WT_Trans[s])(i,j), Level, Noise);
      }
      else
      {
          float CorrelLevel = Lamba_Sigma * CorrelConstraint;
          for (i=0; i <  (WT_Trans[s]).nl(); i++)
          for (j=0; j <  (WT_Trans[s]).nc(); j++)
          {
	     float AdaptLevel = Level + CorrelLevel * ABS((WT_TransCorrel[s])(i,j));
            (WT_Trans[s])(i,j) = mca_update((WT_Trans[s])(i,j), AdaptLevel, Noise);
          }
      }
   }
   if (AT_KillLastScale == True) (WT_Trans[WT_NbrBand-1]).init();
   WT->recons(WT_Trans, Sol, WT_NbrScale2D, WT_TabDec);
}
/****************************************************************************/

float MCA::wp_max(Ifloat & ImaWP)
{
   float WP_SigmaNoise = DataSigmaNoise;
   float MAX_NormCoef=0.;
   Ifloat Band;

   WP->transform(ImaWP, WP_TabTrans);
   for (int s=0; s < WP_TabTrans->nbr_band()-1; s++)
   {
      float Noise =  WP_SigmaNoise;
      WP_TabTrans->get_band(s,Band);
      float MaxLevel = ABS( Band.maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
    if (Verbose == True)
         cout << "WP MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

float MCA::wp_init(Ifloat & ImaWP, Bool SetMAD)
{

   float WP_SigmaNoise = DataSigmaNoise;
   float MAX_NormCoef=0.;
   Ifloat Band;

   WP->transform(ImaWP, WP_TabTrans);

   WP_TabNoise.alloc(WP_TabTrans->nbr_band());
   for (int s=0; s < WP_TabTrans->nbr_band()-1; s++)
   {
      float Noise =  WP_SigmaNoise;
      WP_TabTrans->get_band(s,Band);
      if (SetMAD == True)
      {
         Noise = detect_noise_from_mad(Band);
         if (Verbose == True)
	   cout << "WP Band " << s + 1 << " Noise Level =  " << Noise  << endl;
      }
      // cout << " band " << s << "  " << Noise << endl;
      WP_TabNoise(s) = Noise;
      float MaxLevel = ABS( Band.maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
   if (Verbose == True)
         cout << "WP MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

void MCA::wp_proj(Ifloat & Sol, float Lamba_Sigma)
{
   int s,i,j;
   float NSigma = Lamba_Sigma;

   WP->transform(Sol, WP_TabTrans);
   if (UseCorrelConstraint == True) WPC->transform(ImaCorrel, WP_TabTransCorrel);

   for (s=0; s < WP_TabTrans->nbr_band()-1; s++)
   {
      float NSig =  NSigma;
      float Noise = WP_TabNoise(s);
      float Level = NSig*Noise;
      if (UseCorrelConstraint == False)
      {
         for (i=0; i <  WP_TabTrans->size_band_nl(s); i++)
         for (j=0; j <  WP_TabTrans->size_band_nc(s); j++)
         {
            float coef = (*WP_TabTrans)(s,i,j);
            (*WP_TabTrans)(s,i,j) = mca_update(coef, Level, Noise);
         }
      }
      else
      {
 	 float CorrelLevel = Lamba_Sigma * CorrelConstraint;
         for (i=0; i <  WP_TabTrans->size_band_nl(s); i++)
         for (j=0; j <  WP_TabTrans->size_band_nc(s); j++)
         {
            float coef = (*WP_TabTrans)(s,i,j);
	    float AdaptLevel = Level + CorrelLevel * ABS((*WP_TabTransCorrel)(s,i,j));
            (*WP_TabTrans)(s,i,j) = mca_update(coef, AdaptLevel, Noise);
         }
      }
   }
   if (WP_KillLastScale == True)
   {
      s = WP_TabTrans->nbr_band()-1;
      for (i=0; i < WP_TabTrans->size_band_nl(s); i++)
      for (j=0; j < WP_TabTrans->size_band_nc(s); j++) (*WP_TabTrans)(s,i,j) = 0;
   }
   WP->recons(WP_TabTrans, Sol);
}

/****************************************************************************/

void MCA::dwt_proj(Ifloat & Sol, float Lamba_Sigma, float Angle)
// Resi = IN: residual image
// WT_Resi = Ifloat[0..WT_NbrScale2D-1] = IN: buffer for WT computation
// WT_Trans = Ifloat[0..WT_NbrScale2D-1] = IN/OUT decomposition on WT
// ImaRec = OUT: reconstruction from WT_Trans
// WT_NbrScale2D = IN: number of scales
// WT_SigmaNoise = IN: noise standard deviation
// NSigma = IN: N sigma filtering
{
   int i,j;
   float WT_SigmaNoise = DataSigmaNoise;
   float Level = Lamba_Sigma*WT_SigmaNoise;

   LC.directional_transform(Sol, DWT_Trans, Angle, DWT_NbrScale2Di, DWT_NbrScale2Dj);
   for (i=0; i < DWT_Trans.nl(); i++)
   for (j=0; j < DWT_Trans.nc(); j++)
            DWT_Trans(i,j) = mca_update(DWT_Trans(i,j), Level, WT_SigmaNoise);
   LC.directional_recons(DWT_Trans, Sol, Angle, DWT_NbrScale2Di, DWT_NbrScale2Dj);
}

/****************************************************************************/

static void contraint_isotrop_fft(Ifloat & ImaRec, Bool Verbose)
{
   if (Verbose == True) cout << "Isoptropic PS Constraint " << endl;
   int i,j;
   int Nl = ImaRec.nl();
   int Nc = ImaRec.nc();
   int Np = Nl*Nc;
   // float Dens = 1.;
   Icomplex_f Ima1_cf (Nl,Nc, "Buffer1 conv");
   fltarray Spectrum;
   float Resol=0.5;

   get_isotropic_spectrum(ImaRec, Spectrum, Resol);
   double Step = Spectrum(1,0) - Spectrum(0,0);
   FFTN_2D FFT;
   FFT.fftn2d(ImaRec, Ima1_cf);
   int NL = Spectrum.nx();
   float LastL = Spectrum(NL-1,0);
   for (i=0; i < Nl; i++)
   for (j=0; j < Nc; j++)
   {
      float y = ((float) i - Nl / 2.) / Nl;
      float x = ((float) j - Nc / 2.) / Nc;
      double IsotropPow, Rad = sqrt(x*x+y*y);
      if (Rad > LastL) Rad = LastL;
      int IndMin = (int) (Rad/Step);
      int IndMax = (int) (Rad/Step+1.);
      if (IndMax >= Spectrum.nx()) IndMax = Spectrum.nx() - 1;
      double CoordMin = Spectrum(IndMin,0);
      double CoordMax = Spectrum(IndMax,0);
      double CoefMin = Spectrum(IndMin,1);
      double CoefMax = Spectrum(IndMax,1);
      double Weight = 0;
      if ((CoordMax-CoordMin) > FLOAT_EPSILON) Weight = ABS((Rad-CoordMin)/(CoordMax-CoordMin));
      IsotropPow = (CoefMin + (CoefMax-CoefMin)*Weight) * Np;
      // double  PowVal = sqrt((float) norm(Ima1_cf(i,j)));
      Ima1_cf(i,j) = polar((float) sqrt(IsotropPow),(float) arg(Ima1_cf(i,j)));
   }
   Ima1_cf(Nl/2,Nc/2)  = complex_f(0.,0.);

   FFT.fftn2d(Ima1_cf, True);
   for (i = 0; i < Nl; i++)
   for (j = 0; j < Nc; j++) ImaRec(i,j) = Ima1_cf(i,j).real();
   // INFO_X(ImaRec, "OUT constraint");
}

/****************************************************************************/

float MCA::cos_max(Ifloat & ImaCos)
{
   float CosNoise = DataSigmaNoise;
   float MAX_NormCoef=0.;
   LDCT.transform(ImaCos);
   MAX_NormCoef = ABS(LDCT.maxfabs_without_zerofreq()) / LDCT.norm() / CosNoise;

   if (Verbose == True)
         cout << "COS MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

float MCA::cos_init(Ifloat & ImaCos)
{
   float CosNoise = DataSigmaNoise;
   float MAX_NormCoef=0.;
   LDCT.transform(ImaCos);
   // io_write_ima_float("xx_cos.fits", LDCT.DCTIma);
   cout << "MAXCOS = " << LDCT.max() << endl;
   MAX_NormCoef = ABS(LDCT.maxfabs_without_zerofreq()) / LDCT.norm() / CosNoise;

   if (Verbose == True)
         cout << "COS MAX_NormCoef = " << MAX_NormCoef << endl;
				 cout << " maxfabs_without_zerofreq= " << ABS(LDCT.maxfabs_without_zerofreq()) << endl;
				 cout << " LDCT.norm() = " <<  LDCT.norm() << endl;
				 cout << " CosNoise= " <<  CosNoise << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

void MCA::cos_proj(Ifloat & ImaRec, float Lamba_Sigma)
// Resi = IN: residual image
// COS_Resi = Ifloat = IN: buffer for COS computation
// COS_Trans = Ifloat  = IN/OUT decomposition on cos
// ImaRec = OUT: reconstruction from cos. transf.
// WT_SigmaNoise = IN: noise standard deviation
// NSigma = IN: N sigma filtering
{
   int i,j,bi,bj;
   float CosNoise = DataSigmaNoise;
   float NSigma = Lamba_Sigma;
   float Noise = CosNoise * LDCT.norm();
   // float HardT = NSigma*Noise;
   // float SoftT = Lamba_Sigma*Noise;
   Bool IsotropContraint = False;
   int BS = LDCT.B2DTrans.block_size();
   Ifloat BlockCosIma(BS, BS, "blockima");
   Ifloat BlockCosImaCorrel;
   Ifloat MeanBlockCosIma(BS, BS, "blockima");
   float Level = NSigma*Noise*COS_Sensibility;
   float COSSupportMin = COS_Min ;
   if ((COSSupportMin < 0) || (COSSupportMin >= 0.5)) COSSupportMin = 0;
   COSSupportMin *= BS;

   LDCT.transform(ImaRec);
   if (UseCorrelConstraint == True)
   {
      float MeanImaCorrel = ImaCorrel.mean();
      for (i=0; i < ImaCorrel.nl(); i++)
      for (j=0; j < ImaCorrel.nc(); j++) ImaCorrel(i,j) -= MeanImaCorrel;
      LDCT_Correl.transform(ImaCorrel);
      BlockCosImaCorrel.alloc(BS, BS, "blockimacor");
   }
   if (COSSupportMin == 0)
   {
      // cout << " LDCT.nl() = " << LDCT.nl() << " " <<  LDCT.nc() << endl;
      if (UseCorrelConstraint == False)
      {
          for (i=0; i < LDCT.nl(); i++)
          for (j=0; j < LDCT.nc(); j++) LDCT(i,j) = mca_update(LDCT(i,j), Level, Noise);
      }
      else
      {
         float CorrelLevel = Lamba_Sigma * CorrelConstraint;
         for (i=0; i < LDCT.nl(); i++)
         for (j=0; j < LDCT.nc(); j++)
	 {
	    float AdaptLevel = Level + CorrelLevel * ABS(LDCT_Correl(i,j));
	    LDCT(i,j) = mca_update(LDCT(i,j), AdaptLevel, Noise);
	 }
      }
   }
   else
   {
      cout << "COSSupportMin = " << COSSupportMin << " BS = " << BS << endl;
      //cout << "nbl = " << LDCT.B2DTrans.nbr_block_nl()  << endl;
      //cout << "nbc = " << LDCT.B2DTrans.nbr_block_nc()  << endl;
      //INFO_X(LDCT.DCTIma, "BEF");
      for (bi = 0; bi < LDCT.B2DTrans.nbr_block_nl(); bi++)
      for (bj = 0; bj < LDCT.B2DTrans.nbr_block_nc(); bj++)
      {
         LDCT.B2DTrans.get_block_ima(bi,bj, LDCT.DCTIma, BlockCosIma);
	 if (UseCorrelConstraint == False)
         {
	    for (i=0; i < BS; i++)
            for (j=0; j < BS; j++)
            {
                BlockCosIma(i,j) =  mca_update(BlockCosIma(i,j), Level, Noise);
 	        float Rad = sqrt((float)(i*i) + (float)(j*j));
	        // cout << "ii = " << i*i << " jj = " << j*j << " Rad = " << Rad  << " COSSupportMin = " << COSSupportMin << endl;
                if (Rad < COSSupportMin) BlockCosIma(i,j) = 0;
 	    }
	 }
	 else
	 {
	     LDCT_Correl.B2DTrans.get_block_ima(bi,bj, LDCT_Correl.DCTIma, BlockCosImaCorrel);
	     float CorrelLevel = Lamba_Sigma * CorrelConstraint;
	     for (i=0; i < BS; i++)
             for (j=0; j < BS; j++)
             {
                float AdaptLevel = Level + CorrelLevel * ABS(BlockCosImaCorrel(i,j));
                BlockCosIma(i,j) =  mca_update(BlockCosIma(i,j), AdaptLevel, Noise);
 	        float Rad = sqrt((float)(i*i) + (float)(j*j));
	        // cout << "ii = " << ii << " jj = " << jj << " Rad = " << Rad  << " COSSupportMin = " << COSSupportMin << endl;
                if (Rad < COSSupportMin) BlockCosIma(i,j) = 0;
 	     }
	 }
	 LDCT.B2DTrans.put_block_ima(bi,bj, LDCT.DCTIma, BlockCosIma);
      }
      //INFO_X(LDCT.DCTIma, "AFTER");
   }

   if (IsotropContraint == True)
   {
      BSPLINE_DEC BD;
      Ifloat BCoef(BS,BS,"block");
      float Resol = 0.5;
      float Dens = 1.;
      int NpSpec = (int) (BS / Resol + 0.5);
      fltarray Spectrum(NpSpec);

      // Spectrum Calculation
      for (int r = 0; r < NpSpec; r++)
      {
         float Rad = (r+1)*Resol;
         int Nu=0;
         int Np = (int) (Dens*PI/2.*Rad+0.5);  // number of points on the circle of radius r
         for (i=0; i < Np; i++)
         {
            float Angle =  PI/2. / (float) Np * i;
	    double x = Rad * cos(Angle);
	    double y = Rad * sin(Angle);
 	    if ((x >= 0) && (y >= 0) && (x < BS) && (y < BS))
	    {
	       Spectrum(r) += BD.InterpolatedValue(ImaRec,x,y);
	       Nu ++;
	    }
         }
         Spectrum(r) /= (float) Nu;
      }


      for (bi = 0; bi < LDCT.B2DTrans.nbr_block_nl(); bi++)
      for (bj = 0; bj < LDCT.B2DTrans.nbr_block_nc(); bj++)
      {
         LDCT.B2DTrans.get_block_ima(bi,bj, LDCT.DCTIma, BlockCosIma);
         BCoef=BlockCosIma;
	 BD.SamplesToCoefficients(BCoef);
         Spectrum(0) = BlockCosIma(0,0);

   	 for (i=0; i < BS; i++)
         for (j=0; j < BS; j++)
         {
 	     float Rad = sqrt((float)(i*i+j*j));
             int r1 = (int) Rad;
             int r2 = (int) Rad + 1;
             if ((r1 >= NpSpec) || (r2 >= NpSpec))
             {
                cout << "Error in mtest: NpSpec = " << NpSpec <<  " r1 = " << r1 <<  " r2 = " << r2 << endl;
                exit(-1);
             }
             // double ValNorm = Spectrum(r2) * (Rad - r1) / (r2 - r1)
             //         + Spectrum(r1) * (r2 - Rad) / (r2 - r1);
             // double NormData = sqrt(ValNorm);
         }

         for (i=0; i < BS; i++)
         for (j=0; j < BS; j++)
	 {
	    MeanBlockCosIma(i,j) += ABS(BlockCosIma(i,j));
	 }
      }
      for (i=0; i < BS; i++)
      for (j=0; j < BS; j++)
                         MeanBlockCosIma(i,j) /= (float) LDCT.B2DTrans.nbr_block();
      for (int bi = 0; bi < LDCT.B2DTrans.nbr_block_nl(); bi++)
      for (int bj = 0; bj < LDCT.B2DTrans.nbr_block_nc(); bj++)
      {
         LDCT.B2DTrans.get_block_ima(bi,bj, LDCT.DCTIma, BlockCosIma);
	 for (i=0; i < BS; i++)
         for (j=0; j < BS; j++)
	 {
            if (BlockCosIma(i,j) >= 0) BlockCosIma(i,j) = MeanBlockCosIma(i,j);
            else BlockCosIma(i,j) = - MeanBlockCosIma(i,j);
         }
	 LDCT.B2DTrans.put_block_ima(bi,bj, LDCT.DCTIma, BlockCosIma);
      }
   }
   // io_write_ima_float("xx_cos.fits", LDCT.DCTIma);
   ImaRec.init();
   LDCT.recons(ImaRec);

   // im_dct(COS_Trans, ImaRec, True);
}

/****************************************************************************/

void MCA::mcos_proj(Ifloat & ImaRec, float Lamba_Sigma)
{
   int s,i,j;
   float CosNoise = DataSigmaNoise;
   float NSigma = Lamba_Sigma;

   // INFO_X(ImaRec, "MCOS IN");
   M_DCT.transform(ImaRec);
   for (s=0; s < M_DCT.nbr_scale(); s++)
   {
      float Noise = CosNoise * M_DCT.norm_band(s);
      // cout << "  Local DCT, band " << s + 1 << endl;
      // INFO_X(M_DCT.band(s), "DCT band in");

      for (i=0; i < M_DCT.nl(s); i++)
      for (j=0; j < M_DCT.nc(s); j++)
      {
         float Level = NSigma*Noise;
         float Coef = M_DCT(s,i,j);
         M_DCT(s,i,j) = mca_update(Coef, Level, Noise);
      }
      // INFO_X(M_DCT.band(s), "DCT band out");
   }
   ImaRec.init();
   M_DCT.recons(ImaRec);
   // INFO_X(ImaRec, "MCOS OUT");
}

/****************************************************************************/

float MCA::cur_max(Ifloat & ImaCur)
{
   float MAX_NormCoef=0.;
   Ifloat Band;
   Ridgelet *Rid;
   int b,s2d,s1d;

   CUR.transform(ImaCur, CUR_Trans);
   for (b=0; b < CUR.nbr_band()-1; b++)
   {
      float Noise =  CUR.sigma_noise(b);
      CUR.get_scale_number(b, s2d, s1d);
      Rid = CUR.get_ridgelet(s2d);
      Rid->get_scale(CUR_Trans[s2d], Band, s1d);
      float MaxLevel = ABS(Band.maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
     if (Verbose == True)
         cout << "CUR MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

float MCA::cur_init(Ifloat & ImaCur, Bool SetMAD)
{
   float CUR_SigmaNoise = DataSigmaNoise;
   float MAX_NormCoef=0.;
   float N_Sigma=3.;
   Ifloat Band;
   Ridgelet *Rid;
   int b,s2d,s1d;

   CUR.transform(ImaCur, CUR_Trans);
   if (SetMAD == False) CUR.set_noise_model_gaussian(N_Sigma, CUR_SigmaNoise);
   else CUR.set_noise_model_using_mad(CUR_Trans, N_Sigma);
   for (b=0; b < CUR.nbr_band()-1; b++)
   {
      float Noise =  CUR.sigma_noise(b);
      CUR_TabNoise(b) = Noise;
      CUR.get_scale_number(b, s2d, s1d);
      Rid = CUR.get_ridgelet(s2d);
      Rid->get_scale(CUR_Trans[s2d], Band, s1d);
      float MaxLevel = ABS(Band.maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
   if (Verbose == True)
         cout << "CUR MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

void MCA::cur_proj(Ifloat & ImaCur, float Lamba_Sigma)
// Resi = IN: residual image
// CUR_Trans = Ifloat[0..AT_NbrScale2D-1] = IN/OUT decomposition on
//                                           a trous algorithm
// ImaCur = OUT: reconstruction from the curvelet transform
// CUR_NbrScale2D = IN: number of scales
// CUR_SigmaNoise = IN: noise standard deviation
// NSigma = IN: N sigma filtering
// CUR_KillLastScale = IN: the last scale is taken into account
// Bord = IN: type of border management
{
   int b,s2d,s1d,i,j;
   // int Nls,Ncs,Depi,Depj;
   float Coef;
   float CurNoise = DataSigmaNoise;
   float NSigma = Lamba_Sigma;
   Ifloat Band,BandCorrel;
   Ridgelet *Rid;

   // CUR.Verbose = True;
//   cout << " TRANS " << endl;
   CUR.transform(ImaCur, CUR_Trans);
   if (UseCorrelConstraint == True)
   {
      CUR.transform(ImaCorrel, CUR_Correl);
   }
//   cout << " TRANS OK: " << CUR.nbr_band() << endl;

    for (b=0; b < CUR.nbr_band()-1; b++)
    {
        CUR.get_scale_number(b, s2d, s1d);
        Rid = CUR.get_ridgelet(s2d);
        Rid->get_scale(CUR_Trans[s2d], Band, s1d);

        // cout << "Band " << b+1 << " NL = " << Band.nl() << " " << Band.nc() << endl;
        float NSig = NSigma; // (s1d == 0) ? NSigma + 1: NSigma;
        if (NSigma == 0) NSig = 0;
        float Noise = (s1d == Rid->NbrScale-1) ? CUR_TabNoise(b-1): CUR_TabNoise(b);
        float Level = NSig*Noise;
	if (UseCorrelConstraint == False)
	{
	    // float T=0.;
 	    for (i=0; i < Band.nl(); i++)
            for (j=0; j < Band.nc(); j++)
	     {
	         // T += ABS(Band(i,j));
  	         Band(i,j) =  mca_update(Band(i,j),Level, Noise);
 	     }
	     // cout << " Band " << b+1 << " MeanABS = " << T / (float)(Band.nl()*Band.nc());
         }
	else
	{
	   float CorrelLevel = Lamba_Sigma * CorrelConstraint;
	   Rid->get_scale(CUR_Correl[s2d], BandCorrel, s1d);
	   for (i=0; i < Band.nl(); i++)
           for (j=0; j < Band.nc(); j++)
	   {
	      float AdaptLevel = Level + CorrelLevel * ABS(BandCorrel(i,j));
	      Band(i,j) =  mca_update(Band(i,j), AdaptLevel, Noise);
	   }
        }
        Rid->put_scale(CUR_Trans[s2d], Band, s1d);
   }

   s2d = CUR_NbrScale2D-1;
   if (CUR_KillLastScale == False)
   {
       for (i=0; i < Nl; i++)
       for (j=0; j < Nc; j++)
       {
          float Norm = AWT.norm_band(s2d-1);
          float Noise = CurNoise*Norm;
          Coef = CUR_Trans[s2d](i,j);
          // float Level = 0.;
          float LRid = Lamba_Sigma*Noise;
          CUR_Trans[s2d](i,j) = mca_update(Coef,LRid,Noise);
       }
   }

   CUR.recons(CUR_Trans, ImaCur);
//   cout << " REC OK " << endl;
}

/****************************************************************************/

float MCA::fcur_max(Ifloat & ImaFCur)
{
   float MAX_NormCoef=0.;
   Ifloat Band;
   int b,s,Indb=0;

   FCur->cur_trans(ImaFCur);
   Indb=0;
   for (s=0; s < FCur->nbr_scale()-1; s++)
   for (b=0; b < FCur->nbr_band(s); b++)
   {
      FCur->get_band(s,b,Band);
      float Noise =  FCUR_TabNoise(Indb++);
      float MaxLevel = ABS(Band.maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
    if (Verbose == True)
         cout << "FCUR MAX_NormCoef = " << MAX_NormCoef << endl;

   return MAX_NormCoef;
}
/****************************************************************************/

float MCA::fcur_init(Ifloat & ImaFCur, Bool SetMAD)
{
   float MAX_NormCoef=0.;
   Ifloat Band;
   int b,s,Indb=0;

   if (SetMAD == False)
   {
        for (s=0; s < FCur->nbr_scale()-1; s++)
	for (b=0; b < FCur->nbr_band(s); b++)
        {
 	    FCUR_TabNoise(Indb++) =  FCur->norm_band(s,b) * DataSigmaNoise;
 	}
   }

   FCur->cur_trans(ImaFCur);
   Indb=0;
   for (s=0; s < FCur->nbr_scale()-1; s++)
   for (b=0; b < FCur->nbr_band(s); b++)
   {
      FCur->get_band(s,b,Band);
      if (SetMAD == True) FCUR_TabNoise(Indb) = detect_noise_from_mad(Band);
      float Noise =  FCUR_TabNoise(Indb++);
      float MaxLevel = ABS(Band.maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
   if (Verbose == True)
         cout << "FCUR MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}
/****************************************************************************/

void MCA::fcur_proj(Ifloat & ImaCur, float Lamba_Sigma)
{
   int b,s,i,j,Indb=0;
   // int Nls,Ncs,Depi,Depj;
   // float Coef;
   // float CurNoise = DataSigmaNoise;
   float NSigma = Lamba_Sigma;
   Ifloat Band,BandCorrel;

   // PCUR.Verbose = True;
//   cout << " TRANS " << endl;
   FCur->cur_trans(ImaCur);
   if (UseCorrelConstraint == True)
   {
      FCUR_Correl->cur_trans(ImaCorrel);
   }
//   cout << " TRANS OK: " << CUR.nbr_band() << endl;

   for (s=0; s < FCur->nbr_scale()-1; s++)
   for (b=0; b < FCur->nbr_band(s); b++)
   {
        float NSig = (s == 0) ? NSigma + 1: NSigma;
        if (NSigma == 0) NSig = 0;
        float Noise = FCUR_TabNoise(Indb++);
        float Level = NSig*Noise;
	// cout << " FCUR Band " << b+1 << " Noise = " << Noise << " Level  = " <<   Level << endl;
	// FCur->get_band(s,b,Band);
	if (UseCorrelConstraint == False)
	{
   	    for (i=0; i < FCur->size_band_nl(s,b); i++)
            for (j=0; j < FCur->size_band_nc(s,b); j++)
	                (*FCur)(s,b,i,j) = mca_update((*FCur)(s,b,i,j), Level, Noise);
        }
	else
	{
	   float CorrelLevel = Lamba_Sigma * CorrelConstraint;
	   // FCUR_Correl->get_band(s,b,BandCorrel);
	   for (i=0; i < FCur->size_band_nl(s,b); i++)
           for (j=0; j < FCur->size_band_nc(s,b); j++)
  	   {
	      float AdaptLevel = Level + CorrelLevel * ABS((*FCUR_Correl)(s,b,i,j));
	      (*FCur)(s,b,i,j) =  mca_update((*FCur)(s,b,i,j), AdaptLevel, Noise);
	   }
        }
	// FCur->put_band(s,b,Band);
    }

   s = FCur->nbr_scale()-1;
   if (FCUR_KillLastScale == True) (FCur->TabCF_Band[s][0]).init();

   FCur->cur_recons(ImaCur);
//   cout << " REC OK " << endl;
}

/****************************************************************************/

float MCA::pcur_max(Ifloat & ImaCur)
{
   float MAX_NormCoef=0.;
   Ifloat Band;
   int b;

   PCUR.transform(ImaCur);
   for (b=0; b < PCUR.nbr_band()-1; b++)
   {
      PCUR.get_band(b,Band);
      float Noise =  PCUR_TabNoise(b);
      float MaxLevel = ABS(Band.maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
    if (Verbose == True)
         cout << "CUR MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}


/****************************************************************************/

float MCA::pcur_init(Ifloat & ImaCur, Bool SetMAD)
{
   float CUR_SigmaNoise = DataSigmaNoise;
   float MAX_NormCoef=0.;
   Ifloat Band;
   int b;

   if (SetMAD == False)
   {
        Ifloat ImSimu(Nl,Nc,"ImSimu");
        unsigned int InitRnd = 10;
        im_noise_gaussian (ImSimu, CUR_SigmaNoise, InitRnd);
	PCUR.transform(ImSimu);
        for (b=0; b < PCUR.nbr_band()-1; b++)
        {
            PCUR.get_band(b,Band);
	    PCUR_TabNoise(b) = Band.sigma();
	    // cout << "Band  = " << b << " Sigma = " << PCUR_TabNoise(b) << endl;
	}
   }
   PCUR.transform(ImaCur);
   for (b=0; b < PCUR.nbr_band()-1; b++)
   {
      PCUR.get_band(b,Band);
      if (SetMAD == True) PCUR_TabNoise(b) = detect_noise_from_mad(Band);
      float Noise =  PCUR_TabNoise(b);
      float MaxLevel = ABS(Band.maxfabs());
      float NormMaxLevel = MaxLevel / Noise;
      if (MAX_NormCoef < NormMaxLevel) MAX_NormCoef = NormMaxLevel;
   }
   if (Verbose == True)
         cout << "CUR MAX_NormCoef = " << MAX_NormCoef << endl;
   return MAX_NormCoef;
}

/****************************************************************************/

void MCA::pcur_proj(Ifloat & ImaCur, float Lamba_Sigma)
{
   int b,s2d,s1d,i,j;
   // int Nls,Ncs,Depi,Depj;
   // float Coef;
   // float CurNoise = DataSigmaNoise;
   float NSigma = Lamba_Sigma;
   Ifloat Band,BandCorrel;

   // PCUR.Verbose = True;
//   cout << " TRANS " << endl;
   PCUR.transform(ImaCur);
   if (UseCorrelConstraint == True)
   {
      PCUR_Correl.transform(ImaCorrel);
   }
//   cout << " TRANS OK: " << CUR.nbr_band() << endl;

    for (b=0; b < PCUR.nbr_band()-1; b++)
    {
        PCUR.get_scale_number(b, s2d, s1d);
        float NSig = (s1d == 0) ? NSigma + 1: NSigma;
        if (NSigma == 0) NSig = 0;
        float Noise = (s1d == PCUR.nbr_rid_scale(s2d)-1) ? PCUR_TabNoise(b-1): PCUR_TabNoise(b);
        float Level = NSig*Noise;
	// cout << " PCUR Band " << b+1 << " Noise = " << Noise << " Level  = " <<   Level << endl;
	if (UseCorrelConstraint == False)
	{
	    // float T=0.;
 	    for (i=0; i < PCUR.size_band_nl(b); i++)
            for (j=0; j < PCUR.size_band_nc(b); j++)
	     {
	         // T += ABS(Band(i,j));
  	         PCUR(b,i,j) =  mca_update(PCUR(b,i,j),Level, Noise);
 	     }
	     // cout << " Band " << b+1 << " MeanABS = " << T / (float)(Band.nl()*Band.nc());
         }
	else
	{
	   float CorrelLevel = Lamba_Sigma * CorrelConstraint;
 	   for (i=0; i < PCUR.size_band_nl(b); i++)
           for (j=0; j < PCUR.size_band_nc(b); j++)
	   {
	      float AdaptLevel = Level + CorrelLevel * ABS(PCUR_Correl(b,i,j));
	      PCUR(b,i,j) =  mca_update(PCUR(b,i,j), AdaptLevel, Noise);
	   }
        }
    }

   b = PCUR.nbr_band()-1;
   if (PCUR_KillLastScale == True)
   {
       for (i=0; i < Nl; i++)
       for (j=0; j < Nc; j++)  PCUR(b,i,j) = 0;
   }
   PCUR.recons(ImaCur);
//   cout << " REC OK " << endl;
}

/******************************************************/

void MCA::reconstruction(Ifloat &Result)
{
   int b;
   Result = TabImaRec[0];
   for (b=1; b < NbrBase; b++)  Result += TabImaRec[b];
//  INFO_X(Result, "RES");
//   if (PositivRecIma == True) threshold(Result);
//    if (UseZoom == True)
//    {
//      for (int i = 0; i < Nl; i++)
//      for (int j = 0; j < Nc; j++) if (Result(i,j) > 255.) Result(i,j) = 255;
//    }
     if (WriteAll == True) io_write_ima_float("xx_res.fits", Result);
}

/****************************************************************************/

void MCA::make_residual(Ifloat &Data, Ifloat &Residual)
{
   int i,j;

   // Get the new solution
   reconstruction(Residual);

   // Get the Residual
   if (UseMask == False)
   {
      if (UseZoom == True)
      {
         for (i = 0; i < Nl; i+=ZoomFactor)
         for (j = 0; j < Nc; j+=ZoomFactor)
	 {
	    float IM = 0.;
	    for (int k=i; k < i+ZoomFactor; k++)
	    for (int l=j; l < j+ZoomFactor; l++)  IM += Residual(k,l, I_MIRROR);
	    IM  = UnZoomData(i/ZoomFactor,j/ZoomFactor) - IM / (float) (ZoomFactor*ZoomFactor);
	    for (int k=i; k < MIN(Nl,i+ZoomFactor); k++)
	    for (int l=j; l < MIN(Nc,j+ZoomFactor); l++)  Residual(k,l) = IM;
	 }
      }
      else
      {
         for (i = 0; i < Nl; i++)
         for (j = 0; j < Nc; j++) Residual(i,j) = Data(i,j) - Residual(i,j);
      }
   }
   else
   {
         for (i = 0; i < Nl; i++)
         for (j = 0; j < Nc; j++) Residual(i,j) = (Data(i,j) - Residual(i,j))*MaskedData(i,j);
    }
}

/****************************************************************************/

void MCA::decomposition(Ifloat &Data)
{
   int i,j,b,l;
   float BoundMin;
   float BoundMax;
   ATROUS_2D_WT AWT;
   AWT.alloc(AT_Trans, Nl, Nc, AT_NbrScale2D);
	 AWT.alloc(AT_Trans1, Nl, Nc, AT_NbrScale2D);
	 AWT.alloc(AT_Trans2, Nl, Nc, AT_NbrScale2D);
   // RegulIma RIM;
   MR_Regul RIM;
   RIM.NbrScale = TV_NbrScale2D;
   RIM.ExpDecreasingLambda = True;

   // int NbrScaleTV = 5;
   // RIM.GradOperType = OPER_ENERGY;
   // RIM.GradOperType = OPER_LAPLACIAN;
   Ifloat TVIma;
   if ((Data.nl() != Nl) || (Data.nc() != Nc))
     {
     cout << "Error: MCA class not initialized with this image size" << Data.nl() << " " << Data.nc() << endl;
     cout << "       Expected size is " << Nl << " " << Nc << endl;
     exit(-1);
     }

    if (RemoveLastScale == True)
      {
      Bool DOATAlloc = False;
        if (AT_Trans == NULL)
         {
	       DOATAlloc = True;
	       AWT.alloc(AT_Trans, Nl, Nc, AT_NbrScale2D);
  	     }
	    AWT.transform(Data, AT_Trans, AT_NbrScale2D);
	    LastScale = AT_Trans[AT_NbrScale2D-1];
	    INFO_X(LastScale, "LAST SCALE");
	    Data -= LastScale;
 	    if (DOATAlloc == True) AWT.free(AT_Trans, AT_NbrScale2D);
      }

    if (Bounded == True)
      {
      BoundMin = 1e6;
      BoundMax = -1e6;
      for (i = 0; i < Nl; i++)
        for (j = 0; j < Nc; j++)
          {
          if (Data(i,j) > BoundMax)  BoundMax = Data(i,j);
          if (Data(i,j) < BoundMin)  BoundMin = Data(i,j);
          }
      cout << "BoundMax="<< BoundMax<< endl;
      cout << "BoundMin="<< BoundMin<< endl;
      }

    // Initialisation
    float AllMax=0.;
    float GetMax=0.;
    float SecondMax = 0.;
    if (Verbose == True)
      cout << " Gaussian noise estimation =  " <<  DataSigmaNoise << endl;
    for (b=0; b < NbrBase; b++)
      {
      switch(TabSelect[b])
        {
        case MCA_RID:
        case MCA_FFT2D_WT1D:
        case MCA_DDWT:
        case MCA_MCOS:
          break;
        case MCA_COS:  GetMax = cos_init(Data); break;
        case MCA_PIXEL: GetMax = pixel_init(Data,UseMad); break;
        case MCA_ATROU: GetMax = atrou_init(Data,UseMad); break;
        case MCA_WT: GetMax = wt_init(Data,UseMad); break;
        case MCA_CUR: GetMax = cur_init(Data,UseMad); break;
        case MCA_PCUR: GetMax = pcur_init(Data,UseMad); break;
        case MCA_FCUR:GetMax = fcur_init(Data,UseMad); break;
        case MCA_WP: GetMax = wp_init(Data,UseMad); break;
        default:
        cout << "Error: not implemeted transform ... " << endl;
        exit(-1);
        }
	    if (GetMax > AllMax)
	      {
	      SecondMax = AllMax;
	      AllMax=GetMax;
	      }
	    else if ((GetMax > SecondMax) && (GetMax < AllMax)) SecondMax = GetMax;
    }
    if (Verbose == True)
      cout << " Maximum SNR of all coeff is " << AllMax <<  " Second max = " << SecondMax << endl;
    if (SecondMax > 0) AllMax = SecondMax + (AllMax-SecondMax)*0.05;
   // if (RemoveLastScale == False) Data += LastScale;

   if (FirstSoftThreshold <  LastSoftThreshold) FirstSoftThreshold = AllMax;
   float DeltaThreshold = FirstSoftThreshold - LastSoftThreshold;
   StepL = DeltaThreshold / (float) (Nbr_Iter -1);
   Lambda = FirstSoftThreshold;

    if (Verbose == True)
      cout << " FirstThreshold =  " << FirstSoftThreshold <<  " LastThreshold = " << LastSoftThreshold << " Step = " << StepL << endl;
   Resi = Data;
   if ((TotalVariation == True) && (LambdaTV > 0))
       TVIma.alloc(Nl,Nc, "TV");
    // if (LambdaTV > 0)   LambdaTV *= sqrt((double) 2.) / Nbr_Iter;



     // *****  START ITERATE *****
    int Iter=0;
    while  ((Iter < Nbr_Iter) && (Lambda > LastSoftThreshold))
    {
       if (Iter>0)
       {
          switch (T_Decreasing)
	  {
	     case MCA_THRES_LINEAR:
	          Lambda -= StepL;
		  if (Lambda < LastSoftThreshold) Lambda = LastSoftThreshold;
		  break;
	     case MCA_THRES_EXP:
	          Lambda =  LastSoftThreshold
                          + DeltaThreshold  *(1.-erf(2.8*Iter/ Nbr_Iter));
	          if (Lambda < LastSoftThreshold) Lambda = LastSoftThreshold;
		  break;
	     case MCA_THRES_MOM:
	          DeltaThreshold = FirstSoftThreshold - LastSoftThreshold;
                  StepL = DeltaThreshold / (float)(Nbr_Iter-Iter);
	          if  (StepL < DataSigmaNoise/2.) StepL = DataSigmaNoise/2.;
                  Lambda = MIN(FirstSoftThreshold, Lambda - StepL);
		  if (Lambda < 0) Lambda = 0;
	          break;
	  }
       }

       // if (Verbose == True)
       if (Verbose == True)
               cout << endl << "Iteration " << Iter+1 << " Lambda = " << Lambda << endl;

       for (b=0; b < NbrBase; b++)
       {
          TVIma = TabImaRec[b];
          if (UseCorrelConstraint == True)
	  {
	     ImaCorrel.init();
	     for (int b1=0; b1 < NbrBase; b1++)
	             if (b1 != b) ImaCorrel += TabImaRec[b1];
 	  }
	  if (TabSelect[b] != MCA_DDWT)
	  {
 	     TabImaRec[b] += Resi;
	  }

          switch(TabSelect[b])
          {
             case MCA_DDWT:
                  for (int a =0; a < DWT_NbrAngle; a++)
		  {
		      if (Verbose == True) cout << "  ANGLE " << a+1 << " " << DWT_TabAngle(a) / PI * 180. << endl;
		      TabImaRec[b] -= DWT_TabImaRec[a];
		      if (UseNormL1 == True)
		      {
		         DWT_TabImaRec[a] += Resi;
 		         dwt_proj(DWT_TabImaRec[a], Lambda, DWT_TabAngle(a));
 		      }
	              else
		      {
		         dwt_proj(Resi, Lambda, DWT_TabAngle(a));
		         DWT_TabImaRec[a] += Resi;
 		      }
		      if (DWT_PositivRecIma == True) threshold(DWT_TabImaRec[a]);
		      TabImaRec[b] += DWT_TabImaRec[a];
 		      make_residual(Data, Resi);
                      if (Verbose == True) INFO_X(DWT_TabImaRec[a], "    REC ");
 		  }
		  if (Verbose == True) cout << endl;
	          break;
	     case MCA_MCOS:
                  if (Verbose == True) cout << "Local MDCT proj " << endl;
                  mcos_proj(TabImaRec[b], Lambda);
                  break;
	     case MCA_COS:
                  if (Verbose == True) cout << "Local DCT proj " << endl;
                  cos_proj(TabImaRec[b], Lambda);
                  break;
            case MCA_PIXEL:
                  if (Verbose == True) cout << "PIXEL basis proj " << endl;
                  pixel_proj(TabImaRec[b], Lambda);
                  break;
	    case MCA_ATROU:
                  if (Verbose == True) cout << "Atrou proj " << endl;
                  atrou_proj(TabImaRec[b], Lambda);
                  break;
             case MCA_RID:
                  if (Verbose == True) cout << "Rid proj " << endl;
                  rid_proj(TabImaRec[b], Lambda);
                  break;
             case MCA_WT:
                  if (Verbose == True) cout << "WT proj " << endl;
                  wt_proj(TabImaRec[b],  Lambda);
                  break;
             case MCA_WP:
                  if (Verbose == True) cout << "WP proj " << endl;
                  wp_proj(TabImaRec[b],  Lambda);
                  break;
	     case MCA_CUR:
                  if (Verbose == True) cout << "Curvelet proj " << endl;
                  cur_proj(TabImaRec[b], Lambda);
                  break;
	     case MCA_PCUR:
                  if (Verbose == True) cout << "Pyr. Curvelet proj " << endl;
                  pcur_proj(TabImaRec[b], Lambda);
                  break;
	     case MCA_FCUR:
                  if (Verbose == True) cout << "Fast Curvelet proj " << endl;
                  fcur_proj(TabImaRec[b], Lambda);
                  break;
	     case MCA_FFT2D_WT1D:
	          if (Verbose == True) cout << "FFT2D-WT1D proj " << endl;
		  // fft2dwt1d_proj(TabImaRec[b], Lambda);
		  break;
	     default:
               cout << "Error: not implemeted transform ... " << endl;
               exit(-1);
          }

	  if (TabSelect[b] != MCA_DDWT)
	  {
 	     for (i = 0; i < Nl; i++)
             for (j = 0; j < Nc; j++)  Resi(i,j) = (TabImaRec[b])(i,j) - TVIma(i,j);

            // New residual calculation
            if ((TotalVariation == True) && (LambdaTV > 0) &&
	         (TabSelect[b] != MCA_FFT2D_WT1D)
		 && (TabSelect[b] != MCA_DDWT)
		 && (TabSelect[b] != MCA_COS)
		  && (TabSelect[b] != MCA_WP)
		 && (TabSelect[b] != MCA_PIXEL)
		 && (TabSelect[b] != MCA_ATROU))
	    {
               // RIM.obj_regul(TVIma, TabImaRec[b], LambdaTV);
	       // regul_tv_haarwt(TabImaRec[b], TabImaRec[b], LambdaTV, NbrScaleTV);
 	       // RIM.obj_regul(TVIma, TabImaRec[b], LambdaTV*Noise_Ima);
	       RIM.im_soft_threshold(TabImaRec[b], TabImaRec[b], LambdaTV*DataSigmaNoise);
	    }
	 }

	  // Positivity constraint
	  switch(TabSelect[b])
          {
             case MCA_ATROU:
	          if (AT_PositivRecIma == True) threshold(TabImaRec[b]);
                  break;
             case MCA_RID:
                   if (RID_PositivRecIma == True) threshold(TabImaRec[b]);
                  break;
             case MCA_WT:
                   if (WT_PositivRecIma == True) threshold(TabImaRec[b]);
                  break;
             case MCA_CUR:
                   if (CUR_PositivRecIma == True) threshold(TabImaRec[b]);
		   if (UseZoom == True)
		   {
		     for (i = 0; i < Nl; i++)
                     for (j = 0; j < Nc; j++)
		        if ( TabImaRec[b](i,j)> 255) TabImaRec[b](i,j) = 255;
	          }
                  break;
	     case MCA_PCUR:
                   if (PCUR_PositivRecIma == True) threshold(TabImaRec[b]);
		   if (UseZoom == True)
		   {
		     for (i = 0; i < Nl; i++)
                     for (j = 0; j < Nc; j++)
		        if ( TabImaRec[b](i,j)> 255) TabImaRec[b](i,j) = 255;
	          }
                  break;
	     case MCA_FCUR:
                   if (FCUR_PositivRecIma == True) threshold(TabImaRec[b]);
		   if (UseZoom == True)
		   {
		     for (i = 0; i < Nl; i++)
                     for (j = 0; j < Nc; j++)
		        if ( TabImaRec[b](i,j)> 255) TabImaRec[b](i,j) = 255;
	          }
                  break;
	     case MCA_WP:
                   if (WP_PositivRecIma == True) threshold(TabImaRec[b]);
		  if ((b == 1) && (UseZoom == True))
		  {
		     for (i = 0; i < Nl; i++)
                     for (j = 0; j < Nc; j++)
		     {
		        if ( TabImaRec[b](i,j) < -TabImaRec[b-1](i,j)) TabImaRec[b](i,j) = -TabImaRec[b-1](i,j);
			if ( (TabImaRec[b](i,j)  + TabImaRec[b-1](i,j)) > 255) TabImaRec[b](i,j) = 255-TabImaRec[b-1](i,j);
	             }
		  }
                  break;
	     case MCA_FFT2D_WT1D:
	     case MCA_DDWT:
	     case MCA_PIXEL:
	          {
		     float M = TabImaRec[b].mean();
		     float Max=5* DataSigmaNoise;
 		     for (i = 0; i < Nl; i++)
                     for (j = 0; j < Nc; j++)
		     {
		        float Coef = (TabImaRec[b])(i,j) - M;
			if (Coef > Max) Coef = Max;
			else if (Coef < -Max) Coef = -Max;
		        (TabImaRec[b])(i,j) = Coef;
                     }
  		  }
		  if (COS_Isotrop == True) contraint_isotrop_fft(TabImaRec[b], Verbose);
	          break;
	     case MCA_MCOS:
	          if (MCOS_PositivRecIma == True) threshold(TabImaRec[b]);
		  break;
	     case MCA_COS:
 	          if (COS_Isotrop == True) contraint_isotrop_fft(TabImaRec[b], Verbose);
		  if ((b == 1) && (UseZoom == True))
		  {
		     for (i = 0; i < Nl; i++)
                     for (j = 0; j < Nc; j++)
		     {
		        if ( TabImaRec[b](i,j) < -TabImaRec[b-1](i,j)) TabImaRec[b](i,j) = -TabImaRec[b-1](i,j);
	                if ( (TabImaRec[b](i,j)  + TabImaRec[b-1](i,j)) > 255) TabImaRec[b](i,j) = 255-TabImaRec[b-1](i,j);
 		     }
		  }
		  break;
	     default:
               cout << "Error: not implemeted transform ... " << endl;
               exit(-1);
          }


		// ADD SIGMA BOUNDED CONDITION //
     if (SigmaBounded == True)
      {
      // wavelet transform
      //AWT.transform(Data, AT_Trans1, AT_NbrScale2D);
			AWT.transform(TabImaRec[b], AT_Trans2, AT_NbrScale2D);
      //float percentage = 0.95;
      float percentage = 1.;
      for (l = 0; l < AT_NbrScale2D-1; l++)
        {
        float SigmaInt = 0;
        float SigmaOut = 0;
        float NInt = 0;
        float NOut = 0;
        float XInt = 0;
        float XOut = 0;
        float X2Int = 0;
        float X2Out = 0;
        for (i = 0; i < Nl; i++)
          for (j = 0; j < Nc; j++)
            {
            if (MaskedData(i,j) == 0)
              {
              NInt = NInt + 1;
              XInt = XInt + (AT_Trans2[l])(i,j);
              X2Int = X2Int + (AT_Trans2[l])(i,j)*(AT_Trans2[l])(i,j);
              }
            else
              {
              NOut = NOut + 1;
              //XOut = XOut + (AT_Trans1[l])(i,j);
							XOut = XOut + (AT_Trans2[l])(i,j);
              //X2Out = X2Out + (AT_Trans1[l])(i,j)*(AT_Trans1[l])(i,j);
							X2Out = X2Out + (AT_Trans2[l])(i,j)*(AT_Trans2[l])(i,j);
              }
            }
          SigmaInt = sqrt(X2Int/NInt - (XInt/NInt)*(XInt/NInt));
          SigmaOut = sqrt(X2Out/NOut - (XOut/NOut)*(XOut/NOut));
          //cout << "SigmaInt ="<< SigmaInt << endl;
          //cout << "SigmaOut ="<< SigmaOut << endl;

          //if (SigmaInt > 0.95*SigmaOut)
					if (SigmaInt > 0.2*SigmaOut)
            {
            for (i = 0; i < Nl; i++)
              for (j = 0; j < Nc; j++)
                {
                (AT_Trans2[l])(i,j)= (AT_Trans2[l])(i,j)*percentage*(SigmaOut/SigmaInt);
                }
            }
         }
      AWT.recons(AT_Trans2,TabImaRec[b], AT_NbrScale2D);
       //if (percentage < 1.) percentage = percentage + 0.05;
       }

    //ADD BOUNDED CONDITION//

    if (Bounded == True)
      {
      //float SigmaInt = 0;
      //float SigmaOut = 0;
      //float NInt = 0;
      //float NOut = 0;
      //float XInt = 0;
      //float XOut = 0;
      //float X2Int = 0;
      //float X2Out = 0;
      //for (i = 0; i < Nl; i++)
        //for (j = 0; j < Nc; j++)
         // {
         // if (MaskedData(i,j) == 0)
          //  {
           // NInt = NInt + 1;
           // XInt = XInt + (TabImaRec[b])(i,j);
           // X2Int = X2Int + (TabImaRec[b])(i,j)*(TabImaRec[b])(i,j);
           // }
         // else
           // {
           // NOut = NOut + 1;
            //XOut = XOut + (TabImaRec[b])(i,j);
            //X2Out = X2Out + (TabImaRec[b])(i,j)*(TabImaRec[b])(i,j);
           // }
          //}
      //if ((X2Int/NInt - (XInt/NInt)*(XInt/NInt) > 0) && (X2Out/NOut - (XOut/NOut)*(XOut/NOut) > 0))
        //{
       // SigmaInt = sqrt(X2Int/NInt - (XInt/NInt)*(XInt/NInt));
       // SigmaOut = sqrt(X2Out/NOut - (XOut/NOut)*(XOut/NOut));
       // }
      //else
       // {
       // SigmaInt = 1;
       // SigmaOut = 1;
       // }
      //cout << "SigmaInt2 ="<< X2Int/NInt - (XInt/NInt)*(XInt/NInt) << endl;
      //cout << "SigmaInt ="<< SigmaInt << endl;
      //cout << "SigmaOut ="<< SigmaOut << endl;

      for (i = 0; i < Nl; i++)
        for (j = 0; j < Nc; j++)
          {
         // if (SigmaInt > 0.95*SigmaOut)  (TabImaRec[b])(i,j)	= (TabImaRec[b])(i,j)*0.95*(SigmaOut/SigmaInt);
          if ((TabImaRec[b])(i,j) > BoundMax)  (TabImaRec[b])(i,j) = BoundMax;
          if ((TabImaRec[b])(i,j) < BoundMin)  (TabImaRec[b])(i,j) = BoundMin;
          }
      }

		make_residual(Data, Resi);

 	  if (Verbose == True)
	  {
             // cout << " Base " << b+1 << ": Reconstruction" << endl;
             INFO_X(TabImaRec[b], "   REC");
             INFO_X(Resi, "   Residual");
	  }
        } // ENDFOR (b=0 ...)


        if (((Iter+1) % 5 == 0) && (WriteAll == True)) write_allima("xx", Iter);


 	if ((T_Decreasing == MCA_THRES_MOM) && (Lambda > LastSoftThreshold))
	{
	   // cout << "   MOM calculation " << endl;

	   GetMax=0.;
           SecondMax = 0.;
	   AllMax = 0.;
           for (b=0; b < NbrBase; b++)
           {
                switch(TabSelect[b])
                {
         	    case MCA_RID:
	 	    case MCA_FFT2D_WT1D:
         	    case MCA_DDWT:
  	 	    case MCA_MCOS:
		             cout << "Error: MOM is not implemented for this transform ... " << endl;
			     exit(-1);
         	          break;
	 	    case MCA_COS:  GetMax = cos_max(Resi); break;
         	    case MCA_PIXEL: GetMax = pixel_max(Resi); break;
	 	    case MCA_ATROU: GetMax = atrou_max(Resi); break;
         	    case MCA_WT: GetMax = wt_max(Resi); break;
         	    case MCA_CUR: GetMax = cur_max(Resi); break;
	  	    case MCA_PCUR: GetMax = pcur_max(Resi); break;
	  	    case MCA_FCUR:GetMax = fcur_max(Resi); break;
 	  	    case MCA_WP: GetMax = wp_max(Resi); break;
 	   	  default:
          	     cout << "Error: not implemeted transform ... " << endl;
           	    exit(-1);
        	 }
		 if (GetMax > AllMax)
		 {
		    SecondMax = AllMax;
		    AllMax=GetMax;
		 }
		 else if ((GetMax > SecondMax) && (GetMax < AllMax)) SecondMax = GetMax;

    	   }  // endfor b

   	   if (Verbose == True)
   	   cout << " Maximum SNR of all coeff is " << AllMax <<  " Second max = " << SecondMax << endl;
 	   if (SecondMax > 0)
	   {
	       AllMax = SecondMax + (AllMax-SecondMax)*0.05;
	       float A2 = SecondMax + SecondMax * 0.05;
	       if (A2 < AllMax) AllMax  = A2;
           }
           FirstSoftThreshold = AllMax;
           if (Lambda <= LastSoftThreshold) Iter = Nbr_Iter;

       } // endif (T_Decreasing == MCA_THRES_MOM)
       Iter++;
    } // ENDFOR (Iter=...)
}

/****************************************************************************/

void MCA::write_allima(char *FileName, fitsstruct & Header)
{
   char Pre[512];
   char FName[512];
   io_strcpy_prefix(Pre, FileName);
   for (int b=0; b < NbrBase; b++)
   {
      switch(TabSelect[b])
      {
         case MCA_ATROU:
	       sprintf(FName,"%s_atrou", Pre);
	       break;
          case MCA_RID:
	       sprintf(FName,"%s_rid", Pre);
               break;
          case MCA_CUR:
	       sprintf(FName,"%s_cur", Pre);
               break;
          case MCA_PCUR:
	       sprintf(FName,"%s_pcur", Pre);
               break;
	  case MCA_FCUR:
	       sprintf(FName,"%s_fcur", Pre);
               break;
	  case MCA_WT:
	       sprintf(FName,"%s_wt", Pre);
	       break;
  	  case MCA_FFT2D_WT1D:
	       sprintf(FName,"%s_ps", Pre);
	       break;
	  case MCA_PIXEL:
	       sprintf(FName,"%s_pix", Pre);
	       break;
	  case MCA_COS:
	       sprintf(FName,"%s_cos", Pre);
	       break;
	  case MCA_MCOS:
	       sprintf(FName,"%s_mcos", Pre);
	       break;
	  case MCA_WP:
	       sprintf(FName,"%s_wp", Pre);
	       break;
	  case MCA_DDWT:
	       for (int a =0; a < DWT_NbrAngle; a++)
	       {
		  sprintf(FName,"%s_dwt_%d", Pre, a+1);
		  io_write_ima_float(FName, DWT_TabImaRec[a], &Header);
	       }
	       sprintf(FName,"%s_dwt", Pre);
	       break;
	  default:
               cout << "Error: not implemeted transform ... " << endl;
               exit(-1);
       }
       io_write_ima_float(FName, TabImaRec[b], &Header);
   }
   sprintf(FName,"%s_resi", Pre);
   io_write_ima_float(FName, Resi, &Header);

}

/****************************************************************************/

/****************************************************************************/

void MCA::write_allima(char *FileName, int It)
{
   char Pre[512];
   char FName[512];
   io_strcpy_prefix(Pre, FileName);
   for (int b=0; b < NbrBase; b++)
   {
      switch(TabSelect[b])
      {
	  case MCA_PIXEL:
	       sprintf(FName,"%s_pix", Pre);
	       break;
          case MCA_MCOS:
	       sprintf(FName,"%s_mcos", Pre);
	       break;
	  case MCA_COS:
	       sprintf(FName,"%s_cos", Pre);
	       break;
	  case MCA_ATROU:
	       sprintf(FName,"%s_atrou", Pre);
	       break;
          case MCA_RID:
	       sprintf(FName,"%s_rid", Pre);
               break;
          case MCA_CUR:
	       sprintf(FName,"%s_cur", Pre);
               break;
          case MCA_PCUR:
	       sprintf(FName,"%s_pcur", Pre);
               break;
	  case MCA_FCUR:
	       sprintf(FName,"%s_fcur", Pre);
               break;
	  case MCA_WT:
	       sprintf(FName,"%s_wt", Pre);
	       break;
  	  case MCA_WP:
	       sprintf(FName,"%s_wp", Pre);
	       break;
	  case MCA_FFT2D_WT1D:
	       sprintf(FName,"%s_ps", Pre);
	       break;
	  case MCA_DDWT:
	       sprintf(FName,"%s_dwt", Pre);
	       break;
          default:
               cout << "Error: not implemeted transform ... " << endl;
               exit(-1);
       }
       sprintf(FName, "%s_%d", (char *) FName, It+1);
       io_write_ima_float(FName, TabImaRec[b]);
   }
   sprintf(FName,"%s_resi", Pre);
   sprintf(FName, "%s_%d", FName, It+1);
   io_write_ima_float(FName, Resi);
 }

/****************************************************************************/


void MCA::free()
{
   for (int b=0; b < NbrBase; b++)
   {
      switch(TabSelect[b])
      {
            case MCA_ATROU: AWT.free(AT_Trans, AT_NbrScale2D);
                  break;
            case MCA_MCOS:
	           M_DCT.free();
	           break;
	     case MCA_RID:
	     case MCA_FFT2D_WT1D:
             case MCA_COS:
	           break;
             case MCA_WT:
                 if  (WT_NbrScale2D > 2)
                 {
                    WT->free(WT_Trans, WT_NbrScale2D);
                    delete WT_SelectFilter;
                    delete WT_SB1D;
                    delete WT;
                    delete [] WT_TabDec;
                    WT_NbrScale2D = 0;
                    WT_SelectFilter = NULL;
                    WT_SB1D = NULL;
                    WT_TabDec = NULL;
                    WT = NULL;
                 }
                 break;
   	    case MCA_DDWT:
	    case MCA_PIXEL:
	          break;
            case MCA_CUR:
		 CUR_NbrScale2D = 0;
                 break;
	    case MCA_PCUR:
		 PCUR_NbrScale2D = 0;
                 break;
	    case MCA_FCUR:
		 PCUR_NbrScale2D = 0;
		 if (FCur != NULL) delete FCur;
                 break;
	    case MCA_WP:
	          delete WP;
	         break;
	    default:
               cout << "Error: not implemeted transform ... " << endl;
               exit(-1);
      }
   }
   NbrBase = 0;
}

/***********************************************************************/
