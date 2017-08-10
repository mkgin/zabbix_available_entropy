# zabbix_available_entropy

As configured, the available entropy monitor collects data about the 
available entropy about every second and calculates mean, high and low
available entropy over 60 iterations. It prints the results to stdout 
in a format suitable to be piped to zabbix sender. Error and debug information
sent to stderr.

The template will also be set up to monitor various parameters and settings 
having to do with /dev/random and it's configuration via the proc filesystem
and/or sysctl and provide information about the configuration and perhaps 
warn about setting that are not optimal


## installation

compile the entropy monitor and add to path

``gcc entropy_monitor.c -o entropy_monitor``

or try the Makefile.
autoreconf --install
./configure
Makefile

The following keys need to be configured on the server. They are available in the template.

kernel.random.entropy_avail.mean
kernel.random.entropy_avail.high
kernel.random.entropy_avail.low
kernel.random.entropy_avail.maxsamplesbelow_read_wu_threshhold
kernel.random.entropy_avail.maxsamplesbelow_write_wu_threshhold

## running 

For example, send stdout to zabbix sender and error information to a log in /tmp

``stdbuf -oL ./entropy_monitor -z 2>&1 >/tmp/entropy_monitor_log | zabbix_sender -v -r -i - PLUS_YOUR_DEPLOYMENT_SPECIFIC_SETTINGS``

* NOTE: ``stdbuf -oL`` makes sure the buffer is sent through the pipe on each newline
  otherwise, data may stay in the pipe and not get send to zabbix_sender until the 
  pipe is full (and the information out of date)

* TODO:
  * could eventually look into starting it as a daemon. ``bg`` and ``disown`` work well enough
    for now as this is mainly used for experimental monitoring on servers that I am troubleshooting
  * submit messages to syslog... (warn on low entropy, info for measurements)
  would be easier to correlate with problems when information is available in the logfile.
  * when entropy is low could get a list running processes
    *  probably enough information in fuser.c from psmisc utilities  
  * could have a console or logging to a file mode... 
  * could also keep track of when the low entropy dips significantly
