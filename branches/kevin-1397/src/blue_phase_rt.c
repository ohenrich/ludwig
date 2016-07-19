/*****************************************************************************
 *
 *  blue_phase_rt.c
 *
 *  Run time input for blue phase free energy, and related parameters.
 *  Also relevant Beris Edwards parameters.
 *
 *  $Id$
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *
 *  (c) 2009-2016 The University of Edinburgh
 *
 *****************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "pe.h"
#include "coords.h"
#include "runtime.h"
#include "blue_phase_init.h"
#include "blue_phase_rt.h"
#include "physics.h"

/*****************************************************************************
 *
 *  blue_phase_run_time
 *
 *  Pick up the liquid crystal parameters from the input.
 *
 *****************************************************************************/

__host__ int blue_phase_run_time(field_t * q, field_grad_t * dq,
				 fe_lc_t * fe,
				 beris_edw_t ** be) {

  int n;
  int redshift_update;
  char method[FILENAME_MAX];
  char type[FILENAME_MAX];
  char type_wall[FILENAME_MAX];

  double epsilon;
  double redshift;
  double zeta;
  double gamma;

  /* Wall anchoring strengths */

  double w1;
  double w2;
  double w1_wall;
  double w2_wall;

  /* Derived quantities */
  double amp0;
  double ck;
  double tred;

  fe_lc_param_t fe_param = {0};
  beris_edw_param_t be_param = {0};

  int io_grid[3] = {1,1,1};
  int io_format_out = IO_FORMAT_DEFAULT;
  char value[BUFSIZ] = "BINARY";

  assert(fe);

  info("Blue phase free energy selected.\n");

  /* PARAMETERS */

  n = RUN_get_double_parameter("lc_a0", &fe_param.a0);
  if (n != 1) fatal("Please specify lc_a0 <value>\n");

  n = RUN_get_double_parameter("lc_gamma", &fe_param.gamma);
  if (n != 1) fatal("Please specify lc_gamma <value>\n");

  n = RUN_get_double_parameter("lc_q0", &fe_param.q0);
  if (n != 1) fatal("Please specify lc_q0 <value>\n");

  n = RUN_get_double_parameter("lc_kappa0", &fe_param.kappa0);
  if (n != 1) fatal("Please specify lc_kappa0 <value>\n");

  n = RUN_get_double_parameter("lc_kappa1", &fe_param.kappa1);
  if (n != 1) fatal("Please specify lc_kappa1 <value>\n");

  n = RUN_get_double_parameter("lc_xi", &fe_param.xi);
  if (n != 1) fatal("Please specify lc_xi <value>\n");

  n = RUN_get_double_parameter("lc_q_init_amplitude", &fe_param.amplitude0);
  if (n != 1) fatal("Please specify lc_q_init_amplitude <value>\n");

  /* Use a default redshift of 1 */
  redshift = 1.0;
  RUN_get_double_parameter("lc_init_redshift", &redshift);
  fe_param.redshift = redshift;

  redshift_update = 0;
  RUN_get_int_parameter("lc_redshift_update", &redshift_update);
  fe_param.is_redshift_updated = redshift_update;

  /* Use a default zeta (no activity) of 0 */
  zeta = 0.0;
  RUN_get_double_parameter("lc_active_zeta", &zeta);
  fe_param.zeta = zeta;

  info("\n");
  info("Liquid crystal blue phase free energy\n");
  info("Bulk parameter A0:         = %14.7e\n", fe_param.a0);
  info("Magnitude of order gamma   = %14.7e\n", fe_param.gamma);
  info("Pitch wavevector q0        = %14.7e\n", fe_param.q0);
  info("... gives pitch length     = %14.7e\n", 2.0*4.0*atan(1.0)/fe_param.q0);
  info("Elastic constant kappa0    = %14.7e\n", fe_param.kappa0);
  info("Elastic constant kappa1    = %14.7e\n", fe_param.kappa1);
  info("Amplitude (uniaxial) order = %14.7e\n", fe_param.amplitude0);

  /* One-constant approximation enforced. */
  assert(fe_param.kappa0 == fe_param.kappa1);

  fe_lc_param_set(fe, fe_param);

  fe_lc_chirality(fe, &ck);
  fe_lc_reduced_temperature(fe, &tred);

  info("Effective aspect ratio xi  = %14.7e\n", fe_param.xi);
  info("Chirality                  = %14.7e\n", ck);
  info("Reduced temperature        = %14.7e\n", tred);
  info("Initial redshift           = %14.7e\n", fe_param.redshift);
  info("Dynamic redshift update    = %14s\n",
       redshift_update == 0 ? "no" : "yes");
  info("LC activity constant zeta  = %14.7e\n", fe_param.zeta);


  /* Default electric field stuff zero */

  epsilon = 0.0;
  RUN_get_double_parameter("lc_dielectric_anisotropy", &epsilon);
  fe_param.epsilon = epsilon;

  n = RUN_get_double_parameter_vector("electric_e0", fe_param.electric);

  if (n == 1) {
    double ered;
    fe_lc_dimensionless_field_strength(fe, &ered);
    info("Dielectric anisotropy      = %14.7e\n", epsilon);
    info("Dimensionless field e      = %14.7e\n", ered);
  }

  /* Surface anchoring */

  RUN_get_string_parameter("lc_anchoring_method", method, FILENAME_MAX);

  if (strcmp(method, "two") != 0) {
    /* There's a bit of an historical problem here, as 'two'
     * is now the only valid choice. However, it is worth
     * not getting a load a irrelevant output if no solids.
     * So I assert 'none' is the only other option. */
    if (strcmp(method, "none") != 0) fatal("Check anchoring method input\n");
  }
  else {

    /* Find out type */

    n = RUN_get_string_parameter("lc_anchoring", type, FILENAME_MAX);

    if (n == 1) {
      info("Please replace lc_anchoring by lc_wall_anchoring and/or\n");
      info("lc_coll_anchoring types\n");
      fatal("Please check input file and try agains.\n");
    }

    RUN_get_string_parameter("lc_coll_anchoring", type, FILENAME_MAX);

    if (strcmp(type, "normal") == 0) {
      fe_param.anchoring_coll = LC_ANCHORING_NORMAL;
    }

    if (strcmp(type, "planar") == 0) {
      fe_param.anchoring_coll = LC_ANCHORING_PLANAR;
    }

    /* Surface free energy parameter */

    w1 = 0.0;
    w2 = 0.0;
    RUN_get_double_parameter("lc_anchoring_strength", &w1);
    RUN_get_double_parameter("lc_anchoring_strength_2", &w2);

    info("\n");
    info("Liquid crystal anchoring\n");
    info("Anchoring method:          = %14s\n", method);
    info("Anchoring type (colloids): = %14s\n", type);

    /* Walls (if present) separate type allowed but same strength */

    RUN_get_string_parameter("lc_wall_anchoring", type_wall, FILENAME_MAX);

    if (strcmp(type_wall, "normal") == 0) {
      fe_param.anchoring_wall = LC_ANCHORING_NORMAL;
      w1_wall = w1;
      w2_wall = 0.0;
    }

    if (strcmp(type_wall, "planar") == 0) {
      fe_param.anchoring_wall = LC_ANCHORING_PLANAR;
      w1_wall = w1;
      w2_wall = w2;
    }

    if (strcmp(type_wall, "fixed") == 0) {
      fe_param.anchoring_wall = LC_ANCHORING_FIXED;
      w1_wall = w1;
      w2_wall = 0.0;
    }

    /* Colloids default, then look for specific value */

    if (strcmp(type, "normal") == 0) w2 = 0.0;
    if (strcmp(type, "fixed")  == 0) w2 = 0.0;
      
    n =  RUN_get_double_parameter("lc_anchoring_strength_colloid", &w1);

    if ( n == 1 ) {
      if (strcmp(type, "normal") == 0) w2 = 0.0;
      if (strcmp(type, "planar") == 0) w2 = w1;
      if (strcmp(type, "fixed")  == 0) w2 = 0.0;
    }

    fe_param.w1_coll = w1;
    fe_param.w2_coll = w2;

    /* Wall */

    n =  RUN_get_double_parameter("lc_anchoring_strength_wall", &w1_wall);
    if ( n == 1 ) {
      if (strcmp(type_wall, "normal") == 0) w2_wall = 0.0;
      if (strcmp(type_wall, "planar") == 0) w2_wall = w1_wall;
      if (strcmp(type_wall, "fixed")  == 0) w2_wall = 0.0;
    }

    fe_param.w1_wall = w1_wall;
    fe_param.w2_wall = w2_wall;
    fe_lc_amplitude_compute(&fe_param, &amp0);

    info("Anchoring type (walls):          = %14s\n",   type_wall);
    info("Surface free energy (colloid)w1: = %14.7e\n", w1);
    info("Surface free energy (colloid)w2: = %14.7e\n", w2);
    info("Surface free energy (wall) w1:   = %14.7e\n", w1_wall);
    info("Surface free energy (wall) w2:   = %14.7e\n", w2_wall);
    info("Ratio (colloid) w1/kappa0:       = %14.7e\n", w1/fe_param.kappa0);
    info("Ratio (wall) w1/kappa0:          = %14.7e\n", w1_wall/fe_param.kappa0);
    info("Computed surface order f(gamma)  = %14.7e\n", amp0);

    /* For computed anchoring order [see fe_lc_amplitude_compute()] */
    if (fe_param.gamma < (8.0/3.0)) fatal("Please check anchoring amplitude\n");
  }

  fe_lc_param_set(fe, fe_param);

  /* initialise the free energy io */
  n = RUN_get_int_parameter_vector("default_io_grid", io_grid);
  n = RUN_get_string_parameter("fed_format", value, BUFSIZ);

  if (strcmp(value, "ASCII") == 0) {
    io_format_out = IO_FORMAT_ASCII;
  }
#ifdef OLD_SHIT
  fed_io_info_set(io_grid, io_format_out);
#endif

  /* Beris Edwards */

  beris_edw_create(be);
  assert(*be);

  info("\n");
  info("Using Beris-Edwards solver:\n");

  n = RUN_get_double_parameter("lc_Gamma", &gamma);

  if (n == 0) {
    fatal("Please specify diffusion constant lc_Gamma in the input\n");
  }
  else {
    be_param.gamma = gamma;
    be_param.xi = fe_param.xi;
    beris_edw_param_set(*be, be_param);
    info("Rotational diffusion const = %14.7e\n", gamma);
  }

  return 0;
}

