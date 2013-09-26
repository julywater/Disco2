#ifndef SIM_H
#define SIM_H
struct Sim;
struct Cell;
struct MPIsetup;

#ifdef SIM_PRIVATE_DEFS
struct Sim {
  double *r_faces;
  double *z_faces;
  int *N_p;
  int NPCAP;
  int N0[2];
  int N_noghost[2];
  int Nghost_min[2];
  int Nghost_max[2];
  int Ncells;
  int Ncells_global;
  int offset;
  //fixed parameters
  int InitialDataType;
  int GravMassType;
  int BoundTypeR;
  int BoundTypeZ;
  int ZeroPsiBndry;
  int NoInnerBC;
  int Restart;
  int N_global[2];
  int NP_CONST;
  double aspect;
  int ng;
  double T_MAX;
  int NUM_CHECKPOINTS;
  int NUM_DIAG_DUMP;
  int NUM_DIAG_MEASURE;
  double MIN[2];
  double MAX[2];
  int NUM_C;
  int NUM_N;
  int NUM_Q;
  int MOVE_CELLS;   
  int NumGravMass;
  double MassRatio;
  int Riemann;
  double GAMMALAW;
  double EXPLICIT_VISCOSITY;
  double DIVB_CH;
  double DIVB_L;
  double CFL;
  double PLM;
  int POWELL;
  int GRAV2D;
  double G_EPS;
  double RhoSinkTimescale;
  double PHI_ORDER;
  double RHO_FLOOR;
  double CS_FLOOR;
  double CS_CAP;
  double VEL_CAP;
  int SET_T;
  int runtype;
  double DAMP_TIME;
  double RDAMP_INNER;
  double RDAMP_OUTER;
  double RLogScale;
  double ZLogScale;
  double HiResSigma;
  double HiResR0;
  double HiResFac;
  int W_A_TYPE;
  double Mdisk_ovr_Ms; //For LiveBinary sets Sigma scale
  double sep0;
};
#endif

//create and destroy
struct Sim *sim_create(struct MPIsetup * );
void sim_alloc_arr(struct Sim * , struct MPIsetup * );
void sim_destroy(struct Sim *); 
//access Sim data
double sim_MIN(struct Sim * ,int);
double sim_MAX(struct Sim * ,int);
int sim_N0(struct Sim * ,int );
int sim_N_p(struct Sim *,int);
double sim_FacePos(struct Sim *,int,int);
int sim_N(struct Sim *,int );
int sim_NoInnerBC(struct Sim *);
int sim_Restart(struct Sim *);
int sim_BoundTypeR(struct Sim *);
int sim_BoundTypeZ(struct Sim *);
int sim_ZeroPsiBndry(struct Sim *);
int sim_N_global(struct Sim *,int);
int sim_Ncells(struct Sim *);
int sim_Ncells_global(struct Sim *);
int sim_offset(struct Sim *);
int sim_ng(struct Sim *);
int sim_Nghost_min(struct Sim *,int);
int sim_Nghost_max(struct Sim *,int);
int sim_MOVE_CELLS(struct Sim *);
int sim_NumGravMass(struct Sim *);
double sim_MassRatio(struct Sim *);
int sim_Riemann(struct Sim *);
double sim_GAMMALAW(struct Sim *);
double sim_EXPLICIT_VISCOSITY(struct Sim *);
double sim_DIVB_CH(struct Sim *);
double sim_DIVB_L(struct Sim *);
double sim_CFL(struct Sim *);
double sim_PLM(struct Sim *);
int sim_POWELL(struct Sim *);
int sim_W_A_TYPE(struct Sim *);
int sim_GRAV2D(struct Sim *);
double sim_G_EPS(struct Sim *);
double sim_RhoSinkTimescale(struct Sim *);
double sim_PHI_ORDER(struct Sim *);
double sim_RHO_FLOOR(struct Sim *);
double sim_CS_FLOOR(struct Sim *);
double sim_CS_CAP(struct Sim *);
double sim_VEL_CAP(struct Sim *);
int sim_SET_T(struct Sim *);
int sim_NUM_C(struct Sim *);
int sim_NUM_Q(struct Sim *);
double sim_get_T_MAX(struct Sim * );
int sim_NUM_CHECKPOINTS(struct Sim * );
int sim_NUM_DIAG_DUMP(struct Sim * );
int sim_NUM_DIAG_MEASURE(struct Sim * );
int sim_runtype(struct Sim * );
int sim_InitialDataType(struct Sim * );
double sim_DAMP_TIME(struct Sim *);
double sim_RDAMP_INNER(struct Sim *);
double sim_RDAMP_OUTER(struct Sim *);
double sim_Mdisk_ovr_Ms(struct Sim *);
double sim_sep0(struct Sim *);

int sim_GravMassType(struct Sim * );
//set Grid data
int sim_read_par_file(struct Sim * ,struct MPIsetup *, char * );
void sim_set_N_p(struct Sim *);
void sim_set_rz(struct Sim *,struct MPIsetup *);
void sim_set_misc(struct Sim *,struct MPIsetup *);
// W_A stuff
double sim_W_A(struct Sim * ,double );
double sim_OM_A_DERIV(struct Sim * ,double );
#endif
