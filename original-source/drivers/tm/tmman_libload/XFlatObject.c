
#include "FlatObject.h"
#include "ObjCreate.h"
#include "ObjRepr.h"
#include "ModBldRepr.h"
#include "Lib_Local.h"
#include "Lib_List.h"
#include "Lib_Set.h"
#include "Lib_Mapping.h"
#include "Lib_StdFuncs.h"
#include "Lib_Exceptions.h"
#include "Scatter_Shuffle.h"




typedef enum {
	Unread, Read, Unpacked
}               LifetimeStatus;


struct FlatObject_Module {

	struct {
		Pointer         base;
		Pointer         ceiling;
		TMObj_Section   section;
	}               section_info[TMObj_LAST_SECTION_ID][TMObj_LAST_LIFE];

	LifetimeStatus  lifetime_status[TMObj_LAST_LIFE];

	TMObj_Section_Lifetime start_lifetime;
	TMObj_Section_Lifetime embedded_modlist_lifetime;

	FlatObject_SectionLoaded_Fun section_loaded;
	Pointer         data;

	Lib_IODriver    object;
	Pointer         default_section_bytes;
	Bool            encode_sysrefs;
	Bool            endian_difference;
	Bool            unshuffle_dynamic_scatters;

	TMObj_Module    header;
	TMObj_Section   section_table;

	Int             streamsize;
	Address         streambuff;

	Lib_Mapping     unshuffle_memo;
};


