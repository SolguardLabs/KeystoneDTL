#include "kdtl_internal.h"

#include <string.h>

int kdtl_find_asset_index(const kdtl_engine *engine, const char *asset) {
    if (engine == NULL || asset == NULL) {
        return -1;
    }

    for (size_t i = 0; i < engine->asset_count; i++) {
        if (engine->assets[i].used && kdtl_id_eq(engine->assets[i].asset, asset)) {
            return (int)i;
        }
    }
    return -1;
}

int kdtl_find_account_index(const kdtl_engine *engine, const char *account) {
    if (engine == NULL || account == NULL) {
        return -1;
    }

    for (size_t i = 0; i < engine->account_count; i++) {
        if (engine->accounts[i].used && kdtl_id_eq(engine->accounts[i].account, account)) {
            return (int)i;
        }
    }
    return -1;
}

kdtl_status kdtl_register_asset(kdtl_engine *engine, const kdtl_asset_config *config) {
    kdtl_asset_record *record;

    if (engine == NULL || config == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    if (config->decimals > 18u || config->asset[0] == '\0') {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid asset configuration");
    }
    if (kdtl_find_asset_index(engine, config->asset) >= 0) {
        return kdtl_fail(engine, KDTL_ERR_EXISTS, "asset already registered");
    }
    if (engine->asset_count >= KDTL_MAX_ASSETS) {
        return kdtl_fail(engine, KDTL_ERR_CAPACITY, "asset registry capacity reached");
    }

    record = &engine->assets[engine->asset_count++];
    memset(record, 0, sizeof(*record));
    record->used = true;
    if (!kdtl_copy_id(record->asset, sizeof(record->asset), config->asset)) {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid asset id");
    }
    record->decimals = config->decimals;
    record->liquidation_fee_bps = config->liquidation_fee_bps;
    record->reserve_factor_bps = config->reserve_factor_bps;
    return kdtl_record_event(engine, KDTL_EVENT_ASSET_REGISTERED, 0u, "", record->asset, 0u, 0u,
                             "asset");
}

kdtl_status kdtl_register_account(kdtl_engine *engine, const char *account) {
    kdtl_account_record *record;

    if (engine == NULL || account == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    if (kdtl_find_account_index(engine, account) >= 0) {
        return kdtl_fail(engine, KDTL_ERR_EXISTS, "account already registered");
    }
    if (engine->account_count >= KDTL_MAX_ACCOUNTS) {
        return kdtl_fail(engine, KDTL_ERR_CAPACITY, "account registry capacity reached");
    }

    record = &engine->accounts[engine->account_count++];
    memset(record, 0, sizeof(*record));
    record->used = true;
    if (!kdtl_copy_id(record->account, sizeof(record->account), account)) {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid account id");
    }
    return kdtl_record_event(engine, KDTL_EVENT_ACCOUNT_REGISTERED, 0u, "", "", 0u, 0u,
                             "account");
}

kdtl_status kdtl_deposit(kdtl_engine *engine, const char *account, const char *asset,
                         kdtl_amount amount) {
    int account_index;
    int asset_index;
    kdtl_amount next;

    if (engine == NULL || account == NULL || asset == NULL || amount == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    account_index = kdtl_find_account_index(engine, account);
    asset_index = kdtl_find_asset_index(engine, asset);
    if (account_index < 0 || asset_index < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "deposit target not registered");
    }
    if (!kdtl_amount_add(engine->accounts[account_index].liquid[asset_index], amount, &next)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "deposit overflow");
    }
    engine->accounts[account_index].liquid[asset_index] = next;
    return kdtl_record_event(engine, KDTL_EVENT_DEPOSITED, 0u, "", asset, 0u, amount, "deposit");
}

kdtl_status kdtl_debit_liquid(kdtl_engine *engine, const char *account, const char *asset,
                              kdtl_amount amount) {
    int account_index;
    int asset_index;
    kdtl_amount next;

    if (engine == NULL || account == NULL || asset == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (amount == 0u) {
        return KDTL_OK;
    }
    account_index = kdtl_find_account_index(engine, account);
    asset_index = kdtl_find_asset_index(engine, asset);
    if (account_index < 0 || asset_index < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "debit target not registered");
    }
    if (!kdtl_amount_sub(engine->accounts[account_index].liquid[asset_index], amount, &next)) {
        return kdtl_fail(engine, KDTL_ERR_BALANCE, "insufficient liquid balance");
    }
    engine->accounts[account_index].liquid[asset_index] = next;
    return KDTL_OK;
}

kdtl_status kdtl_credit_liquid(kdtl_engine *engine, const char *account, const char *asset,
                               kdtl_amount amount) {
    int account_index;
    int asset_index;
    kdtl_amount next;

    if (engine == NULL || account == NULL || asset == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (amount == 0u) {
        return KDTL_OK;
    }
    account_index = kdtl_find_account_index(engine, account);
    asset_index = kdtl_find_asset_index(engine, asset);
    if (account_index < 0 || asset_index < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "credit target not registered");
    }
    if (!kdtl_amount_add(engine->accounts[account_index].liquid[asset_index], amount, &next)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "credit overflow");
    }
    engine->accounts[account_index].liquid[asset_index] = next;
    return KDTL_OK;
}

