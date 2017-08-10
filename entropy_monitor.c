/******
 *
 * avail_entropy_mon.c 
 *
 * 
 * collect available entropy for a number of iterations
 * with an sleep interval in between each iteration.
 *  
 * after the number of iterations 
 * print maximum, minimum and average 
 * to stdout in a zabbix-sender friendly format
 * send information to syslog local1
 *  
 * the math may be a bit sloppy 
 * ... what if entropy is over 32768?  Maybe cap it at maximum 
 * ... what if one is using alot of iterations
 * just keeping it simple for now.
 *
 ***********/

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/random.h>
#include <fcntl.h>
#include <syslog.h>

#define TRUE 1
#define FALSE 0

// eventually, these will be default values and set by options
// measure once a second, log results about once a minute
#define ITERATIONS 60 // iterations to between outputting data
#define ITERATIONS 6  // for testing
 // iterations to between outputting data
#define SLEEP 1 // sleep interval between iterations in seconds
// these should be options
//#define NO_STDERR     1
#define NO_STDERR     0 // for testing
#define NO_ZABBIX_OUT 0
#define NO_SYSLOG     0
#define READ_WAKEUP_THRESHOLD    64
#define WRITE_WAKEUP_THRESHOLD  128
#define WRITE_WAKEUP_THRESHOLD  4000 // for testing

