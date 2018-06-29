#ifndef STOCK_CANDLE_H
#define STOCK_CANDLE_H

#include <iostream>
#include <ctime>
#include <cmath>

#include "BacktestCriteria.h"

using namespace std;

namespace finance {

const string kDateTimeFormat = "%c";
const double eps = 0.00001;

struct CandleColour {
public:
	enum Colour {
		RED, GREEN
	};

	Colour colour;

	string GetColor() const {
		if(colour == CandleColour::RED) {
			return "RED";
		} else {
			return "GREEN";
		}
	}
};

struct CandleDuration {
public:
	enum Duration {
		DAY
	};

	Duration duration;

	string GetDuration() const {
		return "DAY";
	}
};

class StockCandle {
public:
	StockCandle() {}

	/* 
	 * This constructor should be used to crete the stock candle from Google finance splits.
	 * The order for string variables in the vector should be:
	 * Date, Open, High, Low, Close, Volume
	 */
	StockCandle(const vector<string>& google_finance_splits, const string& symbol, const string& date_format) {
		this->symbol = symbol;
		this->duration.duration = CandleDuration::DAY;
		strptime(google_finance_splits[0].c_str(), date_format.c_str(), &this->close_time);
		this->open = stod(google_finance_splits[1], nullptr);
		this->high = stod(google_finance_splits[2], nullptr);
		this->low = stod(google_finance_splits[3], nullptr);
		this->close = stod(google_finance_splits[4], nullptr);
		this->volume = stod(google_finance_splits[5], nullptr);

		/* Computing other fields. */
		if(this->open > this->close) {
			this->colour.colour = CandleColour::RED;
			this->upper_shadow = (this->high - this->open)/this->open;
			this->lower_shadow = (this->close - this->low)/this->close;
		} else {
			this->colour.colour = CandleColour::GREEN;
			this->upper_shadow = (this->high - this->close)/this->close;
			this->lower_shadow = (this->open - this->low)/this->open;
		}
		this->body = fabs(this->close - this->open)/this->open;
	}

	bool IsMarubozu(MarubozuCriteria criteria) const {
		return ((body > criteria.body_minimum_threshold - eps) &&
			(body < criteria.body_maximum_threshold + eps) &&
			(lower_shadow < criteria.lower_shadow_threshold + eps) &&
			(upper_shadow < criteria.upper_shadow_threshold + eps));
	}

	bool IsBullishMarubozu(MarubozuCriteria criteria) const {
		return ((colour.GetColor() == "GREEN") && IsMarubozu(criteria));
	}

	bool IsBearishMarubozu(MarubozuCriteria criteria) const {
		return ((colour.GetColor() == "RED") && IsMarubozu(criteria));
	}

	string symbol;
	CandleDuration duration;
	tm close_time;

	double open;
	double low;
	double high;
	double close;
	double volume;

	double body;
	double lower_shadow;
	double upper_shadow;
	CandleColour colour;

	double average_volume;
	int average_volume_days;

	friend ostream &operator<<(ostream &output, const StockCandle &candle) { 
        output << "StockCandle {" << endl;
        output << "  symbol: " << candle.symbol << endl;
        output << "  duration: " << candle.duration.GetDuration() << endl;
        output << "  open: " << candle.open << endl;
        output << "  low: " << candle.low << endl;
        output << "  high: " << candle.high << endl;
        output << "  close: " << candle.close << endl;
        output << "  volume: " << candle.volume << endl;
        output << "  colour: " << candle.colour.GetColor() << endl;
        output << "  body: " << candle.body << endl;
        output << "  upper_shadow: " << candle.upper_shadow << endl;
        output << "  lower_shadow: " << candle.lower_shadow << endl;

        char time_format_buffer[32];
        std::strftime(time_format_buffer, 32, kDateTimeFormat.c_str(), &candle.close_time);
        output << "  close_time: " << time_format_buffer << endl;

        output << "}" << endl;
        return output;            
    }
};

}

#endif