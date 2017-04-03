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

struct Arguments{

	vector<string> args;

	unordered_map<string, vector<string>> values;


	Arguments(int argc, char* argv[]){
		for(int i = 1; i < argc; i++){
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

	vector<string> get(string key){
		vector<string> keys = split(key, ',');

		vector<string> values;
		for(string key : keys){
			auto &v = this->values[key];
			values.insert(values.end(), v.begin(), v.end());
		}

		return values;
	}

	string get(string key, int index){
		auto values = get(key);

		if(values.size() > index){
			return values[index];
		}else{
			return string();
		}

	}
};