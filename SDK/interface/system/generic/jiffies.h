#ifndef JIFFIES_H
#define JIFFIES_H
/* timer interface */
/* Parameters used to convert the timespec values: */
#define HZ				100L
#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL


#ifndef __ASSEMBLY__
extern volatile unsigned long jiffies;
extern unsigned long jiffies_msec();
extern unsigned long jiffies_offset2msec(unsigned long begin_msec, int offset_msec);
extern int jiffies_msec2offset(unsigned long begin_msec, unsigned long end_msec);

extern unsigned long jiffies_usec();
extern unsigned long jiffies_offset2usec(unsigned long base_usec, int offset_usec);
extern int jiffies_usec2offset(unsigned long begin, unsigned long end);

extern unsigned long audio_jiffies_usec(void);
#endif


#define time_after(a,b)					((long)(b) - (long)(a) < 0)
#define time_before(a,b)				time_after(b,a)


#define msecs_to_jiffies(msec) 		    ((msec)/10)
#define jiffies_to_msecs(j) 		    ((j)*10)


static inline int jiffies_offset_to_msec(unsigned long begin, unsigned long end)
{
    int offset = end - begin;
    if (offset < 0 && time_after(end, begin)) {
        offset += 0xffffffff;
    }
    return jiffies_to_msecs(offset);
}


#endif

