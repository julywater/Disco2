//                         ...WWn+,....
//                     ..JWyyZyZZXyyyZWL
//                   .XyZyZZZyyXyyZZyZyWWl
//                 .dWyZyZZyyZZyZZZyZyZyyWh.                       .J7!J
//                .,WyyZZyyZyZyZyyZyyZZyZyX\                    .JC..J=
//               .HZZZyZZyZyZyZZyZZyZyyZZZyW.                  .=?^^J&..
//               jZyyZyZyZyZZyyZyyyZyZyZyyZW`                ..:.JZWC.J\
//              .WyZyZyZyZyXkXXWWVWWVWWQkyZyF              .CJJVn.`3J=^J\
//              .HZyZyZyZyyW!`^^^^^^^^^?HZyX'            .CJ!    1,^?X!
//              KyyZyyZyZZX%.JgJJ.^^^.J+dkX%           .C`J:      j2!
//              ?WyZyXyZyyW+XHWkpHJJ+jHWHHF          .V76.r        l
//               ,kZyyZyWYY+WHHHWK`^^^?HkV!       .,Cv  ,=         j
//                .7HyZWIr^^^?77!^^^^^^J%       .J:J^              ,
//                  TWkXkJJ:^^^.JJ:`+1z6      .?.C7               .;
//                    .4WWD^^^J1.^^^^^^^?l .,!                ..?`
//                      .Z$^^^!^^??77Tf7!.?`               .?!
//                      ydt^^^^^^^^^J..J!              .,?`
//                     Jwdr^^^^^^^^.S?n.            .,^
//                  .,v!w1S.^^^^^^^`R?1Wl,       .,^
//               .,^  2 0?J2^^^^^^^^?k??4e..  .,^
//             .?7?i, ?J3??vo^^^^...^jAy&X.h.?`
//          .,^     JC?K??1udh.^J$j$^J#| ?tj
//        .?...      .qCuv= jvk`!!J,^dMr   ?
//      .Oz1?`        dP`    4de^`T:J#d$   j.
//    .?,:J:          .Y,    ,ZMN,^.M3dw   2l
//  .J.J:^J`         ,^ `,   .SWJWNdb,dw  .\.,
// .6CJ,J?        .J\    .,   0vhH1??dkd. ,  l
// v I?!?+4`   .?! .:     .+  w?JNm&z60d:.\  ?
// u.C^^^J\   ``   .`      .o j=?vh,^JId;J   .!
// j^`S:J'    .|   .      d.`4jz???ZGD?zyCJ$  |
// ,.^^J\      ?.  J      `+^^Jz???????zP^:r  1
// .cJ,?.C!     t  r       `+:JI???????zP^J`  ,
//  $?:^^J`     .,.:        1:JI???????zPJ:.  .,
//  j.^^J\       j,`   `  J7!^JI???????1$jJ4.  |
//  .xA+z         .`      .l^^JI???????1P^^j   l
//   6.^?i   ..JZ!2...     ,x^JI???????1$^.r  Jo
//    .JC??!`^^^I.$J`    .c I^JI????jx?1$^J, .CI
//    $^^^^^^^^.I2^?...,=?C,C^Jk???j=jxz$^^?7:^z
//    ?&.^.J`J.JJ:^^^^^^^^^^:^J;7wv'  ?7$^^^^JJ^
//       .777=! `1+J.:^:^^:^:.J!     .  7777?t
//                   `?P777?`       .^       r

#include <stdio.h>
#include <stdlib.h>
#include "Headers/MPIsetup.h"
#include "Headers/Cell.h"
#include "Headers/Sim.h"
#include "Headers/Face.h"
#include "Headers/GravMass.h"
#include "Headers/IO.h"
#include "Headers/TimeStep.h"
#include "Headers/Diagnostics.h"
#include "Headers/header.h"


