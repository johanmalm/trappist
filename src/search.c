// SPDX-License-Identifier: GPL-2.0-only
#include <sway-client-helpers/log.h>
#include <sway-client-helpers/unicode.h>

struct buffer {
	size_t len;
	char data[64];
};

static struct buffer buffer = { 0 };

/**
 * utf8_nr_bytes_to_prev_character - find distance to previous utf8 character
 * @buf: C string buffer containing utf8 data
 * @pos: position to start working backwards from
 *
 * This function does not validate that the buffer contains correct utf8 data;
 * it merely looks for the first byte that correctly denotes the beginning of a
 * utf8 character.
 */
static int
utf8_nr_bytes_to_prev_character(const char *buf, const char *pos)
{
	int len = 0;
	while (pos > buf) {
		--pos; ++len;
		if ((*pos & 0xc0) != 0x80) {
			return len;
		}
	}
	LOG(LOG_ERROR, "data is not valid utf8");
	return 0;
}

void
search_remove_last_uft8_character(void)
{
	if (!buffer.len) {
		return;
	}
	char *pos = &buffer.data[buffer.len];
	buffer.len -= utf8_nr_bytes_to_prev_character(buffer.data, pos);
	buffer.data[buffer.len] = 0;
}

void
search_add_utf8_character(uint32_t codepoint)
{
	size_t len = utf8_chsize(codepoint);
	if (buffer.len + len + 1 >= sizeof(buffer.data)) {
		LOG(LOG_ERROR, "sting truncated");
		return;
	}
	utf8_encode(&buffer.data[buffer.len], codepoint);
	buffer.data[buffer.len + len] = 0;
	buffer.len += len;
}

char *
search_str(void)
{
	return buffer.data;
}
