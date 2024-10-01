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

	void Encode(const string& output_file_name, const string& code_map_file_name) {
		vector<pair<char, double>> probs = InitProps();

		sort(probs.begin(), probs.end(), [](const pair<char, double>& rhs, const pair<char, double>& lhs) {
			return rhs.second != lhs.second ? rhs.second > lhs.second : rhs.first < lhs.first;
			});

		Fano(probs, codes_, "");

		InitCodeMapFile(code_map_file_name);
		InitCodedTextFile(output_file_name);		
	}

	string GetText() {
		return text_;
	}
	string GetEncodedText() {
		return coded_text_;
	}

	unordered_map<char, string> GetCodes() {
		return codes_;
	}

	double CalculateHentropy() {
		vector<pair<char, double>> probs = InitProps();
		return Hentropy(probs);
	}
private:
	string text_;
	string coded_text_;
	unordered_map<char, string> codes_;

	void InitCodedTextFile(const string& output_file_name) {
		uint8_t buffer = 0;
		int bit_count = 0;
		ofstream file_out(output_file_name);
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
		file_out.close();
	}

	void InitCodeMapFile(const string& code_map_file_name) {
		string coded_text;
		ofstream file_codes_out(code_map_file_name);
		for (const auto& [letter, code] : codes_) {
			file_codes_out << "'" << letter << "' " << code << endl;
		}
		file_codes_out.close();
	}

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

void LogFile(const string& file_to_log, FanoCoder coder) {
	auto c_start = high_resolution_clock::now();
	//coder.Encode("coder-files/coded/" + file_to_log);
	auto c_stop = high_resolution_clock::now();
	auto c_duration = duration_cast<milliseconds>(c_stop - c_start);
	
	ofstream f_out("logs/log-" + file_to_log);
	f_out << "Время кодирования " + file_to_log + ": " << c_duration.count() << " миллисекунд" << endl;

	string encoded_text = coder.GetEncodedText();
	unordered_map<char, string> code_map = coder.GetCodes();
	FanoDecoder decoder(encoded_text, code_map);
	string decoded_text = decoder.Decode();

	if (coder.GetText() == decoded_text) f_out << "Декодирование успешно: тексты совпадают." << endl;
	else f_out << "Ошибка декодирования: тексты не совпадают!" << endl;

	size_t encoded_length_in_bits = encoded_text.size();
	size_t original_length_in_bits = coder.GetText().size() * 8;

	double compression_ratio = static_cast<double>(encoded_length_in_bits) / original_length_in_bits;
	f_out << "Длина закодированного сообщения в битах: " << encoded_length_in_bits << endl;
	f_out << "Коэффициент сжатия: " << compression_ratio << endl;

	double hentropy = coder.CalculateHentropy();
	f_out << "Энтропия: " << hentropy << " бит на символ" << endl;

	double total_cost = hentropy * coder.GetText().size();
	f_out << "Общая стоимость кодирования: " << total_cost << " бит" << endl << endl;

	f_out.close();
}

void Code(const string& file_name, Commands log) {
	FanoCoder coder(ReadFile("coder-files/original/text-" + file_name + ".txt"));
	coder.Encode("coder-files/coded/code-" + file_name + ".txt", "coder-files/code-maps/code-map-" + file_name + ".txt");
	if (log == LOG) {
		LogFile("coder-files/logs/log-" + file_name + ".txt", coder);
	}
}

unordered_map<char, string> ReadCodes(const string& file_name) {
	ifstream file(file_name);
	unordered_map<char, string> code_map;
	string input_text;
	while (!file.eof()) {
		string letter_s;
		string code;
		file >> letter_s >> code;
		char letter_c = letter_s[1] == '\0' ? ' ' : letter_s[1];
		code_map.insert({ letter_c, code });
	}
	file.close();
	return code_map;
}

void Decode(const string& file_name, Commands log) {
	FanoDecoder coder(ReadFile("coder-files/code/code-" + file_name + ".txt"), ReadCodes("coder-files/code-maps/code-map-" + file_name + ".txt"));
}

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "rus");
	
	Commands code_decode = CODE;
	Commands log = UNLOG;
	if (InitComands(code_decode, log, argc, argv) == 1) return 1;

	switch (code_decode) {
	case CODE:
		Code(argv[3], log);
		break;
	case DECODE:
		Decode(argv[3], log);
		break;
	default:
		break;
	}
	cout << "Done!" << endl;
	return 0;
}