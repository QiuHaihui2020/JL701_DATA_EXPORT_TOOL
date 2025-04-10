#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lp_touch_key_range_algo.data.bss")
#pragma data_seg(".lp_touch_key_range_algo.data")
#pragma const_seg(".lp_touch_key_range_algo.text.const")
#pragma code_seg(".lp_touch_key_range_algo.text")
#endif
#include "system/includes.h"
#include "asm/lp_touch_key_range_algo.h"


#define ABS(a)                      ((a)>0?(a):-(a))
#define ELEM_SWAP_USHORT(a,b)       { unsigned short t=(a);(a)=(b);(b)=t; }

#define INIT_SIGMA                  (5)
#define MIN_SIGMA                   (5)
#define OUTLIER_ZSCORE              (6)
#define PRESSED_ZSCORE              (12)
#define PULSE_WIDTH                 (7)
#define MIN_VALLEY_SAMPLE           (3)
#define FIFO_BUF_SIZE               (8)
#define HISTORY_BUF_SIZE            (12)
#define UNBALANCED_TH               (2) //0.2
#define RANGE_TH_ALPHA              (5)
#define RANGE_TH_DOWN_ALPHA         (2)
#define RANGE_TH_DOWNWARD_EN        (0)
#define RANGE_STABLE_CHECK_EN       (1)
#define RANGE_STABLE_VALUE_TH       (3)     //0.3
#define RANGE_STABLE_COUNT_TH       (10)
#define RANGE_STABLE_ALPHA          (12)    //0.04296875

typedef struct {
    u32 count;

    //for median filter
    s16 med_dat0;
    s16 med_dat1;
    u8 med_sign;

    //for gradient
    s16 fifo_buf[FIFO_BUF_SIZE];
    u8  fifo_pos;
    u8  fifo_size;
    s32 grad_max;

    u8  key_state;

    //for sigma tracing
    s32 std_sum;
    s32 std_count;
    s32 sigma_iir_state;
    s16 sigma_iir_alpha;
    s16 sigma_iir_alpha_fast;

    //stable count
    s32 stable_count;
    u8  is_stable;

    //for valley tracing
    s16 valley_grad;
    s16 delayed_valley_grad;
    s16 peak_grad;
    s16 valley_val;
    s16 peak_max;
    s16 valley_duration;
    //down continued
    u8  down_cont;
    //up continued
    u8  up_cont;
    u8  delayed_keyup;

#if RANGE_TH_DOWNWARD_EN
    s32 high_undetect_count;
    s32 low_detect_count;

    u8  th_downward;
#endif

    //for delta filter
    u16 prev_val;

    u16 range;
    u16 range_median;
    u16 range_for_th;
    u16 range_list[HISTORY_BUF_SIZE];
    u16 range_list_median[HISTORY_BUF_SIZE];
    u8  range_size;
    u8  range_pos;
    u8  range_valid;

    u16 range_min;
    u16 range_max;

    u8  maybe_noise;

#if RANGE_STABLE_CHECK_EN
    s32 range_stable_iir_state;
    s32 range_stable_count;
    u8 range_isstable;
#endif
} TouchRangeAlgo_t;

static TouchRangeAlgo_t *tra_ch[5];

static inline unsigned short kth_smallest_ushort(unsigned short a[], int n, int k)
{
    int i, j, l, m ;
    unsigned short x ;
    l = 0 ;
    m = n - 1 ;
    while (l < m) {
        x = a[k] ;
        i = l ;
        j = m ;
        do {
            while (a[i] < x) {
                i++ ;
            }
            while (x < a[j]) {
                j-- ;
            }
            if (i <= j) {
                ELEM_SWAP_USHORT(a[i], a[j]) ;
                i++ ;
                j-- ;
            }
        } while (i <= j) ;
        if (j < k) {
            l = i ;
        }
        if (k < i) {
            m = j ;
        }
    }
    return a[k] ;
}

static u16 medfilt(TouchRangeAlgo_t *tra, u16 x)
{
    u16 ret = 0;
    if (tra->med_sign) {
        //dat0 <= dat1;
        if (x <= tra->med_dat0) {
            ret = tra->med_dat0;
            tra->med_sign = 0;
        } else if (x >= tra->med_dat1) {
            ret = tra->med_dat1;
            tra->med_sign = 1;
        } else {
            ret = x;
            tra->med_sign = 0;
        }
    } else {
        //da0 > dat1;
        if (x <= tra->med_dat1) {
            ret = tra->med_dat1;
            tra->med_sign = 0;
        } else if (x >= tra->med_dat0) {
            ret = tra->med_dat0;
            tra->med_sign = 1;
        } else {
            ret = x;
            tra->med_sign = 1;
        }
    }
    tra->med_dat0 = tra->med_dat1;
    tra->med_dat1 = x;
    return ret;
}

