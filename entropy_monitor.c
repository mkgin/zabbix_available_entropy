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
 * 
 * the math may be a bit sloppy 
 * ... what if entropy is over 32768
 * ... what if one is using alot of iterations
 * just keeping it simple for now.
 *
 ***********/

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/random.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0
#define ITERATIONS 60 // iterations to between outputting data
#define SLEEP 1 // sleep interval between iterations in seconds

int main ()
{
  int dev_random = open ("/dev/random", O_RDONLY, O_NONBLOCK);
  int avail_ent;
  int result;
  int error_found = FALSE;
  fprintf (stderr, "Collecting available entropy from /dev/random for %d iterations every %d seconds\n",
	   ITERATIONS, SLEEP);  
  // loop ... until error
  // maybe check for sigint or something else later
  while (!error_found)
    {
      // loop 60 terations (about 60 s)
      long avail_entropy_sum = 0;
      int avail_entropy_avg = 0;
      int avail_entropy_high = 0;
      int avail_entropy_low = 0;
      int i;
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
	  if (avail_ent > avail_entropy_high )
	    avail_entropy_high = avail_ent;
	  if (avail_ent < avail_entropy_low )
	    avail_entropy_low = avail_ent;
	  avail_entropy_sum = avail_entropy_sum + avail_ent;
	 	  fprintf (stderr, "/dev/random avail_ent = %d  iters %d  sum %d\n",
		   avail_ent, i, avail_entropy_sum);
	  sleep ( SLEEP );
	} // end iteration loop
      avail_entropy_avg = (long) avail_entropy_sum / (long) ITERATIONS ;
      fprintf (stderr, "/dev/random last %d iterations" , ITERATIONS );
      fprintf (stderr, "high: %d ", avail_entropy_high);
      fprintf (stderr, "low : %l ", avail_entropy_low);
      // lets round it and keep it an int
      fprintf (stderr, "mean: %d\n", avail_entropy_avg );
      // data to stdout for piping to be piped to zabbix_sender 
      fprintf (stdout, "- kernel.random.entropy_avail.mean %d\n", avail_entropy_avg );
      fprintf (stdout, "- kernel.random.entropy_avail.high %d\n", avail_entropy_high );
      fprintf (stdout, "- kernel.random.entropy_avail.low %d\n", avail_entropy_low );
      // end until error  loop
    }
  fprintf (stderr, "exiting due to error\n");
  close (dev_random);
  return 1;
}
