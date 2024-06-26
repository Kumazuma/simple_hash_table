#define NOMINMAX
#include <iostream>
#include <string>
#include <array>
#include <iostream>
#include <functional>
#include <windows.h>
#pragma comment(lib, "Rpcrt4.lib")

template<typename K, typename V, typename HASHER = std::hash<K>>
class HashTable {
	struct Seek {
		Seek* prev;
		Seek* next;
		size_t hash;
		std::pair<const K, V>* value;
	};
public:
	struct Iterator
	{
		Seek* it;
		std::pair<const K, V>& operator*()
		{
			return *(it->value);
		}

		std::pair<const K, V>* operator->()
		{
			return it->value;
		}

		Iterator& operator++()
		{
			it = it->next;
			return *this;
		}

		Iterator& operator--()
		{
			it = it->prev;
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator ret{*this};
			++ret;
			return ret;
		}

		Iterator operator--(int)
		{
			Iterator ret{*this};
			--ret;
			return ret;
		}
		bool operator==(const Iterator& rhs) const
		{
			return it == rhs.it;
		}

		bool operator!=(const Iterator& rhs) const
		{
			return it != rhs.it;
		}
	};

	HashTable()
			: m_dummyHead()
			, m_dummyTail()
			, m_budget()
			, m_count()
	{
		m_dummyHead.next = &m_dummyTail;
		m_dummyTail.prev = &m_dummyHead;
		m_dummyTail.hash = std::numeric_limits<size_t>::max();
	}

	void insert(const K& key, const V& value)
	{
		size_t hash = HASHER{}(key) % 521;

		if(m_budget[hash] == nullptr)
		{
			Seek* newSeek = new Seek{};
			newSeek->value = new std::pair<const K, V>{key, value};
			newSeek->hash = hash;
			Seek* const old_tail = m_dummyTail.prev;
			old_tail->next = newSeek;
			newSeek->prev = old_tail;
			newSeek->next = &m_dummyTail;
			m_dummyTail.prev = newSeek;
			m_budget[hash] = newSeek;
			m_count += 1;
			return;
		}

		Seek* it = m_budget[hash];
		while(it->hash == hash)
		{
			if(it->value->first == key)
			{
				// 삽입에 실패했다.
				return;
			}

			it = it->next;
		}

		Seek* newSeek = new Seek{};
		newSeek->value = new std::pair<const K, V>{key, value};
		newSeek->hash = hash;
		Seek* prev = it->prev;
		newSeek->prev = prev;
		prev->next = newSeek;
		newSeek->next = it;
		it->prev = newSeek;
		m_count += 1;
	}

	Iterator erase(const Iterator& it)
	{
		if(it == end())
			return it;

		Seek* prev = it.it->prev;
		Seek* next = it.it->next;
		prev->next = next;
		next->prev = prev;
		m_count -= 1;
		size_t hash = it.it->hash;
		delete it->value;
		delete it;
		if(m_budget[hash] == it)
		{
			m_budget[hash] = nullptr;
		}

		return Iterator{next};
	}

	template<typename U, typename U_HASHER = std::hash<U>>
	Iterator find(const U& key)
	{
		size_t hash = U_HASHER{}(key) % 521;
		if(m_budget[hash] == nullptr)
		{
			return end();
		}

		Seek* it = m_budget[hash];
		while(it->hash == hash)
		{
			if(it->value->first == key)
			{
				return Iterator{it};
			}

			it = it->next;
		}

		return end();
	}

	void erase(const K& key)
	{
		size_t hash = HASHER{}(key) % 521;
		if(m_budget[hash] == nullptr)
		{
			return;
		}

		Seek* it = m_budget[hash];
		while(it->hash == hash)
		{
			if(it->value->first == key)
			{
				Seek* prev = it->prev;
				Seek* next = it->next;
				prev->next = next;
				next->prev = prev;
				m_count -= 1;
				delete it->value;
				delete it;
				if(m_budget[hash] == it)
				{
					m_budget[hash] = nullptr;
				}

				return;
			}

			it = it->next;
		}
	}

	V& operator[](const K& key)
	{
		size_t hash = HASHER{}(key) % 521;
		if(m_budget[hash] == nullptr)
		{
			Seek* newSeek = new Seek{};
			newSeek->value = new std::pair<const K, V>{key, {}};
			newSeek->hash = hash;
			Seek* const old_tail = m_dummyTail.prev;
			old_tail->next = newSeek;
			newSeek->prev = old_tail;
			newSeek->next = &m_dummyTail;
			m_dummyTail.prev = newSeek;
			m_budget[hash] = newSeek;
			m_count += 1;
			return newSeek->value->second;
		}

		Seek* it = m_budget[hash];
		while(it->hash == hash)
		{
			if(it->value->first == key)
			{
				return it->value->second;
			}

			it = it->next;
		}

		Seek* newSeek = new Seek{};
		newSeek->value = new std::pair<const K, V>{key, {}};
		newSeek->hash = hash;
		Seek* prev = it->prev;
		newSeek->prev = prev;
		prev->next = newSeek;
		newSeek->next = it;
		it->prev = newSeek;
		m_count += 1;
		return newSeek->value->second;
	}

	Iterator begin() { return Iterator{m_dummyHead.next}; }
	Iterator end() { return Iterator{&m_dummyTail}; }

private:
	Seek m_dummyHead;
	Seek m_dummyTail;
	Seek* m_budget[521];
	size_t m_count;
};

int main() {

	HashTable<std::string, UUID> table;
	table.insert("qaa", {});
	UuidCreate(&table["a"]);
	for(auto& it: table)
	{

	}

	auto it = table.find(std::string_view{"a"});

	return 0;
}