static s32 iir_filter(s32 *state, u16 x, s16 alpha)
{
    *state = (*state * (256 - alpha) + ((s32)x << 6) * alpha + 128) >> 8;
    return *state;
}

static s32 newton_sqrt(s32 in)
{
    int x = 1;
    int y = (x + in / x) >> 1;

    while (ABS(y - x) > 1) {
        x = y;
        y = (x + in / x) >> 1;
    }
    return y;
}

static void TouchRangeAlgo_tracing(TouchRangeAlgo_t *tra, u16 x)
{

    s32 sigma;
    s32 delta, delta_abs;
    s32 outlier_th;
    s32 pressed_th, sigma_pressed_th;
    s32 grad, grad_abs, grad_abs_max;
    s32  grad_sign_max;

    sigma = (tra->sigma_iir_state + 32) >> 6;

    sigma_pressed_th = sigma * PRESSED_ZSCORE;
    outlier_th = sigma * OUTLIER_ZSCORE;

    if (tra->range_valid || tra->range_size > 0) {
        pressed_th = tra->range_for_th / 2;

        if (pressed_th < sigma_pressed_th) {
            pressed_th = sigma_pressed_th;
        }
    } else {
        pressed_th = sigma_pressed_th;
    }

    //delta filter
    delta = (s32)x - (s32)tra->prev_val;
    tra->prev_val = x;

    delta_abs = delta;
    if (delta_abs < 0) {
        delta_abs = -delta_abs;
    }

    if (delta_abs < outlier_th) {
        tra->std_sum += delta * delta;
        tra->std_count += 1;

        if (tra->std_count == 128) {
            s32 var = newton_sqrt((tra->std_sum + tra->std_count / 2) / tra->std_count);
            tra->std_count = 0;
            tra->std_sum = 0;

            if (var < MIN_SIGMA) {
                var = MIN_SIGMA;
            }

            if (tra->count < 128 * 8) {
                iir_filter(&tra->sigma_iir_state, var, tra->sigma_iir_alpha_fast);
            } else {
                iir_filter(&tra->sigma_iir_state, var, tra->sigma_iir_alpha);
            }
        }

        tra->stable_count += 1;
        if (tra->stable_count > 10) {
            tra->is_stable = 1;
        }
    } else {
        tra->stable_count = 0;
        tra->is_stable = 0;
    }
    // printf("%d\n", sigma);

    //gradient
    grad_abs_max = delta_abs;
    grad_sign_max = delta >> 31;
    for (int i = 1; i < tra->fifo_size; i += 2) {
        int idx = (FIFO_BUF_SIZE + tra->fifo_pos - 1 - i) % FIFO_BUF_SIZE;

        grad = x - tra->fifo_buf[idx];
        grad_abs = ABS(grad);

        if (grad_abs > grad_abs_max) {
            grad_abs_max = grad_abs;
            grad_sign_max = grad >> 31;
        }
    }

    if (grad_sign_max == -1) {
        tra->grad_max = -grad_abs_max;
    } else {
        tra->grad_max = grad_abs_max;
    }

    //append to fifo
    tra->fifo_buf[tra->fifo_pos] = x;
    tra->fifo_pos = (tra->fifo_pos + 1) % FIFO_BUF_SIZE;
    tra->fifo_size += 1;
    if (tra->fifo_size > FIFO_BUF_SIZE) {
        tra->fifo_size = FIFO_BUF_SIZE;
    }

#if RANGE_TH_DOWNWARD_EN
    if (grad_abs_max <= pressed_th) {
        tra->high_undetect_count += 1;
        if (grad_abs_max > sigma_pressed_th) {
            tra->low_detect_count += 1;
        }

        if (tra->high_undetect_count >= 60000 && tra->low_detect_count > 0) { //10min
            tra->th_downward = 1;
        }

        if (tra->th_downward && tra->high_undetect_count % 100 == 0) {
            tra->range_for_th = (tra->range_for_th * (256 - RANGE_TH_DOWN_ALPHA) + sigma_pressed_th * 2 * RANGE_TH_DOWN_ALPHA + 128) >> 8;
        }
    } else {
        tra->high_undetect_count = 0;
        tra->low_detect_count = 0;

        tra->th_downward = 0;
    }
#endif

    if (tra->maybe_noise) {
        if (tra->is_stable) {
            tra->maybe_noise = 0;
        } else {
            return;
        }
    }

    //key state machine
    if (tra->key_state == 0) {
        if (grad_abs_max > pressed_th && grad_sign_max == 0) {
            //key up
            //nothing to do
            if (tra->up_cont == 0) {
                tra->maybe_noise = 1;
            }

            if (tra->up_cont) {
                if (tra->peak_max < x) {
                    tra->peak_max = x;
                    tra->peak_grad = tra->peak_max - tra->valley_val;
                }
            }

            tra->down_cont = 0;
            tra->up_cont = 1;
        } else if (grad_abs_max > pressed_th && grad_sign_max == -1) {
            //key down
            tra->key_state = 1;
            tra->valley_duration = 1;
            tra->valley_val = x;
            tra->valley_grad = grad_abs_max;

            tra->down_cont = 1;
            tra->up_cont = 0;
        } else {
            //nothing to do
            tra->down_cont = 0;
            tra->up_cont = 0;
        }
    } else {
        if (grad_abs_max > pressed_th && grad_sign_max == 0) {
            //key up
            tra->key_state = 0;

            tra->peak_max = x;
            tra->peak_grad = tra->peak_max - tra->valley_val;

            if (tra->valley_duration >= PULSE_WIDTH) {
                tra->delayed_keyup = 1;
                tra->delayed_valley_grad = tra->valley_grad;
            }

            tra->down_cont = 0;

            tra->up_cont = 1;

        } else if (grad_abs_max > pressed_th && grad_sign_max == -1) {
            //key down

            if (tra->down_cont) {
                tra->valley_duration += 1;
                if (x < tra->valley_val) {
                    tra->valley_grad += tra->valley_val - x;
                    tra->valley_val = x;
                }
            } else {
                //keydown  when it's already in keydown state
                //Discarding all history information, cause it's not reliable
                // tra->range_size = 0;
                // tra->range_pos = 0;
                // tra->range_valid = 0;
                // printf("keydown  when it's already in keydown state\n");

                //reset keydown state
                tra->key_state = 1;
                tra->valley_duration = 1;
                tra->valley_val = x;
                tra->valley_grad = grad_abs_max;
            }

            tra->down_cont = 1;
            tra->up_cont = 0;

        } else {
            tra->valley_duration += 1;
            if (x < tra->valley_val) {
                tra->valley_grad += tra->valley_val - x;
                tra->valley_val = x;
            }
            tra->down_cont = 0;
            tra->up_cont = 0;

        }

        if (tra->delayed_keyup && tra->up_cont == 0) {
            s16 peak_grad = tra->peak_grad;

            s16 min_grad = MIN(peak_grad, tra->delayed_valley_grad);
            s16 max_grad = MAX(peak_grad, tra->delayed_valley_grad);
            s32 stablized_range;
            s32 range_th;

            tra->delayed_keyup = 0;

#if RANGE_STABLE_CHECK_EN
            stablized_range = (tra->range_stable_iir_state + 32) >> 6;
            range_th = stablized_range * RANGE_STABLE_VALUE_TH / 10;

            if ((max_grad - min_grad) * 10 < max_grad * UNBALANCED_TH
                && min_grad > tra->range_min
                && max_grad < tra->range_max
                && ((tra->range_isstable && ABS(stablized_range - max_grad) < range_th) || tra->range_isstable == 0))
#else
            if ((max_grad - min_grad) * 10 < max_grad * UNBALANCED_TH
                && min_grad > tra->range_min
                && max_grad < tra->range_max)
#endif
            {
                tra->range_list[tra->range_pos] = max_grad;
                tra->range_pos = (tra->range_pos + 1) % HISTORY_BUF_SIZE;
                if (tra->range_size < HISTORY_BUF_SIZE) {
                    tra->range_size += 1;
                }

                tra->range = tra->range_list[0];
                for (int i = 1; i < tra->range_size; i++) {
                    if (tra->range < tra->range_list[i]) {
                        tra->range = tra->range_list[i];
                    }
                }


                memcpy(&tra->range_list_median, &tra->range_list, tra->range_size * sizeof(u16));
                tra->range_median = kth_smallest_ushort(tra->range_list_median, tra->range_size, tra->range_size / 2);

                // if(tra->range_size > HISTORY_BUF_SIZE/2)
                // {
                tra->range =  tra->range_median;
                // }

                if (tra->range_valid == 0) {
                    tra->range_for_th =  sigma_pressed_th * 2;
#if RANGE_STABLE_CHECK_EN
                    tra->range_stable_iir_state = tra->range << 6;
#endif
                } else {

                    tra->range_for_th = (tra->range_for_th * (256 - RANGE_TH_ALPHA) + tra->range_median * RANGE_TH_ALPHA + 128) >> 8;

#if RANGE_STABLE_CHECK_EN

                    iir_filter(&tra->range_stable_iir_state, tra->range, RANGE_STABLE_ALPHA);

                    if (tra->range > stablized_range - range_th
                        && tra->range < stablized_range  + range_th
                       ) {
                        tra->range_stable_count += 1;
                    } else {
                        tra->range_stable_count = 0;
                    }

                    if (tra->range_stable_count > RANGE_STABLE_COUNT_TH) {
                        tra->range_isstable = 1;
                    }
#endif
                }

                if (tra->range_size >= MIN_VALLEY_SAMPLE) {
                    tra->range_valid = 1;
                }
                if (tra->range_valid) {
                    // printf("range:%d, median:%d\n", tra->range, tra->range_median);
                }
            } else {
                /* printf("invalid touch value\n"); */
            }
        }
    }

    return;
}


