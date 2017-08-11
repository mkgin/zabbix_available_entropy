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
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/random.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>


#define TRUE 1
#define FALSE 0

// eventually, these will be default values and set by options
// measure once a second, log results about once a minute
#define ITERATIONS_DEFAULT 60 // iterations to between outputting data
#define ITERATIONS_TESTING 10  // for testing
// iterations to between outputting data
#define SLEEP_DEFAULT 1 // sleep interval between iterations in seconds
// should get these from proc filesytem
#define READ_WAKEUP_THRESHOLD    64
#define WRITE_WAKEUP_THRESHOLD  128

void show_help()
{
  fprintf ( stderr,
	  "Usage: entropy_monitor -dhltvz\n"
	  "Collect and output information about available entropy in /dev/random\n\n"
	  "  -d     debugging output to stderr\n"
	  "  -h     show this help\n"
	  "  -l     log to syslog\n"
	  "  -v     verbose output to stderr\n"
	  "  -z     output for suitable for zabbix_sender to stdout\n"
	  );
}
int main (int argc, char **argv)
{
  // constants
  const char dev_random_file[] = "/dev/random";
  // flags  to set with getopt.
  int debug_flag = FALSE;
  int zabbix_flag = FALSE;
  int syslog_flag = FALSE;
  int verbose_flag = FALSE;
  int test_flag = FALSE; // test values

  // other variables with a defaults
  int samples_of_avail_ent = ITERATIONS_DEFAULT;
  int error_found = FALSE;
  int read_wakeup_threshold = 0;
  int write_wakeup_threshold = 0;
  // 
  int avail_ent;
  int result;
  int dev_random_filehandle;
  struct stat dev_random_stat;
  

  // getopt
  while (( result = getopt (argc, argv, "dhltvz")) != -1)
    {
      switch (result)
	{
	case 'd':
	  debug_flag = TRUE;
	  break;
	case 'h':
	    show_help();
	    return 0;
	    break;
	case 'l':
	  syslog_flag = TRUE;
	  break;
	case 't':  //testing values 
	  test_flag = TRUE;
          read_wakeup_threshold = 3400;
          write_wakeup_threshold = 4000;
	  samples_of_avail_ent = ITERATIONS_TESTING;
	  break;
	case 'v':
	   verbose_flag = TRUE;
	   break;
	case 'z':
	   zabbix_flag = TRUE;
	   break;
	default:
          show_help();
	  abort ();
	}
      
    }
  if ( debug_flag )
    {
      fprintf (stderr, "DEBUG: "
	       "debug_flag: %d syslog_flag: %d test_flag: %d verbose_flag: %d zabbix_flag: %d\n",
	       debug_flag, syslog_flag, test_flag, verbose_flag, zabbix_flag);
    }
  // open files
  dev_random_filehandle = open ( dev_random_file , O_RDONLY, O_NONBLOCK);
  // get inode number
  stat( dev_random_file, &dev_random_stat );
  if ( debug_flag )
    fprintf(stderr, "DEBUG: %s I-node number: %ld\n",dev_random_file , (long) dev_random_stat.st_ino);
  if ( syslog_flag )
    {
      openlog ("entropy_monitor", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    }

  if ( verbose_flag )
    fprintf (stderr, "Collecting available entropy from %s for %d iterations every %d seconds\n", dev_random_file,
	   samples_of_avail_ent, SLEEP_DEFAULT);
  // TODO log start
  if (!test_flag)
    {
  // get read_wakeup_threshold
  read_wakeup_threshold = READ_WAKEUP_THRESHOLD;
  // get write_wakeup_threshold
  write_wakeup_threshold = WRITE_WAKEUP_THRESHOLD;
    }
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
      for ( i = 1; i <= samples_of_avail_ent; i++)
	{
	  // test dev random that dev random is random and get avail entropy
	  result = ioctl (dev_random_filehandle, RNDGETENTCNT, &avail_ent);
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
                   if ( debug_flag || verbose_flag ) 
 	  	       fprintf(stderr, "/dev/random avail_ent = %d  iters %d  sum %ld\n",
		       avail_ent, i, avail_entropy_sum);
	  // check if entropy is lower than the thresholds and increment counters
          // and if it is not the first iteration check for consecutive low entropy and increment counters
          if ( avail_ent < below_write_wakeup_threshold_count )
	    {
	      below_write_wakeup_threshold_count++;
	      // read is always lower than write
              if ( avail_ent < below_read_wakeup_threshold_count )
		{
		  below_read_wakeup_threshold_count++;
		}
	      // if we are below a threshold check which processes and users are using /dev/random

	      // log them (maybe log every sample as in theory these shouldn't last long)
              fprintf (stdout, "- kernel.random.entropy_avail.log \"%s\"\n",
			"low avalable entropy log text here" );
	    }
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
	  sleep ( SLEEP_DEFAULT );
	} // end iteration loop
      // account for first measurement ( if there is a streak)
      if ( max_consecutive_below_read_wakeup_threshold_count > 0 )
        max_consecutive_below_read_wakeup_threshold_count++;
      if ( max_consecutive_below_write_wakeup_threshold_count > 0 )
        max_consecutive_below_write_wakeup_threshold_count++;
      avail_entropy_avg = (long) avail_entropy_sum / (long) samples_of_avail_ent ;
      // notify times below read or write wakeup threshold
      // send to zabbix always if enabled...even if value is 0, can alert of the zast value is > 0
      if ( zabbix_flag )
	{
           fprintf (stdout, "- kernel.random.entropy_avail.maxsamplesbelow_read_wu_threshhold %d\n",
			 max_consecutive_below_read_wakeup_threshold_count );
           fprintf (stdout, "- kernel.random.entropy_avail.maxsamplesbelow_write_wu_threshhold %d\n",
			 max_consecutive_below_write_wakeup_threshold_count );
	      }
	    // other only send if below threshold
      if ( max_consecutive_below_read_wakeup_threshold_count > 0 )
        {
	    if ( syslog_flag )
	      {
		syslog ( LOG_WARNING, "entropy below read wakeup value %d for a maximum of %d consecutive measurements",
			 read_wakeup_threshold, max_consecutive_below_read_wakeup_threshold_count );
	      }
           if ( debug_flag || verbose_flag )
	     {
	       fprintf (stderr,  "entropy below read wakeup value %d for a maximum of %d consecutive measurements\n",
			write_wakeup_threshold, max_consecutive_below_read_wakeup_threshold_count);
	     }
	}
      if ( max_consecutive_below_write_wakeup_threshold_count > 0 )
        {
	    if ( syslog_flag )
	      {
		syslog ( LOG_WARNING, "entropy below write wakeup value %d for a maximum of %d consecutive measurements",
			 write_wakeup_threshold, max_consecutive_below_write_wakeup_threshold_count );
	      }
           if ( debug_flag || verbose_flag )
	     {
	       fprintf (stderr,  "entropy below write wakeup value %d for a maximum of %d consecutive measurements\n",
			write_wakeup_threshold, max_consecutive_below_write_wakeup_threshold_count);
	     }
	}
      // info ... avg, log, high
      if ( syslog_flag )
	{
	  syslog ( LOG_INFO , "avail.low %d high %d mean %d for last period of (%d) iterations", avail_entropy_low, avail_entropy_high, avail_entropy_avg, samples_of_avail_ent );
	}
      if ( debug_flag || verbose_flag )
	{
	  fprintf ( stderr, "avail.low %d high %d mean %d for last period of (%d) iterations\n", avail_entropy_low, avail_entropy_high, avail_entropy_avg, samples_of_avail_ent );
	}
      if ( zabbix_flag )
	{
          // data to stdout for piping to be piped to zabbix_sender 
          fprintf (stdout, "- kernel.random.entropy_avail.mean %d\n", avail_entropy_avg );
          fprintf (stdout, "- kernel.random.entropy_avail.high %d\n", avail_entropy_high );
          fprintf (stdout, "- kernel.random.entropy_avail.low %d\n", avail_entropy_low );
	}
      // console output mode?

      // this is probably not necessary. 
      fflush_unlocked(stdout);
    }
  // print this even if no NO_STDERR set
  fprintf (stderr, "exiting due to error\n");
  // send a log message too!
  if ( syslog_flag )
    {
      syslog ( LOG_ERR , "exiting due to error");
      closelog ();
    }  
  close (dev_random_filehandle);
  return 1;
}
