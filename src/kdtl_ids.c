#include "kdtl_internal.h"

#include <stdio.h>
#include <string.h>

static const uint64_t KDTL_FNV_OFFSET = 1469598103934665603ull;
static const uint64_t KDTL_FNV_PRIME = 1099511628211ull;

uint64_t kdtl_fnv1a_bytes(const void *data, size_t len, uint64_t seed) {
    const unsigned char *bytes = (const unsigned char *)data;
    uint64_t hash = seed == 0u ? KDTL_FNV_OFFSET : seed;

    for (size_t i = 0; i < len; i++) {
        hash ^= (uint64_t)bytes[i];
        hash *= KDTL_FNV_PRIME;
    }
    return hash;
}

uint64_t kdtl_fnv1a_string(const char *text, uint64_t seed) {
    if (text == NULL) {
        return kdtl_fnv1a_bytes("", 0u, seed);
    }
    return kdtl_fnv1a_bytes(text, strlen(text), seed);
}

static uint64_t hash_u64(uint64_t hash, uint64_t value) {
    return kdtl_fnv1a_bytes(&value, sizeof(value), hash);
}

uint64_t kdtl_hash_terms(const kdtl_route_terms *terms) {
    uint64_t hash = kdtl_fnv1a_string("keystone:terms:v1", 0u);

    if (terms == NULL) {
        return hash;
    }

    hash = kdtl_fnv1a_string(terms->payer, hash);
    hash = kdtl_fnv1a_string(terms->beneficiary, hash);
    hash = kdtl_fnv1a_string(terms->operator_id, hash);
    hash = kdtl_fnv1a_string(terms->asset, hash);
    hash = kdtl_fnv1a_string(terms->route_id, hash);
    hash = hash_u64(hash, terms->guarantee);
    hash = hash_u64(hash, terms->premium);
    hash = hash_u64(hash, terms->margin_bps);
    hash = hash_u64(hash, terms->opened_at);
    hash = hash_u64(hash, terms->window_start);
    hash = hash_u64(hash, terms->window_end);
    hash = hash_u64(hash, terms->expires_at);
    hash = hash_u64(hash, terms->nonce);
    return hash;
}

uint64_t kdtl_hash_position_view(const kdtl_position_view *view) {
    uint64_t hash = kdtl_fnv1a_string("keystone:position:v1", 0u);

    if (view == NULL) {
        return hash;
    }

    hash = hash_u64(hash, view->id);
    hash = hash_u64(hash, (uint64_t)view->state);
    hash = kdtl_fnv1a_string(view->payer, hash);
    hash = kdtl_fnv1a_string(view->beneficiary, hash);
    hash = kdtl_fnv1a_string(view->operator_id, hash);
    hash = kdtl_fnv1a_string(view->asset, hash);
    hash = kdtl_fnv1a_string(view->route_id, hash);
    hash = hash_u64(hash, view->guarantee);
    hash = hash_u64(hash, view->locked);
    hash = hash_u64(hash, view->paid_out);
    hash = hash_u64(hash, view->refunded);
    hash = hash_u64(hash, view->premium);
    hash = hash_u64(hash, view->margin_bps);
    hash = hash_u64(hash, view->opened_at);
    hash = hash_u64(hash, view->updated_at);
    hash = hash_u64(hash, view->window_start);
    hash = hash_u64(hash, view->window_end);
    hash = hash_u64(hash, view->expires_at);
    return hash;
}

void kdtl_digest_hex(uint64_t value, char out[KDTL_HASH_LEN]) {
    uint64_t x = value;
    uint64_t y = value ^ 0x9e3779b97f4a7c15ull;
    uint64_t z = value + 0xbf58476d1ce4e5b9ull;
    uint64_t w = value ^ (value >> 32u);

    (void)snprintf(out, KDTL_HASH_LEN, "%016llx%016llx%016llx%016llx", (unsigned long long)x,
                   (unsigned long long)y, (unsigned long long)z, (unsigned long long)w);
}
