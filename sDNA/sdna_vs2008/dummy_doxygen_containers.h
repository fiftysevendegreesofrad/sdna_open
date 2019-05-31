#error "this should not be compiled"

namespace std
{

template<class E> class vector
{
public:
E elements;
};

template<class E> class set 
{
public:
E elements;
};

template<class E,class C> class multiset 
{
public:
E elements;
C comparison_class;
};

template<class K,class V> class map 
{
public:
K keys;
V values;
};

template<class K,class V> class multimap 
{
public
K keys;
V values;
};

template<class P> class shared_ptr
{
P* p;
};

}

