/*
 * Copyright (c) 2016 Andy Ray.
 *
 * This file is distributed under the terms of the MIT License.
 * See the file LICENSE for details.
 */

#if !__USE_MINGW_ANSI_STDIO && (defined(__MINGW32__) || defined(__MINGW64__))
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include <caml/mlvalues.h>
#include <caml/custom.h>
#include <caml/alloc.h>
#include <caml/intext.h>
#include <caml/fail.h>
#include <caml/hash.h>
#include <caml/memory.h>

#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <complex.h>

#include "ctypes_ldouble_stubs.h"

/*********************** long double *************************/

#define LDOUBLE_STORAGE_BYTES sizeof(long double)
#if (LDBL_MANT_DIG == 53)      // 64 bit - same as double
#define LDOUBLE_VALUE_BYTES 8
#elif (LDBL_MANT_DIG == 64)    // intel 80 bit extended 
#define LDOUBLE_VALUE_BYTES 10
#elif (LDBL_MANT_DIG == 106)   // __ibm128 (pair of doubles)
#define LDOUBLE_VALUE_BYTES 16
#elif (LDBL_MANT_DIG == 113)   // ieee __float128
#define LDOUBLE_VALUE_BYTES 16
#else
#define LDOUBLE_VALUE_BYTES LDOUBLE_STORAGE_BYTES
#endif

#define ldouble_custom_val(V) (*(long double *)(Data_custom_val(V)))

static int ldouble_cmp(long double u1, long double u2) {
  return (u1 == u2) ? 0 : (u1 < u2 ? -1 : 1);
}

static int ldouble_cmp_val(value v1, value v2)
{
  long double u1 = ldouble_custom_val(v1);
  long double u2 = ldouble_custom_val(v2);
  return ldouble_cmp(u1, u2);
}

static uint32_t ldouble_mix_hash(uint32_t hash, long double d) {
  union {
    long double d;
    uint32_t a[(LDOUBLE_STORAGE_BYTES+3)/4];
  } u;
  u.d = d;
 
  if (LDOUBLE_VALUE_BYTES == 16) {
    // ieee quad or __ibm128
#ifdef ARCH_BIG_ENDIAN
    hash = caml_hash_mix_uint32(hash, u.a[0]);
    hash = caml_hash_mix_uint32(hash, u.a[1]);
    hash = caml_hash_mix_uint32(hash, u.a[2]);
    hash = caml_hash_mix_uint32(hash, u.a[3]);
#else
    hash = caml_hash_mix_uint32(hash, u.a[1]);
    hash = caml_hash_mix_uint32(hash, u.a[0]);
    hash = caml_hash_mix_uint32(hash, u.a[3]);
    hash = caml_hash_mix_uint32(hash, u.a[2]);
#endif
  } else if (LDOUBLE_VALUE_BYTES == 10) {
    // intel extended 
    hash = caml_hash_mix_uint32(hash, u.a[0]);
    hash = caml_hash_mix_uint32(hash, u.a[1]);
    hash = caml_hash_mix_uint32(hash, u.a[2] & 0xFFFF);
  } else {
    // either LDOUBLE_VALUE_BYTES == 8, or we dont know what else to do.
    hash = caml_hash_mix_double(hash,  (double) d);
  }
  return hash;

}

static intnat ldouble_hash(value v) {
  return ldouble_mix_hash(0, ldouble_custom_val(v));
}

static struct custom_operations caml_ldouble_ops = {
  "ctypes:ldouble",
  custom_finalize_default,
  ldouble_cmp_val,
  ldouble_hash,
  NULL, //ldouble_serialize,
  NULL, //ldouble_deserialize,
  custom_compare_ext_default
};

value ctypes_copy_ldouble(long double u)
{
  value res = caml_alloc_custom(&caml_ldouble_ops, sizeof(long double), 0, 1);
  ldouble_custom_val(res) = u;
  return res;
}

long double ctypes_ldouble_val(value v) { return ldouble_custom_val(v); }

CAMLprim value ctypes_ldouble_of_float(value a) { 
  CAMLparam1(a); 
  CAMLreturn(ctypes_copy_ldouble(Double_val(a))); 
}
CAMLprim value ctypes_ldouble_to_float(value a) { 
  CAMLparam1(a); 
  CAMLreturn(caml_copy_double(ldouble_custom_val(a))); 
}
CAMLprim value ctypes_ldouble_of_int(value a) { 
  CAMLparam1(a); 
  CAMLreturn(ctypes_copy_ldouble(Int_val(a))); 
}
CAMLprim value ctypes_ldouble_to_int(value a) { 
  CAMLparam1(a); 
  CAMLreturn(Val_int(ldouble_custom_val(a))); 
}

#define OP2(OPNAME, OP)                                                               \
  CAMLprim value ctypes_ldouble_ ## OPNAME(value a, value b) {                        \
    CAMLparam2(a, b);                                                                 \
    CAMLreturn(ctypes_copy_ldouble( ldouble_custom_val(a) OP ldouble_custom_val(b))); \
  }

