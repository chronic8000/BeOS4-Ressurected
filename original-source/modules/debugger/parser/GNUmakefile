const_iterator end() const { return _M_t.end(); }
  reverse_iterator rbegin() { return _M_t.rbegin(); }
  const_reverse_iterator rbegin() const { return _M_t.rbegin(); }
  reverse_iterator rend() { return _M_t.rend(); }
  const_reverse_iterator rend() const { return _M_t.rend(); }
  bool empty() const { return _M_t.empty(); }
  size_type size() const { return _M_t.size(); }
  size_type max_size() const { return _M_t.max_size(); }
  _Tp& operator[](const key_type& __k) {
    iterator __i = lower_bound(__k);
    // __i->first is greater than or equivalent to __k.
    if (__i == end() || key_comp()(__k, (*__i).first))
      __i = insert(__i, value_type(__k, _Tp()));
    return (*__i).second;
  }
  void swap(map<_Key,_Tp,_Compare,_Alloc>& __x) { _M_t.swap(__x._M_t); }

  // insert/erase

  pair<iterator,bool> insert(const value_type& __x) 
    { return _M_t.insert_unique(__x); }
  iterator insert(iterator position, const value_t