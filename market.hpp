#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <musl/upstream/include/bits/stdint.h>

namespace dex
{
using namespace eosio;
using boost::container::flat_map;
using eosio::asset;
using eosio::symbol_type;
using std::string;
typedef double real_type;

/**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    */


uint128_t name_to_num(string s);

// @abi table markets
struct market
{
    uint64_t id;
    uint128_t idxkey;
    string name;
    asset supply;
    bool open;
    struct connector
    {
        account_name contract;
        asset balance;
        uint64_t weight = 500;

        EOSLIB_SERIALIZE(connector, (contract)(balance)(weight))
    };

    connector base;
    connector quote;
    uint64_t primary_key() const { return id; }
    uint128_t by_contract_sym() const { return idxkey; }
    uint128_t by_name() const { return name_to_num(name); }

    asset convert_to_exchange(connector &c, asset in);
    asset convert_from_exchange(connector &c, asset in);
    asset convert(asset from, symbol_type to);

    EOSLIB_SERIALIZE(market, (id)(idxkey)(name)(supply)(open)(base)(quote))
};

typedef eosio::multi_index<N(markets), market,
                           indexed_by<N(idxkey), const_mem_fun<market, uint128_t, &market::by_contract_sym>>,
                           indexed_by<N(idxname), const_mem_fun<market, uint128_t, &market::by_name>>>
    markets;

struct fee
{
    uint64_t id = 0;
    uint64_t token_contract = 0;
    uint64_t token_symbol = 0;
    uint64_t amount = 0;
    auto primary_key() const { return id; }
    uint128_t by_token_id() const { return (uint128_t(token_contract) << 64) | token_symbol; }
    EOSLIB_SERIALIZE(fee, (id)(token_contract)(token_symbol)(amount))
};
typedef eosio::multi_index<N(fees), fee, indexed_by<N(token_id), const_mem_fun<fee, uint128_t, &fee::by_token_id>>> fees;

} // namespace dex
