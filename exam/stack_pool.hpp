/* A pool of blazingly fast stacks */

#include <iostream>
#include <vector>
#include <iterator> // iterator tags
#include <utility> // std::move

// Iterator class template: P is stack_pool, T is either T or const T, N is the same of the stack_pool class
template <typename P, typename T, typename N>
class MyIterator {
public:
// Iterator tags
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag; // equivalent to typedef std::forward_iterator_tag iterator_category;
  using reference = value_type&;
  using pointer = value_type*;

  using pool_type = P;
  using pool_pointer = pool_type*;

// Iterator constructors
  MyIterator() noexcept = default;
  ~MyIterator() = default;
  // the custom constructor takes two arguments
  MyIterator(pool_pointer pool_ptr, N x) : PoolPointer{pool_ptr}, StackIndex{x} {};

  const reference operator*() const noexcept { return PoolPointer->value(StackIndex); } // dereference operator
  reference operator*() noexcept { return PoolPointer->value(StackIndex); }

  MyIterator& operator++() {  // Pre-increment
    StackIndex = PoolPointer->next(StackIndex);
    return *this;
  }

  // Equality and inequality
  friend bool operator== (const MyIterator& lhs, const MyIterator& rhs) noexcept { return lhs.StackIndex == rhs.StackIndex; };
  friend bool operator!= (const MyIterator& lhs, const MyIterator& rhs) noexcept { return lhs.StackIndex != rhs.StackIndex; };

private:
// the iterator needs to know where the pool begins in the memory, so we pass a pointer to a stack_pool object
  pool_pointer PoolPointer;
// the iterator needs to know where a stack begins in the pool, so we pass the head of the stack
  N StackIndex;
};

template <typename T, typename N = std::size_t>
class stack_pool{

  struct node_t{
    T value;
    N next; // perhaps N next{end()};?
  // node constructors:
    node_t() noexcept = default; // default constructor
    node_t(T _value, const N x): value{_value}, next{x} {} // custom constructor
    node_t(const node_t&) = default; // copy constructor
    node_t(node_t&&) noexcept = default; // move constructor
    node_t& operator=(const node_t&) = default; // copy assignment
    node_t& operator=(node_t&&) noexcept = default; // move assignment
    ~node_t() noexcept = default; // default destructor
  };

  std::vector<node_t> pool;
  using stack_type = N;
  using value_type = T;

  using size_type = typename std::vector<node_t>::size_type; // see end of file

  stack_type free_nodes{end()}; // at the beginning, it is empty: we use end() as zero
  // equivalent to free_nodes = new_stack();

  node_t& node(stack_type x) noexcept { return pool[x-1]; }
  const node_t& node(stack_type x) const noexcept { return pool[x-1]; }

  template <typename O> // forwarding reference template
  stack_type _push(O&& val, stack_type head); // declaration: this function is implemented outside the class

public:
// stack_pool constructors:
  stack_pool() noexcept = default; // default constructor: we have to set free_nodes to empty
  stack_pool(const stack_pool&) = default; // copy constructor
  stack_pool(stack_pool&&) noexcept = default; // move constructor
  stack_pool& operator=(const stack_pool&) = default; // copy assignment
  stack_pool& operator=(stack_pool&&) noexcept = default; // move assignment
  ~stack_pool() noexcept = default; // default destructor
  explicit stack_pool(size_type n) { reserve(n); } // custom constructor: we reserve n nodes in the pool

  stack_type new_stack() noexcept { return end(); } // returns an empty stack (aka end())

  size_type capacity() const noexcept { return pool.capacity(); } // the capacity of the pool (std::vector<>::capacity)

  void reserve(size_type n) { pool.reserve(n); } // calls std::vector<>::reserve; here we do not use noexcept!!

  bool empty(stack_type x) const noexcept { return x == end(); } // a stack is empty if x == end();

// "zero" must be consistent with the type specified in the template (there are several types of integers):
  stack_type end() const noexcept { return stack_type(0); } // we use end() instead of 0 everywhere

  T& value(stack_type x) { return node(x).value; }
  const T& value(stack_type x) const { return node(x).value; }

  stack_type& next(stack_type x) { return node(x).next; }
  const stack_type& next(stack_type x) const { return node(x).next; }

// push is implemented outside the class; we make use of a forwarding reference
  stack_type push(const T& val, stack_type head) { return _push(val, head); } // l-value reference
  stack_type push(T&& val, stack_type head) { return _push(std::move(val), head); } // r-value reference

// pop removes the first node and returns the new head:
  stack_type pop(stack_type x) { // x is the current head of the stack (before popping)
   stack_type tmp = next(x); // next(x) is the new head of the stack: we store it in a temporary variable
   next(x) = free_nodes; //
   free_nodes = x; // popping the node of index x means that x is now the new head of the free_nodes stack
   return tmp; // return the new head
  }

  stack_type free_stack(stack_type x) {
    while (x != end()) { x = pop(x); } // call pop(x) as long as the stack is not empty
    return x; // return the new head
  }

// Iterator stuff
  using iterator = MyIterator<stack_pool, T, N>;
  using const_iterator = MyIterator<stack_pool, const T, N>;

  iterator begin(stack_type x) { return iterator(this, x); }
  iterator end(stack_type ) { return iterator(this, end()); } // this is not a typo
  // since we simply return "zero", there's no need to pass any argument to the functions end, cend

  const_iterator begin(stack_type x) const { return const_iterator(this, x); }
  const_iterator end(stack_type ) const { return const_iterator(this, end()); }

  const_iterator cbegin(stack_type x) const { return const_iterator(this, x); }
  const_iterator cend(stack_type ) const { return const_iterator(this, end()); }
};

template <typename T, typename N>
template <typename O>
N stack_pool<T, N>::_push(O&& val, N head) { // forwarding reference
  if (empty(free_nodes)) { // if there's no free nodes, call push_back***:
    pool.push_back(node_t{std::forward<O>(val), head}); // push_back always allows the use of uniform initialization
    // the new head of the stack is now the last node in the pool (the index is pool.size())
    return static_cast<N>(pool.size()); // cast to the type of integer on which the class is templated
  } else { // if instead there is at least one free node, overwrite the value of that node
    // we create a temporary copy of free_nodes
    auto tmp = free_nodes;
    // the current free node is being overwritten, so it won't be free anymore:
    // the current next (second) free node becomes the new first free node
    free_nodes = next(free_nodes);
    // the new value is written on the first free node
    value(tmp) = std::forward<O>(val);
    // update the head
    next(tmp) = head;
    // return the new head
    return tmp;
  }
};

// *** push_back will call only constructors that are implicit. Implicit constructors are supposed to be safe.

/*
using size_type = typename std::vector<node_t>::size_type; is equivalent to using size_type = std::size_t;
since the standard containers define size_type as a typedef to Allocator::size_type, which for std::allocator
is defined to be size_t (from stackoverflow)
*/
