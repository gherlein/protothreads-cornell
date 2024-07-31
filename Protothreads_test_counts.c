
/*
 -- Multicore --
 Protothreads 1.3 test code:
 Core 0:
 -- blinky thread which is locked to stay in sync with a thread on core 1
 -- serial thread which prints statistics for priority dispatcher

 Core 1:
-- THREE separate threads, each just a time waster to check the priority dispatcher.
   Each has a settable yield time, plus a settable blocking sleep (to simulate real work).
   By setting the sleep time high enough, lower priority threads will be starved out.
-- blinky thread which  is locked to stay in sync with a thread on core 0
 
 */

#include "hardware/gpio.h"
#include "hardware/timer.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/uart.h"
#include "stdio.h"
#include <string.h>
#include <pico/multicore.h>
#include "hardware/sync.h"
#include "stdlib.h"
//

// ==========================================
// === protothreads globals
// ==========================================
// protothreads header
#include "pt_cornell_rp2040_v1_3.h"

// === global thread communicaiton
// locks to force two threads to alternate execution
// (useful for timing a thread context switch)
// uses low level hardware spin lock 
struct pt_sem thread2_go_s, thread3_go_s ;
//
// counters for detecting thread swap lock errors
// the difference between two threads that should alternate
int delta_count ;
// total times delta_count was greater than 1 or less than zero
int error_count ;
//  total number of times thru each  of the threads
// core_thread
int thread_count ;
// data protections
spin_lock_t * lock_delta_count ;
spin_lock_t * lock_error_count ;

