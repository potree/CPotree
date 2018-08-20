#pragma once

#include "StandardIncludes.h"


// http://stackoverflow.com/questions/236129/split-a-string-in-c
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

// http://stackoverflow.com/questions/236129/split-a-string-in-c
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

// http://www.toptip.ca/2010/03/trim-leading-or-trailing-white-spaces.html
string trim(const string& s, string tokens){
	string tmp = s;

	size_t p = s.find_first_not_of(tokens);
	tmp.erase(0, p);

	p = tmp.find_last_not_of(tokens);
	if (string::npos != p)
		tmp.erase(p+1);

	return tmp;
}

// from: http://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
bool endsWith(string const &fullString, string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

// http://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
	string tmp = str;

	if(from.empty()){
		return tmp;
	}

	size_t start_pos = 0;
	while((start_pos = tmp.find(from, start_pos)) != std::string::npos) {
		tmp.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}

	return tmp;
}

string join(vector<string> list, string seperator = string(", ")){

	string result;

	for(size_t i = 0; i < list.size(); i++){
		result += list[i];

		if(i < list.size() - 1){
			result += seperator;
		}
	}

	return result;
}

struct Arguments{

	vector<string> args;

	unordered_map<string, vector<string>> values;


	Arguments(int argc, char* argv[]){
		size_t argc_s(argc);
		for(size_t i = 1; i < argc_s; i++){
			args.push_back(argv[i]);
		}

		string currentKey = "";
		for(string arg : args){
			if(arg[0] == '-'){
				string key = arg;

				int first = key.find_first_not_of("-");
				key = key.substr(first);

				currentKey = key;
				values[currentKey] = {};
			}else{
				values[currentKey].push_back(arg);
			}
		}
	}

	vector<string> keyToKeys(string key){
		vector<string> keys = split(key, ',');
		if(keys.size() == 0){
			keys = {key};
		}

		return keys;
	}

	bool hasKey(string key){
		auto keys = keyToKeys(key);

		for(string k : keys){
			if(values.find(k) != values.end()){
				return true;
			}
		}

		return false;
	}

	vector<string> get(string key){
		auto keys = keyToKeys(key);

		vector<string> values;
		for(string key : keys){
			auto &v = this->values[key];
			values.insert(values.end(), v.begin(), v.end());
		}

		return values;
	}

	string get(string key, size_t index){
		auto values = get(key);

		if(values.size() > index){
			return values[index];
		}else{
			return string();
		}
	}

	string get(string key, size_t index, string defaultVal){
		auto values = get(key);

		if(values.size() > index){
			return values[index];
		}else{
			return defaultVal;
		}
	}

	double getDouble(string key, size_t index){
		auto values = get(key);

		if(values.size() > index){
			double value = std::stod(values[index]);

			return value;
		}else{
			return 0.0;
		}
	}

	int getInt(string key, size_t index){
		auto values = get(key);

		if(values.size() > index){
			int value = std::stoi(values[index]);

			return value;
		}else{
			return 0;
		}
	}

	int getInt(string key, size_t index, int defaultVal){
		auto values = get(key);

		if(values.size() > index){
			int value = std::stoi(values[index]);

			return value;
		}else{
			return defaultVal;
		}
	}

	string toString(){

		string msg = "";

		for(auto item : values){

			msg += item.first + ": [" + join(item.second) + "]\n";

		}

		return msg;
	}
};

class Timer{
public:

	string name;

	std::chrono::steady_clock::time_point begin;
	std::chrono::steady_clock::time_point end;

	Timer(string name){
		this->name = name;
	}

	Timer& start(){
		begin = std::chrono::steady_clock::now();

		return *this;
	}

	Timer& stop(){
		end = std::chrono::steady_clock::now();

		return *this;
	}

	long long getMilli(){
		return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
	}

	long long getMicro(){
		return std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
	}

	double getSeconds(){
		auto micro = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

		auto seconds = double(micro) / 1000000.0;

		return seconds;
	}

};