int main ()
{
  int dev_random = open ("/dev/random", O_RDONLY, O_NONBLOCK);
  openlog ("entropy_monitor", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  int avail_ent;
  int result;
  int error_found = FALSE;
  int read_wakeup_threshold = 0;
  int write_wakeup_threshold = 0;
  if ( !NO_STDERR )
    fprintf (stderr, "Collecting available entropy from /dev/random for %d iterations every %d seconds\n",
	   ITERATIONS, SLEEP);
  // TODO log start
  // get read_wakeup_threshold
  read_wakeup_threshold = READ_WAKEUP_THRESHOLD;
  // get write_wakeup_threshold
  write_wakeup_threshold = WRITE_WAKEUP_THRESHOLD;
  // Log threshold values
  // could send threshold values to zabbix to set the triggers.

  // loop ... until error, maybe check for sigint or something else later
  while (!error_found)
    {
      // loop 60 iterations (about 60 s)
      long avail_entropy_sum = 0;
      int avail_entropy_avg = 0;
      int avail_entropy_high = 0;
      int avail_entropy_low = 0;
      int i;
      int below_read_wakeup_threshold_count = 0;
      int below_write_wakeup_threshold_count = 0;
      int consecutive_below_read_wakeup_threshold_count = 0;
      int consecutive_below_write_wakeup_threshold_count = 0;
      int max_consecutive_below_read_wakeup_threshold_count = 0;
      int max_consecutive_below_write_wakeup_threshold_count = 0;
      int avail_ent_previous = -1;
      for ( i = 1; i <= ITERATIONS; i++)
	{
	  // test dev random that dev random is random and get avail entropy
	  result = ioctl (dev_random, RNDGETENTCNT, &avail_ent);
	  if (result)
	    {
	      fprintf (stderr, "/dev/random result = %d\n", result);
	      fprintf (stderr, "/dev/random is not a random device\n");
	      // the available entropy is not valid set to -1 
	      avail_ent = -1;
	      error_found = TRUE;
	      break;
	    }
	  // on first iteration set high and low available entropy to the current value
	  if ( i == 1 )
	    {
	      avail_entropy_high = avail_ent;
	      avail_entropy_low = avail_ent;
	    }
	  // check for and set highs and lows
	  if ( avail_ent > avail_entropy_high )
	    avail_entropy_high = avail_ent;
	  if ( avail_ent < avail_entropy_low )
	    avail_entropy_low = avail_ent;
          // collect the sum to calculate the average later on
	  avail_entropy_sum = avail_entropy_sum + avail_ent;
                   if ( !NO_STDERR ) 
 	  	       fprintf(stderr, "dev random avail_ent = %d  iters %d  sum %ld\n",
		       avail_ent, i, avail_entropy_sum);
	  // check if entropy is lower than the thresholds and increment counters
          // and if it is not the first iteration check for consecutive low entropy and increment counters
          if ( avail_ent < below_read_wakeup_threshold_count )
	    {
	      below_read_wakeup_threshold_count++;
	    }
          if ( avail_ent < below_write_wakeup_threshold_count )
	    {
	      below_write_wakeup_threshold_count++;
	    }

	  // if we are below a threshold check which processes and users are using /dev/random
	  // 

          // calculate consecutive low entropy streaks
          // and check if it is the biggest
          if ( avail_ent_previous != -1 ) // don't do this if it is the first iteration
            {
              // check read wakeup threshold 
	      if ( ( avail_ent_previous < read_wakeup_threshold   ) && ( avail_ent < read_wakeup_threshold  ))
                {
		  // in a streak ... if not zero add one before printing, as a streak means at least 2 consecutive measurements
		  consecutive_below_read_wakeup_threshold_count++ ;
                  if (consecutive_below_read_wakeup_threshold_count > max_consecutive_below_read_wakeup_threshold_count)
		    {
                      // this is the longest streak
		      max_consecutive_below_read_wakeup_threshold_count = consecutive_below_read_wakeup_threshold_count;
                    }                  
                }
              else 
                {
                  // we are no longer in a streak zero the counter
                  consecutive_below_read_wakeup_threshold_count = 0; 
                }
              // check write wakeup threshold 
	      if ( ( avail_ent_previous < write_wakeup_threshold   ) && ( avail_ent < write_wakeup_threshold  ))
                {
		  // in a streak ... if not zero add one before printing, as a streak means at least 2 consecutive measurements
		  consecutive_below_write_wakeup_threshold_count++ ;
                  if (consecutive_below_write_wakeup_threshold_count > max_consecutive_below_write_wakeup_threshold_count)
		    {
                      // this is the longest streak
		      max_consecutive_below_write_wakeup_threshold_count = consecutive_below_write_wakeup_threshold_count;
                    }                  
                }
              else 
                {
                  // we are no longer in a streak zero the counter
                  consecutive_below_write_wakeup_threshold_count = 0; 
                }
            } 
          // remember entropy previous for the next time around
          avail_ent_previous = avail_ent;   
	  sleep ( SLEEP );
	} // end iteration loop
      // account for first measurement ( if there is a streak)
      if ( max_consecutive_below_read_wakeup_threshold_count > 0 )
        max_consecutive_below_read_wakeup_threshold_count++;
      if ( max_consecutive_below_write_wakeup_threshold_count > 0 )
        max_consecutive_below_write_wakeup_threshold_count++;
      avail_entropy_avg = (long) avail_entropy_sum / (long) ITERATIONS ;
      if ( !NO_STDERR )
	{
      fprintf (stderr, "/dev/random last %d iterations" , ITERATIONS );
      fprintf (stderr, "high: %d ", avail_entropy_high);
      fprintf (stderr, "low : %d ", avail_entropy_low);
      // lets round it and keep it an int
      fprintf (stderr, "mean: %d\n", avail_entropy_avg );
        }
      // determine what log messages to send
      // notify times below read or write wakeup threshold

      if ( max_consecutive_below_read_wakeup_threshold_count > 0 )

        {
	syslog ( LOG_WARNING, "entropy below read wakeup value %d for a maximum of %d consecutive measurements",
                 read_wakeup_threshold, max_consecutive_below_read_wakeup_threshold_count );
              if ( !NO_STDERR )
           fprintf (stderr,  "entropy below read wakeup value %d for a maximum of %d consecutive measurements",
		 write_wakeup_threshold, max_consecutive_below_read_wakeup_threshold_count);
	}
      
      //send this to zabbix too 
      if ( max_consecutive_below_write_wakeup_threshold_count > 0 )
	{
	syslog ( LOG_WARNING, "entropy below write wakeup value %d for a maximum of %d consecutive measurements",
		 write_wakeup_threshold, max_consecutive_below_write_wakeup_threshold_count);
        if ( !NO_STDERR )
           fprintf (stderr,  "entropy below write wakeup value %d for a maximum of %d consecutive measurements",
		 write_wakeup_threshold, max_consecutive_below_write_wakeup_threshold_count);
	}

      // info ... avg, log, high
      syslog ( LOG_INFO , "avail.low %d high %d mean %d for last period", avail_entropy_low, avail_entropy_high, avail_entropy_avg);
      // data to stdout for piping to be piped to zabbix_sender 
      fprintf (stdout, "- kernel.random.entropy_avail.mean %d\n", avail_entropy_avg );
      fprintf (stdout, "- kernel.random.entropy_avail.high %d\n", avail_entropy_high );
      fprintf (stdout, "- kernel.random.entropy_avail.low %d\n", avail_entropy_low );
      // send

      // console output mode?

      // this is probably not necessary. 
      fflush_unlocked(stdout);
    }
  // print this even if no NO_STDERR set
  fprintf (stderr, "exiting due to error\n");
  // send a log message too!
  syslog ( LOG_ERR , "exiting due to error");
  close (dev_random);
  closelog ();
  return 1;
}