// ==================================================
// === user's serial input thread
// ==================================================
// serial_read an serial_write do not block any thread
// except this one
static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);
    PT_INTERVAL_INIT() ;
      //
      while(1) {
        // print once/second
        PT_YIELD_INTERVAL(1000000);
        printf("Time = %6.4f\n\r\n\r", (float)PT_GET_TIME_usec()/1e6 );
         // execution stats
         // print core 0 thread executions
         sprintf(pt_serial_out_buffer, "core0 thread execution counts: ") ;
        // spawn a thread to do the non-blocking serial write
         serial_write ; 
          for(int i=0; i < pt_task_count; i++){
            printf("T%d=%d ",i,  sched_thread_stats[i]);
          }
          printf("\n\r");
          //
          // print core 0 thread times
          sprintf(pt_serial_out_buffer, "core0 thread aummws execution time: ") ;
        // spawn a thread to do the non-blocking serial write
         serial_write ; 
          for(int i=0; i < pt_task_count; i++){
            printf("T%d=%lld ",i,  sched_thread_time[i]);
          }
          printf("\n\r\n\r");
          
          ///////
          // print core 1 thread executions
          sprintf(pt_serial_out_buffer, "core1 thread execution counts: ") ;
        // spawn a thread to do the non-blocking serial write
         serial_write ; 
          for(int i=0; i < pt_task_count1; i++){
            printf("T%d=%d ",i,  sched_thread_stats1[i]);
          }
          printf("\n\r");
          //
          // print core 0 thread times
          sprintf(pt_serial_out_buffer, "core1 thread summed execution time: ") ;
        // spawn a thread to do the non-blocking serial write
         serial_write ; 
          for(int i=0; i < pt_task_count1; i++){
            printf("T%d=%lld ",i,  sched_thread_time1[i]);
          }
          printf("\n\r");
          printf("======================\n\r");
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// ==================================================
// === cycle threads 
// ==================================================
// if you make sleep_time too big, then thread toggle_3 will fail
// due to thread starvation
int sleep_time = 30 ;
int yield_time = 100;
//
static PT_THREAD (protothread_cycle_1(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        // yield time 
        PT_YIELD_usec(yield_time) ;
        //blockking work time
        sleep_us(sleep_time) ;
      } // END WHILE(1)
  PT_END(pt);
} // cycle1  thread

//
static PT_THREAD (protothread_cycle_2(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        // yield time 
        PT_YIELD_usec(yield_time) ;
        //blockking work time
        sleep_us(sleep_time) ;
      } // END WHILE(1)
  PT_END(pt);
} // cycle2  thread

//
static PT_THREAD (protothread_cycle_3(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        // yield time 
        PT_YIELD_usec(yield_time) ;
        //blockking work time
        sleep_us(sleep_time) ;
      } // END WHILE(1)
  PT_END(pt);
} // cycle31 3  thread

// ==================================================
// === toggle2 thread 
// ==================================================
// toggle gpio 2  while alternating with
// another thread 
static PT_THREAD (protothread_toggle2(struct pt *pt))
{
    PT_BEGIN(pt);
    static bool LED_state = false ;
    //
     // set up LED gpio 2
     gpio_init(2) ;	
     gpio_set_dir(2, GPIO_OUT) ;
     gpio_put(2, true);
     // data structure for interval timer
     PT_INTERVAL_INIT() ;

      while(1) {
        // lock thread2_go is signaled from 
        // thread toggle3
        PT_SEM_SAFE_WAIT(pt, &thread2_go_s) ;
        //
        // toggle gpio 2
        LED_state = !LED_state ;
        gpio_put(2, LED_state);
        //
        PT_YIELD_INTERVAL(200000) ;

        // alternation error detection
        PT_LOCK_WAIT(pt, lock_delta_count) ;
        delta_count++;
        PT_LOCK_RELEASE(lock_delta_count);

        // total thread count
        thread_count++;
        //
        // tell toggle3 thread to run
        PT_SEM_SAFE_SIGNAL(pt, &thread3_go_s) ;
        //
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // blink thread

// ==================================================
// === toggle3 thread -- RUNNING on core 1
// ==================================================
// toggle gpio 3 while alternating with
// another thread
static PT_THREAD (protothread_toggle3(struct pt *pt))
{
    PT_BEGIN(pt);
    static bool LED_state = false ;
    //
     // set up gpio 3
     gpio_init(3) ;	
     gpio_set_dir(3, GPIO_OUT) ;
     gpio_put(3, false);
     // data structure for interval timer
     PT_INTERVAL_INIT() ;

      while(1) {
        // lock thread3_go is signaled from 
        // thread toggle2
        PT_SEM_SAFE_WAIT(pt, &thread3_go_s) ;

        // toggle gpio 3
        LED_state = !LED_state ;
        gpio_put(3, LED_state);
        // interval
        PT_YIELD_INTERVAL(50) ;
        //
        PT_LOCK_WAIT(pt, lock_delta_count) ;
        delta_count--;
        PT_LOCK_RELEASE(lock_delta_count);

        // tell toggle2 to run
        PT_SEM_SAFE_SIGNAL(pt, &thread2_go_s) ;
        //
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // blink thread

// ========================================
// === core 1 main -- started in main below
// ========================================
void core1_main(){ 
  // reset FIFO sending data to core 0
  PT_FIFO_FLUSH ;
  //  === add threads  ====================
  // for core 1
  // NOTE first added is HIGHEST priority
  pt_add_thread(protothread_cycle_1);
  pt_add_thread(protothread_cycle_2);
  pt_add_thread(protothread_cycle_3);
  pt_add_thread(protothread_toggle3);
  //
  // === initalize the scheduler ==========
  pt_sched_method = SCHED_PRIORITY ;
  pt_schedule_start ;
  // NEVER exits
  // ======================================
}

// ========================================
// === core 0 main
// ========================================
int main(){
  // set the clock
  //set_sys_clock_khz(250000, true); 

  // start the serial i/o
  stdio_init_all() ;
  // announce the threader version on system reset
  printf("\n\rProtothreads RP2040 v1.3 two-core, priority\n\r");

  // init global locks
  // to enforce alernation between threads
  // BEFORE starting any threads
  // use locks 31 downto 22
  
  // lock 31 init
   PT_LOCK_INIT(lock_error_count, 31, UNLOCKED) ;
   // lock 30 init 
   PT_LOCK_INIT(lock_delta_count, 30, UNLOCKED) ;

   // two safe semaphores
   PT_SEM_SAFE_INIT(&thread2_go_s, 1) ;
   PT_SEM_SAFE_INIT(&thread3_go_s, 0) ;

  // start core 1 threads
  multicore_reset_core1();
  multicore_launch_core1(&core1_main);

  // === config threads ========================
  // for core 0
  // NOTE first added is HIGHEST priority
  //pt_add_thread(protothread_toggle2);

  pt_add_thread(protothread_toggle2);
  pt_add_thread(protothread_serial);
  
  //
  // === initalize the scheduler ===============
  // method is either:
  //   SCHED_PRIORITY or SCHED_ROUND_ROBIN
  pt_sched_method = SCHED_PRIORITY ;
  pt_schedule_start ;
  // NEVER exits
  // ===========================================
} // end main
///////////
// end ////
///////////