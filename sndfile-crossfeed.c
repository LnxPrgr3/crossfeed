#include <stdlib.h>
#include <sndfile.h>
#include "crossfeed.h"

int main(int argc, char *argv[]) {
	char *in_filename = argv[1];
	char *out_filename = argv[2];
	SF_INFO info = {0};
	SNDFILE *in_file, *out_file;
	crossfeed_t filter;
	float buf[2048], obuf[2048], tmp;
	int read, i;
	in_file = sf_open(in_filename, SFM_READ, &info);
	info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
	out_file = sf_open(out_filename, SFM_WRITE, &info);
	crossfeed_init(&filter);
	while((read = sf_read_float(in_file, buf, 2048)) > 0) {
		crossfeed_filter(&filter, buf, obuf, read/2);
		for(i=0;i<read;++i) {
			obuf[i] = obuf[i] > 1 ? 1 : (obuf[i] < -1 ? -1 : obuf[i]);
		}
		sf_write_float(out_file, obuf, read);
	}
	sf_close(out_file);
	sf_close(in_file);
	return EXIT_SUCCESS;
}