OP2(add, +)
OP2(sub, -)
OP2(mul, *)
OP2(div, /)

CAMLprim value ctypes_ldouble_neg(value a) { 
  CAMLparam1(a);
  CAMLreturn(ctypes_copy_ldouble( - ldouble_custom_val(a))); 
}

#define FN1(OP)                                                   \
  CAMLprim value ctypes_ldouble_ ## OP (value a) {                \
    CAMLparam1(a);                                                \
    CAMLreturn(ctypes_copy_ldouble( OP (ldouble_custom_val(a)))); \
  }

#define FN2(OP)                                                                          \
  CAMLprim value ctypes_ldouble_ ## OP (value a, value b) {                              \
    CAMLparam2(a, b);                                                                    \
    CAMLreturn(ctypes_copy_ldouble( OP (ldouble_custom_val(a), ldouble_custom_val(b)))); \
  }

FN2(powl)
FN1(sqrtl)
FN1(expl)
FN1(logl)
FN1(log10l)
FN1(expm1l)
FN1(log1pl)
FN1(cosl)
FN1(sinl)
FN1(tanl)
FN1(acosl)
FN1(asinl)
FN1(atanl)
FN2(atan2l)
FN2(hypotl)
FN1(coshl)
FN1(sinhl)
FN1(tanhl)
FN1(acoshl)
FN1(asinhl)
FN1(atanhl)
FN1(ceill)
FN1(floorl)
FN1(fabsl)
FN2(remainderl)
FN2(copysignl)

#undef OP2
#undef FN1
#undef FN2

CAMLprim value ctypes_ldouble_frexp(value v) {
  CAMLparam1(v);
  CAMLlocal2(r, rfv);
  long double f = ldouble_custom_val(v);
  int ri;
  long double rf;
  r = caml_alloc_tuple(2);
  rf = frexpl(f, &ri);
  rfv = ctypes_copy_ldouble(rf);
  Store_field(r,0, rfv);
  Store_field(r,1, Val_int(ri));
  CAMLreturn(r);
}

CAMLprim value ctypes_ldouble_ldexp(value vf, value vi) {
  CAMLparam2(vf, vi);
  CAMLlocal1(r);
  long double f = ldouble_custom_val(vf);
  int i = Int_val(vi);
  long double rf = ldexpl(f, i);
  r = ctypes_copy_ldouble(rf);
  CAMLreturn(r);
}

CAMLprim value ctypes_ldouble_modf(value v) {
  CAMLparam1(v);
  CAMLlocal1(r);
  long double f = ldouble_custom_val(v);
  long double rf2;
  long double rf1 = modfl(f, &rf2);
  r = caml_alloc_tuple(2);
  Store_field(r, 0, ctypes_copy_ldouble(rf1));
  Store_field(r, 1, ctypes_copy_ldouble(rf2));
  CAMLreturn(r);
}

enum {
  ml_FP_NORMAL = 0,
  ml_FP_SUBNORMAL,
  ml_FP_ZERO,
  ml_FP_INFINITE,
  ml_FP_NAN,
};

CAMLprim value ctypes_ldouble_classify(value v){
  CAMLparam1(v);
  CAMLlocal1(r);
  long double f = ldouble_custom_val(v);
  switch (fpclassify(f)){
  case FP_NORMAL    : r = Val_int(ml_FP_NORMAL); break;
  case FP_SUBNORMAL : r = Val_int(ml_FP_SUBNORMAL); break;
  case FP_ZERO      : r = Val_int(ml_FP_ZERO); break;
  case FP_INFINITE  : r = Val_int(ml_FP_INFINITE); break;
  case FP_NAN       : 
  default           : r = Val_int(ml_FP_NAN); break;
  }
  CAMLreturn(r);
}

static char *format_ldouble(char *fmt, long double d) {
  static size_t buf_len = 10;
  static char *buf = NULL;
  static char *empty = "";
  size_t print_len = 0;

  // allocate initial string
  if (buf == NULL) buf = malloc(buf_len);
  if (buf == NULL) return empty; // oops...stuck

  // try to print
  print_len = snprintf(buf, buf_len, fmt, d);

  // not enough space - reallocate and try again
  if (print_len >= buf_len) {
    // re-allocate string
    if (buf) free(buf);
    buf = malloc(print_len+1);
    buf_len = print_len+1;
    // try again
    return format_ldouble(fmt, d);
  } else 
    // ok
    return buf;
}

CAMLprim value ctypes_ldouble_format(value fmt, value d) {
  CAMLparam2(fmt, d);
  CAMLreturn(caml_copy_string(format_ldouble( String_val(fmt), ldouble_custom_val(d))));
}

CAMLprim value ctypes_ldouble_of_string(value v) {
  CAMLparam1(v);
  char *str = String_val(v);
  char *end = str + caml_string_length(v);
  long double r = strtold(str, &end);
  CAMLreturn(ctypes_copy_ldouble(r));
}

