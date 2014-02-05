#define DIAGNOSTICS_PRIVATE_DEFS
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../Headers/Sim.h"
#include "../Headers/Cell.h"
#include "../Headers/GravMass.h"
#include "../Headers/Diagnostics.h"
#include "../Headers/TimeStep.h"
#include "../Headers/MPIsetup.h"
#include "../Headers/header.h"

void diagnostics_set(struct Diagnostics * theDiagnostics,struct Cell *** theCells,struct Sim * theSim,struct TimeStep * theTimeStep,struct MPIsetup * theMPIsetup,struct GravMass * theGravMasses){
  if (time_global <= 0.006 ){
    //Make a File that only writes once to output constant parameters                                                                                
    double q      = sim_MassRatio(theSim); 
    double sep0   = sim_sep0(theSim);
    double alpha  = sim_EXPLICIT_VISCOSITY(theSim);
    double Mach0  = sim_Mach(theSim);
    double MdoMs  = sim_Mdisk_ovr_Ms(theSim);   
    double rmin   = sim_MIN(theSim, R_DIR);
    double rmax   = sim_MAX(theSim, R_DIR);
    double tmigon = sim_tmig_on(theSim); 
    double tacc   = sim_RhoSinkTimescale(theSim);    
    double eps    = sim_G_EPS(theSim);
    double Rcut   = sim_Rcut(theSim);
    if(mpisetup_MyProc(theMPIsetup)==0){
      char CstParamFilename[256];
      sprintf(CstParamFilename,"2CstParam.dat");
      FILE * CstParamFile = fopen(CstParamFilename,"a");
      fprintf(CstParamFile,"%e %e %e %e %e %e %e %e %e %e %e\n", q, sep0, alpha, Mach0, MdoMs, rmin, rmax, tmigon, tacc, eps, Rcut);
      fclose(CstParamFile);
    }
  }

  if (timestep_get_t(theTimeStep)>diagnostics_tdiag_measure(theDiagnostics)){
    int num_r_points = sim_N(theSim,R_DIR)-sim_Nghost_min(theSim,R_DIR)-sim_Nghost_max(theSim,R_DIR);
    int num_r_points_global = sim_N_global(theSim,R_DIR);

    int NUM_SCAL = theDiagnostics->NUM_DIAG;   //DD: change the number of diagnostics in create and destroy NUM_DIAG=20 now
    int NUM_VEC = theDiagnostics->NUM_DIAG+1;
    int NUM_EQ = theDiagnostics->NUM_DIAG+2;
    int NUM_TST = 1;
    int NUM_TSTV = NUM_TST+1;


    int i,j,k,n;

    double * EquatDiag_temp = malloc(sizeof(double) * theDiagnostics->N_eq_cells*NUM_EQ);
    double * EquatDiag_reduce = malloc(sizeof(double) * theDiagnostics->N_eq_cells*NUM_EQ);
    double * VectorDiag_temp = malloc(sizeof(double) * num_r_points_global*NUM_VEC);
    double * VectorDiag_reduce = malloc(sizeof(double) * num_r_points_global*NUM_VEC);
    double * ScalarDiag_temp = malloc(sizeof(double)*NUM_SCAL);
    double * ScalarDiag_reduce = malloc(sizeof(double)*NUM_SCAL);
    
    double * TrVec_temp    = malloc(sizeof(double) * num_r_points_global); //store Trqs each measure step

    double TrScal_temp   = 0.0; // sum total trq each measure
    double TrScal_reduce = 0.0;

    double mass_near_bh0_r0p1_temp = 0.0;
    double mass_near_bh1_r0p1_temp = 0.0;
    double mass_near_bh0_r0p1_reduce = 0.0;
    double mass_near_bh1_r0p1_reduce = 0.0;

    double mass_near_bh0_r0p2_temp = 0.0;
    double mass_near_bh1_r0p2_temp = 0.0;
    double mass_near_bh0_r0p2_reduce = 0.0;
    double mass_near_bh1_r0p2_reduce = 0.0;

    double mass_near_bh0_r0p4_temp = 0.0;
    double mass_near_bh1_r0p4_temp = 0.0;
    double mass_near_bh0_r0p4_reduce = 0.0;
    double mass_near_bh1_r0p4_reduce = 0.0;


    double dtout = timestep_get_t(theTimeStep)-theDiagnostics->toutprev; 


    int imin = sim_Nghost_min(theSim,R_DIR);
    int imax = sim_N(theSim,R_DIR)-sim_Nghost_max(theSim,R_DIR);
    int kmin = sim_Nghost_min(theSim,Z_DIR);
    int kmax = sim_N(theSim,Z_DIR)-sim_Nghost_max(theSim,Z_DIR);

    for (n=0;n<NUM_SCAL;++n){
      ScalarDiag_temp[n]=0.0;
      ScalarDiag_reduce[n]=0.0;
    }
    for (i=0;i<num_r_points_global;++i){
      for (n=0;n<NUM_VEC;++n){
        VectorDiag_temp[i*NUM_VEC+n]=0.0;
      }
    }
    

    for (i=0;i<num_r_points_global;++i){
        TrVec_temp[i]=0.0;
    }


    int position=0;
    for (i=0;i<num_r_points_global;++i){
      for(j = 0; j < theDiagnostics->N_p_global[i]; j++){
        for (n=0;n<NUM_EQ;++n){
          EquatDiag_temp[position]=0.0;
          ++position;
        }
      }
    }
    double Rcut = sim_Rcut(theSim);
    position=0;
    for (k=kmin;k<kmax;++k){
      double zp = sim_FacePos(theSim,k,Z_DIR);
      double zm = sim_FacePos(theSim,k-1,Z_DIR);
      double z = 0.5*(zm+zp);
      double dz = zp-zm;
      for (i=imin;i<imax;++i){
        double rp = sim_FacePos(theSim,i,R_DIR);
        double rm = sim_FacePos(theSim,i-1,R_DIR);
        double r = 0.5*(rm+rp);
        for (j=0;j<sim_N_p(theSim,i);++j){
          double phi = cell_tiph(cell_single(theCells,i,j,k));
          double dphi = cell_dphi(cell_single(theCells,i,j,k));
          double rho = cell_prim(cell_single(theCells,i,j,k),RHO);
          double press = cell_prim(cell_single(theCells,i,j,k),PPP);
          double vr = cell_prim(cell_single(theCells,i,j,k),URR);
          double vp = cell_prim(cell_single(theCells,i,j,k),UPP)*r;
          double vz = cell_prim(cell_single(theCells,i,j,k),UZZ);
          double Br = cell_prim(cell_single(theCells,i,j,k),BRR);
          double Bp = cell_prim(cell_single(theCells,i,j,k),BPP);
          double Bz = cell_prim(cell_single(theCells,i,j,k),BZZ);
          double v2   = vr*vr + vp*vp + vz*vz;
          double B2   = Br*Br + Bp*Bp + Bz*Bz;
          double rhoe = press/(sim_GAMMALAW(theSim)-1.);
          double psi = cell_prim(cell_single(theCells,i,j,k),PSI);

          double passive_scalar = cell_prim(cell_single(theCells,i,j,k),5);
          double Omega = gravMass_omega(theGravMasses,1); //DD: changed this from double Omega = 1.;
          double t = timestep_get_t(theTimeStep);


         
	  double r_bh0 = gravMass_r(theGravMasses,0);
          double phi_bh0 = gravMass_phi(theGravMasses,0);
          double r_bh1 = gravMass_r(theGravMasses,1);
          double phi_bh1 = gravMass_phi(theGravMasses,1);
	  double q = sim_MassRatio(theSim);	  // Added Mass ratio DD

	  double eps1 = sim_G_EPS(theSim);
	  //double abhbin = sqrt(r_bh0*r_bh0 + r_bh1*r_bh1 - 2.*r_bh0*r_bh1*cos(phi_bh1-phi_bh0)); //Added abhbin DD
	  double abhbin = r_bh0 + r_bh1;
			
			//double dPhi_dphi = 1/4.*r*sin(phi-Omega*t) * (
			//pow(r*r+.25-r*cos(phi-Omega*t),-1.5) -  
			//pow(r*r+.25+r*cos(phi-Omega*t),-1.5));
	  double dPhi_dphi =  abhbin*r/((1.+q)*(1.+1./q))*sin(phi-Omega*t) * (  
							  pow(r*r+(abhbin/(1+1./q))*(abhbin/(1.+1./q))-2.*r*abhbin/(1.+1./q)*cos(phi-Omega*t),-1.5) -  
							  pow(r*r+(abhbin/(1+q))*(abhbin/(1+q))+2.*r*abhbin/(1+q)*cos(phi-Omega*t),-1.5)) ;  //DD: for general q (for entire binary)
			
	  // double dPhi_dphi_S =  -abhbin*r/((1.+q)*(1.+1./q))*sin(phi-Omega*t) * (  
	  //			pow( (eps1*eps1 +  r*r + (abhbin/(1.+q))*(abhbin/(1.+q)) + 2.*r*abhbin/(1.+q)*cos(phi-Omega*t) ),-1.5)) ;  //DD: for general q (for secondary only)

          double dist_bh0 = sqrt(r_bh0*r_bh0 + r*r - 2.*r_bh0*r*cos(phi_bh0-phi));
          double dist_bh1 = sqrt(r_bh1*r_bh1 + r*r - 2.*r_bh1*r*cos(phi_bh1-phi));

	  double dPhi_dphi_S =  -abhbin*r/((1.+q)*(1.+1./q))*sin(phi-Omega*t) * (
	       			pow( (eps1*eps1 + dist_bh1*dist_bh1 ),-1.5)) ;  //DD: for general q (for secondary only) 

	  double Rhill = abhbin * pow((q/3.),(1./3.));

          if (dist_bh0<0.1){
            double dV = 0.5*(rp*rp-rm*rm)*dphi;
            double dM = rho*dV;
            mass_near_bh0_r0p1_temp +=dM;
          }
          if (dist_bh1<0.1){
            double dV = 0.5*(rp*rp-rm*rm)*dphi;
            double dM = rho*dV;
            mass_near_bh1_r0p1_temp +=dM;
          }

          if (dist_bh0<0.2){
            double dV = 0.5*(rp*rp-rm*rm)*dphi;
            double dM = rho*dV;
            mass_near_bh0_r0p2_temp +=dM;
          }
          if (dist_bh1<0.2){
            double dV = 0.5*(rp*rp-rm*rm)*dphi;
            double dM = rho*dV;
            mass_near_bh1_r0p2_temp +=dM;
          }

          if (dist_bh0<0.4){
            double dV = 0.5*(rp*rp-rm*rm)*dphi;
            double dM = rho*dV;
            mass_near_bh0_r0p4_temp +=dM;
          }
          if (dist_bh1<0.4){
            double dV = 0.5*(rp*rp-rm*rm)*dphi;
            double dM = rho*dV;
            mass_near_bh1_r0p4_temp +=dM;
          }



          if ((fabs(zp)<0.0000001)||(fabs(z)<0.0000001)){
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+0] = r;
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+1] = phi;
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+2] = rho;
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+3] = vr;
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+4] = vp;
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+5] = press;
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+6] = rho*vr;            
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+7] = rho*vp;            
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+8] = rho*vr*cos(phi);            
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+9] = rho*vr*sin(phi);            
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+10] = rho*vr*cos(2.*phi);            
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+11] = rho*vr*sin(2.*phi);            
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+12] = r*rho*dPhi_dphi_S/r; //DD added only secondary Pot
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+13] = 0.5*B2;
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+14] = Br*Bp;
            EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+15] = psi;
	    EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+16] = 180./M_PI*0.5*asin(-Br*Bp/(0.5*B2));
	    EquatDiag_temp[(theDiagnostics->offset_eq+position)*NUM_EQ+17] = passive_scalar;
            ++position;
          }
          // divide by number of phi cells to get phi average, mult by dz because we are doing a z integration;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+0] += (r/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+1] += (rho/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+2] += (vr/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+3] += (vp/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+4] += (press/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+5] += (rho*vr/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+6] += (rho*vp/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+7] += (rho*vr*cos(phi)/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+8] += (rho*vr*sin(phi)/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+9] += (rho*vr*cos(2.*phi)/sim_N_p(theSim,i)*dz) ;
	  VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+10] += (rho*vr*sin(2.*phi)/sim_N_p(theSim,i)*dz) ;
          // Don't count torques within a certain radius from secondary add r and 2pi for integration
	  if (dist_bh1 > Rcut*Rhill && r>0.15){ //add 2pi to Trq to not azi average - see Scalar int below
	    	   VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+11] += (2.*M_PI*r*rho*dPhi_dphi_S/sim_N_p(theSim,i));
		   TrVec_temp[(sim_N0(theSim,R_DIR)+i-imin)]                 += (2.*M_PI*r*rho*dPhi_dphi_S/sim_N_p(theSim,i));//2pi/Nphi = dphi
	    //	   VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+20] += (2.*M_PI*rho*dPhi_dphi_S/sim_N_p(theSim,i)*dz); //DD added only secondary Pot 
	  }else{
	          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+11] += 0.0;
		  TrVec_temp[(sim_N0(theSim,R_DIR)+i-imin)]                 += 0.0;
	  }
	  //TESTING
	  // VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+11] += (2.*M_PI*r/sim_N_p(theSim,i)) ;
	  //
	  VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+12] += (0.5*B2/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+13] += (Br*Bp/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+14] += (psi/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+15] += (180./M_PI*0.5*asin(-Br*Bp/(0.5*B2))/sim_N_p(theSim,i)*dz) ;
          VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+16] += (passive_scalar/sim_N_p(theSim,i)*dz) ; 

    			// the above are just placeholders. Put the real diagnostics you want here, then adjust NUM_DIAG accordingly.
        }
      }
    }
	
    

    for (i=imin;i<imax;++i){
      double rp = sim_FacePos(theSim,i,R_DIR);
      double rm = sim_FacePos(theSim,i-1,R_DIR);
      TrScal_temp += TrVec_temp[(sim_N0(theSim,R_DIR)+i-imin)]*(rp-rm);
      for (n=0;n<NUM_SCAL;++n){
	if (n==10){ // the vol. el. 'r' already added so dr not dr^2 - does not need extra 1/2 in /(dz) below
	    ScalarDiag_temp[n] += VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+n+1]*(rp-rm);
	}else{
	  // mult by delta r^2 because we are doing an r integration (needs extra factor of 1/2 comes with /(dz) below) 
	    ScalarDiag_temp[n] += VectorDiag_temp[(sim_N0(theSim,R_DIR)+i-imin)*NUM_VEC+n+1]*(rp*rp-rm*rm); 
	}
	       
      }
    }

    MPI_Allreduce( ScalarDiag_temp,ScalarDiag_reduce , NUM_SCAL, MPI_DOUBLE, MPI_SUM, sim_comm);
    MPI_Allreduce( VectorDiag_temp,VectorDiag_reduce , num_r_points_global*NUM_VEC, MPI_DOUBLE, MPI_SUM, sim_comm);
    MPI_Allreduce( EquatDiag_temp,EquatDiag_reduce ,theDiagnostics->N_eq_cells*NUM_EQ, MPI_DOUBLE, MPI_SUM, sim_comm);

    //MPI_Allreduce( TrVec_temp, TrVec_reduce , num_r_points_global*NUM_TST, MPI_DOUBLE, MPI_SUM, sim_comm);
    MPI_Allreduce( &TrScal_temp, &TrScal_reduce , 1, MPI_DOUBLE, MPI_SUM, sim_comm);

    MPI_Allreduce( &mass_near_bh0_r0p1_temp,&mass_near_bh0_r0p1_reduce , 1, MPI_DOUBLE, MPI_SUM, sim_comm);
    MPI_Allreduce( &mass_near_bh1_r0p1_temp,&mass_near_bh1_r0p1_reduce , 1, MPI_DOUBLE, MPI_SUM, sim_comm);

    MPI_Allreduce( &mass_near_bh0_r0p2_temp,&mass_near_bh0_r0p2_reduce , 1, MPI_DOUBLE, MPI_SUM, sim_comm);
    MPI_Allreduce( &mass_near_bh1_r0p2_temp,&mass_near_bh1_r0p2_reduce , 1, MPI_DOUBLE, MPI_SUM, sim_comm);
 
    MPI_Allreduce( &mass_near_bh0_r0p4_temp,&mass_near_bh0_r0p4_reduce , 1, MPI_DOUBLE, MPI_SUM, sim_comm);
    MPI_Allreduce( &mass_near_bh1_r0p4_temp,&mass_near_bh1_r0p4_reduce , 1, MPI_DOUBLE, MPI_SUM, sim_comm);
 

    double RMIN = sim_MIN(theSim,R_DIR);
    double RMAX = sim_MAX(theSim,R_DIR);
    double ZMIN = sim_MIN(theSim,Z_DIR);
    double ZMAX = sim_MAX(theSim,Z_DIR);
	  

    // Torques at each measure - not averaged - pipe to Binary Params output file
    //double Tr   = TrScal_reduce; // /(ZMAX-ZMIN);
 
 
    for (n=0;n<NUM_SCAL;++n){
      if (n!=10){ //Don't Vol avg Torques
	ScalarDiag_reduce[n] *= dtout/((ZMAX-ZMIN)*(RMAX*RMAX-RMIN*RMIN));
      }else if(n==10){
      	ScalarDiag_reduce[n] *= dtout;// /(ZMAX-ZMIN);
      }
    }


    int req1_found = 0;
    double Mdot_near_req1,r_near_req1;
    for (i=0;i<num_r_points_global;++i){
      double r = VectorDiag_reduce[i*NUM_VEC]/(ZMAX-ZMIN);
      if (r>1.0 && req1_found==0){
        Mdot_near_req1 = VectorDiag_reduce[i*NUM_VEC+5]*2.*M_PI*r/(ZMAX-ZMIN);
        r_near_req1 = r;
        req1_found = 1;
      }
      for (n=0;n<NUM_VEC;++n){
	if (n!=11){ //time and vol averge
	  VectorDiag_reduce[i*NUM_VEC+n] *= dtout/(ZMAX-ZMIN);
	}else if(n==11){ //dz left out of original integral
	  VectorDiag_reduce[i*NUM_VEC+n] *= dtout;
	}	
      }
    }
	  
    if(mpisetup_MyProc(theMPIsetup)==0){
      char DiagMdotFilename[256];
      sprintf(DiagMdotFilename,"2DiagMdot.dat");
      FILE * DiagMdotFile = fopen(DiagMdotFilename,"a");
      fprintf(DiagMdotFile,"%e %e %e %e %e %e %e %e %e\n",timestep_get_t(theTimeStep), Mdot_near_req1,r_near_req1,mass_near_bh0_r0p1_reduce,mass_near_bh1_r0p1_reduce,mass_near_bh0_r0p2_reduce,mass_near_bh1_r0p2_reduce,mass_near_bh0_r0p4_reduce,mass_near_bh1_r0p4_reduce);       
      fclose(DiagMdotFile);
    }
	  
	// Try the same thing as for Mdot below for binary params? -DD ------ ADDED below DD---------//
	double t = timestep_get_t(theTimeStep);
	double r_bh0 = gravMass_r(theGravMasses,0);
	double r_bh1 = gravMass_r(theGravMasses,1);
	double phi_bh0 = gravMass_phi(theGravMasses,0);
	double phi_bh1 = gravMass_phi(theGravMasses,1);
	
	double L0 = gravMass_L(theGravMasses,0);
	double L1 = gravMass_L(theGravMasses,1);
	double M0 = gravMass_M(theGravMasses,0);
	double M1 = gravMass_M(theGravMasses,1);
	double E  = gravMass_E(theGravMasses,1);
	double Om = gravMass_omega(theGravMasses,1);
	  
	double vr0 = gravMass_vr(theGravMasses,0);
	double vr1 = gravMass_vr(theGravMasses,1);

	  
	double Fr1 = gravMass_Fr(theGravMasses,1);
	double Fp1 = gravMass_Fp(theGravMasses,1);
	//If CM of binary strays from r=0, then phi1-phi0 != pi  
	//double a_bin = sqrt(r_bh0*r_bh0 + r_bh1*r_bh1 - 2.*r_bh0*r_bh1*cos(phi_bh1-phi_bh0));
	double a_bin = r_bh0+r_bh1;
	double mu = M0*M1/(M0+M1);
	double l2 = mu*mu*pow(a_bin,4)*Om*Om;
	double ecc = sqrt( 1. + 2.*l2*E/(mu*(M0*M1)*(M0*M1)) );

	  
	if(mpisetup_MyProc(theMPIsetup)==0){
		char DiagBPFilename[256];
		sprintf(DiagBPFilename,"2BinaryParams.dat");
		FILE * DiagBpFile = fopen(DiagBPFilename,"a");
		fprintf(DiagBpFile,"%e %e %e %e %e %e %e %e %e %e %e %e %e %e %e \n",t, r_bh0, r_bh1, a_bin, phi_bh0, phi_bh1, ecc, E, L0, L1, vr0, Om, Fr1, Fp1, TrScal_reduce);       
		fclose(DiagBpFile);
	}	



	  