static void 
Unpack_Scatter_Source(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Scatter_Source restrict __base;
				TMObj_Scatter_Source restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{
						((__base)->mask) = LIENDIAN_SWAP4((__base)->mask);;
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Scatter_Source restrict __base;
				TMObj_Scatter_Source restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{;
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_Scatter(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Scatter restrict __base;
				TMObj_Scatter restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->dest);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_Scatter_Dest_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Dest_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->dest) = (Pointer) _unpacked;
				} {
					UInt            _tmp_obj = (UInt) ((__base)->source);
					Int             _lifetime = _tmp_obj & 0xff;
					Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
					Address         _floor = (module)->section_info[TMObj_Scatter_Source_Section][_lifetime].base;
					Address         _ceiling = (module)->section_info[TMObj_Scatter_Source_Section][_lifetime].ceiling;
					Address         _unpacked = _floor + _offset;
					CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
					CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
					CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
					((__base)->source) = (Pointer) _unpacked;
				}
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Scatter restrict __base;
				TMObj_Scatter restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->dest);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_Scatter_Dest_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Dest_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->dest) = (Pointer) _unpacked;
				} {
					UInt            _tmp_obj = (UInt) ((__base)->source);
					Int             _lifetime = _tmp_obj >> 24;
					Int             _offset = _tmp_obj & 0x00ffffff;
					Address         _floor = (module)->section_info[TMObj_Scatter_Source_Section][_lifetime].base;
					Address         _ceiling = (module)->section_info[TMObj_Scatter_Source_Section][_lifetime].ceiling;
					Address         _unpacked = _floor + _offset;
					CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
					CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
					CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
					((__base)->source) = (Pointer) _unpacked;
				}
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_Symbol(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Symbol restrict __base;
				TMObj_Symbol restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->name);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_String_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_String_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->name) = (Pointer) _unpacked;
				} ((__base)->jtab_index) = LIENDIAN_SWAP2((__base)->jtab_index);;
				switch ((__base)->type) {
				case TMObj_UnresolvedSymbol:
					break;
				case TMObj_AbsoluteSymbol:
					((__base)->attr.value) = LIENDIAN_SWAP4((__base)->attr.value);;
					break;
				case TMObj_CommonSymbol:
					((__base)->attr.memspec.length) = LIENDIAN_SWAP4((__base)->attr.memspec.length);;
					((__base)->attr.memspec.alignment) = LIENDIAN_SWAP2((__base)->attr.memspec.alignment);;
					break;
				case TMObj_RelativeSymbol:{{
							UInt            _tmp_obj = (UInt) ((&(__base)->attr.marker)->section);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((&(__base)->attr.marker)->section) = (Pointer) _unpacked;
					} ((&(__base)->attr.marker)->offset) = LIENDIAN_SWAP4((&(__base)->attr.marker)->offset);;
					(&(__base)->attr.marker)->offset += (UInt) ((&(__base)->attr.marker)->section->bytes);
					if ((&(__base)->attr.marker)->section->is_system && (module)->encode_sysrefs) {
						(&(__base)->attr.marker)->section = ((TMObj_Section) ((((&(__base)->attr.marker)->section->system_section_id) << 1) | 0x1));
					}
					} break;
				}
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Symbol restrict __base;
				TMObj_Symbol restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->name);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_String_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_String_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->name) = (Pointer) _unpacked;
				};
				switch ((__base)->type) {
				case TMObj_UnresolvedSymbol:
					break;
				case TMObj_AbsoluteSymbol:;
					break;
				case TMObj_CommonSymbol:;;
					break;
				case TMObj_RelativeSymbol:{{
							UInt            _tmp_obj = (UInt) ((&(__base)->attr.marker)->section);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((&(__base)->attr.marker)->section) = (Pointer) _unpacked;
					};
					(&(__base)->attr.marker)->offset += (UInt) ((&(__base)->attr.marker)->section->bytes);
					if ((&(__base)->attr.marker)->section->is_system && (module)->encode_sysrefs) {
						(&(__base)->attr.marker)->section = ((TMObj_Section) ((((&(__base)->attr.marker)->section->system_section_id) << 1) | 0x1));
					}
					} break;
				}
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_External_Module(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_External_Module restrict __base;
				TMObj_External_Module restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->name);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_String_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_String_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->name) = (Pointer) _unpacked;
				} ((__base)->checksum) = LIENDIAN_SWAP4((__base)->checksum);;
				((__base)->nrof_exported_symbols) = LIENDIAN_SWAP2((__base)->nrof_exported_symbols);;
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_External_Module restrict __base;
				TMObj_External_Module restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->name);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_String_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_String_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->name) = (Pointer) _unpacked;
				};;
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_SectionUnit(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_SectionUnit restrict __base;
				TMObj_SectionUnit restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{{
								UInt            _tmp_obj = (UInt) ((&(__base)->marker)->section);
								Int             _lifetime = _tmp_obj & 0xff;
								Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
								Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
								Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
								Address         _unpacked = _floor + _offset;
								CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
								CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
								CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
								((&(__base)->marker)->section) = (Pointer) _unpacked;
				} ((&(__base)->marker)->offset) = LIENDIAN_SWAP4((&(__base)->marker)->offset);;
				(&(__base)->marker)->offset += (UInt) ((&(__base)->marker)->section->bytes);
				if ((&(__base)->marker)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->marker)->section = ((TMObj_Section) ((((&(__base)->marker)->section->system_section_id) << 1) | 0x1));
				}
				} ((__base)->length) = LIENDIAN_SWAP4((__base)->length);;
				((__base)->alignment) = LIENDIAN_SWAP2((__base)->alignment);;
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_SectionUnit restrict __base;
				TMObj_SectionUnit restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{{
								UInt            _tmp_obj = (UInt) ((&(__base)->marker)->section);
								Int             _lifetime = _tmp_obj >> 24;
								Int             _offset = _tmp_obj & 0x00ffffff;
								Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
								Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
								Address         _unpacked = _floor + _offset;
								CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
								CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
								CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
								((&(__base)->marker)->section) = (Pointer) _unpacked;
				};
				(&(__base)->marker)->offset += (UInt) ((&(__base)->marker)->section->bytes);
				if ((&(__base)->marker)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->marker)->section = ((TMObj_Section) ((((&(__base)->marker)->section->system_section_id) << 1) | 0x1));
				}
				};;
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_Symbol_Reference(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Symbol_Reference restrict __base;
				TMObj_Symbol_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->scatter) = (Pointer) _unpacked;
				} {
					UInt            _tmp_obj = (UInt) ((__base)->value);
					Int             _lifetime = _tmp_obj & 0xff;
					Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
					Address         _floor = (module)->section_info[TMObj_Symbol_Section][_lifetime].base;
					Address         _ceiling = (module)->section_info[TMObj_Symbol_Section][_lifetime].ceiling;
					Address         _unpacked = _floor + _offset;
					CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
					CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
					CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
					((__base)->value) = (Pointer) _unpacked;
				} ((__base)->offset) = LIENDIAN_SWAP4((__base)->offset);; {{
						UInt            _tmp_obj = (UInt) ((&(__base)->dest)->section);
						Int             _lifetime = _tmp_obj & 0xff;
						Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->dest)->section) = (Pointer) _unpacked;
				} ((&(__base)->dest)->offset) = LIENDIAN_SWAP4((&(__base)->dest)->offset);;
				(&(__base)->dest)->offset += (UInt) ((&(__base)->dest)->section->bytes);
				if ((&(__base)->dest)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->dest)->section = ((TMObj_Section) ((((&(__base)->dest)->section->system_section_id) << 1) | 0x1));
				}
				}
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Symbol_Reference restrict __base;
				TMObj_Symbol_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
							((__base)->scatter) = (Pointer) _unpacked;
				} {
					UInt            _tmp_obj = (UInt) ((__base)->value);
					Int             _lifetime = _tmp_obj >> 24;
					Int             _offset = _tmp_obj & 0x00ffffff;
					Address         _floor = (module)->section_info[TMObj_Symbol_Section][_lifetime].base;
					Address         _ceiling = (module)->section_info[TMObj_Symbol_Section][_lifetime].ceiling;
					Address         _unpacked = _floor + _offset;
					CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
					CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
					CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
					((__base)->value) = (Pointer) _unpacked;
				}; {{
						UInt            _tmp_obj = (UInt) ((&(__base)->dest)->section);
						Int             _lifetime = _tmp_obj >> 24;
						Int             _offset = _tmp_obj & 0x00ffffff;
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->dest)->section) = (Pointer) _unpacked;
				};
				(&(__base)->dest)->offset += (UInt) ((&(__base)->dest)->section->bytes);
				if ((&(__base)->dest)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->dest)->section = ((TMObj_Section) ((((&(__base)->dest)->section->system_section_id) << 1) | 0x1));
				}
				}
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_Marker_Reference(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Marker_Reference restrict __base;
				TMObj_Marker_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
							((__base)->scatter) = (Pointer) _unpacked;
				} {{
						UInt            _tmp_obj = (UInt) ((&(__base)->value)->section);
						Int             _lifetime = _tmp_obj & 0xff;
						Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->value)->section) = (Pointer) _unpacked;
				} ((&(__base)->value)->offset) = LIENDIAN_SWAP4((&(__base)->value)->offset);;
				(&(__base)->value)->offset += (UInt) ((&(__base)->value)->section->bytes);
				if ((&(__base)->value)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->value)->section = ((TMObj_Section) ((((&(__base)->value)->section->system_section_id) << 1) | 0x1));
				}
				} {{
						UInt            _tmp_obj = (UInt) ((&(__base)->dest)->section);
						Int             _lifetime = _tmp_obj & 0xff;
						Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->dest)->section) = (Pointer) _unpacked;
				} ((&(__base)->dest)->offset) = LIENDIAN_SWAP4((&(__base)->dest)->offset);;
				(&(__base)->dest)->offset += (UInt) ((&(__base)->dest)->section->bytes);
				if ((&(__base)->dest)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->dest)->section = ((TMObj_Section) ((((&(__base)->dest)->section->system_section_id) << 1) | 0x1));
				}
				}
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_Marker_Reference restrict __base;
				TMObj_Marker_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
							((__base)->scatter) = (Pointer) _unpacked;
				} {{
						UInt            _tmp_obj = (UInt) ((&(__base)->value)->section);
						Int             _lifetime = _tmp_obj >> 24;
						Int             _offset = _tmp_obj & 0x00ffffff;
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->value)->section) = (Pointer) _unpacked;
				};
				(&(__base)->value)->offset += (UInt) ((&(__base)->value)->section->bytes);
				if ((&(__base)->value)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->value)->section = ((TMObj_Section) ((((&(__base)->value)->section->system_section_id) << 1) | 0x1));
				}
				} {{
						UInt            _tmp_obj = (UInt) ((&(__base)->dest)->section);
						Int             _lifetime = _tmp_obj >> 24;
						Int             _offset = _tmp_obj & 0x00ffffff;
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->dest)->section) = (Pointer) _unpacked;
				};
				(&(__base)->dest)->offset += (UInt) ((&(__base)->dest)->section->bytes);
				if ((&(__base)->dest)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->dest)->section = ((TMObj_Section) ((((&(__base)->dest)->section->system_section_id) << 1) | 0x1));
				}
				}
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_JTab_Reference(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_JTab_Reference restrict __base;
				TMObj_JTab_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
							((__base)->scatter) = (Pointer) _unpacked;
				} {
					UInt            _tmp_obj = (UInt) ((__base)->module);
					Int             _lifetime = _tmp_obj & 0xff;
					Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
					Address         _floor = (module)->section_info[TMObj_External_Module_Section][_lifetime].base;
					Address         _ceiling = (module)->section_info[TMObj_External_Module_Section][_lifetime].ceiling;
					Address         _unpacked = _floor + _offset;
					CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
					CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
					CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
					((__base)->module) = (Pointer) _unpacked;
				} {{
						UInt            _tmp_obj = (UInt) ((&(__base)->dest)->section);
						Int             _lifetime = _tmp_obj & 0xff;
						Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->dest)->section) = (Pointer) _unpacked;
				} ((&(__base)->dest)->offset) = LIENDIAN_SWAP4((&(__base)->dest)->offset);;
				(&(__base)->dest)->offset += (UInt) ((&(__base)->dest)->section->bytes);
				if ((&(__base)->dest)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->dest)->section = ((TMObj_Section) ((((&(__base)->dest)->section->system_section_id) << 1) | 0x1));
				}
				} ((__base)->jtab_offset) = LIENDIAN_SWAP4((__base)->jtab_offset);;
				((__base)->offset) = LIENDIAN_SWAP4((__base)->offset);;
				if ((section->lifetime) >= TMObj_DynamicLoading_Life && (module)->unshuffle_dynamic_scatters && (__base)->dest.section->is_code) {
					(__base)->scatter = ScatShuf_unshuffle((__base)->scatter, ((Int) (__base)->dest.offset) - ((Int) (__base)->dest.section->bytes), &(module)->unshuffle_memo);
				}
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_JTab_Reference restrict __base;
				TMObj_JTab_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
							((__base)->scatter) = (Pointer) _unpacked;
				} {
					UInt            _tmp_obj = (UInt) ((__base)->module);
					Int             _lifetime = _tmp_obj >> 24;
					Int             _offset = _tmp_obj & 0x00ffffff;
					Address         _floor = (module)->section_info[TMObj_External_Module_Section][_lifetime].base;
					Address         _ceiling = (module)->section_info[TMObj_External_Module_Section][_lifetime].ceiling;
					Address         _unpacked = _floor + _offset;
					CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
					CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
					CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
					((__base)->module) = (Pointer) _unpacked;
				} {{
						UInt            _tmp_obj = (UInt) ((&(__base)->dest)->section);
						Int             _lifetime = _tmp_obj >> 24;
						Int             _offset = _tmp_obj & 0x00ffffff;
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->dest)->section) = (Pointer) _unpacked;
				};
				(&(__base)->dest)->offset += (UInt) ((&(__base)->dest)->section->bytes);
				if ((&(__base)->dest)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->dest)->section = ((TMObj_Section) ((((&(__base)->dest)->section->system_section_id) << 1) | 0x1));
				}
				};;
				if ((section->lifetime) >= TMObj_DynamicLoading_Life && (module)->unshuffle_dynamic_scatters && (__base)->dest.section->is_code) {
					(__base)->scatter = ScatShuf_unshuffle((__base)->scatter, ((Int) (__base)->dest.offset) - ((Int) (__base)->dest.section->bytes), &(module)->unshuffle_memo);
				}
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_FromDefault_Reference(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_FromDefault_Reference restrict __base;
				TMObj_FromDefault_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
							((__base)->scatter) = (Pointer) _unpacked;
				} {
					((&(__base)->dest)->offset) = LIENDIAN_SWAP4((&(__base)->dest)->offset);;
					(&(__base)->dest)->offset += (UInt) ((module)->default_section_bytes);
				} {{
						UInt            _tmp_obj = (UInt) ((&(__base)->value)->section);
						Int             _lifetime = _tmp_obj & 0xff;
						Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->value)->section) = (Pointer) _unpacked;
				} ((&(__base)->value)->offset) = LIENDIAN_SWAP4((&(__base)->value)->offset);;
				(&(__base)->value)->offset += (UInt) ((&(__base)->value)->section->bytes);
				if ((&(__base)->value)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->value)->section = ((TMObj_Section) ((((&(__base)->value)->section->system_section_id) << 1) | 0x1));
				}
				}
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_FromDefault_Reference restrict __base;
				TMObj_FromDefault_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
							((__base)->scatter) = (Pointer) _unpacked;
				} {;
					(&(__base)->dest)->offset += (UInt) ((module)->default_section_bytes);
				} {{
						UInt            _tmp_obj = (UInt) ((&(__base)->value)->section);
						Int             _lifetime = _tmp_obj >> 24;
						Int             _offset = _tmp_obj & 0x00ffffff;
						Address         _floor = (module)->section_info[TMObj_Section_Section][_lifetime].base;
						Address         _ceiling = (module)->section_info[TMObj_Section_Section][_lifetime].ceiling;
						Address         _unpacked = _floor + _offset;
						CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule));
						CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule));
						CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule));
						((&(__base)->value)->section) = (Pointer) _unpacked;
				};
				(&(__base)->value)->offset += (UInt) ((&(__base)->value)->section->bytes);
				if ((&(__base)->value)->section->is_system && (module)->encode_sysrefs) {
					(&(__base)->value)->section = ((TMObj_Section) ((((&(__base)->value)->section->system_section_id) << 1) | 0x1));
				}
				}
				};
				__base++;
				}
			}
	};
	}
} static void 
Unpack_DefaultToDefault_Reference(FlatObject_Module module, TMObj_Section section)
{
	if (module->endian_difference) {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_DefaultToDefault_Reference restrict __base;
				TMObj_DefaultToDefault_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj & 0xff;
							Int             _offset = (((((UInt) (_tmp_obj)) & 0x0000ff00) << 8) | ((((UInt) (_tmp_obj)) & 0x00ff0000) >> 8) | ((((UInt) (_tmp_obj)) & 0xff000000) >> 24));
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->scatter) = (Pointer) _unpacked;
				} {
					((&(__base)->value)->offset) = LIENDIAN_SWAP4((&(__base)->value)->offset);;
					(&(__base)->value)->offset += (UInt) ((module)->default_section_bytes);
				} {
					((&(__base)->dest)->offset) = LIENDIAN_SWAP4((&(__base)->dest)->offset);;
					(&(__base)->dest)->offset += (UInt) ((module)->default_section_bytes);
				}
				};
				__base++;
				}
			}
	};
	} else {{
			if (section != Null) {
				Address         __sectionbase;
				TMObj_DefaultToDefault_Reference restrict __base;
				TMObj_DefaultToDefault_Reference restrict __ceiling;
				__sectionbase = (Address) (section->bytes);
				__base = (Pointer) __sectionbase;
				__ceiling = (Pointer) (__sectionbase + section->size);
				while (__base < __ceiling) {{{
							UInt            _tmp_obj = (UInt) ((__base)->scatter);
							Int             _lifetime = _tmp_obj >> 24;
							Int             _offset = _tmp_obj & 0x00ffffff;
							Address         _floor = (module)->section_info[TMObj_Scatter_Section][_lifetime].base;
							Address         _ceiling = (module)->section_info[TMObj_Scatter_Section][_lifetime].ceiling;
							Address         _unpacked = _floor + _offset;
							CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
							CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
							CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
							((__base)->scatter) = (Pointer) _unpacked;
				} {;
					(&(__base)->value)->offset += (UInt) ((module)->default_section_bytes);
				} {;
					(&(__base)->dest)->offset += (UInt) ((module)->default_section_bytes);
				}
				};
				__base++;
				}
			}
	};
	}
}











