#include "Serializable.h"
#include "StreamWrapper.h"
#include "CRC32.h"
#include <string>

namespace
{
	// UTF-8 encoding of a wide string. This replaces the deprecated
	// std::wstring_convert/std::codecvt machinery and works identically on
	// Windows (16-bit wchar_t) and macOS (32-bit wchar_t).
	std::string wideToUtf8(const std::wstring& input)
	{
		std::string out;
		out.reserve(input.size());
		for (std::wstring::const_iterator it = input.begin(); it != input.end(); ++it)
		{
			unsigned int cp = (unsigned int)*it;
			if (cp < 0x80)
				out.push_back((char)cp);
			else if (cp < 0x800)
			{
				out.push_back((char)(0xC0 | (cp >> 6)));
				out.push_back((char)(0x80 | (cp & 0x3F)));
			}
			else if (cp < 0x10000)
			{
				out.push_back((char)(0xE0 | (cp >> 12)));
				out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
				out.push_back((char)(0x80 | (cp & 0x3F)));
			}
			else
			{
				out.push_back((char)(0xF0 | (cp >> 18)));
				out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
				out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
				out.push_back((char)(0x80 | (cp & 0x3F)));
			}
		}
		return out;
	}
}

Serializable::Serializable(std::wstring readableName, std::wstring id) :
	name(readableName),
	id(id),
	root(false),
	legacyFixedIndex(-1),
	index(-1)
{
	std::string converted_str = wideToUtf8(id);

	hash = crc32buf((char*)converted_str.c_str(), sizeof(char) * converted_str.length());
}

void Serializable::serialize(Stream* s)
{
	serialize_children(s);
}

void Serializable::serialize_children(Stream* s)
{
	for (auto it = children.begin(); it != children.end(); it++)
	{
		s->writeUInt((*it)->hash);

		int sizePos = s->getPosition();
		s->writeUInt(0);//prepare for param size

		(*it)->serialize(s);

		//Write the size of the serialized parameter so it can be skipped over 
		//incase the plugin loading this data can't find the parameter
		int finalPos = s->getPosition();
		s->setPosition(sizePos);
		s->writeUInt(((finalPos - sizePos) - sizeof(unsigned int)));
		s->setPosition(finalPos);
	}

	//Write a '0' to signal the end of data.
	s->writeUInt(0);
}

void Serializable::deserialize(Stream* s)
{
	unsigned int childID = s->readUInt();
	while (childID != 0)
	{
		unsigned int paramSize = s->readUInt();
		auto it = hashmap.find(childID);
		if (it == hashmap.end())
			s->advance(paramSize); //could not find child
		else
			(*it).second->deserialize(s);

		childID = s->readUInt();
	}
}

void Serializable::legacy_deserialize(Stream* s)
{
	unsigned int childID = s->readUInt();
	while (childID != 0)
	{
		unsigned int paramSize = s->readUInt();
		auto it = hashmap.find(childID);
		if(it == hashmap.end())
			s->advance(paramSize); //could not find child
		else
		{
			//legacyFixedIndex is used for re-assigning note controls later
			if (root)
				(*it).second->legacyFixedIndex = s->readUShort(); 

			(*it).second->root = root;
			(*it).second->legacy_deserialize(s);
		}

		childID = s->readUInt();
	}
}

void Serializable::map(Serializable* child)
{
	all.push_back(child);
	children.push_back(child);
	hashmap[child->hash] = child;
	namemap[child->id] = child;
	for (auto it = child->all.begin(); it != child->all.end(); it++)
	{
		namemap[(*it)->id] = (*it);
		hashmap[(*it)->hash] = (*it);
		all.push_back((*it));
	}	
}

void Serializable::assign_indexes()
{
	for (int i = 0; i < all.size(); i++)
	{
		all[i]->index = i;
		indexmap[all[i]->hash] = i;
	}
}