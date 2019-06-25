/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>
#include <string>


using namespace eosio;
using std::string;
using std::vector;

#define OPT_CREATE_MARKET "create_market"
#define OPT_OPEN_MARKET "open_market"
#define OPT_CLOSE_MARKET "close_market"
#define OPT_UPDATE_MARKET "update_market"
#define OPT_TAKE_FEE "take_fee"

const static double FEE_RATE = 0.002;

struct memo_param
{
  string opt;
  string market_name;
  uint64_t eos_amount = 0;
  uint64_t token_amount = 0;
  uint64_t limit = 0;
};

CONTRACT exchange : public eosio::contract
{
public:
  using contract::contract;

  exchange(name self, name first_receiver, eosio::datastream<const char*> ds);

  void transfer(name from, name to, asset quantity, string memo);

  [[eosio::action]]
  void clear();

private:

  void parse_memo_param(string memo, memo_param &param);

  void create_market(string market_name, asset eos_amount, name token_contract, asset token_amount);

  void update_market(asset eos_amount, name token_contract, asset token_amount);

  void open_market(name token_contract, symbol token_symbol, bool open);

  void buy_token(name payer, const asset &eos_quant, string token_symbol);

  void sell_token(name receiver, name token_contract, asset quant);

  void add_fee(uint64_t amount, name token_contract, symbol token_symbol);

  void take_fee(name account, int limit = 10);
};
