#include <iostream>
#include <sstream>
#include <vector>
#include "StockCandle.h"

using namespace std;

namespace finance {

const string kGoogleFinanceDateTimeFormat = "%m/%d/%Y %H:%M:%S";

/*
 * This method returns the stock candles read from a CSV generated from Goolge finance.
 * The format expected for the CSV file is:
 * Date, Open, High, Low, Close, Volume
 *
 * filename: the relative path to the file that contains the stock data.
 */
vector<StockCandle> GetStockCandles(string filename, string stock_symbol, BacktestCriteria criteria) {
	vector<StockCandle> candles;
	std::ifstream file(filename);
	bool first_line_skipped = false;

	string line;
	while(getline(file, line)) {

		/* The first line contains the column headers, so skipping that line. */
		if(!first_line_skipped) {
			first_line_skipped = true;
			continue;
		}

		/* Splitting the read line into columns. */
		vector<string> line_splits;
		stringstream string_stream(line);
		string token;
		while(getline(string_stream, token, ',')) {
			line_splits.push_back(token);
		}


		StockCandle candle = StockCandle(line_splits, stock_symbol, kGoogleFinanceDateTimeFormat);
		candles.push_back(candle);
	}

	file.close();

	/* Computing and setting average volume in candles. */
	double total_volume = 0;
	int index = candles.size() - 1;
	for(int i=0; i<criteria.buy_volume_criteria.num_days && index>=0; i++, index--) {
		total_volume += candles[index].volume;
		candles[index].average_volume = total_volume/(i+1);
		candles[index].average_volume_days = criteria.buy_volume_criteria.num_days;
	}

	while(index >= 0) {
		total_volume -= candles[index+criteria.buy_volume_criteria.num_days].volume;
		total_volume += candles[index].volume;
		candles[index].average_volume = total_volume/criteria.buy_volume_criteria.num_days;
		candles[index].average_volume_days = criteria.buy_volume_criteria.num_days;
		index--;
	}

	return candles;
}

}