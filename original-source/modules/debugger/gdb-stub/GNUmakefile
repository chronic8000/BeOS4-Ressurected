  void _M_create_nodes(_Tp** __nstart, _Tp** __nfinish);
  void _M_destroy_nodes(_Tp** __nstart, _Tp** __nfinish);
  enum { _S_initial_map_size = 8 };

protected:
  iterator _M_start;
  iterator _M_finish;
};

#else /* __STL_USE_STD_ALLOCATORS */

template <class _Tp, class _Alloc, size_t __bufsiz>
class _Deque_base {
public:
#ifndef __STL_NON_TYPE_TMPL_PARAM_BUG
  typedef _Deque_iterator<_Tp,_Tp&,_Tp*,__bufsiz>              iterator;
  typedef _Deque_iterator<_Tp,const _Tp&,const _Tp*, __bufsiz> const_iterator;
#else /* __STL_NON_TYPE_TMPL_PARAM_BUG */
  typedef _Deque_iterator<_Tp,_Tp&,_Tp*>                      iterator;
  typedef _Deque_iterator<_Tp,const _Tp&,const _Tp*>          const_iterator;
#endif /* __STL_NON_TYPE_TMPL_PARAM_BUG */

  typedef _Alloc allocator_type;
  allocator_type get_allocator() const { return allocator_type(); }

  _Deque_base(const allocator_type&, size_t __num_elements)
    : _M_map(0), _M_map_si