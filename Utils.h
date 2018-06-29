#ifndef UTILS_H
#define UTILS_H

#include <iostream>

void PrintTimeString(tm time_struct, const string& date_time_format) {
	char time_format_buffer[32];
    std::strftime(time_format_buffer, 32, date_time_format.c_str(), &time_struct);
    std::cout << time_format_buffer;
}

#endif