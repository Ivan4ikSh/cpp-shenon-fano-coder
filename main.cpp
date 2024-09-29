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

class FanoCodec {
public:
	FanoCodec(const string& file_name) : original_text_(ReadFile(file_name)) {}

	string Encode() {
		vector<pair<char, double>> probs = InitProps();
		hentropy_ = Hentropy(probs);

		sort(probs.begin(), probs.end(), [](const pair<char, double>& rhs, const pair<char, double>& lhs) {
			return rhs.second != lhs.second ? rhs.second > lhs.second : rhs.first < lhs.first;
			});

		FanoRec(probs, codes_, "");
		string coded_text;
		for (const auto& letter : original_text_) {
			coded_text += codes_[letter];
		}

		coded_text_ = coded_text;
		return coded_text;
	}

	string Decode() {
		unordered_map<string, char> reverse_codes;
		for (const auto& [symbol, code] : codes_) {
			reverse_codes[code] = symbol;
		}

		string decoded_text;
		string current_code;
		for (const char& bit : coded_text_) {
			current_code += bit;
			if (reverse_codes.find(current_code) != reverse_codes.end()) {
				decoded_text += reverse_codes[current_code];
				current_code.clear();
			}
		}

		return decoded_text;
	}

	void PrintInfo(ostream& out) {
		out << "Избыточность оригинального текста (%): " << TextRedondance(codes_.size()) * 100 << "\n";
		out << "Избыточность закодированного текста (%): "  << TextRedondance(coded_text_.size()/ static_cast<double>(original_text_.size())) *100 << "\n";
	}

	string GetOriginalText() {
		return original_text_;
	}
private:
	string original_text_;
	string coded_text_;
	unordered_map<char, string> codes_;
	double hentropy_;

	double Hentropy(const vector<pair<char, double>>& probs) {
		double h = 0;
		for (const auto& p : probs) {
			h += p.second * log2(p.second);
		}
		h *= -1;
		return h;
	}

	double TextRedondance(const double& p_byte) {
		return 1 - (hentropy_ / p_byte);
	}

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

	unordered_map<char, int> InitFreqMap(const string& text) {
		unordered_map<char, int> freq_map;
		for (const auto& letter : text)
			++freq_map[letter];
		return freq_map;
	}

	size_t FindSplitIndex(const vector<pair<char, double>>& probs) {
		double total_sum = accumulate(probs.begin(), probs.end(), 0.0, [](double sum, const pair<char, double>& p) {
			return sum + p.second;
			});

		double left_sum = 0;
		double min_diff = total_sum;
		size_t split_index = 0;

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

	void FanoRec(vector<pair<char, double>>& probs, unordered_map<char, string>& codes, string prefix) {
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

		FanoRec(left, codes, prefix + "1");
		FanoRec(right, codes, prefix + "0");
	}

	vector<pair<char, double>> InitProps() {
		unordered_map<char, int> freq_map = InitFreqMap(original_text_);
		vector<pair<char, double>> probs;

		for (const auto& [letter, count] : freq_map) {
			double prob = count / static_cast<double>(original_text_.size());
			probs.push_back({ letter, prob });
		}

		return probs;
	}
};

int main() {
	setlocale(LC_ALL, "rus");

	FanoCodec codec("input.txt");
	fstream out_file("output.txt");

	auto c_start = high_resolution_clock::now();
	string encoded_text = codec.Encode();
	auto c_stop = high_resolution_clock::now();

	auto c_duration = duration_cast<milliseconds>(c_stop - c_start);
	out_file << "Закодированный текст: " << encoded_text << endl;
	out_file << "Время кодирования: " << c_duration.count() << " милисекунд" << endl;

	out_file << "Декодированный текст: " << codec.Decode() << endl;
	out_file << endl;
	codec.PrintInfo(out_file);
	out_file.close();
	return 0;
}

//ИНН 637322757237
//Hello World!
//Lorem ipsum dolor sit amet, consetetur sadipscing elitr