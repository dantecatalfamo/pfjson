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
