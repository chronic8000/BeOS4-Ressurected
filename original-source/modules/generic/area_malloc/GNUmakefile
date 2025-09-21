lloc>&
operator+= (rope<_CharT,_Alloc>& __left, 
      const rope<_CharT,_Alloc>& __right)
{
    __left.append(__right);
    return __left;
}

template <class _CharT, class _Alloc>
inline
rope<_CharT,_Alloc>
operator+ (const rope<_CharT,_Alloc>& __left,
           const _CharT* __right) {
    size_t __rlen = rope<_CharT,_Alloc>::_S_char_ptr_len(__right);
    return rope<_CharT,_Alloc>(
      rope<_CharT,_Alloc>::_S_concat_char_iter(
        __left._M_tree_ptr, _