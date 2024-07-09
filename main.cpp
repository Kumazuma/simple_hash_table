#define NOMINMAX
#include <iostream>
#include <string>
#include <array>
#include <iostream>
#include <functional>
#include <windows.h>
#include <unordered_set>
#pragma comment(lib, "Rpcrt4.lib")

template<typename K, typename V, typename HASHER = std::hash<K>, int BUDGET_SIZE = 521>
class HashTable
{
	using pair = std::pair<const K, V>;
	struct Seek
	{
		Seek* prev;
		Seek* next;
		size_t budgetIndex;
		size_t hash;
		pair* value;
	};

	struct Node
	{
		Seek seek;
		pair value;
	};

public:
	static constexpr int budget_size = BUDGET_SIZE;
	template<typename PAIR_TYPE>
	struct Iterator
	{
		Seek* it;
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
			, m_recycleNode()
	{
		m_dummyHead.next = &m_dummyTail;
		m_dummyTail.prev = &m_dummyHead;
		m_dummyTail.budgetIndex = std::numeric_limits<size_t>::max();
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
		m_recycleNode = rhs.m_recycleNode;
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
			it->value->~pair();
			free(it);
			it = next;
		}

		it = m_recycleNode;
		while(it != nullptr)
		{
			Seek* const next = it->next;
			free(it);
			it = next;
		}
	}

	void insert(const K& key, const V& value)
	{
		const size_t hash = HASHER{}(key);
		const size_t budgetIndex = hash % BUDGET_SIZE;
		Seek* it = m_budget[budgetIndex];
		while(it->budgetIndex == budgetIndex)
		{
			if(it->hash == hash) {
				if(it->value->first == key) {
					// 삽입에 실패했다.
					return;
				}
			}

			it = it->next;
		}


		Seek* newSeek = m_recycleNode;
		if(newSeek != nullptr)
		{
			m_recycleNode = newSeek->next;
		}
		else
		{
			Node* newNode = (Node*)malloc(sizeof(Node));
			if (newNode == nullptr)
				throw std::bad_alloc{};

			newSeek = &newNode->seek;
			newSeek->value = &newNode->value;
		}

		new(newSeek->value) pair{key, value};
		newSeek->budgetIndex = budgetIndex;
		newSeek->hash = hash;
		Seek* prev = it->prev;
		newSeek->prev = prev;
		prev->next = newSeek;
		newSeek->next = it;
		it->prev = newSeek;
		m_count += 1;
		if(m_budget[budgetIndex]->budgetIndex != budgetIndex)
		{
			m_budget[budgetIndex] = newSeek;
		}
	}

	Iterator<pair> erase(Iterator<pair> it)
	{
		if(it == end())
			return it;

		Seek* prev = it.it->prev;
		Seek* next = it.it->next;
		prev->next = next;
		next->prev = prev;
		m_count -= 1;
		const size_t budgetIndex = it.it->budgetIndex;
		Seek* dropped = it.it;
		dropped->value->~pair();
		dropped->next = m_recycleNode;
		m_recycleNode = dropped;
		if(m_budget[budgetIndex] == it.it)
		{
			m_budget[budgetIndex] = it.it->next;
		}

		return Iterator<pair>{next};
	}

	Iterator<pair> find(const K& key)
	{
		const size_t hash = HASHER{}(key) % BUDGET_SIZE;
		const size_t budgetIndex = hash % BUDGET_SIZE;
		Seek* it = m_budget[budgetIndex];
		while(it->budgetIndex == budgetIndex)
		{
			if(hash == it->hash) {
				if(it->value->first == key) {
					return Iterator<pair>{it};
				}
			}

			it = it->next;
		}

		return end();
	}

	void erase(const K& key)
	{
		const size_t hash = HASHER{}(key);
		const size_t budgetIndex = hash % BUDGET_SIZE;
		Seek* it = m_budget[budgetIndex];
		Seek* item = nullptr;
		while(it->budgetIndex == budgetIndex)
		{
			if(hash == it->hash) {
				if(it->value->first == key) {
					item = it;
					break;
				}
			}

			it = it->next;
		}

		if(item != nullptr)
		{
			Seek* prev = item->prev;
			Seek* next = item->next;
			prev->next = next;
			next->prev = prev;
			m_count -= 1;
			item->value->~pair();
			item->next = m_recycleNode;
			m_recycleNode = item;
			if(m_budget[budgetIndex] == item)
			{
				m_budget[budgetIndex] = item->next;
			}
		}
	}

	[[nodiscard]] V& get(const K& key)
	{
		const size_t hash = HASHER{}(key);
		const size_t budgetIndex = hash % BUDGET_SIZE;
		Seek* it = m_budget[budgetIndex];
		while(it->budgetIndex == budgetIndex)
		{
			if(hash == it->hash) {
				if(it->value->first == key) {
					return it->value->second;
				}
			}

			it = it->next;
		}

		throw std::out_of_range("can not find item");
	}

