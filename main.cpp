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
	template<typename PAIR_TYPE>
	struct Iterator
	{
		const Seek* it;
		PAIR_TYPE& operator*()
		{
			return *(it->value);
		}

		PAIR_TYPE* operator->()
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
		for(auto& it: m_budget)
		{
			it = &m_dummyTail;
		}
	}

	HashTable(const HashTable& rhs)
			: HashTable()
	{
		for(auto& it: rhs)
		{
			insert(it.first, it.second);
		}
	}

	HashTable(HashTable&& rhs) noexcept
			: HashTable()
	{
		memcpy(m_budget, rhs.m_budget, sizeof(m_budget));
		m_count = rhs.m_count;
		rhs.m_count = 0;
		m_dummyHead.next = rhs.m_dummyHead.next;
		m_dummyTail.prev = rhs.m_dummyTail.prev;
		m_dummyHead.next->prev = &m_dummyHead;
		m_dummyTail.prev->next = &m_dummyTail;
		for(auto& it: m_budget)
		{
			if(it == &rhs.m_dummyTail)
			{
				it = &m_dummyTail;
			}
		}

		new(&rhs) HashTable{};
	}

	~HashTable()
	{
		Seek* it = m_dummyHead.next;
		while(it != &m_dummyTail)
		{
			Seek* const next = it->next;
			delete it->value;
			delete it;
			it = next;
		}
	}

	void insert(const K& key, const V& value)
	{
		size_t hash = HASHER{}(key) % 521;
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
		if(m_budget[hash] == &m_dummyTail)
		{
			m_budget[hash] = newSeek;
		}
	}

	Iterator<std::pair<const K, V>> erase(const Iterator<std::pair<const K, V>>& it)
	{
		if(it == end())
			return it;

		Seek* prev = it.it->prev;
		Seek* next = it.it->next;
		prev->next = next;
		next->prev = prev;
		m_count -= 1;
		size_t hash = it.it->hash;
		delete it.it->value;
		delete it.it;
		if(m_budget[hash] == it.it)
		{
			m_budget[hash] = &m_dummyTail;
		}

		return Iterator<std::pair<const K, V>>{next};
	}

	Iterator<std::pair<const K, V>> find(const K& key)
	{
		size_t hash = HASHER{}(key) % 521;
		Seek* it = m_budget[hash];
		while(it->hash == hash)
		{
			if(it->value->first == key)
			{
				return Iterator<std::pair<const K, V>>{it};
			}

			it = it->next;
		}

		return end();
	}

	void erase(const K& key)
	{
		size_t hash = HASHER{}(key) % 521;
		Seek* it = m_budget[hash];
		while(it->hash == hash)
		{
			if(it->value->first == key)
			{
				continue;
			}

			it = it->next;
		}

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
				m_budget[hash] = &m_dummyTail;
			}
		}
	}

	[[nodiscard]] V& get(const K& key)
	{
		size_t hash = HASHER{}(key) % 521;
		Seek* it = m_budget[hash];
		while(it->hash == hash)
		{
			if(it->value->first == key)
			{
				return it->value->second;
			}

			it = it->next;
		}

		throw std::out_of_range("can not find item");
	}

	[[nodiscard]] const V& get(const K& key) const
	{

		size_t hash = HASHER{}(key) % 521;
		Seek* it = m_budget[hash];
		while(it->hash == hash)
		{
			if(it->value->first == key)
			{
				return it->value->second;
			}

			it = it->next;
		}

		throw std::out_of_range("can not find item");
	}

	[[nodiscard]] V& operator[](const K& key)
	{
		size_t hash = HASHER{}(key) % 521;
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
		if(m_budget[hash] == &m_dummyTail)
		{
			m_budget[hash] = newSeek;
		}

		return newSeek->value->second;
	}

	Iterator<std::pair<const K, V>> begin() { return Iterator<std::pair<const K, V>>{m_dummyHead.next}; }
	Iterator<std::pair<const K, V>> end() { return Iterator<std::pair<const K, V>>{&m_dummyTail}; }

	[[nodiscard]] Iterator<const std::pair<const K, V>> begin() const { return Iterator<const std::pair<const K, V>>{m_dummyHead.next}; }
	[[nodiscard]] Iterator<const std::pair<const K, V>> end() const { return Iterator<const std::pair<const K, V>>{&m_dummyTail}; }
private:
	Seek m_dummyHead;
	Seek m_dummyTail;
	Seek* m_budget[521];
	size_t m_count;
};

int main() {

	std::unordered_map<std::string, UUID> table5;
	HashTable<std::string, UUID> table;
	table.insert("qaa", {});
	UuidCreate(&table["a"]);
	for(auto& it: table)
	{

	}

	const HashTable<std::string, UUID>& t3 = table;
	const auto& t4 = table5;
	for(auto& it: t4)
	{
	}

	auto it = table.find("a");
	auto t2 = table;
	table.erase(it);
	table.erase("qaa");

	return 0;
}
