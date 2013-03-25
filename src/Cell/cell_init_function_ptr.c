#include <stdio.h>
#include <stdlib.h>
#include "../Headers/Cell.h"
#include "../Headers/Sim.h"
#include "../Headers/header.h"
void (*cell_init_ptr(struct Sim * theSim))(struct Cell *** , struct Sim *,struct MPIsetup * ){
  if (sim_InitialDataType(theSim)==FLOCK){
    return(&cell_init_flock);
  } else if (sim_InitialDataType(theSim)==SHEAR){
    return(&cell_init_shear);
  } else if (sim_InitialDataType(theSim)==VORTEX){
    return(&cell_init_vortex);
  } else if (sim_InitialDataType(theSim)==STONE){
    return(&cell_init_stone);
  } else if (sim_InitialDataType(theSim)==FIELDLOOP){
    return(&cell_init_fieldloop);
   } else{
    printf("ERROR\n");
    exit(0);
  }
}

void (*cell_single_init_ptr(struct Sim * theSim))(struct Cell * , struct Sim *,int,int,int ){
  if (sim_InitialDataType(theSim)==FLOCK){
    return(&cell_single_init_flock);
  } else if (sim_InitialDataType(theSim)==SHEAR){
    return(&cell_single_init_shear);
  } else if (sim_InitialDataType(theSim)==VORTEX){
    return(&cell_single_init_vortex);
  } else if (sim_InitialDataType(theSim)==STONE){
    return(&cell_single_init_vortex);
  } else if (sim_InitialDataType(theSim)==FIELDLOOP){
    return(&cell_single_init_fieldloop);
    } else{
    printf("ERROR\n");
    exit(0);
  }
}