int main(int argc, char **argv) {
  char* inputfilename = argv[1];
  
  // start MPI 
  struct MPIsetup * theMPIsetup = mpisetup_create(argc,argv);
  mpisetup_setprocs(theMPIsetup,inputfilename);
  mpisetup_cart_create(theMPIsetup);
  mpisetup_left_right(theMPIsetup);

  // Set up everything that will not change throughout the simulation
  struct Sim * theSim = sim_create(theMPIsetup);
  sim_read_par_file(theSim,theMPIsetup,inputfilename);
  sim_alloc_arr(theSim,theMPIsetup); 
  sim_set_rz(theSim,theMPIsetup);
  sim_set_N_p(theSim);
  sim_set_misc(theSim,theMPIsetup);

  // set up gravitating masses. 
  struct GravMass *theGravMasses = gravMass_create(sim_NumGravMass(theSim));
  (*gravMass_init_ptr(theSim))(theGravMasses);
  gravMass_clean_pi(theGravMasses,theSim);

  // allocate memory for data 
  struct Cell ***theCells = cell_create(theSim,theMPIsetup);

  // make sure that phi is between 0 and 2 pi. I'm not sure why this matters.
  cell_clean_pi(theCells,theSim);

  // inter-processor syncs. cell_create currently sets up tiph_0 randomly, 
  // so we need to make sure ghost-zones are set appropriately
  cell_syncproc_r(theCells,theSim,theMPIsetup);
  cell_syncproc_z(theCells,theSim,theMPIsetup);

  // create TimeStep struct
  struct TimeStep * theTimeStep = timestep_create(theSim);

  // create IO struct
  struct IO *theIO = io_create(theSim);

  // set initial data 
  if (sim_Restart(theSim)==1){ // getting initial data from checkpoint file
    io_allocbuf(theIO,theSim); // allocate memory for a buffer to store checkpoint data
    io_hdf5_in(theIO,theSim,theTimeStep); // read from hdf5 file into buffer
    io_readbuf(theIO,theCells,theSim); // read from buffer into theCells
    io_deallocbuf(theIO); // get rid of buffer
  }else{
    (*cell_init_ptr(theSim))(theCells,theSim,theMPIsetup); // setup initial data using routine specified in .par file
  }

  // set tcheck, dtcheck, and nfile. Needs to be done after ID because it depends on current time.
  io_setup(theIO,theSim,theTimeStep);

  // inter-processor syncs
  cell_syncproc_r(theCells,theSim,theMPIsetup);
  cell_syncproc_z(theCells,theSim,theMPIsetup);

  // set conserved quantities
  cell_calc_cons(theCells,theSim);

  // set up diagnostics struct
  struct Diagnostics * theDiagnostics = diagnostics_create(theSim,theTimeStep);

  while( timestep_get_t(theTimeStep) < sim_get_T_MAX(theSim) ){
    /*
    cell_clear_w(theCells,theSim);
    cell_set( theCells ,theSim);
    timestep_set_dt(theTimeStep,theCells,theSim);
    cell_copy(theCells,theSim);
    gravMass_copy(theGravMasses,theSim);
    timestep_set_RK(theTimeStep,0.0);
    timestep_substep(theTimeStep,theCells,theSim,theGravMasses,theMPIsetup,1.0);
    gravMass_move(theGravMasses,1.0*timestep_dt(theTimeStep));
    timestep_set_RK(theTimeStep,0.5);
    timestep_substep(theTimeStep,theCells,theSim,theGravMasses,theMPIsetup,0.5);
    gravMass_move(theGravMasses,0.5*timestep_dt(theTimeStep));
    if (sim_runtype(theSim)==1) timestep_update_Psi(theTimeStep,theCells,theSim,theMPIsetup);
    timestep_update_t(theTimeStep); 
    */
    timestep_rk2(theTimeStep,theSim,theCells,theGravMasses,theMPIsetup);

    // calculate diagnostics
    diagnostics_set(theDiagnostics,theCells,theSim,theTimeStep);
    //write diagnostics to file
    diagnostics_print(theDiagnostics,theTimeStep,theSim,theMPIsetup);
    
    if( timestep_get_t(theTimeStep)>io_tcheck(theIO)){ // time to write checkpoint file
      io_allocbuf(theIO,theSim); // allocate memory for a buffer to store simulation data
      io_setbuf(theIO,theCells,theSim); // fill the buffer
      io_hdf5_out(theIO,theSim,theTimeStep); // write contents to file
      io_deallocbuf(theIO); // get rid of buffer
    }
  }

  //inter-processor syncs
  cell_syncproc_r(theCells,theSim,theMPIsetup);
  cell_syncproc_z(theCells,theSim,theMPIsetup);
 
  // clean up
  diagnostics_destroy(theDiagnostics,theSim);
  cell_destroy(theCells,theSim);
  sim_destroy(theSim);
  gravMass_destroy(theGravMasses);
  io_destroy(theIO);

  // exit MPI 
  mpisetup_destroy(theMPIsetup);
  return(0);
}
