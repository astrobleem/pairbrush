/* Host-only fault injection for actual RESMEM.H transaction logic. */
#include <stdio.h>
#include <stdlib.h>
static unsigned current_strategy, allocated, next_segment;
static unsigned get_error, set_error, alloc_error, free_error;
static unsigned restore_errors, set_calls, free_calls, alloc_calls;
static int valid;
static unsigned dos_get_strategy(unsigned *v)
{ if(get_error)return get_error; *v=current_strategy;return 0; }
static unsigned dos_set_strategy(unsigned v)
{
    set_calls++;
    if(set_error)return set_error;
    if(v!=2 && restore_errors) {restore_errors--;return 1;}
    current_strategy=v;return 0;
}
static unsigned dos_allocate(unsigned n,unsigned *s)
{
    alloc_calls++;
    if(n!=0x400)abort();
    if(alloc_error)return alloc_error;
    allocated=next_segment;*s=allocated;return 0;
}
static unsigned dos_free_block(unsigned s)
{
    free_calls++;
    if(free_error)return free_error;
    if(s!=allocated)abort();
    allocated=0;return 0;
}
static int dos_valid_video_block(unsigned s)
{ return valid && s==0x9800; }
#include "RESMEM.H"
static unsigned checks;
static void check(int ok,const char *msg)
{ checks++;if(!ok){fprintf(stderr,"FAIL %s\n",msg);exit(1);} }
static void reset(void)
{
 current_strategy=1;allocated=0;next_segment=0x9800;
 get_error=set_error=alloc_error=free_error=restore_errors=0;
 set_calls=free_calls=alloc_calls=0;valid=1;
 reservation=saved_strategy=strategy_pending=cleanup_error=last_dos_error=0;
 last_free_result=0;
}
int main(void)
{
 reset();check(reserve_block(),"normal acquire");
 check(reservation==0x9800 && current_strategy==1 && !strategy_pending,
       "normal strategy restored");
 check(release_reserved() && !reservation && !allocated,"normal free");
 check(release_reserved() && free_calls==1,"cleanup idempotence");
 reset();get_error=1;check(!reserve_block(),"get failure");
 check(!set_calls && !alloc_calls && !reservation,"get failure no mutations");
 reset();set_error=1;check(!reserve_block(),"last-fit set failure");
 check(current_strategy==1 && !alloc_calls && !strategy_pending,
       "failed set leaves original strategy");
 reset();alloc_error=8;check(!reserve_block(),"allocation failure");
 check(current_strategy==1 && !strategy_pending && !reservation && !free_calls,
       "allocation failure restores strategy, owns no block");
 reset();next_segment=0x9000;check(!reserve_block(),"wrong segment rejected");
 check(!allocated && !reservation && free_calls==1 && current_strategy==1,
       "wrong segment freed and strategy restored");
 reset();valid=0;check(!reserve_block(),"bad MCB rejected");
 check(!allocated && !reservation && free_calls==1,"bad MCB allocation freed");
 reset();restore_errors=1;check(!reserve_block(),"restore failure refuses mode");
 check(strategy_pending && current_strategy==2 && !reservation && !allocated,
       "pending restore tracked, block freed");
 check(restore_strategy() && !strategy_pending && current_strategy==1,
       "early cleanup retries restoration");
 reset();restore_errors=2;alloc_error=8;check(!reserve_block(),"dual failure");
 check(!restore_strategy() && strategy_pending,"repeated restore failure retained");
 check(restore_strategy() && current_strategy==1,"later retry succeeds");
 reset();check(reserve_block(),"acquire for failed free");
 free_error=7;check(!release_reserved(),"free error detected");
 check(reservation==0x9800 && allocated==0x9800 && cleanup_error &&
       last_dos_error==7,"free error preserves ownership/evidence");
 free_error=0;check(release_reserved() && !reservation && !allocated,
                    "free retry succeeds");
 reset();next_segment=0x9000;free_error=7;
 check(!reserve_block(),"wrong segment and failed free");
 check(reservation==0x9000 && cleanup_error && current_strategy==1,
       "failed early free preserves ownership");
 free_error=0;check(release_reserved() && !reservation,"early free retry");
 printf("PASS %u allocation/cleanup fault assertions\n",checks);
 return 0;
}
