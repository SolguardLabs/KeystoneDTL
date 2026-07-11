#include "keystone_dtl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSET_USDC "USDC"
#define PAYER "payer-alpha"
#define BENEFICIARY "merchant-beta"
#define OPERATOR "operator-keystone"
#define LIQUIDATOR "liquidator-gamma"
#define REFUND_VAULT "refund-vault"

typedef struct scenario_env {
    kdtl_engine *engine;
} scenario_env;

static void die(kdtl_engine *engine, const char *step, kdtl_status status) {
    fprintf(stderr, "%s failed: %s %s\n", step, kdtl_status_name(status),
            engine == NULL ? "" : kdtl_last_error(engine));
    exit(2);
}

static void must(kdtl_engine *engine, const char *step, kdtl_status status) {
    if (status != KDTL_OK) {
        die(engine, step, status);
    }
}

static void copy_text(char *dst, size_t len, const char *src) {
    if (snprintf(dst, len, "%s", src) < 0) {
        exit(2);
    }
}

static kdtl_asset_config asset_usdc(void) {
    kdtl_asset_config config;
    memset(&config, 0, sizeof(config));
    copy_text(config.asset, sizeof(config.asset), ASSET_USDC);
    config.decimals = 6u;
    config.liquidation_fee_bps = 25u;
    config.reserve_factor_bps = 500u;
    return config;
}

static kdtl_operator_policy policy_usdc(kdtl_amount active, kdtl_amount window) {
    kdtl_operator_policy policy;
    memset(&policy, 0, sizeof(policy));
    copy_text(policy.operator_id, sizeof(policy.operator_id), OPERATOR);
    copy_text(policy.asset, sizeof(policy.asset), ASSET_USDC);
    policy.max_active_debt = active;
    policy.max_window_debt = window;
    policy.max_open_positions = 16u;
    policy.min_margin_bps = 1200u;
    policy.window_grace_seconds = 90u;
    return policy;
}

static kdtl_route_terms terms_for(const char *route, kdtl_amount guarantee, kdtl_amount premium,
                                  kdtl_timestamp opened_at, kdtl_timestamp window_start,
                                  kdtl_timestamp window_end, kdtl_timestamp expires_at,
                                  uint64_t nonce) {
    kdtl_route_terms terms;
    memset(&terms, 0, sizeof(terms));
    copy_text(terms.payer, sizeof(terms.payer), PAYER);
    copy_text(terms.beneficiary, sizeof(terms.beneficiary), BENEFICIARY);
    copy_text(terms.operator_id, sizeof(terms.operator_id), OPERATOR);
    copy_text(terms.asset, sizeof(terms.asset), ASSET_USDC);
    copy_text(terms.route_id, sizeof(terms.route_id), route);
    terms.guarantee = guarantee;
    terms.premium = premium;
    terms.margin_bps = 1500u;
    terms.opened_at = opened_at;
    terms.window_start = window_start;
    terms.window_end = window_end;
    terms.expires_at = expires_at;
    terms.nonce = nonce;
    return terms;
}

static scenario_env setup_env(kdtl_amount active, kdtl_amount window) {
    scenario_env env;
    kdtl_asset_config asset = asset_usdc();
    kdtl_operator_policy policy = policy_usdc(active, window);

    env.engine = kdtl_engine_create();
    if (env.engine == NULL) {
        fprintf(stderr, "engine allocation failed\n");
        exit(2);
    }

    must(env.engine, "register asset", kdtl_register_asset(env.engine, &asset));
    must(env.engine, "register payer", kdtl_register_account(env.engine, PAYER));
    must(env.engine, "register beneficiary", kdtl_register_account(env.engine, BENEFICIARY));
    must(env.engine, "register operator", kdtl_register_account(env.engine, OPERATOR));
    must(env.engine, "register liquidator", kdtl_register_account(env.engine, LIQUIDATOR));
    must(env.engine, "register refund", kdtl_register_account(env.engine, REFUND_VAULT));
    must(env.engine, "set policy", kdtl_set_operator_policy(env.engine, &policy));
    must(env.engine, "deposit payer", kdtl_deposit(env.engine, PAYER, ASSET_USDC, 1000000u));
    return env;
}

