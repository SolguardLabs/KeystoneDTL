#include "kdtl_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

void kdtl_clear_error(kdtl_engine *engine) {
    if (engine != NULL) {
        engine->last_error[0] = '\0';
    }
}

kdtl_status kdtl_failv(kdtl_engine *engine, kdtl_status status, const char *fmt, va_list args) {
    if (engine != NULL) {
        if (fmt == NULL) {
            engine->last_error[0] = '\0';
        } else {
            (void)vsnprintf(engine->last_error, sizeof(engine->last_error), fmt, args);
        }
    }
    return status;
}

kdtl_status kdtl_fail(kdtl_engine *engine, kdtl_status status, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kdtl_status out = kdtl_failv(engine, status, fmt, args);
    va_end(args);
    return out;
}

const char *kdtl_status_name(kdtl_status status) {
    switch (status) {
    case KDTL_OK:
        return "ok";
    case KDTL_ERR_ARGUMENT:
        return "argument";
    case KDTL_ERR_NOT_FOUND:
        return "not_found";
    case KDTL_ERR_EXISTS:
        return "exists";
    case KDTL_ERR_CAPACITY:
        return "capacity";
    case KDTL_ERR_BALANCE:
        return "balance";
    case KDTL_ERR_STATE:
        return "state";
    case KDTL_ERR_LIMIT:
        return "limit";
    case KDTL_ERR_OVERFLOW:
        return "overflow";
    case KDTL_ERR_BUFFER:
        return "buffer";
    default:
        return "unknown";
    }
}

const char *kdtl_position_state_name(kdtl_position_state state) {
    switch (state) {
    case KDTL_POSITION_OPEN:
        return "open";
    case KDTL_POSITION_LIQUIDATED:
        return "liquidated";
    case KDTL_POSITION_CANCELLED:
        return "cancelled";
    case KDTL_POSITION_EXPIRED:
        return "expired";
    default:
        return "unknown";
    }
}

bool kdtl_copy_id(char *dst, size_t dst_len, const char *src) {
    size_t len;

    if (dst == NULL || src == NULL || dst_len == 0u) {
        return false;
    }

    len = strlen(src);
    if (len == 0u || len >= dst_len) {
        return false;
    }

    memset(dst, 0, dst_len);
    memcpy(dst, src, len);
    return true;
}

bool kdtl_id_eq(const char *lhs, const char *rhs) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    return strcmp(lhs, rhs) == 0;
}

bool kdtl_amount_add(kdtl_amount lhs, kdtl_amount rhs, kdtl_amount *out) {
    if (out == NULL) {
        return false;
    }
    if (UINT64_MAX - lhs < rhs) {
        return false;
    }
    *out = lhs + rhs;
    return true;
}

bool kdtl_amount_sub(kdtl_amount lhs, kdtl_amount rhs, kdtl_amount *out) {
    if (out == NULL) {
        return false;
    }
    if (lhs < rhs) {
        return false;
    }
    *out = lhs - rhs;
    return true;
}

bool kdtl_amount_mul_bps(kdtl_amount amount, uint16_t bps, kdtl_amount *out) {
    uint64_t q;
    uint64_t r;
    uint64_t part;

    if (out == NULL) {
        return false;
    }
    if (bps == 0u || amount == 0u) {
        *out = 0u;
        return true;
    }

    q = amount / 10000u;
    r = amount % 10000u;
    if (q > UINT64_MAX / bps) {
        return false;
    }
    part = q * bps;
    if (r > UINT64_MAX / bps) {
        return false;
    }
    r = (r * bps) / 10000u;
    if (UINT64_MAX - part < r) {
        return false;
    }
    *out = part + r;
    return true;
}

kdtl_amount kdtl_amount_min(kdtl_amount lhs, kdtl_amount rhs) {
    return lhs < rhs ? lhs : rhs;
}

kdtl_amount kdtl_amount_zero_floor_sub(kdtl_amount lhs, kdtl_amount rhs) {
    return lhs > rhs ? lhs - rhs : 0u;
}