	[[nodiscard]] const V& get(const K& key) const
	{

		const size_t hash = HASHER{}(key);
		const size_t budgetIndex = hash % BUDGET_SIZE;
		Seek* it = m_budget[budgetIndex];
		while(it->budgetIndex == budgetIndex)
		{
			if(it->hash == hash) {
				if(it->value->first == key) {
					return it->value->second;
				}
			}

			it = it->next;
		}

		throw std::out_of_range("can not find item");
	}

	[[nodiscard]] V& operator[](const K& key)
	{
		const size_t hash = HASHER{}(key);
		const size_t budgetIndex = hash % BUDGET_SIZE;
		Seek* it = m_budget[budgetIndex];
		while(it->budgetIndex == budgetIndex)
		{
			if(it->hash == hash) {
				if(it->value->first == key) {
					return it->value->second;
				}
			}

			it = it->next;
		}

		Seek* newSeek = m_recycleNode;
		if(newSeek != nullptr)
		{
			m_recycleNode = newSeek->next;
		}
		else
		{
			Node* newNode = (Node*)malloc(sizeof(Node));
			if (newNode == nullptr)
				throw std::bad_alloc{};

			newSeek = &newNode->seek;
			newSeek->value = &newNode->value;
		}

		new(newSeek->value) std::pair<const K, V>{key, {}};
		newSeek->budgetIndex = budgetIndex;
		newSeek->hash = hash;
		Seek* prev = it->prev;
		newSeek->prev = prev;
		prev->next = newSeek;
		newSeek->next = it;
		it->prev = newSeek;
		m_count += 1;
		if(m_budget[budgetIndex] != budgetIndex)
		{
			m_budget[budgetIndex] = newSeek;
		}

		return newSeek->value->second;
	}

	Iterator<pair> begin() { return Iterator<pair>{m_dummyHead.next}; }
	Iterator<pair> end() { return Iterator<pair>{&m_dummyTail}; }

	[[nodiscard]] Iterator<const pair> begin() const { return Iterator<const pair>{m_dummyHead.next}; }
	[[nodiscard]] Iterator<const pair> end() const { return Iterator<const pair>{const_cast<Seek*>(&m_dummyTail)}; }\

	[[nodiscard]] int get_budget_count(int budgetIndex) const {
		int ret = 0;
		Seek* it = m_budget[budgetIndex];
		while(it->budgetIndex == budgetIndex)
		{
			ret += 1;
			it = it->next;
		}

		return ret;
	}

	constexpr int get_budget_size() const
	{
		return BUDGET_SIZE;
	}

private:
	Seek m_dummyHead;
	Seek m_dummyTail;
	Seek* m_recycleNode;
	Seek* m_budget[BUDGET_SIZE];
	size_t m_count;
};

namespace std {
	template<>
	struct hash<UUID> {
		size_t operator()(const UUID& obj) const noexcept {
			// FNV1V
			static constexpr uint32_t prime = 16777619;
			static constexpr uint32_t offsetBasis = 2166136261;
			size_t ret = offsetBasis;
			const uint8_t(&octets)[16] = *reinterpret_cast<const uint8_t(*)[16]>(&obj);
			for(auto octet: octets)
			{
				ret = ret ^ octet;
				ret = ret * prime;
			}

			return ret;
		}
	};
}

int main() {

	HashTable<UUID, std::string, std::hash<UUID>, 32> t;
	std::unordered_set<UUID> t2;
	for(int i = 0; i < 100; ++i)
	{
		UUID uuid{};
		(void)UuidCreate(&uuid);
		RPC_CSTR uuidStr;
		(void)UuidToStringA(&uuid, &uuidStr);
		t.insert(uuid, reinterpret_cast<const char*>(uuidStr));
		t2.insert(uuid);
		RpcStringFreeA(&uuidStr);

	}

	for(int i = 0; i < t.get_budget_size(); ++i)
	{
		std::cout << "[" << i << "] " << t.get_budget_count(i) << std::endl;
	}

	std::cout << '\n';

	for(auto& it: t2)
	{
		auto& s = t.get(it);
		std::cout << s << std::endl;
	}

	std::unordered_map<std::string, UUID> table5;
	HashTable<std::string, UUID, std::hash<std::string>, 31> table;
	table.insert("qaa", {});
	(void)UuidCreate(&table["a"]);
	for(auto& it: table)
	{
		it.second.Data1 = 5;
	}

	const auto& t3 = table;
	const auto& t4 = table5;
	for(auto& it: t4)
	{
		// it.second.Data1 = 5;
	}

	auto it = table.find("a");

	return 0;
}