static void format_position(kdtl_engine *engine, kdtl_position_id id, char *buffer, size_t len) {
    kdtl_position_view view;
    must(engine, "get position", kdtl_get_position(engine, id, &view));
    must(engine, "format position", kdtl_format_position_json(&view, buffer, len));
}

static void format_risk(kdtl_engine *engine, kdtl_timestamp now, char *buffer, size_t len) {
    kdtl_risk_view view;
    must(engine, "get risk", kdtl_get_operator_risk(engine, OPERATOR, ASSET_USDC, now, &view));
    must(engine, "format risk", kdtl_format_risk_json(&view, buffer, len));
}

static void format_account(kdtl_engine *engine, const char *account, char *buffer, size_t len) {
    kdtl_account_view view;
    must(engine, "get account", kdtl_get_account(engine, account, ASSET_USDC, &view));
    must(engine, "format account", kdtl_format_account_json(&view, buffer, len));
}

static void format_asset(kdtl_engine *engine, char *buffer, size_t len) {
    kdtl_asset_view view;
    must(engine, "get asset", kdtl_get_asset(engine, ASSET_USDC, &view));
    must(engine, "format asset", kdtl_format_asset_json(&view, buffer, len));
}

static void format_policy(kdtl_engine *engine, kdtl_timestamp now, char *buffer, size_t len) {
    kdtl_policy_view view;
    must(engine, "get policy", kdtl_get_operator_policy(engine, OPERATOR, ASSET_USDC, now, &view));
    must(engine, "format policy", kdtl_format_policy_json(&view, buffer, len));
}

static void format_reconciliation(kdtl_engine *engine, char *buffer, size_t len) {
    kdtl_reconciliation_view view;
    must(engine, "reconcile asset", kdtl_reconcile_asset(engine, ASSET_USDC, &view));
    must(engine, "format reconciliation", kdtl_format_reconciliation_json(&view, buffer, len));
}

static void format_event(kdtl_engine *engine, uint64_t sequence, char *buffer, size_t len) {
    kdtl_event_view view;
    must(engine, "get event", kdtl_get_event(engine, sequence, &view));
    must(engine, "format event", kdtl_format_event_json(&view, buffer, len));
}

static void print_catalog(void) {
    printf("{\"scenario\":\"catalog\",\"library\":\"KeystoneDTL\",\"language\":\"C11\","
           "\"wire\":\"json\",\"scenarios\":[\"open-basic\",\"limit-update\","
           "\"liquidation\",\"cancellation\",\"expiration\",\"window-rollover\","
           "\"event-journal\",\"rejections\",\"operator-overview\"]}\n");
}

static void scenario_open_basic(void) {
    scenario_env env = setup_env(600000u, 300000u);
    kdtl_route_terms terms =
        terms_for("route-open-001", 250000u, 1000u, 1000u, 1100u, 1600u, 1900u, 1u);
    kdtl_position_id id;
    char position[2048];
    char risk[2048];
    char payer[512];
    char operator_account[512];
    char reconciliation[1024];

    must(env.engine, "open", kdtl_open_position(env.engine, &terms, &id));
    format_position(env.engine, id, position, sizeof(position));
    format_risk(env.engine, 1200u, risk, sizeof(risk));
    format_account(env.engine, PAYER, payer, sizeof(payer));
    format_account(env.engine, OPERATOR, operator_account, sizeof(operator_account));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"open-basic\",\"position_id\":%llu,\"position\":%s,"
           "\"risk\":%s,\"payer\":%s,\"operator\":%s,\"reconciliation\":%s}\n",
           (unsigned long long)id, position, risk, payer, operator_account, reconciliation);
    kdtl_engine_destroy(env.engine);
}