static void
read_sections(
	      FlatObject_Module module,
	      TMObj_Section_Lifetime life
)
{
	TMObj_Section   first, last;
	TMObj_Section   sections_base, sections_ceiling;

	Address         memory;

	UInt            first_pos_in_file;
	UInt            last_pos_in_file;
	UInt            size;
	UInt            delta;


	if (module->lifetime_status[life] != Unread) {
		return;
	}
	if (module->header->nrof_sections == 0) {
		return;
	}
	module->lifetime_status[life] = Read;

	sections_base = module->section_table;
	sections_ceiling = module->section_table + module->header->nrof_sections;




	first = sections_base;

	while (first->lifetime > life
	       && first < sections_ceiling
		) {
		first++;
	}

	while (first->lifetime == life
	       && first < sections_ceiling
	       && !first->has_data
		) {
		first++;
	}



	if (first->lifetime != life
	    || !first->has_data) {
		return;
	}

	last = first;

	while (last->lifetime == life) {
		last++;
	}

	last--;
	while (last->lifetime == life
	       && !last->has_data
		) {
		last--;
	}




	first_pos_in_file = (UInt) first->bytes;
	last_pos_in_file = (UInt) last->bytes;
	size = last_pos_in_file - first_pos_in_file + last->size;

	memory = Lib_Mem_MALLOC(size);
	CHECK(Lib_IOD_seek(module->object, first_pos_in_file, Lib_IOD_SEEK_SET) != -1, (LdLibMsg_InvalidModule));;
	CHECK(Lib_IOD_read(module->object, memory, size) == (size), (LdLibMsg_InvalidModule));;

	delta = ((UInt) memory) - first_pos_in_file;

	while (first <= last) {
		if (first->has_data) {
			Address         section_base;
			Address         section_ceiling;
			TMObj_System_Section_Id id;

			section_base =
				((Address) (
					    ((UInt) first->bytes) + delta
					    ));

			section_ceiling = section_base + first->size;

			id = first->system_section_id;

			first->bytes = section_base;

			if (first->is_system) {
				module->section_info[id][life].base = section_base;
				module->section_info[id][life].ceiling = section_ceiling;
			}
		}
		first++;
	}

}



