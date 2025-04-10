#ifndef MATH_FAST_FUNCTION
#define MATH_FAST_FUNCTION

struct data_q_struct {
    long data;
    char q;
};

float add_float(float x, float y);
float sub_float(float x, float y);
long  float_to_int(float x);
float int_to_float(long x);
long  cos_fix(long x);
float cos_float(float x);
long  sin_fix(long x);
float sin_float(float x);
struct data_q_struct complex_abs_fix(long x, long y);
struct data_q_struct complex_dqdt_fix(long x, long y);
float  complex_abs_float(float x, float y);
float  complex_dqdt_float(float x, float y);
struct data_q_struct root_fix(struct data_q_struct x);
float  root_float(float x);
struct data_q_struct mul_fix(long x, long y);
float  mul_float(float x, float y);
struct data_q_struct div_fix(long x, long y);
float  div_float(float x, float y);
struct data_q_struct exp_fix(long x);
float  exp_float(float x);
struct data_q_struct ln_fix(struct data_q_struct x);
float  ln_float(float x);
struct data_q_struct angle_fix(long x, long y);
float angle_float(float x, float y);
struct data_q_struct atanh_fix(long x, long y);
float atanh_float(float x, float y);
long  cosh_fix(long x);
float cosh_float(float x);
long  sinh_fix(long x);
float sinh_float(float x);
struct data_q_struct log10_fix(struct data_q_struct x);
float  log10_float(float x);
struct data_q_struct sigmoid_fix(float x);
float  sigmoid_float(float x);
struct data_q_struct tanh_fix(float x);
float  tanh_float(float x);
float dB_Convert_Mag(float x);

struct data_q_struct complex_dqdt_fix_limit(long x, long y);
struct data_q_struct atanh_fix_limit(long x, long y);
float complex_dqdt_float_limit(float x, float y);
float atanh_float_limit(float x, float y);



#endif

