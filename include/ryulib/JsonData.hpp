#pragma once

#include <string>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <ryulib/strg.hpp>
#include <ryulib/disk.hpp>

using namespace std;
using namespace rapidjson;

class JsonData {
public:
	JsonData()
	{
	}

	JsonData(string text)
	{
		json_.Parse(text.c_str());
	}

	void parse(string text)
	{
		json_.Parse(text.c_str());
	}

	void loadFromFile(string filename)
	{
		string text = loadFileAsText(filename);
		int pos_left = text.find("{");
		int pos_right = text.find("}");
		if ((pos_left < 0) || (pos_right < 0)) text = "{}";
		json_.Parse(text.c_str());
	}

	void saveToFile(string filename)
	{
		saveTextToFile(filename, getText());
	}

	string getText() 
	{
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		json_.Accept(writer);
		return buffer.GetString();
	}

	int size() { return json_.MemberCount(); }

	string getName(int index) 
	{ 
		int i = 0;
		for (Value::ConstMemberIterator itr = json_.MemberBegin(); itr != json_.MemberEnd(); ++itr) {
			if (i == index) return itr->name.GetString();
			i++;
		}
		return "";  
	}

	string getString(int index)
	{
		int i = 0;
		for (Value::ConstMemberIterator itr = json_.MemberBegin(); itr != json_.MemberEnd(); ++itr) {
			if (i == index) return itr->value.GetString();
			i++;
		}
		return "";
	}

	string getString(string name) { return json_[name.c_str()].GetString(); }

	int getInt(int index)
	{
		int i = 0;
		for (Value::ConstMemberIterator itr = json_.MemberBegin(); itr != json_.MemberEnd(); ++itr) {
			if (i == index) return itr->value.GetInt();
			i++;
		}
		return 0;
	}

	int getInt(string name) { return json_[name.c_str()].GetInt(); }

	void setString(int index, string str)
	{
		json_[index].SetString(str.c_str(), str.length(), json_.GetAllocator());
	}

	void setString(string name, string str)
	{
		if (json_.HasMember(name.c_str()) == false) {
			Value key(name.c_str(), json_.GetAllocator());
			json_.AddMember(key, "", json_.GetAllocator());
		}
		json_[name.c_str()].SetString(str.c_str(), str.length(), json_.GetAllocator());
	}

	void setInt(int index, int value)
	{
		json_[index].SetInt(value);
	}

	void setInt(string name, int value)
	{
		if (json_.HasMember(name.c_str()) == false) {
			Value key(name.c_str(), json_.GetAllocator());
			json_.AddMember(key, "", json_.GetAllocator());
		}
		json_[name.c_str()].SetInt(value);
	}

private:
	Document json_;
};
