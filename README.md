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

The following keys need to be configured on the server. They are available in the template.

kernel.random.entropy_avail.mean
kernel.random.entropy_avail.high
kernel.random.entropy_avail.low



## running 

For example, send stdout to zabbix sender and error information to a log in /tmp

``./entropy_monitor 2>&1 >/tmp/entropy_monitor_log | zabbix_sender``
