#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <unordered_map>
#include <stack>

using namespace std;
using namespace std::chrono;

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

class FanoCoder {
public:
	FanoCoder(const string& file_name) : text_(ReadFile(file_name)) {
		vector<pair<char, double>> probs = InitProps();
		sort(probs.begin(), probs.end(), [](const pair<char, double>& rhs, const pair<char, double>& lhs) {
			return rhs.second != lhs.second ? rhs.second > lhs.second : rhs.first < lhs.first;
			});

		Fano(probs, codes_);
	}

	void Encode(const string& file_name) {
		WriteEncodedWithCodeMap(file_name);
	}

	string GetText() const {
		return text_;
	}

	unordered_map<char, string> GetCodes() const {
		return codes_;
	}

	string GetEncodedText() const {
		return coded_text_;
	}

	double CalculateHentropy() {
		vector<pair<char, double>> probs = InitProps();
		return Hentropy(probs);
	}

private:
	string text_;
	string coded_text_;
	unordered_map<char, string> codes_;

	vector<pair<char, double>> InitProps() {
		unordered_map<char, int> freq_map;
		for (const auto& letter : text_)
			++freq_map[letter];

		vector<pair<char, double>> probs;
		for (const auto& [letter, count] : freq_map) {
			double prob = count / static_cast<double>(text_.size());
			probs.push_back({ letter, prob });
		}

		return probs;
	}

	void Fano(vector<pair<char, double>>& probs, unordered_map<char, string>& codes) {
		stack<pair<vector<pair<char, double>>, string>> s;
		s.push({ probs, "" });

		while (!s.empty()) {
			auto [current_probs, prefix] = s.top();
			s.pop();

			if (current_probs.size() == 1) {
				codes[current_probs[0].first] = prefix;
				continue;
			}

			if (current_probs.size() == 2) {
				codes[current_probs[0].first] = prefix + "1";
				codes[current_probs[1].first] = prefix + "0";
				continue;
			}

			size_t split_index = FindSplitIndex(current_probs);
			vector<pair<char, double>> left(current_probs.begin(), current_probs.begin() + split_index);
			vector<pair<char, double>> right(current_probs.begin() + split_index, current_probs.end());

			s.push({ right, prefix + "0" });
			s.push({ left, prefix + "1" });
		}
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

	double Hentropy(const vector<pair<char, double>>& probs) {
		double h = 0;
		for (const auto& p : probs) {
			h += p.second * log2(p.second);
		}
		h *= -1;
		return h;
	}

	void WriteEncodedWithCodeMap(const string& file_name) {
		ofstream file_out(file_name, ios::binary);

		uint64_t text_length = text_.size();
		file_out.write(reinterpret_cast<const char*>(&text_length), sizeof(text_length));

		uint64_t map_size = codes_.size();
		file_out.write(reinterpret_cast<const char*>(&map_size), sizeof(map_size));

		for (const auto& [letter, code] : codes_) {
			file_out.put(letter);
			uint8_t code_length = code.size();
			file_out.write(reinterpret_cast<const char*>(&code_length), sizeof(code_length));

			uint8_t buffer = 0;
			int bit_count = 0;
			for (char bit : code) {
				if (bit == '1') {
					buffer |= (1 << (7 - bit_count));
				}
				++bit_count;
				if (bit_count == 8) {
					file_out.put(buffer);
					buffer = 0;
					bit_count = 0;
				}
			}
			if (bit_count != 0) {
				file_out.put(buffer);
			}
		}

		coded_text_.clear();
		for (const auto& ch : text_) {
			coded_text_ += codes_[ch];
		}

		uint8_t buffer = 0;
		int bit_count = 0;

		for (const auto& bit : coded_text_) {
			if (bit == '1') {
				buffer |= (1 << (7 - bit_count));
			}
			++bit_count;

			if (bit_count == 8) {
				file_out.put(buffer);
				buffer = 0;
				bit_count = 0;
			}
		}
		if (bit_count != 0) {
			file_out.put(buffer);
		}

		file_out.close();
	}

};

class FanoDecoder {
public:
	FanoDecoder(const string& file_name) {
		ReadEncodedFile(file_name);
	}

	void Decode(const string& file_name) {
		ofstream file_out(file_name);
		file_out << encoded_text_;
		file_out.close();
	}

	string GetDecodedText() {
		return encoded_text_;
	}
private:
	unordered_map<char, string> codes_;
	string encoded_text_;

	void ReadCodesMap(ifstream& file_in, const int map_size) {
		for (uint16_t i = 0; i < map_size; ++i) {
			char symbol;
			file_in.get(symbol);

			uint8_t code_length = 0;
			file_in.read(reinterpret_cast<char*>(&code_length), sizeof(code_length));

			string code;
			uint8_t buffer = 0;
			int bit_count = 0;
			for (int j = 0; j < code_length; ++j) {
				if (bit_count == 0) {
					file_in.get(reinterpret_cast<char&>(buffer));
					bit_count = 8;
				}

				code += (buffer & (1 << (7 - (8 - bit_count)))) ? '1' : '0';
				--bit_count;
			}

			codes_[symbol] = code;
		}
	}

	unordered_map<string, char> ReverseCodes() {
		unordered_map<string, char> reverse_codes;
		for (const auto& [symbol, code] : codes_) {
			reverse_codes[code] = symbol;
		}
		return reverse_codes;
	}