//----------------ADDED above DD-------------------------//
	  
    //We are doing time averaged diagnostics, so mult by delta t and add it
    //We will divide by the total delta next time we save to disk;
    for (i=0;i<num_r_points_global;++i){
      for (n=0;n<NUM_VEC;++n){
	  theDiagnostics->VectorDiag[i][n] += VectorDiag_reduce[i*NUM_VEC+n];
      }
  
    }

    for (n=0;n<NUM_SCAL;++n){  
	theDiagnostics->ScalarDiag[n] += ScalarDiag_reduce[n];
    }
  
	  
    position=0;
    for (i=0;i<num_r_points_global;++i){
      for(j = 0; j < theDiagnostics->N_p_global[i]; j++){
        for (n=0;n<NUM_EQ;++n){
          theDiagnostics->EquatDiag[position][n] = EquatDiag_reduce[position*NUM_EQ+n];
        }
        ++position;
      }
    }

    //update output time;
    theDiagnostics->toutprev = timestep_get_t(theTimeStep);
    //  free(TrScal_temp);
    // free(TrScal_reduce);
    free(TrVec_temp);
    //free(TrVec_reduce);
    free(ScalarDiag_temp);
    free(ScalarDiag_reduce);
    free(VectorDiag_temp);
    free(VectorDiag_reduce);
    free(EquatDiag_temp);
    free(EquatDiag_reduce);

    theDiagnostics->tdiag_measure += theDiagnostics->dtdiag_measure;
  } 
}