value ctypes_ldouble_min(void) { return ctypes_copy_ldouble(-LDBL_MAX); }
value ctypes_ldouble_max(void) { return ctypes_copy_ldouble(LDBL_MAX); }
value ctypes_ldouble_epsilon(void) { return ctypes_copy_ldouble(LDBL_EPSILON); }
value ctypes_ldouble_nan(void) { return ctypes_copy_ldouble(nanl("char-sequence")); }
// XXX note; -(log 0) gives +ve inf (and vice versa).  Is this consistent? *)
value ctypes_ldouble_inf(void) { return ctypes_copy_ldouble(-log(0)); } 
value ctypes_ldouble_ninf(void) { return ctypes_copy_ldouble(log(0)); }

value ctypes_ldouble_size(void) {
  value r = caml_alloc_tuple(2);
  Field(r,0) = Val_int(LDOUBLE_STORAGE_BYTES);
  Field(r,1) = Val_int(LDOUBLE_VALUE_BYTES);
  return r;
}

/*********************** complex *************************/

#define ldouble_complex_custom_val(V) (*(long double complex*)(Data_custom_val(V)))

static int ldouble_complex_cmp_val(value v1, value v2)
{
  long double complex u1 = ldouble_custom_val(v1);
  long double complex u2 = ldouble_custom_val(v2);
  int cmp_real = ldouble_cmp(creall(u1), creall(u2));
  return cmp_real == 0 ? ldouble_cmp(cimagl(u1), cimagl(u2)) : cmp_real;
}

static intnat ldouble_complex_hash(value v) {
  long double complex c = ldouble_complex_custom_val(v);
  return ldouble_mix_hash(ldouble_mix_hash(0, creall(c)), cimagl(c));
}

static struct custom_operations caml_ldouble_complex_ops = {
  "ctypes:ldouble_complex",
  custom_finalize_default,
  ldouble_complex_cmp_val,
  ldouble_complex_hash,
  NULL, //ldouble_complex_serialize,
  NULL, //ldouble_complex_deserialize,
  custom_compare_ext_default
};

value ctypes_copy_ldouble_complex(long double complex u)
{
  value res = caml_alloc_custom(&caml_ldouble_complex_ops, sizeof(long double complex), 0, 1);
  ldouble_complex_custom_val(res) = u;
  return res;
}

long double complex ctypes_ldouble_complex_val(value v) {
  return ldouble_complex_custom_val(v);
}

/* make : t -> t -> complex */
CAMLprim value ctypes_ldouble_complex_make(value r, value i) {
  CAMLparam2(r, i);
  long double re = ldouble_custom_val(r);
  long double im = ldouble_custom_val(i);
  CAMLreturn(ctypes_copy_ldouble_complex(re + (im * I)));
}

/* real : complex -> t */
CAMLprim value ctypes_ldouble_complex_real(value v) {
  CAMLparam1(v);
  CAMLreturn(ctypes_copy_ldouble(creall(ldouble_complex_custom_val(v))));
}

/* imag : complex -> t */
CAMLprim value ctypes_ldouble_complex_imag(value v) {
  CAMLparam1(v);
  CAMLreturn(ctypes_copy_ldouble(cimagl(ldouble_complex_custom_val(v))));
}

#define OP2(OPNAME, OP)                                                    \
  CAMLprim value ctypes_ldouble_complex_ ## OPNAME(value a, value b) {     \
    CAMLparam2(a, b);                                                      \
    CAMLreturn(ctypes_copy_ldouble_complex(                                \
        ldouble_complex_custom_val(a) OP ldouble_complex_custom_val(b) )); \
  }

OP2(add, +)
OP2(sub, -)
OP2(mul, *)
OP2(div, /)

CAMLprim value ctypes_ldouble_complex_neg(value a) {
  CAMLparam1(a);
  CAMLreturn(ctypes_copy_ldouble_complex( - ldouble_complex_custom_val(a) )); 
}

#define FN1(OP)                                                                   \
  CAMLprim value ctypes_ldouble_complex_ ## OP (value a) {                        \
    CAMLparam1(a);                                                                \
    CAMLreturn(ctypes_copy_ldouble_complex( OP (ldouble_complex_custom_val(a)))); \
  }

#define FN2(OP)                                                            \
  CAMLprim value ctypes_ldouble_complex_ ## OP (value a, value b) {        \
    CAMLparam2(a, b);                                                      \
    CAMLreturn(ctypes_copy_ldouble_complex(                                \
      OP (ldouble_complex_custom_val(a), ldouble_complex_custom_val(b)))); \
  }

FN1(conjl)
FN1(csqrtl)
FN1(cexpl)
FN1(clogl)
FN2(cpowl)

CAMLprim value ctypes_ldouble_complex_cargl(value a) {
  CAMLparam1(a);
  CAMLreturn(ctypes_copy_ldouble( cargl(ldouble_complex_custom_val(a))));
}