static void scenario_limit_update(void) {
    scenario_env env = setup_env(500000u, 250000u);
    kdtl_route_terms terms =
        terms_for("route-limit-001", 100000u, 0u, 1000u, 1200u, 1800u, 2200u, 2u);
    kdtl_operator_policy policy = policy_usdc(800000u, 400000u);
    kdtl_position_id id;
    char before[2048];
    char after[2048];
    char position[2048];
    char reconciliation[1024];

    must(env.engine, "open", kdtl_open_position(env.engine, &terms, &id));
    format_risk(env.engine, 1300u, before, sizeof(before));
    must(env.engine, "update policy", kdtl_update_operator_policy(env.engine, &policy, 1300u));
    format_risk(env.engine, 1300u, after, sizeof(after));
    format_position(env.engine, id, position, sizeof(position));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"limit-update\",\"position\":%s,\"before\":%s,\"after\":%s,"
           "\"reconciliation\":%s}\n",
           position, before, after, reconciliation);
    kdtl_engine_destroy(env.engine);
}

static void scenario_liquidation(void) {
    scenario_env env = setup_env(700000u, 350000u);
    kdtl_route_terms terms =
        terms_for("route-liq-001", 300000u, 0u, 1000u, 1100u, 1700u, 2100u, 3u);
    kdtl_liquidation_claim claim;
    kdtl_position_id id;
    char position[2048];
    char payer[512];
    char liquidator[512];
    char risk[2048];
    char reconciliation[1024];

    must(env.engine, "open", kdtl_open_position(env.engine, &terms, &id));
    memset(&claim, 0, sizeof(claim));
    claim.position_id = id;
    copy_text(claim.liquidator, sizeof(claim.liquidator), LIQUIDATOR);
    claim.observed_loss = 120000u;
    claim.penalty_bps = 500u;
    claim.now = 1300u;
    must(env.engine, "liquidate", kdtl_liquidate_position(env.engine, &claim));

    format_position(env.engine, id, position, sizeof(position));
    format_account(env.engine, PAYER, payer, sizeof(payer));
    format_account(env.engine, LIQUIDATOR, liquidator, sizeof(liquidator));
    format_risk(env.engine, 1400u, risk, sizeof(risk));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"liquidation\",\"position\":%s,\"payer\":%s,"
           "\"liquidator\":%s,\"risk\":%s,\"reconciliation\":%s}\n",
           position, payer, liquidator, risk, reconciliation);
    kdtl_engine_destroy(env.engine);
}

static void scenario_cancellation(void) {
    scenario_env env = setup_env(600000u, 300000u);
    kdtl_route_terms terms =
        terms_for("route-cancel-001", 180000u, 0u, 1000u, 2000u, 2600u, 2900u, 4u);
    kdtl_cancel_request request;
    kdtl_position_id id;
    char position[2048];
    char payer[512];
    char risk[2048];
    char reconciliation[1024];

    must(env.engine, "open", kdtl_open_position(env.engine, &terms, &id));
    memset(&request, 0, sizeof(request));
    request.position_id = id;
    copy_text(request.requested_by, sizeof(request.requested_by), PAYER);
    copy_text(request.refund_to, sizeof(request.refund_to), PAYER);
    request.now = 1500u;
    must(env.engine, "cancel", kdtl_cancel_position(env.engine, &request));

    format_position(env.engine, id, position, sizeof(position));
    format_account(env.engine, PAYER, payer, sizeof(payer));
    format_risk(env.engine, 1600u, risk, sizeof(risk));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"cancellation\",\"position\":%s,\"payer\":%s,\"risk\":%s,"
           "\"reconciliation\":%s}\n",
           position, payer, risk, reconciliation);
    kdtl_engine_destroy(env.engine);
}

