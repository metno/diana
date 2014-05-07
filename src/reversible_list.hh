
#ifndef REVERSIBLE_LIST_HH
#define REVERSIBLE_LIST_HH 1

#include <algorithm>
#include <iterator>

/**
 * \brief reversible list
 *
 * A doubly linked list that can be extended at front and back,
 * reversed, merged to another such list -- all in constant time.
 *
 * Idea from
 * http://www.chiark.greenend.org.uk/~sgtatham/algorithms/revlist.html
 */
template <class T, class A = std::allocator<T> >
class reversible_list {
private:
    struct list_item;
    typedef list_item* list_item_p;

    struct list_item {
        list_item_p next_or_prev, prev_or_next;
        T data;
        list_item(const T& t)
            : next_or_prev(0), prev_or_next(0), data(t) { }

        list_item_p next(list_item_p& b) 
            { list_item_p p = b; b = this; return away_from(p); }
        list_item_p prev(list_item_p& b) 
            { list_item_p p = b; if (b) { b = b->away_from(this); } return p; }
        list_item_p& towards(list_item_p to) 
            { return (next_or_prev == to) ? next_or_prev : prev_or_next; }
        list_item_p& away_from(list_item_p aw) 
            { return (next_or_prev == aw) ? prev_or_next : next_or_prev; }
        list_item_p& open_end()
            { return towards(0); }
        // must be copyable for new_allocator::construct
    };

    typedef typename A::template rebind<list_item>::other ItemAlloc;

public:
    typedef A allocator_type;
    typedef typename A::value_type value_type; 
    typedef typename A::reference reference;
    typedef typename A::pointer pointer;
    typedef typename A::const_pointer const_pointer;
    typedef typename A::const_reference const_reference;
    typedef typename A::difference_type difference_type;
    typedef typename A::size_type size_type;

    class iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
    public:
        iterator()
            { }
        iterator(const iterator& o)
            : item(o.item), before(o.before) { }
        ~iterator()
            { }

        iterator& operator=(const iterator& o)
            { item = o.item; before = o.before; }
        bool operator==(const iterator& o) const
            { return item == o.item; }
        bool operator!=(const iterator& o) const
            { return item != o.item; }

        iterator& operator++()
            { if(item) item = item->next(before); return *this; }
        iterator& operator--()
            { if (item) item = item->prev(before); return *this; }
 
        reference operator*() const
            { return item->data; }
        pointer operator->() const
            { return &item->data; }

    private:
        iterator(list_item_p i, list_item_p b)
            : item(i), before(b) { }
        list_item_p item, before;

        friend class reversible_list;
    };

    class const_iterator : public std::iterator<std::bidirectional_iterator_tag, const T> {
    public:
        const_iterator()
            { }
        const_iterator(const const_iterator& o)
            : item(o.item), before(o.before) { }
        const_iterator(const iterator& o)
            : item(o.item), before(o.before) { }
        ~const_iterator()
            { }

        const_iterator& operator=(const const_iterator& o)
            { item = o.item; before = o.before; }
        bool operator==(const const_iterator& o) const
            { return item == o.item; }
        bool operator!=(const const_iterator& o) const
            { return item != o.item; }

        const_iterator& operator++()
            { if(item) item = item->next(before); return *this; }
        const_iterator& operator--()
            { if(item) item = item->prev(before); return *this; }
 
        const_reference operator*() const
            { return item->data; }
        const_pointer operator->() const
            { return &item->data; }

    private:
        const_iterator(list_item_p i, list_item_p b)
            : item(i), before(b) { }
        list_item_p item, before;

        friend class reversible_list;
    };

    typedef iterator       reverse_iterator;
    typedef const_iterator const_reverse_iterator;

    reversible_list()
        : head(0), tail(0), count(0) { }

    reversible_list(const reversible_list&);

    ~reversible_list()
        { clear(); }

    reversible_list& operator=(const reversible_list&);
    bool operator==(const reversible_list&) const;
    bool operator!=(const reversible_list&) const;

    iterator begin()
        { return iterator(head, 0); }
    iterator end()
        { return iterator(0, tail); }
    reverse_iterator rbegin()
        { return iterator(tail, 0); }
    reverse_iterator rend()
        { return iterator(0, head); }

    const_iterator begin() const
        { return const_iterator(head, 0); }
    const_iterator end() const
        { return const_iterator(0, tail); }
    const_reverse_iterator rbegin() const
        { return const_iterator(tail, 0); }
    const_reverse_iterator rend() const
        { return const_iterator(0, head); }

    reference front()
        { return *begin(); }
    reference back()
        { return *rbegin(); }
    reference front_or_back(bool front)
        { return front ? *begin() : *rbegin(); }
    const_reference front() const
        { return *begin(); }
    const_reference back() const
        { return *rbegin(); }
    const_reference front_or_back(bool front) const
        { return front ? *begin() : *rbegin(); }

    void push_front(const T& t)
        { push_any(head, t); }
    void push_back(const T& t)
        { push_any(tail, t); }
    void push_front_or_back(bool front, const T& t)
        { push_any(front ? head : tail, t); }

    void pop_front()
        { pop_any(head); }
    void pop_back()
        { pop_any(tail); }
    void pop_front_or_back(bool front)
        { pop_any(front ? head : tail); }

    void clear();

    void swap(reversible_list& o)
        { std::swap(head, o.head); std::swap(tail, o.tail); std::swap(count, o.count); std::swap(alloc, o.alloc); }

    size_type size() const
        { return count; }

    bool empty() const
        { return head == 0; }

    void reverse()
        { std::swap(head, tail); }

    void move_back(reversible_list& other);
    void move_front(reversible_list& other)
        { other.move_back(*this); swap(other); }

