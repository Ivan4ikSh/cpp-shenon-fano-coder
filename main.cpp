#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <unordered_map>

using namespace std;
using namespace std::chrono;

class FanoCoder {
public:
	FanoCoder(const string& text) : text_(text) {}

	string Encode() {
		vector<pair<char, double>> probs = InitProps();
		hentropy_ = Hentropy(probs);

		sort(probs.begin(), probs.end(), [](const pair<char, double>& rhs, const pair<char, double>& lhs) {
			return rhs.second != lhs.second ? rhs.second > lhs.second : rhs.first < lhs.first;
			});

		Fano(probs, codes_, "");
		string coded_text;
		for (const auto& letter : text_) {
			coded_text += codes_[letter];
		}

		return coded_text;
	}

	string GetText() {
		return text_;
	}

	unordered_map<char, string> GetCodes() {
		return codes_;
	}
private:
	string text_;
	unordered_map<char, string> codes_;
	double hentropy_ = 0.0;

	double Hentropy(const vector<pair<char, double>>& probs) {
		double h = 0;
		for (const auto& p : probs) {
			h += p.second * log2(p.second);
		}
		h *= -1;
		return h;
	}

	int FindSplitIndex(const vector<pair<char, double>>& probs) {
		double total_sum = accumulate(probs.begin(), probs.end(), 0.0, [](double sum, const pair<char, double>& p) {
			return sum + p.second;
			});

		double left_sum = 0;
		double min_diff = total_sum;
		int split_index = 0;

		for (size_t i = 0; i < probs.size(); ++i) {
			left_sum += probs[i].second;
			double right_sum = total_sum - left_sum;
			double diff = abs(left_sum - right_sum);

			if (diff < min_diff) {
				min_diff = diff;
				split_index = i + 1;
			}
		}

		return split_index;
	}

	void Fano(vector<pair<char, double>>& probs, unordered_map<char, string>& codes, string prefix) {
		if (probs.size() == 1) {
			codes[probs[0].first] = prefix;
			return;
		}
		if (probs.size() == 2) {
			codes[probs[0].first] = prefix + "1";
			codes[probs[1].first] = prefix + "0";
			return;
		}

		size_t split_index = FindSplitIndex(probs);

		vector<pair<char, double>> left(probs.begin(), probs.begin() + split_index);
		vector<pair<char, double>> right(probs.begin() + split_index, probs.end());

		Fano(left, codes, prefix + "1");
		Fano(right, codes, prefix + "0");
	}

	unordered_map<char, int> InitFreqMap() {
		unordered_map<char, int> freq_map;
		for (const auto& letter : text_)
			++freq_map[letter];
		return freq_map;
	}

	vector<pair<char, double>> InitProps() {
		unordered_map<char, int> freq_map = InitFreqMap();
		vector<pair<char, double>> probs;

		for (const auto& [letter, count] : freq_map) {
			double prob = count / static_cast<double>(text_.size());
			probs.push_back({ letter, prob });
		}

		return probs;
	}
};

class FanoDecoder {
public:
	FanoDecoder(const string& text, unordered_map<char, string> codes) : text_(text), codes_(codes) {}
	string Decode() {
		unordered_map<string, char> reverse_codes;
		for (const auto& [symbol, code] : codes_) {
			reverse_codes[code] = symbol;
		}

		string decoded_text;
		string current_code;
		for (const char& bit : text_) {
			current_code += bit;
			if (reverse_codes.find(current_code) != reverse_codes.end()) {
				decoded_text += reverse_codes[current_code];
				current_code.clear();
			}
		}

		return decoded_text;
	}
private:
	string text_;
	unordered_map<char, string> codes_;
};

string ReadFile(const string& file_name) {
	ifstream file(file_name);
	string input_text;
	while (!file.eof()) {
		string s;
		file >> s;
		input_text += s + ' ';
	}
	file.close();
	return input_text.substr(0, input_text.size() - 1);
}

void TEST(const string& coder_file_number) {
	FanoCoder coder(ReadFile("text-to-code/text" + coder_file_number + ".txt"));
	auto c_start = high_resolution_clock::now();
	string encoded_text = coder.Encode();
	auto c_stop = high_resolution_clock::now();
	auto c_duration = duration_cast<milliseconds>(c_stop - c_start);

	ofstream f_out("text-to-decode/text" + coder_file_number + ".txt");
	f_out << encoded_text;
	f_out.close();
	cout << "Время кодирования " + coder_file_number + ": " << c_duration.count() << " милисекунд" << endl;

	unordered_map<char, string> code_map = coder.GetCodes();

	FanoDecoder decoder(ReadFile("text-to-decode/text" + coder_file_number + ".txt"), code_map);
	cout << decoder.Decode() << endl;
}

int main() {
	setlocale(LC_ALL, "rus");
	TEST("1");
	TEST("2");
	//TEST_CODER("3");
	return 0;
}