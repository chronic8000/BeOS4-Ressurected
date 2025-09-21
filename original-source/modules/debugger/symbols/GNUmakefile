ase<_CharT,_Alloc>::allocator_type allocator_type;
    _Rope_RopeConcatenation(_Rope_RopeRep<_CharT,_Alloc>* __l,
                             _Rope_RopeRep<_CharT,_Alloc>* __r,
                             allocator_type __a)
      : _M_left(__l), _M_right(__r)
      , _Rope_RopeRep<_CharT,_Alloc>(
          _S_concat, max(__l->_M_depth, __r->_M_depth) + 1, false,
          __l->_M_size + __r->_M_size, __a)
      {}
# ifndef __GC
    ~_Rope_RopeConcatenation() {
        _M_free_c_string();
        _M_left->_M_unref_nonnil();
        _M_right->_M_unref_nonnil();
    }
# endif
};

template<class _CharT, class _Alloc>
struct _Rope_RopeFunction : public _Rope_RopeRep<_CharT,_Alloc> {
  public:
    char_producer<_CharT>* _M_fn;
#   ifndef __GC
      bool _M_delete_when_done; // Char_producer is owned by the
                                // rope and should be explicitly
                                // deleted when the rope becomes
                     