#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/pfvar.h>

#define SYSLOG_NAMES
#include <syslog.h>

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FCNT_NAMES { \
	"searches", \
	"inserts", \
	"removals", \
	NULL \
}

void print_since(time_t since);
void print_status(struct pf_status *s, struct pfioc_synflwats *w);
void print_checksum(u_int8_t *chksum);
void print_loginterface(struct pf_status *s);
const char *loglevel_to_string(int level);

const char  *pf_reasons[PFRES_MAX+1] = PFRES_NAMES;
const char  *pf_lcounters[LCNT_MAX+1] = LCNT_NAMES;
const char  *pf_fcounters[FCNT_MAX+1] = FCNT_NAMES;
const char  *pf_scounters[FCNT_MAX+1] = FCNT_NAMES;

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

void
print_status(struct pf_status *s, struct pfioc_synflwats *w)
{
    int i;
    unsigned int sec, min, hrs;
    time_t day, runtime;
    double counter_rate;
    struct timespec uptime;

    day = runtime = 0;

    if (!clock_gettime(CLOCK_BOOTTIME, &uptime))
		runtime = day = uptime.tv_sec - s->since;

    sec = day % 60;
    day /= 60;
    min = day % 60;
    day /= 60;
    hrs = day % 24;
    day /= 24;

    printf("{\n");
    printf("  \"running\": %s,\n", s->running ? "true" : "false");
    printf("  \"debug\": \"%s\",\n", loglevel_to_string(s->debug));
    printf("  \"hostid\": \"0x%08x\",\n", ntohl(s->hostid));
    printf("  \"checksum\": ");
    print_checksum(s->pf_chksum);
    printf(",\n");
    printf("  \"uptime\": ");
    printf("{\"days\": %lld, \"hours\": %d, \"minutes\": %d, \"seconds\": %d }", (long long)day, hrs, min, sec);
    printf(",\n");
    if (s->ifname[0] != 0) {
        print_loginterface(s);
    }
    printf("  \"states\": {\n");
    printf("    \"current entries\": {\n");
    printf("      \"total\": %u,\n", s->states);
    printf("      \"rate\": null\n");
    printf("    },\n");
    printf("    \"half-open tcp\": {\n");
    printf("      \"total\": %u,\n", s->states_halfopen);
    printf("      \"rate\": null\n");
    printf("    },\n");
    for (i = 0; i < FCNT_MAX; i++) {
        if (runtime > 0)
            counter_rate = (double)s->fcounters[i] / (double)runtime;
        else
            counter_rate = -1;

        printf("    \"%s\": {\n", pf_fcounters[i]);
        printf("      \"total\": %llu,\n", (unsigned long long)s->fcounters[i]);
        if (counter_rate < 0)
            printf("      \"rate\": null\n");
        else
            printf("      \"rate\": %.1f\n", counter_rate);
        printf("    }");
        if (i < FCNT_MAX-1)
            printf(",");
        printf("\n");
    }
    printf("  },\n");
    printf("  \"source tracking\": {\n");
    printf("    \"current entries\": {\n");
    printf("      \"total\": %u,\n", s->src_nodes);
    printf("      \"rate\": null\n");
    printf("    },\n");
    for (i = 0; i < SCNT_MAX; i++) {
        if (runtime > 0)
            counter_rate = (double)s->scounters[i] / (double)runtime;
        else
            counter_rate = -1;
        printf("    \"%s\": {\n", pf_scounters[i]);
        printf("      \"total\": %llu,\n", s->scounters[i]);
        if (counter_rate < 0)
            printf("      \"rate\": null\n");
        else
            printf("      \"rate\": %.1f\n", counter_rate);
        printf("    }");
        if (i < SCNT_MAX-1)
            printf(",");
        printf("\n");
    }
    printf("  },\n");
    printf("  \"counters\": {\n");
    for (i = 0; i < PFRES_MAX; i++) {
        if (runtime > 0)
            counter_rate = (double)s->counters[i] / (double)runtime;
        else
            counter_rate = -1;
        printf("    \"%s\": {\n", pf_reasons[i]);
        printf("      \"total\": %llu,\n", s->counters[i]);
        if (counter_rate < 0)
            printf("      \"rate\": null,");
        else
            printf("      \"rate\": %.1f\n", counter_rate);
        printf("    }");
        if (i < PFRES_MAX-1)
            printf(",");
        printf("\n");
    }
    printf("  },\n");
    printf("  \"limit counters\": {\n");
    for (i = 0; i < LCNT_MAX; i++) {
        if (runtime > 0)
            counter_rate = (double)s->lcounters[i] / (double)runtime;
        else
            counter_rate = -1;
        printf("    \"%s\": {\n", pf_lcounters[i]);
        printf("      \"total\": %llu,\n", s->lcounters[i]);
        if (counter_rate < 0)
            printf("      \"rate\": null,\n");
        else
            printf("      \"rate\": %.1f\n", counter_rate);
        printf("    }");
        if (i < LCNT_MAX-1)
            printf(",");
        printf("\n");
    }
    printf("  },\n");
    printf("  \"adaptive syncookies watermarks\": {\n");
    printf("    \"start\": %d,\n", w->hiwat);
    printf("    \"end\": %d\n", w->lowat);
    printf("  }\n");
    printf("}\n");
}