private:
    void push_any(list_item_p& head_or_tail, const T& t);
    void pop_any(list_item_p& head_or_tail);
    list_item_p new_item(const T& t);
    void delete_item(list_item_p li);

private:
    list_item_p head, tail;
    size_type count;
    ItemAlloc alloc;
};

template <class T, class A>
reversible_list<T,A>::reversible_list(const reversible_list& o)
    : head(0), tail(0), count(0)
{
    const const_iterator it_end = o.end();
    for(const_iterator it = o.begin(); it != it_end; ++it)
        push_back(*it);
}

template <class T, class A>
reversible_list<T,A>& reversible_list<T,A>::operator =(const reversible_list& o)
{
    if (&o != this) {
        reversible_list copy_o(o);
        swap(copy_o);
    };
    return *this;
}

template <class T, class A>
void reversible_list<T,A>::clear()
{
    while (not empty())
        pop_front();
}

template <class T, class A>
void reversible_list<T,A>::pop_any(list_item_p& head_or_tail)
{
    if (head_or_tail) {
        list_item_p old = head_or_tail;
        head_or_tail = old->away_from(0);
        if (head_or_tail)
            head_or_tail->towards(old) = 0; // clear pointer to old head/tail
        else
            head = tail = 0; // empty, clear both head and tail pointers
        delete_item(old);
        count -= 1;
    }
}

template <class T, class A>
void reversible_list<T,A>::push_any(list_item_p& head_or_tail, const T& t)
{
    list_item_p n = new_item(t);
    
    if (not head_or_tail) {
        head = tail = n;
    } else {
        n->next_or_prev = head_or_tail;
        head_or_tail->open_end() = n;
        head_or_tail = n;
    }
    count += 1;
}

template <class T, class A>
typename reversible_list<T,A>::list_item_p reversible_list<T,A>::new_item(const T& t)
{
    list_item_p n = alloc.allocate(1, head);
    try {
        alloc.construct(n, t);
    } catch (...) {
        alloc.deallocate(n, 1);
        throw;
    }
    return n;
}

template <class T, class A>
void reversible_list<T,A>::delete_item(list_item_p li)
{
    alloc.destroy(li);
    alloc.deallocate(li, 1);
}

template <class T, class A>
void reversible_list<T,A>::move_back(reversible_list& other)
{
    if (not other.head)
        return;

    if (not head) {
        swap(other);
        return;
    }

    other.head->open_end() = tail;
    tail->open_end() = other.head;

    tail = other.tail;
    other.head = other.tail = 0;
    count += other.count;
    other.count = 0;
}

#endif // REVERSIBLE_LIST_HH
