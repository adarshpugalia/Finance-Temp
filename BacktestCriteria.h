#ifndef BACKTEST_CRITERIA_H
#define BACKTEST_CRITERIA_H

#include <iostream>

namespace finance {
class BaseBacktestCriteria {
public:
	BaseBacktestCriteria() {
		enabled = false;
	}

	bool enabled;
};

/*
 * Criteria for executing stop loss on trades.
 */
class StoplossCriteria: public BaseBacktestCriteria {
public:
	enum Type {
		/* Sell when the low hits the stop loss. */
		LOW,

		/* Sell when the close hits the stop loss. */
		CLOSE
	};

	Type type;
};


/*
 * Volume criteria.
 * 
 * num_days: number of days to calculate the average for.
 * average_volume_threhold: thresold multiplier for average volume. 
 */
class VolumeCriteria: public BaseBacktestCriteria {
public:
	int num_days;
	double average_volume_threshold;
};


/*
 * Exit gain criteria for backtest.
 * gain_percentage: gain percentage when to exit the trade in fraction (0.01 for 1%).
 */
class ExitGainCriteria: public BaseBacktestCriteria {
public:
	double gain_percentage;
};


class MarubozuCriteria: public BaseBacktestCriteria {
public:
	double body_minimum_threshold;
	double body_maximum_threshold;
	double lower_shadow_threshold;
	double upper_shadow_threshold;
};

/*
 * This criteria sets the amount of capital to risk on a single trade.
 * risk_percentage: percentage of capital to risk in fractions (i.e 0.01 for 1%)
 */
class RiskCriteria: public BaseBacktestCriteria {
public:
	double risk_percentage;
};

/*
 * This criteria sets the buying strategy for a candle.
 * CLOSE: Buy the candle at close price.
 * HIGH: Buy the candle at high price.
 * MEAN_CLOSE_HIGH: Buy the candle at the mean of close and high.
 */
class BuyCriteria: public BaseBacktestCriteria {
public:
	enum BuyCriteriaEnum {
		CLOSE, HIGH, MEAN_CLOSE_HIGH
	};

	BuyCriteriaEnum criteria;
};


/*
 * NOTE:
 *	1. Number of days for buy volume criteria and sell volume criteria should be equal.
 */
class BacktestCriteria {
public:
	/* Required criterias to be set. */
	BuyCriteria buy_criteria;
	MarubozuCriteria marubozu_criteria;
	StoplossCriteria stop_loss_criteria;

	/* Optional Criterias. */
	ExitGainCriteria exit_gain_criteria;
	VolumeCriteria buy_volume_criteria;
	VolumeCriteria sell_volume_criteria;
	RiskCriteria risk_criteria;
};
}

#endif