void TouchRangeAlgo_Init(u32 ch_idx, u16 min, u16 max)
{
    if (tra_ch[ch_idx] == NULL) {
        tra_ch[ch_idx] = (TouchRangeAlgo_t *)zalloc(sizeof(TouchRangeAlgo_t));
    }

    TouchRangeAlgo_t *tra = tra_ch[ch_idx];

    tra->count = 0;

    tra->fifo_pos = 0;
    tra->fifo_size = 0;

    tra->key_state = 0;

    tra->med_dat0 = tra->med_dat1 = 0;
    tra->med_sign = 1;

    tra->std_sum = 0;
    tra->std_count = 0;
    tra->sigma_iir_alpha = 5;
    tra->sigma_iir_alpha_fast = 32;
    tra->sigma_iir_state = INIT_SIGMA << 6;

    tra->stable_count = 0;
    tra->is_stable = 0;

    tra->valley_val = 0;
    tra->valley_duration = 0;
    tra->prev_val = 0;

    tra->delayed_keyup = 0;

#if RANGE_TH_DOWNWARD_EN
    tra->high_undetect_count = 0;
    tra->low_detect_count = 0;
    tra->th_downward = 0;
#endif
    tra->range_valid = 0;
    tra->range_median = 0;
    tra->range = 0;
    tra->range_size = 0;
    tra->range_pos = 0;

    tra->range_min = min;
    tra->range_max = max;

    tra->maybe_noise = 0;

#if RANGE_STABLE_CHECK_EN
    tra->range_stable_iir_state = 0;
    tra->range_stable_count = 0;
    tra->range_isstable = 0;
#endif
}

