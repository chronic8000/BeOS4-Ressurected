
class iobuffer
{
	public:

		iobuffer(void *base, int32 size) {
			m_base = (uint8*)base;
			m_size = size;
			m_offs = 0;
			m_consumed = 0;
		};
		
		template <class TYPE>
		bool read(TYPE *val) {
			if ((m_size-m_offs) < (int32)sizeof(TYPE)) return false;
			*val = *((TYPE*)(m_base+m_offs));
			m_offs += sizeof(TYPE);
			return true;
		}
	
		template <class TYPE>
		bool write(TYPE val) {
			if ((m_size-m_offs) < (int32)sizeof(TYPE)) return false;
			*((TYPE*)(m_base+m_offs)) = val;
			m_offs += sizeof(TYPE);
			return true;
		}
	
		int32 drain(int32 size) {
			if (size > (m_size-m_offs)) size = m_size-m_offs;
			m_offs += size;
			return size;
		}
	
		int32 remaining() { return m_size-m_offs; }
		int32 consumed() { return m_consumed; }
		void markConsumed() { m_consumed = m_offs; }
	
		void remainder(void **ptr, int32 *size) {
			*ptr = ((uint8*)m_base)+m_offs;
			*size = m_size - m_offs;
		}

	private:

		uint8 *m_base;
		int32 m_offs;
		int32 m_size;
		int32 m_consumed;
	
};