static void
unpack_sections(FlatObject_Module module)
{
	TMObj_Section_Lifetime lifetime;

	if (module->lifetime_status[module->start_lifetime] == Read) {
		{
			UInt            _tmp_obj = (UInt) (module->header->start);
			Int             _lifetime = _tmp_obj >> 24;
			Int             _offset = _tmp_obj & 0x00ffffff;
			Address         _floor = (module)->section_info[TMObj_Symbol_Section][_lifetime].base;
			Address         _ceiling = (module)->section_info[TMObj_Symbol_Section][_lifetime].ceiling;
			Address         _unpacked = _floor + _offset;
			CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
			CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
			CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
			(module->header->start) = (Pointer) _unpacked;
		};
	}
	if (module->lifetime_status[module->embedded_modlist_lifetime] == Read) {
		{
			UInt            _tmp_obj = (UInt) (module->header->embedded_modlist);
			Int             _lifetime = _tmp_obj >> 24;
			Int             _offset = _tmp_obj & 0x00ffffff;
			Address         _floor = (module)->section_info[TMObj_Symbol_Section][_lifetime].base;
			Address         _ceiling = (module)->section_info[TMObj_Symbol_Section][_lifetime].ceiling;
			Address         _unpacked = _floor + _offset;
			CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
			CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
			CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
			(module->header->embedded_modlist) = (Pointer) _unpacked;
		};
	}
	for (lifetime = 1; lifetime < TMObj_System_Life; lifetime++) {
		if (module->lifetime_status[lifetime] == Read) {
			Unpack_Scatter_Source(module, module->section_info[TMObj_Scatter_Source_Section][lifetime].section);
			Unpack_Scatter(module, module->section_info[TMObj_Scatter_Section][lifetime].section);
			Unpack_Symbol(module, module->section_info[TMObj_Symbol_Section][lifetime].section);
			Unpack_External_Module(module, module->section_info[TMObj_External_Module_Section][lifetime].section);
			Unpack_SectionUnit(module, module->section_info[TMObj_SectionUnit_Section][lifetime].section);
			Unpack_Symbol_Reference(module, module->section_info[TMObj_Symbol_Reference_Section][lifetime].section);
			Unpack_Marker_Reference(module, module->section_info[TMObj_Marker_Reference_Section][lifetime].section);
			Unpack_JTab_Reference(module, module->section_info[TMObj_JTab_Reference_Section][lifetime].section);
			Unpack_FromDefault_Reference(module, module->section_info[TMObj_FromDefault_Reference_Section][lifetime].section);
			Unpack_DefaultToDefault_Reference(module, module->section_info[TMObj_DefaultToDefault_Reference_Section][lifetime].section);;
		}
	}

	for (lifetime = 1; lifetime < TMObj_System_Life; lifetime++) {
		if (module->lifetime_status[lifetime] == Read) {
			module->lifetime_status[lifetime] = Unpacked;
		}
	}
}


