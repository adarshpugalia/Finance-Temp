#include <iostream>
#include <map>

#include "StockCandle.h"
#include "Constants.h"

using namespace std;

namespace finance {

class OngoingTrade {
public:
	OngoingTrade(string symbol) {
		this->symbol = symbol;
		trade_ongoing = false;
		stocks_held = 0;
	}

	bool IsStopLossBreached(const StockCandle& candle, const BacktestCriteria& criteria) {
		if(criteria.stop_loss_criteria.type == StoplossCriteria::LOW) {
			if(candle.low < stop_loss + eps) {
				return true;
			}
		} else if(criteria.stop_loss_criteria.type == StoplossCriteria::CLOSE) {
			/* If the stop loss criteria is CLOSE, we'll check if the close breaches or is close to the stop loss. */
			if(candle.close < stop_loss + eps) {
				return true;
			} else if((candle.close - stop_loss)/candle.close < 0.002) {
				return true;

			}
		}

		return false;
	}

	bool IsExitGainHit(const StockCandle& candle, const BacktestCriteria& criteria) {
		if(criteria.exit_gain_criteria.enabled) {
			if(candle.high > buy_price*(1+criteria.exit_gain_criteria.gain_percentage)) {
				return true;
			}
		}

		return false;
	}

	string symbol;
	bool trade_ongoing;
	int stocks_held;
	double buy_price;
	double stop_loss;
};

class TradeState {
public:
	TradeState() {
		wins = 0;
		losses = 0;

		for(string stock: constants::kNifty50) {
			trade_map.insert(std::make_pair(stock, OngoingTrade(stock)));
		}
	}

	double BuyIfFitsCriteria(const StockCandle& candle, double capital, const BacktestCriteria& criteria) {
		auto it = trade_map.find(candle.symbol);
		if(!it->second.trade_ongoing) {
			/* Checking the volume criteria. */
			if(criteria.buy_volume_criteria.enabled) {
				if(candle.volume < candle.average_volume*criteria.buy_volume_criteria.average_volume_threshold) {
					return capital;
				}
			}

			/* Checking if the capital is enough to buy. */
			if(criteria.buy_criteria.criteria == BuyCriteria::CLOSE && (capital < candle.close)) {
				//std::cout << "Capital not enough to buy candle with buy criteria CLOSE: " << candle << endl;
				return capital;
			} else if(criteria.buy_criteria.criteria == BuyCriteria::HIGH && (capital < candle.high)) {
				//std::cout << "Capital not enough to buy candle with buy criteria HIGH: " << candle << endl;
				return capital;
			} else if(criteria.buy_criteria.criteria == BuyCriteria::MEAN_CLOSE_HIGH && (capital < ((candle.close + candle.high)/2))) {
				//std::cout << "Capital not enough to buy candle with buy criteria MEAN_CLOSE_HIGH: " << candle << endl;
				return capital;
			}

			/* Checking if Marubozu is enabled. */
			if(criteria.marubozu_criteria.enabled) {
				if(!candle.IsBullishMarubozu(criteria.marubozu_criteria)) {
					return capital;
				}

				return Buy(candle, capital, criteria);
			}
		}

		return capital;
	}

	double SellIfFitsCriteria(const StockCandle& candle, double capital, const BacktestCriteria& criteria) {
		auto it = trade_map.find(candle.symbol);
		if(it->second.trade_ongoing) {
			if(it->second.IsStopLossBreached(candle, criteria)) {
				if(it->second.IsExitGainHit(candle, criteria)) {
					std::cout << "Both stop loss and exit gain hit." << endl;
					std::cout << "Candle: " << candle << endl;
					std::cout << "Trade State: " << *(this) << endl;
					return Sell(capital, it->second.stop_loss, candle);
					// exit(0);
				} else {
					return Sell(capital, it->second.stop_loss, candle);
				}
			}

			if(it->second.IsExitGainHit(candle, criteria)) {
				return Sell(capital, it->second.buy_price*(1+criteria.exit_gain_criteria.gain_percentage), candle);
			}
		}

		return capital;
	}

	double GetFinalCapital(double capital) {
		for(auto it = trade_map.begin(); it != trade_map.end(); it++) {
			if(it->second.trade_ongoing) {
				capital += it->second.stocks_held*it->second.buy_price;
			}
		}

		return capital;
	}


	friend ostream &operator<<(ostream &output, const TradeState &state) { 
        output << "TradeState {" << endl;
        output << "  wins: " << state.wins << endl;
        output << "  losses: " << state.losses << endl;
        output << "  OngoingTrades {" << endl;

        for(auto it = state.trade_map.begin(); it!=state.trade_map.end(); it++) {
        	OngoingTrade trade = it->second;
        	if(trade.trade_ongoing) {
        		output << "    OngoingTrade {" << endl;
        		output << "      symbol: " << it->first << endl;
        		output << "      stocks_held: " << trade.stocks_held << endl;
        		output << "      buy_price: " << trade.buy_price << endl;
        		output << "      stop_loss: " << trade.stop_loss << endl;
        		output << "    }" << endl;
        	}
        }

        output << "  }" << endl;
        output << "}" << endl;
        return output;            
    }

    int GetWins() {
    	return wins;
    }

    int GetLosses() {
    	return losses;
    }

private:
	double Sell(double capital, double sell_price, const StockCandle& candle) {
		auto it = trade_map.find(candle.symbol);
		//std::cout << "Sold candle: " << candle;
		capital += it->second.stocks_held*sell_price;
		it->second.trade_ongoing = false;
		it->second.stocks_held = 0;

		if(sell_price > it->second.buy_price) {
			wins++;
		} else {
			losses++;
		}

		//std::cout << "Capital after sell: " << capital << endl;
		return capital;
	}


	double GetStopLoss(const StockCandle& candle, const BacktestCriteria& criteria) {
		if(criteria.marubozu_criteria.enabled) {
			return candle.low;
		}

		return 0;
	}

	/* 
	 * Executes a buy trade on a given candle.
	 *
	 * returns double: capital left after the trade.
	 */
	double Buy(const StockCandle& candle, double capital, const BacktestCriteria& criteria) {
		//std::cout << "Bought candle: " << candle;
		auto it = trade_map.find(candle.symbol);

		/* Setting the buy price. */
		if(criteria.buy_criteria.criteria == BuyCriteria::CLOSE) {
			it->second.buy_price = candle.close;
		} else if(criteria.buy_criteria.criteria == BuyCriteria::HIGH) {
			it->second.buy_price = candle.high;
		} else if(criteria.buy_criteria.criteria == BuyCriteria::MEAN_CLOSE_HIGH) {
			it->second.buy_price = (candle.high + candle.close)/2;
		}

		it->second.stop_loss = GetStopLoss(candle, criteria);

		if(criteria.risk_criteria.enabled) {
			int max_stocks = capital/it->second.buy_price;
			int risk_wise_stocks = (criteria.risk_criteria.risk_percentage*capital)/(it->second.buy_price - it->second.stop_loss);
			if(risk_wise_stocks > max_stocks) {
				risk_wise_stocks = max_stocks;
			}

			it->second.stocks_held = risk_wise_stocks;
		} else {
			it->second.stocks_held = capital/it->second.buy_price;
		}
		
		if(it->second.stocks_held > 0) {
			it->second.trade_ongoing = true;
		} 

		return (capital - it->second.buy_price*it->second.stocks_held);
	}


	std::map<string, OngoingTrade> trade_map;
	double wins, losses;
};

}