void TouchRangeAlgo_Update(u32 ch_idx, u16 x)
{
    TouchRangeAlgo_t *tra = tra_ch[ch_idx];
    u16 dat0;

    dat0 = medfilt(tra, x);
    if (tra->count < 3) {
        tra->count++;

        tra->prev_val = dat0;
        return;
    }

    TouchRangeAlgo_tracing(tra, dat0);

    tra->count++;
    return;
}

void TouchRangeAlgo_Reset(u32 ch_idx, u16 min, u16 max)
{
    TouchRangeAlgo_Init(ch_idx, min, max);
}

s32 TouchRangeAlgo_GetSigma(u32 ch_idx)
{
    TouchRangeAlgo_t *tra = tra_ch[ch_idx];
    return tra->sigma_iir_state;
}

u16 TouchRangeAlgo_GetRange(u32 ch_idx, u8 *valid)
{
    TouchRangeAlgo_t *tra = tra_ch[ch_idx];
    *valid = tra->range_valid;
    return tra->range_valid ? tra->range : 0;
}

void TouchRangeAlgo_SetRange(u32 ch_idx, u16 range)
{
    TouchRangeAlgo_t *tra = tra_ch[ch_idx];
    tra->range = range;
    tra->range_valid = 1;

    tra->range_list[0] = range;
    tra->range_size = 1;
    tra->range_pos = 1;
}

void TouchRangeAlgo_SetSigma(u32 ch_idx, s32 sigma)
{
    TouchRangeAlgo_t *tra = tra_ch[ch_idx];
    tra->sigma_iir_state = sigma;
}