static void
sections_loaded(
		FlatObject_Module module,
		TMObj_Section_Lifetime low_life,
		TMObj_Section_Lifetime high_life
)
{
	TMObj_Section_Lifetime life;
	TMObj_Section   sections_base, sections_ceiling;

	sections_base = module->section_table;
	sections_ceiling = module->section_table + module->header->nrof_sections;

	while (sections_base < sections_ceiling) {
		life = sections_base->lifetime;

		if (life >= low_life && life <= high_life) {
			module->section_loaded(module, sections_base, module->data);
		}
		sections_base++;
	}
}


static void
load_lifetime(
	      FlatObject_Module module,
	      TMObj_Section_Lifetime lifetime
)
{
	if (module->lifetime_status[lifetime] != Unpacked) {
		read_sections(module, lifetime);
		unpack_sections(module);
		sections_loaded(module, lifetime, lifetime);
	}
}






static void 
endian_swap_header(TMObj_Module header)
{
	(header->start) = LAENDIAN_SWAP4(header->start);;
	(header->version) = LIENDIAN_SWAP4(header->version);;
	(header->machine) = LIENDIAN_SWAP4(header->machine);;
	(header->checksum) = LIENDIAN_SWAP4(header->checksum);;
	(header->nrof_exported_symbols) = LIENDIAN_SWAP2(header->nrof_exported_symbols);;
	(header->magic) = LIENDIAN_SWAP4(header->magic);;
	(header->default_section) = LAENDIAN_SWAP4(header->default_section);;
	(header->mod_desc_section) = LAENDIAN_SWAP4(header->mod_desc_section);;
	(header->embedded_modlist) = LAENDIAN_SWAP4(header->embedded_modlist);;
	(header->nrof_sections) = LIENDIAN_SWAP4(header->nrof_sections);;
	(header->szof_section_strtab) = LIENDIAN_SWAP4(header->szof_section_strtab);;

	(header->initialised_size) = LIENDIAN_SWAP4(header->initialised_size);;
	(header->uninitialised_size) = LIENDIAN_SWAP4(header->uninitialised_size);;
	(header->dynamicloading_size) = LIENDIAN_SWAP4(header->dynamicloading_size);;
	(header->downloading_size) = LIENDIAN_SWAP4(header->downloading_size);;
	(header->initialised_alignment) = LIENDIAN_SWAP2(header->initialised_alignment);;
	(header->uninitialised_alignment) = LIENDIAN_SWAP2(header->uninitialised_alignment);;
	(header->text_size) = LIENDIAN_SWAP4(header->text_size);;
	(header->internal_stream_size) = LIENDIAN_SWAP4(header->internal_stream_size);;
	(header->nrof_internal_stream_sections) = LIENDIAN_SWAP2(header->nrof_internal_stream_sections);;
	(header->first_internal_stream_section) = LIENDIAN_SWAP2(header->first_internal_stream_section);;
}