/*****************************************************************************
 *
 *  blue_phase_rt_initial_conditions
 *
 *  There are several choices:
 *
 *****************************************************************************/

__host__ int blue_phase_rt_initial_conditions(fe_lc_t * fe, field_t * q) {

  int  n1, n2;
  int  rmin[3], rmax[3];
  char key1[FILENAME_MAX];

  double nhat[3] = {1.0, 0.0, 0.0};
  double nhat2[3] = {64.0, 3.0, 1.0};

  fe_lc_param_t param;
  fe_lc_param_t * feparam = &param;

  assert(fe);
  assert(q);

  fe_lc_param(fe, feparam);

  info("\n");

  n1 = RUN_get_string_parameter("lc_q_initialisation", key1, FILENAME_MAX);
  if (n1 != 1) fatal("Please specify lc_q_initialisation <value>\n");

  info("\n");

  if (strcmp(key1, "twist") == 0) {
    /* This gives cholesteric_z (for backwards compatibility) */
    info("Initialising Q_ab to cholesteric\n");
    info("Helical axis Z\n");
    blue_phase_twist_init(feparam, q, Z);
  }

  if (strcmp(key1, "cholesteric_x") == 0) {
    info("Initialising Q_ab to cholesteric\n");
    info("Helical axis X\n");
    blue_phase_twist_init(feparam, q, X);
  }

  if (strcmp(key1, "cholesteric_y") == 0) {
    info("Initialising Q_ab to cholesteric\n");
    info("Helical axis Y\n");
    blue_phase_twist_init(feparam, q, Y);
  }

  if (strcmp(key1, "cholesteric_z") == 0) {
    info("Initialising Q_ab to cholesteric\n");
    info("Helical axis Z\n");
    blue_phase_twist_init(feparam, q, Z);
  }

  if (strcmp(key1, "nematic") == 0) {
    info("Initialising Q_ab to nematic\n");
    RUN_get_double_parameter_vector("lc_init_nematic", nhat);
    info("Director:  %14.7e %14.7e %14.7e\n", nhat[X], nhat[Y], nhat[Z]);
    blue_phase_nematic_init(feparam, q, nhat);
  }

  if (strcmp(key1, "active_nematic") == 0) {
    info("Initialising Q_ab to active nematic\n");
    RUN_get_double_parameter_vector("lc_init_nematic", nhat);
    info("Director:  %14.7e %14.7e %14.7e\n", nhat[X], nhat[Y], nhat[Z]);
    blue_phase_active_nematic_init(feparam, q, nhat);
  }

  if (strcmp(key1, "o8m") == 0) {
    info("Initialising Q_ab using O8M (BPI)\n");
    blue_phase_O8M_init(feparam, q);
  }

  if (strcmp(key1, "o2") == 0) {
    info("Initialising Q_ab using O2 (BPII)\n");
    blue_phase_O2_init(feparam, q);
  }

  if (strcmp(key1, "o5") == 0) {
    info("Initialising Q_ab using O5\n");
    blue_phase_O5_init(feparam, q);
  }

  if (strcmp(key1, "h2d") == 0) {
    info("Initialising Q_ab using H2D\n");
    blue_phase_H2D_init(feparam, q);
  }

  if (strcmp(key1, "h3da") == 0) {
    info("Initialising Q_ab using H3DA\n");
    blue_phase_H3DA_init(feparam, q);
  }

  if (strcmp(key1, "h3db") == 0) {
    info("Initialising Q_ab using H3DB\n");
    blue_phase_H3DB_init(feparam, q);
  }

  if (strcmp(key1, "dtc") == 0) {
    info("Initialising Q_ab using DTC\n");
    blue_phase_DTC_init(feparam, q);
  }

  if (strcmp(key1, "bp3") == 0) {
    info("Initialising Q_ab using BPIII\n");
    RUN_get_double_parameter_vector("lc_init_bp3", nhat2);
    info("BPIII specifications: N_DTC=%g,  R_DTC=%g,  ", nhat2[0], nhat2[1]);
    if (nhat2[2] == 0) info("isotropic environment\n");
    if (nhat2[2] == 1) info("cholesteric environment\n");
    blue_phase_BPIII_init(feparam, q, nhat2);
  }

  if (strcmp(key1, "cf1_x") == 0) {
    info("Initialising Q_ab to cholesteric finger (1st kind)\n");
    info("Finger axis X, helical axis Y\n");
    blue_phase_cf1_init(feparam, q, X);
  }

  if (strcmp(key1, "cf1_y") == 0) {
    info("Initialising Q_ab to cholesteric finger (1st kind)\n");
    info("Finger axis Y, helical axis Z\n");
    blue_phase_cf1_init(feparam, q, Y);
  }

  if (strcmp(key1, "cf1_z") == 0) {
    info("Initialising Q_ab to cholesteric finger (1st kind)\n");
    info("Finger axis Z, helical axis X\n");
    blue_phase_cf1_init(feparam, q, Z);
  }

  if (strcmp(key1, "cf1_fluc_x") == 0) {
    info("Initialising Q_ab to cholesteric finger (1st kind)\n");
    info("with added traceless symmetric random fluctuation.\n");
    info("Finger axis X, helical axis Y\n");
    blue_phase_random_cf1_init(feparam, q, X);
  }

  if (strcmp(key1, "cf1_fluc_y") == 0) {
    info("Initialising Q_ab to cholesteric finger (1st kind)\n");
    info("with added traceless symmetric random fluctuation.\n");
    info("Finger axis Y, helical axis Z\n");
    blue_phase_random_cf1_init(feparam, q, Y);
  }

  if (strcmp(key1, "cf1_fluc_z") == 0) {
    info("Initialising Q_ab to cholesteric finger (1st kind)\n");
    info("with added traceless symmetric random fluctuation.\n");
    info("Finger axis Z, helical axis X\n");
    blue_phase_random_cf1_init(feparam, q, Z);
  }

  if (strcmp(key1, "random") == 0) {
    info("Initialising Q_ab randomly\n");
    blue_phase_random_q_init(feparam, q);
  }

  /* Superpose a rectangle of random Q_ab on whatever was above */

  n1 = RUN_get_int_parameter_vector("lc_q_init_rectangle_min", rmin);
  n2 = RUN_get_int_parameter_vector("lc_q_init_rectangle_max", rmax);

  if (n1 == 1 && n2 == 1) {
    info("Superposing random rectangle\n");
    blue_phase_random_q_rectangle(feparam, q, rmin, rmax);
  }

  return 0;
}
