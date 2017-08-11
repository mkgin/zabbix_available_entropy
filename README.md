# entropy_monitor

As configured, the available entropy monitor collects data about the 
available entropy about every second and calculates mean, high and low
available entropy over 60 iterations. When the entropy is below the read or write
threshhold, the monitor will count the maximum consectutive samples below the threshold
for that period.

There are some options to control the output, the monitor was initially made to be used
with zabbix_sender but it can be used without zabbix to log to the console, a file or syslog.

![Chart Example](https://github.com/mkgin/zabbix_available_entropy/raw/master/ent_avail_screenshot.png)

## installation
### building from source

compile the entropy monitor with gcc

``gcc entropy_monitor.c -o entropy_monitor``

or try the Makefile (which just builds and does not install yet.

``autoreconf --install
./configure
Makefile``

Then copy ``entropy_monitor`` to a location where it will be in the $PATH.

### zabbix setup

The keys for the items below need to be configured on the Zabbix server. They are available in the template.
The items in the template monitor various parameters having to do with /dev/random and some alerts
on low available entropy.

Items: 
* kernel.random.entropy_avail.mean
* kernel.random.entropy_avail.high
* kernel.random.entropy_avail.low
* kernel.random.entropy_avail.maxsamplesbelow_read_wu_threshhold
* kernel.random.entropy_avail.maxsamplesbelow_write_wu_threshhold
* kernel.random.entropy_avail.log   (soon)

The server need to be configured to be able to recieve zabbix triggers.


### Monitoring with zabbix

Example: send stdout to zabbix sender and error information to a log in /tmp

``stdbuf -oL ./entropy_monitor -z 2>&1 >/tmp/entropy_monitor_log | zabbix_sender -r -i - PLUS_YOUR_DEPLOYMENT_SPECIFIC_SETTINGS``

... then Ctrl-Z  and  ``bg ; disown`` so the task continure running in the background.

If you use PSK the zabbix part would similar to this ``zabbix_sender -r --host AGENT_HOSTNAME -z ZABBIX_SERVER --tls-psk-file zabbix_agentd.psk --tls-psk-identity PSK123 --tls-connect psk -i -``. The ``-v`` or ``-vv`` can help with troubleshooting connecting to the zabbix server.

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

### Further reading about entropy and /dev/random

* [Cloudflare Blog: Ensuring Randomness with Linux's Random Number Generator](https://blog.cloudflare.com/ensuring-randomness-with-linuxs-random-number-generator/)
* [LWN: LCE: Don't play dice with random numbers](https://lwn.net/Articles/525459/)
* [LWN: The search for truly random numbers in the kernel](https://lwn.net/Articles/567055/)
