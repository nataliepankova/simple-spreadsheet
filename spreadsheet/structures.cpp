#include "common.h"

#include <array>
#include <cctype>
#include <cmath>
#include <sstream>
#include <vector>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = { -1, -1 };

bool Position::operator==(const Position rhs) const {
	return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
	return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
	return row >= 0 && col >= 0 && row < Position::MAX_ROWS && col < Position::MAX_COLS;
}

std::string Position::ToString() const {
	std::string result = "";
	if (!IsValid()) {
		return result;
	}
	int place = col / 26;
	int units = col % 26;
	if (place == 0) {
		 result += units + 65;
	}
	if (result.empty()) {
		std::vector<int>values;
		for (; place > 0; place = units / 26) {
			if (place > 26) {
				values.push_back(units + 65);
				units = place;
				continue;
			}
			result += place - 1 + 65;
			if (units > 26) {
				result += units % 26 + 65 - 1;
			}
			else {
				result += units + 65;
			}
			if (!values.empty()) {
				for (size_t i = 0; i < values.size(); ++i) {
					result += values[values.size() - 1];
				}
			}
			units = place % 26;
		}
	}
 	result += std::to_string(row + 1);
	return result;
}

Position Position::FromString(std::string_view str) {
	// check first char to be an uppercase letter
	if (str.empty() || !std::isupper(static_cast<unsigned char>(str[0])) || str.size() < 2) {
		return Position::NONE;
	}
	bool is_wrong_format = false;
	bool is_prev_symb_digit = false;
	std::string col_index;
	std::string row_index;
	for (const char c : str) {
		if (std::isupper(static_cast<unsigned char>(c))) {
			col_index += c;
			if (is_prev_symb_digit) {
				is_wrong_format = true;
				break;
			}
		}
		else if (std::isdigit(static_cast<unsigned char>(c))) {
			row_index += c;
			is_prev_symb_digit = true;
		}
		else {
			is_wrong_format = true;
			break;
		}
	}
	if (is_wrong_format || col_index.size() > MAX_POS_LETTER_COUNT || col_index.size() + row_index.size() > MAX_POSITION_LENGTH) {
		return Position::NONE;
	}
	Position result;
	result.row = std::stoi(row_index) - 1;
	if (!result.IsValid()) {
		return Position::NONE;
	}
	for (size_t i = 0; i < col_index.size(); ++i) {
		int value = static_cast<int>(std::pow(26, col_index.size() - 1 - i))  * (col_index[i] - 65 + 1);
		if (result.col == 0) {
			--value;
		}
		result.col += value;
	}
	if (!result.IsValid()) {
		return Position::NONE;
	}
	return result;

}

bool Size::operator==(Size rhs) const {
	return  rows == rhs.rows && cols == rhs.cols;
}
