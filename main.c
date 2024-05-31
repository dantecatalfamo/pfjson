#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/pfvar.h>

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pfjson.h"

int
main(int argc, char **argv)
{
    struct pf_status s;
    struct pfioc_synflwats w;
    int dev;

    dev = open("/dev/pf", O_RDWR);
    if (dev == -1)
        err(1, "open(\"/dev/pf\") failed");

    memset(&s, 0, sizeof(struct pf_status));

    if (ioctl(dev, DIOCGETSTATUS, &s) == -1)
        err(1, "DIOCGETSTATUS");

    if(ioctl(dev, DIOCGETSYNFLWATS, &w) == -1)
        err(1, "DIOCGETSYNFLWATS");

    print_status(&s, &w);

    return 0;
}
