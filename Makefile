all: blob2c | $(CLEAN)

CFLAGS ?= -Wall -Os

blob2c: blob2c.c
	$(CC) $(CFLAGS) $(<) -o $(@)

CLEAN=$(filter clean,$(makecmdgoals))
.phony: clean
clean:
	rm -f blob2c
