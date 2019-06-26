#include "exchange.hpp"
#include <cmath>
#include <string>

asset exchange::market::convert_to_exchange(connector& c, const asset& in)
{
    real_type R(supply.amount);
    real_type C(c.balance.amount + in.amount);
    real_type F(c.weight / 1000.0);
    real_type T(in.amount);
    real_type ONE(1.0);

    real_type E = -R * (ONE - std::pow(ONE + T / C, F));

    int64_t issued = int64_t(E);

    supply.amount += issued;
    c.balance.amount += in.amount;

    return asset(issued, supply.symbol);
}

asset exchange::market::convert_from_exchange(connector& c, asset in)
{
    eosio::check(in.symbol == supply.symbol, "unexpected asset symbol input");

    real_type R(supply.amount - in.amount);
    real_type C(c.balance.amount);
    real_type F(1000.0 / c.weight);
    real_type E(in.amount);
    real_type ONE(1.0);

    real_type T = C * (std::pow(ONE + E / R, F) - ONE);

    int64_t out = int64_t(T);

    supply.amount -= in.amount;
    c.balance.amount -= out;

    return asset(out, c.balance.symbol);
}

asset exchange::market::convert(asset from, const symbol& to)
{
    auto sell_symbol = from.symbol;
    auto ex_symbol = supply.symbol;
    auto base_symbol = base.balance.symbol;
    auto quote_symbol = quote.balance.symbol;

    if (sell_symbol != ex_symbol) {
        if (sell_symbol == base_symbol) {
            from = convert_to_exchange(base, from);
        } else if (sell_symbol == quote_symbol) {
            from = convert_to_exchange(quote, from);
        } else {
            eosio::check(false, "invalid sell");
        }
    } else {
        if (to == base_symbol) {
            from = convert_from_exchange(base, from);
        } else if (to == quote_symbol) {
            from = convert_from_exchange(quote, from);
        } else {
            eosio::check(false, "invalid conversion");
        }
    }

    if (to != from.symbol)
        return convert(from, to);

    return from;
}
