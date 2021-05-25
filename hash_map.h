#pragma once
#include <stdexcept>
#include <utility>
#include <vector>

template <typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>>
class HashMap {
private:
    struct Bonds {
        std::vector<size_t> backward;
        std::vector<size_t> forward;

        void resize(size_t new_size) {
            backward.resize(new_size);
            forward.resize(new_size);
        }

        bool operator==(const Bonds& other) {
            return backward == other.backward && forward == other.forward;
        }

        bool operator!=(const Bonds& other) {
            return !(*this == other);
        }

        void swap(Bonds& other) {
            backward.swap(other.backward);
            forward.swap(other.forward);
        }
    };

    struct Node {
        std::pair<KeyType, ValueType> cell;
        bool has_created, has_deleted;

        Node() {
            has_created = false;
            has_deleted = false;
        }

        Node(const KeyType& key, const ValueType& value) : cell(std::make_pair(key, value)) {
            has_created = true;
            has_deleted = false;
        }

        Node(const std::pair<KeyType, ValueType>& cell_) : cell(cell_) {
            has_created = true;
            has_deleted = false;
        }

        const KeyType& get_key() const {
            return cell.first;
        }

        std::pair<KeyType, ValueType>& get_pair() {
            return cell;
        }

        const std::pair<KeyType, ValueType>& get_pair() const {
            return cell;
        }
    };

    size_t inc(size_t x) const {
        x++;
        if (x == table_size) {
            x = 0;
        }
        return x;
    }

    size_t get_hash(const KeyType& key) const {
        return hasher(key) % table_size;
    }

    // https://google.github.io/styleguide/cppguide.html#Constant_Names
    static const size_t kOriginalSize = 8, kOriginalCapacity = 6;
    std::vector<Node> hash_table;
    Bonds bonds;
    size_t pos_begin, pos_end;
    // Может const?
    Hash hasher;
    size_t table_size;
    size_t table_capacity;
    size_t cnt_deleted;
    size_t cnt_non_deleted;

    void rehash(long double size_changing_coef) {
        HashMap<KeyType, ValueType, Hash> new_map(table_size * size_changing_coef, hasher);
        new_map.swap(*this);
        for (auto& it : new_map) {
            insert(it);
        }
    }

    void insert_iterator(size_t pos) {
        if (cnt_non_deleted == 0) {
            pos_begin = pos;
            bonds.forward[pos] = pos_end;
            bonds.backward[pos_end] = pos;
        } else {
            int last = bonds.backward[pos_end];
            bonds.forward[last] = pos;
            bonds.backward[pos] = last;
            bonds.forward[pos] = pos_end;
            bonds.backward[pos_end] = pos;
        }
    }

    void erase_iterator(size_t pos) {
        if (pos == pos_begin) {
            pos_begin = bonds.forward[pos];
        } else {
            int prv = bonds.backward[pos], nxt = bonds.forward[pos];
            bonds.forward[prv] = nxt;
            bonds.backward[nxt] = prv;
        }
    }

    template<class TableType = std::vector<Node>, class BondsType = Bonds>
    class iterator_base {
    protected:
        TableType* table;
        BondsType* bonds;
        size_t pos;
    
    public:
        iterator_base() {
            table = nullptr;
            bonds = nullptr;
            pos = 0;
        }

        iterator_base(TableType& table, BondsType& bonds, size_t pos):
            table(&table), bonds(&bonds), pos(pos) {}

        iterator_base(const iterator_base& other) {
            table = other.table;
            bonds = other.bonds;
            pos = other.pos;
        }

        iterator_base operator++() {
            pos = bonds->forward[pos];
            return *this;
        }

        iterator_base operator++(int) {
            iterator_base save(*this);
            pos = bonds->forward[pos];
            return save;
        }

        iterator_base operator--() {
            pos = bonds->backward[pos];
            return *this;
        }

        iterator_base operator--(int) {
            iterator_base save(*this);
            pos = bonds->backward[pos];
            return save;
        }

        bool operator==(const iterator_base& other) {
            return pos == other.pos &&
                   table == other.table &&
                   bonds == other.bonds;
        }

        bool operator!=(const iterator_base& other) {
            return !(*this == other);
        }
    };

public:
    class iterator : public iterator_base<std::vector<Node>, Bonds>  {
    public:
        iterator() : iterator_base<std::vector<Node>, Bonds>() {}

        iterator(std::vector<Node>& table, Bonds& bonds, size_t pos) :
        iterator_base<std::vector<Node>, Bonds>(table, bonds, pos){}

        std::pair<const KeyType, ValueType>& operator*() const {
            return reinterpret_cast<std::pair<const KeyType, ValueType>&>
                ((*iterator_base<std::vector<Node>, Bonds>::table)
                [iterator_base<std::vector<Node>, Bonds>::pos].get_pair());
        }

        std::pair<const KeyType, ValueType>* operator->() const {
            return reinterpret_cast<std::pair<const KeyType, ValueType>*>
                (&((*iterator_base<std::vector<Node>, Bonds>::table)
                [iterator_base<std::vector<Node>, Bonds>::pos].get_pair()));
        }
    };

    class const_iterator : public iterator_base<const std::vector<Node>, const Bonds> {
    public:
        const_iterator() : iterator_base<const std::vector<Node>, const Bonds>() {}

        const_iterator(const std::vector<Node>& table, const Bonds& bonds, const size_t pos) :
        iterator_base<const std::vector<Node>, const Bonds>(table, bonds, pos) {}

