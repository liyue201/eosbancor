#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>

using namespace eosio;
using eosio::asset;
using eosio::symbol_code;
using std::string;
typedef double real_type;

uint128_t name_to_num(string s);

struct [[eosio::table]] market {
    uint64_t id;
    uint128_t idxkey;
    string market_name;
    asset supply;
    bool open;
    struct connector {
        name contract;
        asset balance;
        uint64_t weight = 500;

        EOSLIB_SERIALIZE(connector, (contract)(balance)(weight))
    };

    connector base;
    connector quote;
    uint64_t primary_key() const { return id; }
    uint128_t by_contract_sym() const { return idxkey; }
    uint128_t by_name() const { return name_to_num(market_name); }

    asset convert_to_exchange(connector & c, const asset& in);
    asset convert_from_exchange(connector & c, asset in);
    asset convert(const asset& from, const symbol& to);

    EOSLIB_SERIALIZE(market, (id)(idxkey)(market_name)(supply)(open)(base)(quote))
};

typedef eosio::multi_index<"markets"_n, market,
    indexed_by<"idxkey"_n, const_mem_fun<market, uint128_t, &market::by_contract_sym>>,
    indexed_by<"idxname"_n, const_mem_fun<market, uint128_t, &market::by_name>>>
    markets;

