/* History file round-trip.  read_history()/write_history() must preserve
 * every entry byte-for-byte:
 *   - lines of any length (a fixed read buffer used to split long lines and
 *     chop a byte mid-glyph for multibyte input), and
 *   - a file filled to capacity -- issue #78 dropped the most recent entry
 *     because read_history() read one fewer than write_history() wrote.
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "editline.h"

#define IN  "history-in.tmp"
#define OUT "history-out.tmp"

static int files_equal(const char *a, const char *b)
{
	FILE *fa = fopen(a, "rb");
	FILE *fb = fopen(b, "rb");
	int ca, cb, eq = 1;

	if (!fa || !fb) {
		if (fa) fclose(fa);
		if (fb) fclose(fb);
		return 0;
	}
	do {
		ca = getc(fa);
		cb = getc(fb);
		if (ca != cb) {
			eq = 0;
			break;
		}
	} while (ca != EOF);
	fclose(fa);
	fclose(fb);

	return eq;
}

int main(void)
{
	int i, n, fail = 0;
	FILE *fp;

	/* Fill the file to capacity -- el_hist_size + 1 entries, the most
	 * write_history() ever emits -- with distinct lines, one of them a
	 * 300-byte multibyte line (150x "é"). */
	n = el_hist_size + 1;
	fp = fopen(IN, "w");
	if (!fp) {
		perror(IN);
		return 77;		/* SKIP: cannot create scratch file */
	}
	for (i = 0; i < n; i++) {
		if (i == n / 2) {
			int k;

			for (k = 0; k < 150; k++)
				fputs("\303\251", fp);
			fputc('\n', fp);
		} else {
			fprintf(fp, "history entry %d\n", i);
		}
	}
	fclose(fp);

	read_history(IN);
	write_history(OUT);

	if (!files_equal(IN, OUT)) {
		fprintf(stderr, "FAIL history-roundtrip  %d-entry file not preserved (issue #78 / long line)\n", n);
		fail++;
	} else {
		printf("PASS history-roundtrip  [%d entries, byte-for-byte]\n", n);
	}

	unlink(IN);
	unlink(OUT);
	printf("\nhistory: 1 tests, %d failures\n", fail);
	return fail ? 1 : 0;
}
