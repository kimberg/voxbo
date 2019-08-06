
void read_queue(string queuedir, map<int, VBSequence> &seqlist);
int should_refract(VBJobSpec &js);

void remove_seqwait(string queuedir, int snum);
void cleanupqueue(map<int, VBSequence> &seqlist, string qdir);
int has_unsatisfied_dependencies(JI js, char donetable[], int maxjnum);