kdtl_status kdtl_lock_account(kdtl_engine *engine, const char *account, const char *asset,
                              kdtl_amount amount) {
    int account_index;
    int asset_index;
    kdtl_amount liquid;
    kdtl_amount locked;

    if (engine == NULL || account == NULL || asset == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (amount == 0u) {
        return KDTL_OK;
    }
    account_index = kdtl_find_account_index(engine, account);
    asset_index = kdtl_find_asset_index(engine, asset);
    if (account_index < 0 || asset_index < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "lock target not registered");
    }
    if (!kdtl_amount_sub(engine->accounts[account_index].liquid[asset_index], amount, &liquid)) {
        return kdtl_fail(engine, KDTL_ERR_BALANCE, "insufficient liquid balance");
    }
    if (!kdtl_amount_add(engine->accounts[account_index].locked[asset_index], amount, &locked)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "locked balance overflow");
    }
    engine->accounts[account_index].liquid[asset_index] = liquid;
    engine->accounts[account_index].locked[asset_index] = locked;
    return KDTL_OK;
}

kdtl_status kdtl_unlock_account(kdtl_engine *engine, const char *account, const char *asset,
                                kdtl_amount amount) {
    int account_index;
    int asset_index;
    kdtl_amount locked;
    kdtl_amount liquid;

    if (engine == NULL || account == NULL || asset == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (amount == 0u) {
        return KDTL_OK;
    }
    account_index = kdtl_find_account_index(engine, account);
    asset_index = kdtl_find_asset_index(engine, asset);
    if (account_index < 0 || asset_index < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "unlock target not registered");
    }
    if (!kdtl_amount_sub(engine->accounts[account_index].locked[asset_index], amount, &locked)) {
        return kdtl_fail(engine, KDTL_ERR_BALANCE, "insufficient locked balance");
    }
    if (!kdtl_amount_add(engine->accounts[account_index].liquid[asset_index], amount, &liquid)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "liquid balance overflow");
    }
    engine->accounts[account_index].locked[asset_index] = locked;
    engine->accounts[account_index].liquid[asset_index] = liquid;
    return KDTL_OK;
}

kdtl_status kdtl_debit_locked(kdtl_engine *engine, const char *account, const char *asset,
                              kdtl_amount amount) {
    int account_index;
    int asset_index;
    kdtl_amount locked;

    if (engine == NULL || account == NULL || asset == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (amount == 0u) {
        return KDTL_OK;
    }
    account_index = kdtl_find_account_index(engine, account);
    asset_index = kdtl_find_asset_index(engine, asset);
    if (account_index < 0 || asset_index < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "locked debit target not registered");
    }
    if (!kdtl_amount_sub(engine->accounts[account_index].locked[asset_index], amount, &locked)) {
        return kdtl_fail(engine, KDTL_ERR_BALANCE, "insufficient locked balance");
    }
    engine->accounts[account_index].locked[asset_index] = locked;
    return KDTL_OK;
}

kdtl_status kdtl_get_account(const kdtl_engine *engine, const char *account, const char *asset,
                             kdtl_account_view *out) {
    int account_index;
    int asset_index;

    if (engine == NULL || account == NULL || asset == NULL || out == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    account_index = kdtl_find_account_index(engine, account);
    asset_index = kdtl_find_asset_index(engine, asset);
    if (account_index < 0 || asset_index < 0) {
        return KDTL_ERR_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    (void)kdtl_copy_id(out->account, sizeof(out->account), account);
    (void)kdtl_copy_id(out->asset, sizeof(out->asset), asset);
    out->liquid = engine->accounts[account_index].liquid[asset_index];
    out->locked = engine->accounts[account_index].locked[asset_index];
    return KDTL_OK;
}
