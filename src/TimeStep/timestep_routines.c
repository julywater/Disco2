#define TIMESTEP_PRIVATE_DEFS
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../Headers/Cell.h"
#include "../Headers/TimeStep.h"
#include "../Headers/Riemann.h"
#include "../Headers/Grid.h"
#include "../Headers/Cell.h"
#include "../Headers/GravMass.h"
#include "../Headers/Face.h"
#include "../Headers/TimeStep.h"
#include "../Headers/MPIsetup.h"
#include "../Headers/header.h"

void timestep_set_dt(struct TimeStep * theTimeStep, struct Cell *** theCells, struct Grid * theGrid){
  theTimeStep->dt = cell_mindt(theCells,theGrid);
  if( theTimeStep->t+theTimeStep->dt > grid_get_T_MAX(theGrid) ) {
    theTimeStep->dt = grid_get_T_MAX(theGrid)-theTimeStep->t;
  }
  printf("t: %e, dt: %e\n",theTimeStep->t,theTimeStep->dt);
}
void timestep_update_t(struct TimeStep * theTimeStep){
  theTimeStep->t += theTimeStep->dt;
}
void timestep_set_RK(struct TimeStep * theTimeStep,double RK){
  theTimeStep->RK = RK;
}
double timestep_get_t(struct TimeStep * theTimeStep){
  return(theTimeStep->t);
}
double timestep_dt(struct TimeStep * theTimeStep){
  return(theTimeStep->dt);
}

