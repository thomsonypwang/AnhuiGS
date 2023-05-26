#include "core_log.h"

static char *core_log_level_name[] = {"[TRA]", "[DBG]", "[INF]", "[WAR]", "[ERR]"};
core_log_level_t g_log_level = CORE_LOG_TRACE;
extern aiot_state_logcb_t g_logcb_handler;

void core_set_log_level(core_log_level_t level) {
    if (level >= CORE_LOG_MAX) {
        return;
    }
    g_log_level = level;
}

void writeLog(const char *fun, const int line, int level, const char *format, ...)
{
    char buffer[CORE_LOG_MAXLEN + 3] = {0};
    uint32_t len = 0;
    if (level < g_log_level) {
		return;
	}

    if (g_logcb_handler == NULL) {
        return;
    }
    
    va_list plist;
    va_start(plist, format);
    len = snprintf(buffer, CORE_LOG_MAXLEN, "%s[%s][%d]:", core_log_level_name[level], fun, line);
    len = vsnprintf(buffer + len, CORE_LOG_MAXLEN - len, format, plist);
    buffer[len] = '\r';
    buffer[len+1] = '\n';
    va_end(plist);
    g_logcb_handler(buffer);

    return;
}

void _log_dump(const char *name, const char *buffer, uint32_t len) {

    uint32_t idx = 0, line_idx = 0, ch_idx = 0, code_len = 0;
    /* [LK-XXXX] + 1 + 1 + 16*3 + 1 + 1 + 1 + 16 + 2*/
    char hexdump[25 + 78] = {0};

    if (g_logcb_handler == NULL || len == 0) {
        return;
    }
    memset(hexdump, 0, sizeof(hexdump));
    memcpy(hexdump, name, strlen(name));
    code_len = strlen(hexdump);
    hexdump[code_len] = ' ';
    code_len = strlen(hexdump);

    for (idx = 0; idx < len;) {
        memset(hexdump + code_len, ' ', 71);
        ch_idx = 2;
        hexdump[code_len + 0] = '>';
        hexdump[code_len + 51] = '|';
        hexdump[code_len + 52] = ' ';
        for (line_idx = idx; ((line_idx - idx) < 16) && (line_idx < len); line_idx++) {
            if ((line_idx - idx) == 8) {
                ch_idx++;
            }
            core_hex2str((uint8_t *)&buffer[line_idx], 1, &hexdump[code_len + ch_idx], 0);
            hexdump[code_len + ch_idx + 2] = ' ';
            if (buffer[line_idx] >= 0x20 && buffer[line_idx] <= 0x7E) {
                hexdump[code_len + 53 + (line_idx - idx)] = buffer[line_idx];
            } else {
                hexdump[code_len + 53 + (line_idx - idx)] = '.';
            }
            ch_idx += 3;
        }
        hexdump[code_len + 69] = '\r';
        hexdump[code_len + 70] = '\n';
        idx += (line_idx - idx);
        g_logcb_handler(hexdump);
    }
    g_logcb_handler("\r\n");
}