static void 
endian_swap_internal_section(TMObj_Section section)
{
	(section->name) = LAENDIAN_SWAP4(section->name);;
	(section->bytes) = LAENDIAN_SWAP4(section->bytes);;
	(section->relocation_offset) = LIENDIAN_SWAP4(section->relocation_offset);;
	(section->size) = LIENDIAN_SWAP4(section->size);;
	(section->alignment) = LIENDIAN_SWAP2(section->alignment);;
}


static          FlatObject_Module
read_handle(
	    Lib_IODriver object,
	    Address dataless_base,
	    Bool encode_sysrefs,
	    Bool unshuffle_dynamic_scatters,
	    FlatObject_SectionLoaded_Fun section_loaded,
	    Pointer data,
	    Int streamsize
)
{
	FlatObject_Module result;
	TMObj_Module    module;
	TMObj_Section   sections_base, sections_ceiling;
	UInt32          section_table_size;
	String          section_name;

    Lib_Mem_NEW(result);
	memset((Pointer) result, 0, sizeof(*result));

	Lib_Mem_NEW(result->header);
	CHECK(Lib_IOD_read(object, result->header, sizeof(*result->header)) == (sizeof(*result->header)), (LdLibMsg_InvalidModule));;

    module = result->header;

	CHECK(memcmp((Pointer) module, "LIFE_Obj", 8) != 0,
	      (LdLibMsg_AncientFormat));


	result->object = object;
	result->endian_difference = module->stored_endian != LCURRENT_ENDIAN;
	result->encode_sysrefs = encode_sysrefs;
	result->unshuffle_dynamic_scatters = unshuffle_dynamic_scatters;
	result->section_loaded = section_loaded;
	result->data = data;
	result->streamsize = streamsize;
	result->streambuff = Lib_Mem_MALLOC(streamsize);
	result->unshuffle_memo = Null;

	if (result->endian_difference) {
		endian_swap_header(module);
	}
        //DL:
        printf("XFlatObject.c: module->magic is 0x%x\n", module->magic);
	CHECK(module->magic == TMObj_Magic_Number,
	      (LdLibMsg_WrongMagic)){}

	CHECK(module->version >= TMObj_Format_Version,
	      (LdLibMsg_Old_Format_Version, module->version, TMObj_Format_Version)){}

	CHECK(module->version <= TMObj_Format_Version,
	      (LdLibMsg_Unknown_Format_Version, module->version, TMObj_Format_Version)){}

	section_table_size = module->nrof_sections * sizeof(TMObj_Section_Rec)
		+ module->szof_section_strtab;

	result->section_table = Lib_Mem_MALLOC(section_table_size);
	CHECK(Lib_IOD_read(object, result->section_table, section_table_size) == (section_table_size), (LdLibMsg_InvalidModule)){}

	sections_base = result->section_table;
	sections_ceiling = result->section_table + module->nrof_sections;
	section_name = (String) sections_ceiling;


	result->section_info[TMObj_Section_Section][TMObj_System_Life].base = (Address) sections_base;
	result->section_info[TMObj_Section_Section][TMObj_System_Life].ceiling = (Address) sections_ceiling;

    while (sections_base < sections_ceiling) {
		TMObj_Section   section = sections_base++;
		TMObj_Section_Lifetime lifetime = section->lifetime;
		TMObj_System_Section_Id id = section->system_section_id;
		Int             length_of_section_name;

		if (result->endian_difference) {
			endian_swap_internal_section(section);
		}
		length_of_section_name = (Int) section->name;
		section->name = section_name;
		section_name += length_of_section_name;

		if (section->is_system) {
			CHECK(section->has_data, (LdLibMsg_InvalidModule)){}
			result->section_info[id][lifetime].section = section;
		} else if (!section->has_data) {
			section->bytes = dataless_base;
		}
	}

    return result;
}