        const std::pair<const KeyType, ValueType>& operator *() const {
            return reinterpret_cast<const std::pair<const KeyType, ValueType>&>
                ((*iterator_base<const std::vector<Node>, const Bonds>::table)
                [iterator_base<const std::vector<Node>, const Bonds>::pos].get_pair());
        }

        const std::pair<const KeyType, ValueType>* operator ->() const {
            return reinterpret_cast<const std::pair<const KeyType, ValueType>*>
                (&((*iterator_base<const std::vector<Node>, const Bonds>::table)
                [iterator_base<const std::vector<Node>, const Bonds>::pos].get_pair()));
        }
    };

    HashMap(Hash hasher = Hash()) : HashMap(kOriginalSize, hasher) {}

    HashMap(const size_t table_size, Hash hasher = Hash()) : table_size(table_size), hasher(hasher) {
        table_capacity = (size_t)(table_size * (long double)kOriginalCapacity / kOriginalSize);
        bonds.resize(table_size + 1);
        pos_begin = pos_end = table_size;
        cnt_deleted = cnt_non_deleted = 0;
        hash_table.resize(table_size);
    }

    template <typename Iterator>
    HashMap(Iterator begin, Iterator end, Hash hasher_ = Hash()) {
        *this = HashMap<KeyType, ValueType, Hash>(hasher_);
        for (auto it = begin; it != end; it++) {
            insert(*it);
        }
    }

    HashMap(const std::initializer_list<std::pair<KeyType, ValueType>>& list, Hash hasher_ = Hash()) :
    HashMap(list.begin(), list.end(), hasher_) {}

    HashMap(const HashMap<KeyType, ValueType, Hash>& other) : hasher(other.hasher) {
        hash_table = other.hash_table;
        bonds = other.bonds;
        pos_begin = other.pos_begin;
        pos_end = other.pos_end;
        table_size = other.table_size;
        table_capacity = other.table_capacity;
        cnt_deleted = other.cnt_deleted;
        cnt_non_deleted = other.cnt_non_deleted;
    }

    void insert(const std::pair<KeyType, ValueType>& bond) {
        if (cnt_deleted + cnt_non_deleted > table_capacity) {
            rehash(2);
        }

        if (2 * (cnt_deleted + cnt_non_deleted) > table_capacity && cnt_deleted > cnt_non_deleted) {
            rehash(1);
        }

        for (size_t pos = get_hash(bond.first); ; pos = inc(pos)) {
            if (!hash_table[pos].has_created) {
                hash_table[pos] = Node(bond);
                insert_iterator(pos);
                cnt_non_deleted++;
                return;
            } else if (!hash_table[pos].has_deleted && hash_table[pos].get_key() == bond.first) {
                return;
            }
        }
    }

    void erase(const KeyType& key) {
        if (4 * cnt_non_deleted < table_capacity) {
            rehash(0.5);
        }

        for (size_t pos = get_hash(key); ; pos = inc(pos)) {
            if (!hash_table[pos].has_created) {
                return;
            } else if (!hash_table[pos].has_deleted && hash_table[pos].get_key() == key) {
                hash_table[pos].has_deleted = true;
                erase_iterator(pos);
                cnt_non_deleted--;
                cnt_deleted++;
                return;
            }
        }
    }

    iterator begin() {
        return iterator(hash_table, bonds, pos_begin);
    }

    iterator end() {
        return iterator(hash_table, bonds, pos_end);
    }

    iterator find(const KeyType& key) {
        for (size_t pos = get_hash(key); ; pos = inc(pos)) {
            if (!hash_table[pos].has_created) {
                return end();
            } else if (!hash_table[pos].has_deleted && hash_table[pos].get_key() == key) {
                return iterator(hash_table, bonds, pos);
            }
        }
    }

    const_iterator begin() const {
        return const_iterator(hash_table, bonds, pos_begin);
    }

    const_iterator end() const {
        return const_iterator(hash_table, bonds, pos_end);
    }

    const_iterator find(const KeyType& key) const {
        for (size_t pos = get_hash(key); ; pos = inc(pos)) {
            if (!hash_table[pos].has_created) {
                return end();
            } else if (!hash_table[pos].has_deleted && hash_table[pos].get_key() == key) {
                return const_iterator(hash_table, bonds, pos);
            }
        }
    }

    size_t size() const {
        return cnt_non_deleted;
    }

    bool empty() const {
        return (cnt_non_deleted == 0);
    }

    Hash hash_function() const {
        return hasher;
    }

    ValueType& operator [](const KeyType& key) {
        auto it = find(key);
        if (it == end()) {
            insert(std::make_pair(key, ValueType()));
            return find(key)->second;
        } else {
            return it->second;
        }
    }

    const ValueType& at(const KeyType& key) const {
        auto it = find(key);
        if (it == end()) {
            throw(std::out_of_range("sad"));
        } else {
            return it->second;
        }
    }

    void clear() {
        auto save = *this;

        for (auto& it : save) {
            erase(it.first);
        }
    }

    void swap(HashMap<KeyType, ValueType, Hash>& other) {
        hash_table.swap(other.hash_table);
        bonds.swap(other.bonds);
        std::swap(pos_begin, other.pos_begin);
        std::swap(pos_end, other.pos_end);
        std::swap(table_capacity, other.table_capacity);
        std::swap(table_size, other.table_size);
        std::swap(cnt_deleted, other.cnt_deleted);
        std::swap(cnt_non_deleted, other.cnt_non_deleted);
    }
};
