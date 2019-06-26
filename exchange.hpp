/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <string>


using namespace eosio;
using namespace eosio;
using eosio::asset;
using eosio::symbol_code;
using std::string;
using std::vector;

typedef double real_type;

#define OPT_CREATE_MARKET "create_market"
#define OPT_OPEN_MARKET "open_market"
#define OPT_CLOSE_MARKET "close_market"
#define OPT_UPDATE_MARKET "update_market"

const static double FEE_RATE = 0.002;

struct memo_param {
    string opt;
    string market_name;
    uint64_t eos_amount = 0;
    uint64_t token_amount = 0;
    uint64_t limit = 0;
};

uint128_t name_to_num(string s);

class[[eosio::contract("eosbancor")]] exchange : public eosio::contract
{
public:
    using contract::contract;

    exchange(name self, name first_receiver, eosio::datastream<const char*> ds);

    void transfer(name from, name to, asset quantity, string memo);

    [[eosio::action]] void clear();

private:
    void parse_memo_param(string memo, memo_param & param);

    void create_market(string market_name, asset eos_amount, name token_contract, asset token_amount);

    void update_market(asset eos_amount, name token_contract, asset token_amount);

    void open_market(name token_contract, symbol token_symbol, bool open);

    void buy_token(name payer, const asset& eos_quant, string token_symbol);

    void sell_token(name receiver, name token_contract, asset quant);

private:
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
        asset convert(asset from, const symbol& to);

        EOSLIB_SERIALIZE(market, (id)(idxkey)(market_name)(supply)(open)(base)(quote))
    };

    typedef eosio::multi_index<"markets"_n, market,
        indexed_by<"idxkey"_n, const_mem_fun<market, uint128_t, &market::by_contract_sym>>,
        indexed_by<"idxname"_n, const_mem_fun<market, uint128_t, &market::by_name>>>
        markets;
};