static void scenario_expiration(void) {
    scenario_env env = setup_env(600000u, 300000u);
    kdtl_route_terms terms =
        terms_for("route-expire-001", 90000u, 0u, 1000u, 1100u, 1400u, 1600u, 5u);
    kdtl_position_id id;
    uint32_t expired = 0u;
    char position[2048];
    char payer[512];
    char risk[2048];
    char reconciliation[1024];

    must(env.engine, "open", kdtl_open_position(env.engine, &terms, &id));
    must(env.engine, "expire", kdtl_expire_positions(env.engine, 1600u, &expired));
    format_position(env.engine, id, position, sizeof(position));
    format_account(env.engine, PAYER, payer, sizeof(payer));
    format_risk(env.engine, 1700u, risk, sizeof(risk));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"expiration\",\"expired\":%u,\"position\":%s,\"payer\":%s,"
           "\"risk\":%s,\"reconciliation\":%s}\n",
           expired, position, payer, risk, reconciliation);
    kdtl_engine_destroy(env.engine);
}

static void scenario_window_rollover(void) {
    scenario_env env = setup_env(500000u, 300000u);
    kdtl_route_terms terms =
        terms_for("route-roll-001", 200000u, 0u, 1000u, 1000u, 1400u, 2400u, 6u);
    kdtl_move_request request;
    kdtl_position_id id;
    char position[2048];
    char risk[2048];
    char payer[512];
    char reconciliation[1024];

    must(env.engine, "open", kdtl_open_position(env.engine, &terms, &id));
    memset(&request, 0, sizeof(request));
    request.position_id = id;
    request.now = 1200u;
    request.new_window_start = 1500u;
    request.new_window_end = 2100u;
    copy_text(request.reason, sizeof(request.reason), "operator-roll");
    must(env.engine, "move", kdtl_move_position(env.engine, &request));

    format_position(env.engine, id, position, sizeof(position));
    format_risk(env.engine, 1200u, risk, sizeof(risk));
    format_account(env.engine, PAYER, payer, sizeof(payer));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"window-rollover\",\"position\":%s,\"risk\":%s,\"payer\":%s,"
           "\"reconciliation\":%s}\n",
           position, risk, payer, reconciliation);
    kdtl_engine_destroy(env.engine);
}

static void scenario_event_journal(void) {
    scenario_env env = setup_env(650000u, 300000u);
    kdtl_route_terms terms =
        terms_for("route-event-001", 125000u, 500u, 1000u, 1300u, 1700u, 2200u, 7u);
    kdtl_move_request request;
    kdtl_position_id id;
    char opened[1024];
    char moved[1024];
    char risk[2048];
    char reconciliation[1024];

    must(env.engine, "open", kdtl_open_position(env.engine, &terms, &id));
    memset(&request, 0, sizeof(request));
    request.position_id = id;
    request.now = 1200u;
    request.new_window_start = 1750u;
    request.new_window_end = 2100u;
    copy_text(request.reason, sizeof(request.reason), "route-delay");
    must(env.engine, "move", kdtl_move_position(env.engine, &request));

    format_event(env.engine, 9u, opened, sizeof(opened));
    format_event(env.engine, 10u, moved, sizeof(moved));
    format_risk(env.engine, 1250u, risk, sizeof(risk));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"event-journal\",\"opened\":%s,\"moved\":%s,\"risk\":%s,"
           "\"reconciliation\":%s}\n",
           opened, moved, risk, reconciliation);
    kdtl_engine_destroy(env.engine);
}