void timestep_substep(struct TimeStep * theTimeStep, struct Cell *** theCells,struct Grid * theGrid,struct GravMass * theGravMasses,struct MPIsetup * theMPIsetup,double timestep_fac){

  double dt = timestep_fac*theTimeStep->dt;
  int N_r_withghost = grid_N_r(theGrid)+grid_Nghost_rmin(theGrid)+grid_Nghost_rmax(theGrid);
  int N_z_withghost = grid_N_z(theGrid)+grid_Nghost_zmin(theGrid)+grid_Nghost_zmax(theGrid);
  struct Face *theFaces_r = face_create(theCells,theGrid,theTimeStep,0); // r-direction
  struct Face *theFaces_z = face_create(theCells,theGrid,theTimeStep,1); // z-direction
  cell_clean_pi(theCells,theGrid);
  gravMass_clean_pi(theGravMasses,theGrid);
  cell_clear_w(theCells,theGrid);
  if( grid_MOVE_CELLS(theGrid) == C_WCELL ) cell_set_wcell( theCells ,theGrid);
  if( grid_MOVE_CELLS(theGrid) == C_RIGID ) cell_set_wrigid( theCells ,theGrid);
  cell_adjust_RK_cons( theCells, theGrid, theTimeStep->RK);
  cell_clear_divB(theCells,theGrid);
  cell_clear_GradPsi(theCells,theGrid);
  //cell_print_cons(theCells,theGrid,1);
  //Phi Flux
  cell_plm_p( theCells,theGrid );
  int i,j,k;
  for( k=0 ; k<N_z_withghost ; ++k ){
    for( i=0 ; i<N_r_withghost ; ++i ){
      for( j=0 ; j<grid_N_p(theGrid,i)-1 ; ++j ){
        struct Riemann * theRiemann = riemann_create(theGrid);
        riemann_setup_p(theRiemann,theCells,theGrid,i,j,j+1,k);
        riemann_hllc( theRiemann,theGrid,dt,1);
        riemann_destroy(theRiemann);
      }
      struct Riemann * theRiemann = riemann_create(theGrid);
      riemann_setup_p(theRiemann,theCells,theGrid,i,grid_N_p(theGrid,i)-1,0,k);
      riemann_hllc(theRiemann, theGrid,dt,1 );
      riemann_destroy(theRiemann);
    }
  }
  //cell_print_cons(theCells,theGrid,2);
  //R Flux
  cell_plm_rz( theCells ,theGrid, theFaces_r , timestep_Nfr(theTimeStep) , 0 );
  int n;
  for( n=0 ; n<timestep_Nfr(theTimeStep) ; ++n ){
    struct Riemann * theRiemann = riemann_create(theGrid);
    riemann_setup_rz(theRiemann,theFaces_r,theGrid,n);
    riemann_hllc(theRiemann,theGrid,dt,0);
    /*
    if ((mpisetup_NumProcs(theMPIsetup)==1)&&(n==81)){
      int q;
      for (q=0;q<grid_NUM_Q(theGrid);++q){
        printf("0 theRiemann->primL[%d]: %16.12e\n",q,theRiemann->primL[q]);
        printf("0 theRiemann->primR[%d]: %16.12e\n",q,theRiemann->primR[q]);
      }
        printf("0 theRiemann->Ss: %16.12e\n",theRiemann->Ss);
        printf("0 theRiemann->dA: %16.12e\n",theRiemann->dA);
        printf("0 theRiemann->F[LLL]: %16.12e\n",theRiemann->F[LLL]);
        printf("0 theRiemann->cL->cons[LLL]: %16.12e\n",theRiemann->cR->cons[LLL]);
    } 
    if ((mpisetup_NumProcs(theMPIsetup)==2)&&(n==41)){
      int q;
      for (q=0;q<grid_NUM_Q(theGrid);++q){
        printf("1 theRiemann->primL[%d]: %16.12e\n",q,theRiemann->primL[q]);
        printf("1 theRiemann->primR[%d]: %16.12e\n",q,theRiemann->primR[q]);
      }
        printf("1 theRiemann->Ss: %16.12e\n",theRiemann->Ss);
        printf("1 theRiemann->dA: %16.12e\n",theRiemann->dA);
        printf("1 theRiemann->F[LLL]: %16.12e\n",theRiemann->F[LLL]);
        printf("1 theRiemann->cL->cons[LLL]: %16.12e\n",theRiemann->cR->cons[LLL]);
    } 


    if ((mpisetup_NumProcs(theMPIsetup)==1)&&(mpisetup_dim_MyProc(theMPIsetup,0)==0)&&(fabs(theRiemann->cR->tiph-2.15984)<0.0001)&&(fabs(theRiemann->r-1.)<0.0001)){
      printf("0 single proc n: %d,theRiemann->cR->cons[LLL]: %16.12e\n",n,theRiemann->cR->cons[LLL]);
    }
    if ((mpisetup_NumProcs(theMPIsetup)==2)&&(mpisetup_dim_MyProc(theMPIsetup,0)==1)&&(fabs(theRiemann->cR->tiph-2.15984)<0.0001)&&(fabs(theRiemann->r-1.)<0.0001)){
      printf("1 single proc n: %d,theRiemann->cR->cons[LLL]: %16.12e\n",n,theRiemann->cR->cons[LLL]);
    }

    if (n==120&&(mpisetup_dim_MyProc(theMPIsetup,0)==0)){
      printf("0 theRiemann->cL->cons[LLL]: %16.12e\n",(theRiemann->cL)->cons[LLL]);    
      printf("0 theRiemann->cL->tiph: %16.12e\n",(theRiemann->cL)->tiph);    
      printf("0 theRiemann->r: %16.12e\n",theRiemann->r);    
      printf("0 theRiemann->primL[UPP]: %16.12e\n",theRiemann->primL[UPP]);    
      printf("0 theRiemann->F[LLL]: %16.12e\n",theRiemann->F[LLL]);
    }
    
    
       if (n==121&&(mpisetup_dim_MyProc(theMPIsetup,0)==0)){
       printf("0 theRiemann->cR->cons[LLL]: %16.12e\n",(theRiemann->cR)->cons[LLL]);    
       printf("0 theRiemann->primR[UPP]: %16.12e\n",theRiemann->primR[UPP]);    
       printf("0 theRiemann->F[LLL]: %16.12e\n",theRiemann->F[LLL]);
       }
       
    if (n==80&&(mpisetup_dim_MyProc(theMPIsetup,0)==1)){
      printf("1 theRiemann->cL->cons[LLL]: %16.12e\n",(theRiemann->cL)->cons[LLL]);
      printf("1 theRiemann->primL[UPP]: %16.12e\n",theRiemann->primL[UPP]);    
      printf("1 theRiemann->F[LLL]: %16.12e\n",theRiemann->F[LLL]);
    }
    */
    riemann_destroy(theRiemann);
  }
 // cell_print_cons(theCells,theGrid,3);

  //Z Flux
  if( grid_N_z_global(theGrid) != 1 ){
    cell_plm_rz( theCells ,theGrid, theFaces_z , timestep_Nfz(theTimeStep) , 1 );
    for( n=0 ; n<timestep_Nfz(theTimeStep) ; ++n ){
      struct Riemann * theRiemann = riemann_create(theGrid);
      riemann_setup_rz(theRiemann,theFaces_z,theGrid,n);
      riemann_hllc(theRiemann,theGrid,dt,2); 
      riemann_destroy(theRiemann);
    }
  }

  //Source Terms
  cell_add_src( theCells ,theGrid, theGravMasses , dt );
  if (grid_INCLUDE_VISCOSITY(theGrid)){
    cell_add_visc_src( theCells ,theGrid,dt );
  }

  //Bookkeeping
  cell_update_phi( theCells ,theGrid, theTimeStep->RK , dt );
  cell_update_dphi( theCells ,theGrid);
  gravMass_update_RK( theGravMasses ,theGrid, theTimeStep->RK );
  cell_calc_prim( theCells ,theGrid);

  //Boundary Data
  //cell_boundary_outflow_r( theCells , theFaces_r ,theGrid, nri );
  //if( N_z_global > 1 ) cell_boundary_z( theCells , theFaces_z ,theGrid, nzk );
  //cell_boundary_fixed_r( theCells, theGrid,theMPIsetup );
  cell_boundary_outflow_r( theCells , theFaces_r ,theGrid,theMPIsetup, theTimeStep->nri );

  face_destroy(theFaces_r);
  if (grid_N_z_global(theGrid)>1){
    face_destroy(theFaces_z);
  }

  //inter-processor syncs
  cell_syncproc_r(theCells,theGrid,theMPIsetup);
  if (grid_N_z_global(theGrid)>1){
    cell_syncproc_z(theCells,theGrid,theMPIsetup);
  }
  cell_calc_cons( theCells,theGrid );

}