FlatObject_Module
FlatObject_open_object(
		       Lib_IODriver object,
		       Address dataless_base,
		       Bool encode_sysrefs,
		       Bool unshuffle_dynamic_scatters,
		       FlatObject_SectionLoaded_Fun section_loaded,
		       Pointer data,
		       Int streamsize
)
{
	FlatObject_Module result;
	TMObj_Module    module;
	TMObj_Section_Lifetime life;


	Lib_Excp_try {

		result = read_handle(object, dataless_base, encode_sysrefs,
				     unshuffle_dynamic_scatters,
				     section_loaded, data, streamsize);

		for (life = TMObj_Loading_Life; life <= TMObj_System_Life; life++) {
			read_sections(result, life);
		}

        module = result->header;

		result->start_lifetime = ((TMObj_Section_Lifetime) (((UInt32) (module->start)) >> 24));
		result->embedded_modlist_lifetime = ((TMObj_Section_Lifetime) (((UInt32) (module->embedded_modlist)) >> 24));

		if (module->default_section != Null) {
			{
				UInt            _tmp_obj = (UInt) (module->default_section);
				Int             _lifetime = _tmp_obj >> 24;
				Int             _offset = _tmp_obj & 0x00ffffff;
				Address         _floor = (result)->section_info[TMObj_Section_Section][_lifetime].base;
				Address         _ceiling = (result)->section_info[TMObj_Section_Section][_lifetime].ceiling;
				Address         _unpacked = _floor + _offset;
				CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
				CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
				CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
				(module->default_section) = (Pointer) _unpacked;
			};
			result->default_section_bytes = module->default_section->bytes;
		}
		if (module->mod_desc_section != Null) {
			{
				UInt            _tmp_obj = (UInt) (module->mod_desc_section);
				Int             _lifetime = _tmp_obj >> 24;
				Int             _offset = _tmp_obj & 0x00ffffff;
				Address         _floor = (result)->section_info[TMObj_Section_Section][_lifetime].base;
				Address         _ceiling = (result)->section_info[TMObj_Section_Section][_lifetime].ceiling;
				Address         _unpacked = _floor + _offset;
				CHECK(_lifetime >= 0, (LdLibMsg_InvalidModule)){}
				CHECK(_lifetime < TMObj_LAST_LIFE, (LdLibMsg_InvalidModule)){}
				CHECK(_unpacked < _ceiling, (LdLibMsg_InvalidModule)){}
				(module->mod_desc_section) = (Pointer) _unpacked;
			};
		}
		unpack_sections(result);
		sections_loaded(result, TMObj_Loading_Life, TMObj_System_Life);
	}

	Lib_Excp_otherwise {
		result = Null;
	}
	Lib_Excp_end_try;

   return result;
}




void
FlatObject_close_object(
			FlatObject_Module module
)
{
	if (module->unshuffle_memo)
		Lib_Mapping_delete(module->unshuffle_memo);
	Lib_Mem_FREE(module->streambuff);
	Lib_Mem_FREE(module);
}



TMObj_Module
FlatObject_get_header(FlatObject_Module module)
{
	return module->header;
}









TMObj_Section
FlatObject_get_section(
		       FlatObject_Module module,
		       String name
)
{
	TMObj_Section   sections_base = module->section_table;
	TMObj_Section   sections_ceiling = sections_base + module->header->nrof_sections;





	while (sections_base < sections_ceiling) {
		if (!sections_base->is_system
		    && LEQSTR(name, sections_base->name)
			) {
			load_lifetime(module, sections_base->lifetime);
			return sections_base;
		}
		sections_base++;
	}

	return Null;
}