static void scenario_rejections(void) {
    scenario_env env = setup_env(400000u, 200000u);
    kdtl_route_terms oversize =
        terms_for("route-reject-001", 250000u, 0u, 1000u, 1200u, 1800u, 2200u, 8u);
    kdtl_route_terms accepted =
        terms_for("route-reject-002", 150000u, 0u, 1000u, 1200u, 1800u, 2200u, 9u);
    kdtl_operator_policy lowered = policy_usdc(300000u, 100000u);
    kdtl_position_id id = 0u;
    kdtl_status oversize_status;
    kdtl_status update_status;
    char oversize_error[256];
    char update_error[256];
    char position[2048];
    char risk[2048];
    char reconciliation[1024];

    oversize_status = kdtl_open_position(env.engine, &oversize, &id);
    copy_text(oversize_error, sizeof(oversize_error), kdtl_last_error(env.engine));
    must(env.engine, "accepted open", kdtl_open_position(env.engine, &accepted, &id));
    update_status = kdtl_update_operator_policy(env.engine, &lowered, 1300u);
    copy_text(update_error, sizeof(update_error), kdtl_last_error(env.engine));

    format_position(env.engine, id, position, sizeof(position));
    format_risk(env.engine, 1300u, risk, sizeof(risk));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"rejections\",\"oversize_status\":\"%s\","
           "\"oversize_error\":\"%s\",\"update_status\":\"%s\",\"update_error\":\"%s\","
           "\"position\":%s,\"risk\":%s,\"reconciliation\":%s}\n",
           kdtl_status_name(oversize_status), oversize_error, kdtl_status_name(update_status),
           update_error, position, risk, reconciliation);
    kdtl_engine_destroy(env.engine);
}

static void scenario_operator_overview(void) {
    scenario_env env = setup_env(900000u, 450000u);
    kdtl_route_terms first =
        terms_for("route-overview-001", 150000u, 700u, 1000u, 1200u, 1800u, 2300u, 10u);
    kdtl_route_terms second =
        terms_for("route-overview-002", 175000u, 300u, 1000u, 1300u, 1900u, 2400u, 11u);
    kdtl_position_id first_id;
    kdtl_position_id second_id;
    char asset[2048];
    char policy[2048];
    char risk[2048];
    char operator_account[512];
    char reconciliation[1024];

    must(env.engine, "open first", kdtl_open_position(env.engine, &first, &first_id));
    must(env.engine, "open second", kdtl_open_position(env.engine, &second, &second_id));

    format_asset(env.engine, asset, sizeof(asset));
    format_policy(env.engine, 1400u, policy, sizeof(policy));
    format_risk(env.engine, 1400u, risk, sizeof(risk));
    format_account(env.engine, OPERATOR, operator_account, sizeof(operator_account));
    format_reconciliation(env.engine, reconciliation, sizeof(reconciliation));

    printf("{\"scenario\":\"operator-overview\",\"first_position_id\":%llu,"
           "\"second_position_id\":%llu,\"asset\":%s,\"policy\":%s,\"risk\":%s,"
           "\"operator\":%s,\"reconciliation\":%s}\n",
           (unsigned long long)first_id, (unsigned long long)second_id, asset, policy, risk,
           operator_account, reconciliation);
    kdtl_engine_destroy(env.engine);
}

int main(int argc, char **argv) {
    const char *scenario = argc > 1 ? argv[1] : "catalog";

    if (strcmp(scenario, "catalog") == 0) {
        print_catalog();
    } else if (strcmp(scenario, "open-basic") == 0) {
        scenario_open_basic();
    } else if (strcmp(scenario, "limit-update") == 0) {
        scenario_limit_update();
    } else if (strcmp(scenario, "liquidation") == 0) {
        scenario_liquidation();
    } else if (strcmp(scenario, "cancellation") == 0) {
        scenario_cancellation();
    } else if (strcmp(scenario, "expiration") == 0) {
        scenario_expiration();
    } else if (strcmp(scenario, "window-rollover") == 0) {
        scenario_window_rollover();
    } else if (strcmp(scenario, "event-journal") == 0) {
        scenario_event_journal();
    } else if (strcmp(scenario, "rejections") == 0) {
        scenario_rejections();
    } else if (strcmp(scenario, "operator-overview") == 0) {
        scenario_operator_overview();
    } else {
        fprintf(stderr, "unknown scenario: %s\n", scenario);
        return 1;
    }
    return 0;
}