const char *
loglevel_to_string(int level)
{
    CODE *c;

    for (c = prioritynames; c->c_name; c++)
        if (c->c_val == level)
            return (c->c_name);

    return ("unknown");
}

void
print_checksum(u_int8_t *chksum)
{
    static const char hex[] = "0123456789abcdef";
    char buf[PF_MD5_DIGEST_LENGTH * 2 + 1];
    int i;

    for (i = 0; i < PF_MD5_DIGEST_LENGTH; i++) {
        buf[i + i] = hex[chksum[i] >> 4];
        buf[i + i + 1] = hex[chksum[i] & 0x0f];
    }
    buf[i + i] = '\0';

    printf("\"0x%s\"", buf);
}

void
print_loginterface(struct pf_status *s)
{
    printf("  \"loginterface\": {\n");
    printf("    \"name\": \"%s\",\n", s->ifname);
    printf("    \"bytes_in\": {\n");
    printf("      \"ipv4\": %llu,\n", (unsigned long long)s->bcounters[0][0]);
    printf("      \"ipv6\": %llu\n",  (unsigned long long)s->bcounters[1][0]);
    printf("    },\n");
    printf("    \"bytes_out\": {\n");
    printf("      \"ipv4\": %llu,\n", (unsigned long long)s->bcounters[0][1]);
    printf("      \"ipv6\": %llu\n",  (unsigned long long)s->bcounters[1][1]);
    printf("    },\n");
    printf("    \"packets_in\": {\n");
    printf("      \"passed\": {\n");
    printf("        \"ipv4\": %llu,\n", s->pcounters[0][0][PF_PASS]);
    printf("        \"ipv6\": %llu\n",  s->pcounters[1][0][PF_PASS]);
    printf("      },\n");
    printf("      \"blocked\": {\n");
    printf("        \"ipv4\": %llu,\n", s->pcounters[0][0][PF_DROP]);
    printf("        \"ipv6\": %llu\n",  s->pcounters[1][0][PF_DROP]);
    printf("      }\n");
    printf("    },\n");
    printf("    \"packets_out\": {\n");
    printf("      \"passed\": {\n");
    printf("        \"ipv4\": %llu,\n", s->pcounters[0][1][PF_PASS]);
    printf("        \"ipv6\": %llu\n",  s->pcounters[1][1][PF_PASS]);
    printf("      },\n");
    printf("      \"blocked\": {\n");
    printf("        \"ipv4\": %llu,\n", s->pcounters[0][1][PF_DROP]);
    printf("        \"ipv6\": %llu\n",  s->pcounters[1][1][PF_DROP]);
    printf("      }\n");
    printf("    }\n");
    printf("  },\n");
}