void timestep_update_Psi( struct TimeStep * theTimeStep, struct Cell *** theCells , struct Grid * theGrid,struct MPIsetup * theMPIsetup){
  int N_r_withghost = grid_N_r(theGrid)+grid_Nghost_rmin(theGrid)+grid_Nghost_rmax(theGrid);
  int N_z_withghost = grid_N_z(theGrid)+grid_Nghost_zmin(theGrid)+grid_Nghost_zmax(theGrid);
  double DIVB_CH = grid_DIVB_CH(theGrid);
  double DIVB_L = grid_DIVB_L(theGrid);

  int i,j,k;
  for( k=0 ; k<N_z_withghost ; ++k ){
    for( i=0 ; i<N_r_withghost ; ++i ){
      for( j=0 ; j<grid_N_p(theGrid,i) ; ++j ){
        cell_mult_psi(cell_single(theCells,i,j,k),exp(-theTimeStep->dt*DIVB_CH/DIVB_L));
      }
    }
  }
  //Boundary Data: WE MAY NEED TO APPLY BCs on PSI NOW.

  //Bookkeeping
  cell_calc_prim( theCells,theGrid );
  cell_calc_cons( theCells,theGrid );

  //inter-processor syncs
  cell_syncproc_r(theCells,theGrid,theMPIsetup);
  if (grid_N_z_global(theGrid)>1){
    cell_syncproc_z(theCells,theGrid,theMPIsetup);
  }
}

int * timestep_nri(struct TimeStep * theTimeStep){
  return(theTimeStep->nri);
}
int * timestep_nzk(struct TimeStep * theTimeStep){
  return(theTimeStep->nzk);
}

void timestep_set_Nfr(struct TimeStep * theTimeStep,struct Grid * theGrid){
  int N_r_withghost = grid_N_r(theGrid)+grid_Nghost_rmin(theGrid)+grid_Nghost_rmax(theGrid);
  theTimeStep->Nfr = theTimeStep->nri[N_r_withghost-1];
}
void timestep_set_Nfz(struct TimeStep * theTimeStep,struct Grid * theGrid){
  int N_z_withghost = grid_N_z(theGrid)+grid_Nghost_zmin(theGrid)+grid_Nghost_zmax(theGrid);
  theTimeStep->Nfz = theTimeStep->nzk[N_z_withghost-1];
}

int timestep_Nfr(struct TimeStep * theTimeStep){
  return(theTimeStep->Nfr);
}
int timestep_Nfz(struct TimeStep * theTimeStep){
  return(theTimeStep->Nfz);
}


