#include <iostream>
#include <fstream>
#include <ctime>
#include <time.h>
#include <algorithm> 

#include "Constants.h"
#include "GoogleFinanceDataReader.h"
#include "TradeState.h"
#include "Utils.h"

using namespace std;

typedef std::pair<tm, std::vector< ::finance::StockCandle> > StockCandlesForTimestamp;

double GetCagr(tm start_time_struct, tm end_time_struct, 
	double initial_capital, double final_capital) {
	time_t start_time = mktime(&start_time_struct);
	time_t end_time = mktime(&end_time_struct);

	double years = ((double) end_time - start_time)/(60*60*24*365);
	return (pow((final_capital/initial_capital), (1.0/years)) - 1)*100;
}

double Backtest(const vector<StockCandlesForTimestamp>& merged_candles, 
	const string& start_time_string, const string& date_time_format, 
	double capital, ::finance::BacktestCriteria criteria) {
	double initial_capital = capital;

	/* Getting the date time for the start. */
	tm start_time_struct;
	strptime(start_time_string.c_str(), date_time_format.c_str(), &start_time_struct);
	time_t start_time = mktime(&start_time_struct);

	/* Skipping the timestamps before the start date. */
	int index = merged_candles.size() - 1;
	while(index >= 0) {
		tm current_time_struct = merged_candles[index].first;
		time_t current_time = mktime(&current_time_struct);
		if(current_time >= start_time) {
			break;
		}

		index--;
	}

	::finance::TradeState state;
	while(index >= 0) {
		for(const ::finance::StockCandle& candle: merged_candles[index].second) {
			capital = state.SellIfFitsCriteria(candle, capital, criteria);
			capital = state.BuyIfFitsCriteria(candle, capital, criteria);
		}

		index--;
	}

	capital = state.GetFinalCapital(capital);
	std::cout << "Exit gain: " << criteria.exit_gain_criteria.gain_percentage 
		<< " Final capital: " << ((long long) capital) 
		<< " Wins: " << state.GetWins() 
		<< " Losses: " << state.GetLosses() 
		<< " CAGR: " << GetCagr(start_time_struct, merged_candles[0].first, 
			initial_capital, capital) << endl;
	return capital;
}

vector<StockCandlesForTimestamp> MergeStockCandles(std::vector<std::vector< ::finance::StockCandle> > multiple_stock_candles) {
	vector<StockCandlesForTimestamp> merged_candles;

	vector<int> indexes;
	for(int i=0; i<multiple_stock_candles.size(); i++) {
		indexes.push_back(multiple_stock_candles[i].size() - 1);
	}

	while(1) {
		int start_index = -1;
		for(int i=0; i<indexes.size(); i++) {
			if(indexes[i] >= 0) {
				start_index = i;
				break;
			}
		}

		if(start_index == -1) {
			break;
		}

		tm minimum_time_struct = multiple_stock_candles[start_index][indexes[start_index]].close_time;
		time_t minimum_time = mktime(&multiple_stock_candles[start_index][indexes[start_index]].close_time);
		for(int i=start_index; i<indexes.size(); i++) {
			if(indexes[i] >= 0) {
				time_t current_time = mktime(&multiple_stock_candles[i][indexes[i]].close_time);
				if(current_time < minimum_time) {
					minimum_time_struct = multiple_stock_candles[i][indexes[i]].close_time;
					minimum_time = current_time;
				}
			}
		}

		StockCandlesForTimestamp candles_for_timestamp;
		candles_for_timestamp.first = minimum_time_struct;
		vector< ::finance::StockCandle> candles;

		for(int i=start_index; i<indexes.size(); i++) {
			if(indexes[i] >= 0) {
				time_t current_time = mktime(&multiple_stock_candles[i][indexes[i]].close_time);
				if(current_time == minimum_time) {
					candles.push_back(multiple_stock_candles[i][indexes[i]]);
					indexes[i]--;
				}
			}
		}
		candles_for_timestamp.second = candles;
		merged_candles.push_back(candles_for_timestamp);
	}

	std::reverse(merged_candles.begin(), merged_candles.end());
	return merged_candles;
} 

int main() {
	::finance::BacktestCriteria criteria;

	/* Setting the buy criteria. */
	criteria.buy_criteria.enabled = true;
	criteria.buy_criteria.criteria = ::finance::BuyCriteria::HIGH;

	/* Setting the marubozu criteria. */
	criteria.marubozu_criteria.enabled = true;
	criteria.marubozu_criteria.body_minimum_threshold = 0.01;
	criteria.marubozu_criteria.body_maximum_threshold = 0.1;
	criteria.marubozu_criteria.lower_shadow_threshold = 0.003;
	criteria.marubozu_criteria.upper_shadow_threshold = 0.003;

	/* Setting the stop loss criteria. */
	criteria.stop_loss_criteria.enabled = true;
	criteria.stop_loss_criteria.type = ::finance::StoplossCriteria::LOW;

	/* Setting the exit gain criteria. */
	criteria.exit_gain_criteria.enabled = true;
	criteria.exit_gain_criteria.gain_percentage = 0.18;

	/* Setting the buy volume criteria. */
	criteria.buy_volume_criteria.enabled = true;
	criteria.buy_volume_criteria.num_days = 10;
	criteria.buy_volume_criteria.average_volume_threshold = 1;

	/* Setting the sell volume criteria. */
	criteria.sell_volume_criteria.enabled = false;

	/* Setting the risk criteria. */
	criteria.risk_criteria.enabled = false;
	criteria.risk_criteria.risk_percentage = 0.04;

	std::vector<std::vector< ::finance::StockCandle> > multiple_stock_candles;
	for(string stock: ::finance::constants::kNifty50) {
		std::vector< ::finance::StockCandle> candles = 
		::finance::GetStockCandles("Data/Stock_OLHC/" + stock + ".csv", 
			stock, criteria);
		multiple_stock_candles.push_back(candles);
	}
	
	for(int i=4; i <= 20; i++) {
		criteria.exit_gain_criteria.gain_percentage = ((double)i)/100;
		Backtest(MergeStockCandles(multiple_stock_candles), "6/22/2008 15:30:00", 
			::finance::kGoogleFinanceDateTimeFormat, 100000, criteria);
	}

	return 0;
}