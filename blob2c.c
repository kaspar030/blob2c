/*
 * Copyright (C) 2019 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.

 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 *
 *
 * blob2c: a simple tool to create C arrays from binary blobs
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#define BASENAME_PREFIX "_blob_"
#define LINE_LEN 8

/* '0xYY, ' */
#define PER_BYTE 6

/* return size of file or -1 on error */
static off_t fsize(const char *filename)
{
    struct stat st;

    if (stat(filename, &st) == 0) {
        return st.st_size;
    }

    return -1;
}

/* read file into buf. return <0 on error. */
int from_file(const char *filename, void *buf, size_t len)
{
    int fd = open(filename, O_RDONLY);

    if (fd > 0) {
        ssize_t res = read(fd, buf, len);
        close(fd);
        return res == (ssize_t)len;
    }
    else {
        return fd;
    }
}

/* print msg to stderr, then exit with error code 1 */
void die(const char *msg)
{
    fprintf(stderr, msg);
    exit(1);
}

void strchrreplace(char *str, int find, int replace)
{
    char *current_pos = strchr(str, find);

    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos + 1, find);
    }
}

static const char _hex_chars[16] = "0123456789ABCDEF";

int main(int argc, char *argv[])
{
    char *filename;
    char *basename = NULL;
    char *type = "const char";
    char *sizetype = "size_t";
    char *prefix = "#include <stddef.h>\n";

    int opt;

    while ((opt = getopt(argc, argv, "s:t:p:b:")) != -1) {
        switch (opt) {
            case 's':
                sizetype = optarg;
                break;
            case 't':
                type = optarg;
                break;
            case 'p':
                prefix = optarg;
                break;
            case 'b':
                basename = optarg;
                break;
            default: /* '?' */
                fprintf(stderr,
                        "Usage: %s [-t type] [-s sizetype] [-p prefix] [-b basename] filename\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        die("missing filename argument\n");
    }
    filename = argv[optind];

    off_t size = fsize(filename);
    if (size < 0) {
        die("error stat'ing, does the file exist?\n");
    }

    char *buf = malloc(size);
    if (!buf) {
        die("cannot allocate memory\n");
    }

    if (from_file(filename, buf, size) < 0) {
        die("cannot read file\n");
    }

    size_t left = size;
    size_t pos = 0;

    /* LINE_LEN * '0xYY, ' + '\0' */
    char line_buf[(LINE_LEN * PER_BYTE) + 1];

    /* if no basename is given, create one from filename.
     * in that case, replace "." and "/" from path with "_".
     */
    if (basename == NULL) {
        basename = malloc(strlen(filename) + strlen(BASENAME_PREFIX) + 1);
        if (!basename) {
            die("cannot allocate memory\n");
        }
        basename[0] = '\0';

        strcat(basename, BASENAME_PREFIX);
        strcat(basename, filename);

        strchrreplace(basename, '/', '_');
        strchrreplace(basename, '.', '_');
    }

    fprintf(stdout, "%s\n", prefix);

    fprintf(stdout, "%s %s_data[] = {\n", type, basename);

    while (left) {
        /* write max(left, LINE_LEN) bytes */
        unsigned line_len = (left > LINE_LEN) ? LINE_LEN : left;

        /* write "0xYY, " snippets into line_buf */
        for (unsigned i = 0; i < line_len; i++) {
            unsigned char byte = buf[pos++];
            char *byte_pos = &line_buf[i * PER_BYTE];

            sprintf(byte_pos, "0x%c%c, ", _hex_chars[byte >> 4],
                    _hex_chars[byte & 0x0F]);
        }

        /* write to file */
        fprintf(stdout, "    %.*s\n", (line_len * PER_BYTE), line_buf);
        left -= line_len;
    }

    fprintf(stdout, "};\n\n");

    fprintf(stdout, "const %s %s_size = %lu;\n", sizetype, basename, size);

    return 0;
}
