/****************************************************************************
 *
 *  util.h
 *
 *  $Id: util.h,v 1.2.6.2 2010-05-19 19:16:51 kevin Exp $
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk) 
 *  (c) 2010 The University of Edinburgh
 *  
 ****************************************************************************/

#ifndef UTIL_H
#define UTIL_H

extern const double d_[3][3];
extern const double e_[3][3][3];
extern const double pi_;

int    is_bigendian(void);
double reverse_byte_order_double(char *);
double dot_product(const double a[3], const double b[3]);
void   cross_product(const double a[3], const double b[3], double result[3]);
double modulus(const double a[3]);
void   rotate_vector(double [3], const double [3]);

#endif