TMObj_Symbol
FlatObject_get_global_symbol(
			     FlatObject_Module module,
			     String name
)
{
	TMObj_Section_Lifetime life;


	for (life = 1; life < TMObj_LAST_LIFE; life++) {
		TMObj_Section   s;

		s = FlatObject_get_system_section(
						  module,
						  TMObj_Symbol_Section,
						  life
			);

		if (s != Null) {
			Address         base = s->bytes;
			TMObj_Symbol    symbol = (TMObj_Symbol) base;
			TMObj_Symbol    ceiling = (TMObj_Symbol) (base + s->size);

			while (symbol < ceiling) {
				if (symbol->scope != TMObj_LocalScope
				    && LEQSTR(symbol->name, name)
					) {
					return symbol;
				}
				symbol++;
			}
		}
	}

	return Null;
}







TMObj_Section
FlatObject_get_system_section(
			      FlatObject_Module module,
			      TMObj_System_Section_Id id,
			      TMObj_Section_Lifetime life
)
{
	TMObj_Section   result = module->section_info[id][life].section;

	if (result != Null) {
		load_lifetime(module, life);
	}
	return result;
}





static void
streamload(
	   FlatObject_Module module,
	   TMObj_Section section,
	   FlatObject_Section_Fun section_fun,
	   Pointer data
)
{
	Pointer         saved_bytes = section->bytes;
	Int32           saved_size = section->size;
	Int32           streamsize = module->streamsize;
	Int32           bytes_left = saved_size;
	/* initialized to 0 to avoid warning, NOT CHECKED */
	Int32           unit_size = 0;



	switch (section->system_section_id) {

	case TMObj_Symbol_Reference_Section:
		unit_size = sizeof(TMObj_Symbol_Reference_Rec);
		break;

	case TMObj_Marker_Reference_Section:
		unit_size = sizeof(TMObj_Marker_Reference_Rec);
		break;

	case TMObj_FromDefault_Reference_Section:
		unit_size = sizeof(TMObj_FromDefault_Reference_Rec);
		break;

	case TMObj_DefaultToDefault_Reference_Section:
		unit_size = sizeof(TMObj_DefaultToDefault_Reference_Rec);
		break;
	}

	streamsize = LROUND_DOWN(streamsize, unit_size);

	CHECK(Lib_IOD_seek(module->object, (UInt) saved_bytes, Lib_IOD_SEEK_SET) != -1, (LdLibMsg_InvalidModule)){}
	section->bytes = module->streambuff;


	while (bytes_left > 0) {
		streamsize = LMIN(streamsize, bytes_left);
		section->size = streamsize;
		bytes_left -= streamsize;

		CHECK(Lib_IOD_read(module->object, module->streambuff, streamsize) == (streamsize), (LdLibMsg_InvalidModule)){}

		switch (section->system_section_id) {

		case TMObj_Symbol_Reference_Section:
			Unpack_Symbol_Reference(module, section);
			break;

		case TMObj_Marker_Reference_Section:
			Unpack_Marker_Reference(module, section);
			break;

		case TMObj_FromDefault_Reference_Section:
			Unpack_FromDefault_Reference(module, section);
			break;

		case TMObj_DefaultToDefault_Reference_Section:
			Unpack_DefaultToDefault_Reference(module, section);
			break;
		}

		module->section_loaded(module, section, module->data);
		section_fun(section, data);
	}

	section->bytes = saved_bytes;
	section->size = saved_size;
}



void
FlatObject_traverse_sections(
			     FlatObject_Module module,
			     FlatObject_Section_Fun section_fun,
			     TMObj_Section_Lifetime start,
			     TMObj_Section_Lifetime end,
			     Pointer data
)
{
	TMObj_Section   sections_base = module->section_table;
	TMObj_Section   sections_ceiling = sections_base + module->header->nrof_sections;



	while (sections_base < sections_ceiling) {
		TMObj_Section_Lifetime life = sections_base->lifetime;

		if (start <= life && life <= end) {
			if (life == TMObj_StreamLoading_Life
			    && module->streamsize > 0
			    && module->lifetime_status[life] != Unpacked
				) {
				streamload(module, sections_base, section_fun, data);
			} else {
				load_lifetime(module, life);
				section_fun(sections_base, data);
			}
		}
		sections_base++;
	}
}



void
FlatObject_traverse_section_info(
				 FlatObject_Module module,
				 FlatObject_Section_Fun section_fun,
				 TMObj_Section_Lifetime start,
				 TMObj_Section_Lifetime end,
				 Pointer data
)
{
	TMObj_Section   sections_base = module->section_table;
	TMObj_Section   sections_ceiling = sections_base + module->header->nrof_sections;



	while (sections_base < sections_ceiling) {
		TMObj_Section_Lifetime life = sections_base->lifetime;

		if (start <= life && life <= end) {
			section_fun(sections_base, data);
		}
		sections_base++;
	}
}