	void ReadEncodedFile(const string& file_name) {
		ifstream file_in(file_name, ios::binary);
		
		uint64_t text_length = 0;
		file_in.read(reinterpret_cast<char*>(&text_length), sizeof(text_length));

		uint64_t map_size = 0;
		file_in.read(reinterpret_cast<char*>(&map_size), sizeof(map_size));

		ReadCodesMap(file_in, map_size);
		unordered_map<string, char> reverse_codes = ReverseCodes();

		string decoded_text;
		string current_code;
		uint8_t buffer = 0;

		int bits_written = 0;

		while (file_in.get(reinterpret_cast<char&>(buffer))) {
			if (decoded_text.size() == text_length) break;
			for (int i = 7; i >= 0; --i) {
				if (decoded_text.size() == text_length) break;
				char bit = (buffer & (1 << i)) ? '1' : '0';
				current_code += bit;

				if (reverse_codes.find(current_code) != reverse_codes.end()) {
					decoded_text += reverse_codes[current_code];
					current_code.clear();
				}

				bits_written++;
			}
		}
		encoded_text_ = decoded_text;
		file_in.close();
	}
};

//main.exe -c -u <num>
//main.exe -c -l <num>
//main.exe -d -u <num>
//main.exe -d -l <num>
enum Commands {
	CODE, DECODE, LOG, UNLOG
};

int InitComands(Commands& code_decode, Commands& log, int argc, char* argv[]) {
	if (argc != 4) {
		cerr << "Usage: <-c/-d> <-l/-u> <FileName>" << endl;
		return 1;
	}

	if (argv[1][0] == '-' && argv[1][1] == 'c') {
		code_decode = CODE;
	}
	else if (argv[1][0] == '-' && argv[1][1] == 'd') {
		code_decode = DECODE;
	}
	else {
		cerr << "Usage: <-c/-d> <-l/-u> <FileName>" << endl;
		return 1;
	}

	if (argv[2][0] == '-' && argv[2][1] == 'l') {
		log = LOG;
	}
	else if (argv[2][0] == '-' && argv[2][1] == 'u') {
		log = UNLOG;
	}
	else {
		cerr << "Usage: <-c/-d> <-l/-u> <FileName>" << endl;
		return 1;
	}
}

void LogCoderFile(const string& file_name, FanoCoder coder, int duration) {
	ofstream f_out("coder-files/logs/log-" + file_name + ".txt");
	f_out << "Время кодирования " + file_name + ": " << duration << " миллисекунд" << endl;

	string encoded_text = coder.GetEncodedText();
	unordered_map<char, string> code_map = coder.GetCodes();
	FanoDecoder decoder("coder-files/coded/code-" + file_name + ".txt");
	string decoded_text = decoder.GetDecodedText();

	if (coder.GetText() == decoded_text) f_out << "Декодирование успешно: тексты совпадают." << endl;
	else f_out << "Ошибка декодирования: тексты не совпадают!" << endl;

	size_t encoded_length_in_bits = encoded_text.size();
	size_t original_length_in_bits = coder.GetText().size() * 8;

	double compression_ratio = static_cast<double>(encoded_length_in_bits) / original_length_in_bits;
	f_out << "Длина закодированного сообщения в битах: " << encoded_length_in_bits << endl;
	f_out << "Коэффициент сжатия: " << compression_ratio << endl;

	double hentropy = coder.CalculateHentropy();
	f_out << "Энтропия: " << hentropy << " бит на символ" << endl;

	f_out.close();
}

void LogDecoderFile(const string& file_name, FanoDecoder decoder) {
	auto c_start = high_resolution_clock::now();
	decoder.Decode("coder-files/coded/code-" + file_name + ".txt");
	auto c_stop = high_resolution_clock::now();
	auto c_duration = duration_cast<milliseconds>(c_stop - c_start);

	ofstream f_out("coder-files/logs/log-" + file_name + ".txt");
	f_out << "Время декодирования " + file_name + ": " << c_duration.count() << " миллисекунд" << endl;
	f_out.close();
}

void Code(const string& file_name, Commands log) {	
	FanoCoder coder("coder-files/original/text-" + file_name + ".txt");
	auto c_start = high_resolution_clock::now();
	coder.Encode("coder-files/coded/code-" + file_name + ".txt");
	auto c_stop = high_resolution_clock::now();
	auto c_duration = duration_cast<milliseconds>(c_stop - c_start);
	
	if (log == LOG) LogCoderFile(file_name, coder, c_duration.count());
}

void Decode(const string& file_name, Commands log) {
	FanoDecoder decoder("coder-files/coded/code-" + file_name + ".txt");
	decoder.Decode("coder-files/original/text-" + file_name + ".txt");
	if (log == LOG) LogDecoderFile(file_name, decoder);
}

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "rus");
	
	Commands code_decode = CODE;
	Commands log = UNLOG;
	if (InitComands(code_decode, log, argc, argv) == 1) return 1;

	string file_name = argv[3];
	switch (code_decode) {
	case CODE:
		Code(file_name, log);
		cout << "File coded succesful!" << endl;
		cout << "Coded file is located at coder-files/coded/code-" + file_name + ".txt" << endl;
		break;
	case DECODE:
		Decode(file_name, log);
		cout << "File decoded succesful!" << endl;
		cout << "Coded file is located at coder-files/original/text-" + file_name + ".txt" << endl;
		break;
	default:
		break;
	}
	if (log == LOG) {
		cout << "Log file is located at coder-files/logs/log-" + file_name + ".txt" << endl;
	}
	return 0;
}