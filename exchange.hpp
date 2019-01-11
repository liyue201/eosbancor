/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/currency.hpp>
#include <string>

namespace dex
{
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

class exchange : public eosio::contract
{
public:
  exchange(account_name self);

  void clear_db();

  void apply(account_name contract, account_name action);

  void parse_memo_param(string memo, memo_param &param);

  void on(const currency::transfer &t, account_name code);

  void create_market(string market_name, asset eos_amount, account_name token_contract, asset token_amount);

  void update_market(asset eos_amount, account_name token_contract, asset token_amount);

  void open_market(account_name token_contract, uint64_t token_symbol, bool open);

  void buy_token(account_name payer, const asset &eos_quant, string token_symbol);

  void sell_token(account_name receiver, account_name token_contract, asset quant);

  void add_fee(uint64_t amount, uint64_t token_contract, uint64_t token_symbol);

  void take_fee(account_name account, int limit = 10);
};
} // namespace dex