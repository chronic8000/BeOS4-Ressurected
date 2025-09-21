#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <image.h>

#include <new.h>

#define LEFT 0
#define TOP 1
#define RIGHT 2
#define BOTTOM 3

#include "DecorDef.h"
#include "DecorVariable.h"
#include "DecorVariableInteger.h"
#include "DecorState.h"
#include "DecorAnchorage.h"

const uint32 DECOR_TAG = 'DECR';

// Increment for any incompatible change to flattened decor format.
const int32 CURRENT_DECOR_VERSION = 120;

#if defined(DECOR_CLIENT)

	#include "map"
	#include "vector"

	typedef std::vector<DecorAtom*>							atom_list;
	typedef atom_list::iterator							atom_list_iter;

	typedef std::map< const std::type_info*,atom_list >			type_map;
	typedef type_map::iterator							type_map_iter;
	

#endif

/**************************************************************/

struct AtomList {
	BString		className;
	int32		listSize;
	void *		(*createList)(int32 count);
	void		(*deleteList)(void *p);
	DecorAtom *	(*index)(void *p, int32 i);
	void *		list;
};

/**************************************************************/

DecorAnchorage * DecorDef::UnitAt(DecorState *instance, int32 index)
{
	return (DecorAnchorage*)m_units[index]->Choose(instance->Space());
}

void DecorDef::InitLocal(DecorState *instance)
{
#if defined(DECOR_CLIENT)
	if (m_atoms) {
		uint32 size,typeCount;
		type_map *atomMap = (type_map*)m_atoms;
		type_map_iter iter(atomMap->begin());
		typeCount = atomMap->size();
		for (uint32 i=0;i<typeCount;i++,iter++) {
			size = iter->second.size();
			for (uint32 j=0;j<size;j++) {
				iter->second[j]->InitLocal(instance);
			};
		};
		return;
	};
#endif
	
	if (m_atomLists) {
		for (int32 i=0;i<m_atomListCount;i++) {
			for (int32 j=0;j<m_atomLists[i].listSize;j++) {
				m_atomLists[i].index(m_atomLists[i].list,j)->InitLocal(instance);
			};
		};
	};
}

void DecorDef::AddVariable(DecorVariable *var)
{
	for (int32 i=0;i<m_varCount;i++)
		if (m_vars[i] == var) return;

	m_vars = (DecorVariable**)grRealloc(m_vars,sizeof(DecorVariable*)*(m_varCount+1), "DecorDef::m_vars (realloc)",MALLOC_CANNOT_FAIL);
	m_vars[m_varCount++] = var;
}

DecorVariable * DecorDef::GetVariable(int32 id)
{
	if ((id < 0) || (id >= m_varCount)) return NULL;
	return m_vars[id];
};

DecorVariable * DecorDef::GetVariable(const char *name)
{
	for (int32 i=0;i<m_varCount;i++) {
		if (!strcmp(m_vars[i]->Name(),name)) return m_vars[i];
	};
	return NULL;
};

int32 DecorDef::HandleEvent(HandleEventStruct *handle)
{
	int32 ret = 0;
	for (int32 i=0;i<m_unitCount;i++) {
		point pt = handle->localPoint;
		ret |= UnitAt(handle->state,i)->HandleEvent(handle);
		if ((ret & EVENT_TERMINAL) && (handle->event->what != B_MOUSE_MOVED)) return ret;
		handle->localPoint = pt;
	};
	return 0;
};

void DecorDef::AdjustLimits(DecorState *instance, int32 *minWidth, int32 *minHeight, int32 *maxWidth, int32 *maxHeight)
{
	for (int32 i=0;i<m_unitCount;i++)
		UnitAt(instance,i)->AdjustLimits(instance,minWidth,minHeight,maxWidth,maxHeight);
};

uint32 DecorDef::Layout(DecorState *instance, region **updateRegion)
{
	uint32 r = 0;
	CollectRegionStruct collect;
	collect.instance = instance;
	collect.theRegion = *updateRegion;
	collect.subRegion = NULL;
	collect.tmpRegion[0] = newregion();
	collect.tmpRegion[1] = newregion();
	clear_region(*updateRegion);
	for (int32 i=0;i<m_unitCount;i++)
		r |= UnitAt(instance,i)->Layout(&collect);

	kill_region(collect.tmpRegion[0]);
	kill_region(collect.tmpRegion[1]);
	*updateRegion = collect.theRegion;
	return r;
}

bool DecorDef::NeedAdjustLimits(DecorState *instance)
{
	bool needMinMax = false;
	for (int32 i=0;i<m_unitCount;i++)
		needMinMax = needMinMax | UnitAt(instance,i)->NeedAdjustLimits(instance);
	return needMinMax;
}

bool DecorDef::NeedLayout(DecorState *instance)
{
	bool needLayout = false;
	for (int32 i=0;i<m_unitCount;i++)
		needLayout = needLayout | UnitAt(instance,i)->NeedLayout(instance);
	return needLayout;
}

bool DecorDef::NeedDraw(DecorState *instance)
{
	bool needDraw = false;
	for (int32 i=0;i<m_unitCount;i++)
		needDraw = needDraw | UnitAt(instance,i)->NeedDraw(instance);
	return needDraw;
}

void DecorDef::Draw(
	DecorState *instance,
	DrawingContext context,
	region *update)
{
	for (int32 i=0;i<m_unitCount;i++)
		UnitAt(instance,i)->Draw(instance,context,update);
}

void DecorDef::GetRegion(DecorState *instance, region **theRegion, region **subRegion)
{
	CollectRegionStruct collect;
	collect.instance = instance;
	collect.theRegion = newregion();
	collect.subRegion = newregion();
	collect.tmpRegion[0] = newregion();
	collect.tmpRegion[1] = newregion();
	for (int32 i=0;i<m_unitCount;i++)
		UnitAt(instance,i)->CollectRegion(&collect);

	kill_region(collect.tmpRegion[0]);
	kill_region(collect.tmpRegion[1]);
	*theRegion = collect.theRegion;
	*subRegion = collect.subRegion;
}

void *lookup_function(const char *func_name)
{
	thread_id	tid = find_thread(NULL);
	thread_info	tinfo;
	team_id		team;
	image_info	info;
	int32		cookie;
	void *		function = NULL;

	get_thread_info(tid, &tinfo);
	team = tinfo.team;
	cookie = 0;

	while (
		!get_next_image_info(team, &cookie, &info) &&
		get_image_symbol(info.id, func_name, B_SYMBOL_TYPE_TEXT, &function)
	);

	if (!function)
		xprintf("*** Failure looking up decor class function: %s\n", func_name);
	
	return function;
};

#if __GNUC__

#if __GNUC__ < 3
const char *instantiateTemplate = "Instantiate__%ld%sl";
const char *indexTemplate = "Index__%ld%sPvl";
const char *deleteTemplate = "Delete__%ld%sPv";
#else
const char *instantiateTemplate = "_ZN%ld%s11InstantiateEl";
const char *indexTemplate = "_ZN%ld%s5IndexEPvl";
const char *deleteTemplate = "_ZN%ld%s6DeleteEPv";
#endif

#elif __MWERKS__
const char *instantiateTemplate = "Instantiate__%ld%sFl";
const char *indexTemplate = "Index__%ld%sFPvl";
const char *deleteTemplate = "Delete__%ld%sFPv";
#else
#error Do not know this compiler!
#endif

status_t DecorDef::Unflatten(DecorStream *f)
{
	char funcName[256];
	int32 typeCount;
	uint32 tag;
	int32 version;
	
	Reset();
	
	// Read header information.
	
	f->StartReadBytes(true);
	f->Read(&tag);
	f->Read(&version);
	f->Read(&m_varSpace);
	f->Read(&m_bitmaskSize);
	f->Read(&m_limitCache);
	f->Read(&m_layoutSize);
	f->Read(&m_mouseContainerCount);
	f->Read(&typeCount);
	f->Read(&m_unitCount);
	f->RequireData(false);
	// Place new data fields here.
	if ((m_status=f->FinishReadBytes()) < B_OK)
		return m_status;
	
	if (tag != DECOR_TAG || version != CURRENT_DECOR_VERSION)
		return (m_status = B_MISMATCHED_VALUES);
	
	m_atomLists = new(nothrow) AtomList[typeCount];
	if (m_atomLists == NULL) {
		m_atomListCount = 0;
		return (m_status = B_NO_MEMORY);
	}
	m_atomListCount = typeCount;
	
	for (int32 i=0;i<typeCount;i++) {
		f->StartReadBytes(true);
		f->Read(&m_atomLists[i].className);
		f->Read(&m_atomLists[i].listSize);
		f->RequireData(false);
		// Place new data fields here.
		m_status = f->FinishReadBytes();
		
		if (m_status >= B_OK) {
			sprintf(funcName, instantiateTemplate, m_atomLists[i].className.Length(), m_atomLists[i].className.String());
	
			m_atomLists[i].createList = (void *(*)(int32))lookup_function(funcName);
			sprintf(funcName, indexTemplate, m_atomLists[i].className.Length(), m_atomLists[i].className.String());
	
			m_atomLists[i].index = (DecorAtom *(*)(void*,int32))lookup_function(funcName);
			sprintf(funcName, deleteTemplate, m_atomLists[i].className.Length(), m_atomLists[i].className.String());
	
			m_atomLists[i].deleteList = (void (*)(void*))lookup_function(funcName);
			if (!m_atomLists[i].createList || !m_atomLists[i].index || !m_atomLists[i].deleteList)
				debugger("could not load class\n");

			m_atomLists[i].list = m_atomLists[i].createList(m_atomLists[i].listSize);
			
			if (m_status >= B_OK && m_atomLists[i].list == NULL)
				m_status = B_NO_MEMORY;
		}
		
		if (m_status < B_OK) {
			while (i<typeCount) {
				m_atomLists[i].list = NULL;
				m_atomLists[i++].listSize = 0;
			}
			return m_status;
		}
	}
	
	for (int32 i=0;i<typeCount;i++) {
		for (int32 j=0;j<m_atomLists[i].listSize;j++) {
			m_atomLists[i].index(m_atomLists[i].list,j)->Unflatten(f);
		};
	};

	m_units = (DecorAtom**)grMalloc(sizeof(DecorAtom*)*m_unitCount, "DecorDef::m_units",MALLOC_CANNOT_FAIL);
	if (m_units == NULL) {
		m_unitCount = 0;
		return (m_status = B_NO_MEMORY);
	}
	for (int32 i=0;i<m_unitCount;i++) {
		f->StartReadBytes(true);
		f->Read(&m_units[i]);
		f->RequireData(false);
		// Place new data fields here.
		m_status = f->FinishReadBytes();
		if (m_status >= B_OK && m_units[i] == NULL)
			m_status = B_NO_MEMORY;
		if (m_status < B_OK) {
			while (i<m_unitCount)
				m_units[i++] = NULL;
			return m_status;
		}
	}

	f->StartReadBytes(true);
	f->Read((DecorAtom**)&m_widthVar);
	f->Read((DecorAtom**)&m_heightVar);
	f->Read((DecorAtom**)&m_focusVar);
	f->RequireData(false);
	// Place new data fields here.
	m_status = f->FinishReadBytes();

	return m_status;
}

DecorAtom * DecorDef::ResolveRef(uint32 ref)
{
	if (ref == 0) return NULL;

	const uint32 index = (ref>>16)-1;
	
	if (index >= m_atomListCount || (ref&0xFFFF) >= m_atomLists[index].listSize) {
		xprintf("Decor atom reference 0x%lx doesn't exist!\n", ref);
		return NULL;
	}
	
	return m_atomLists[index].index(m_atomLists[index].list,ref&0xFFFF);
}

DecorDef::DecorDef(BDataIO *io)
	:	m_status(B_NO_INIT),
		m_atomLists(NULL),
		m_varCount(0),
		m_vars(NULL),
		m_units(NULL)
{
	#if defined(DECOR_CLIENT)
		m_atoms = NULL;
	#endif
	DecorStream stream(this,io);
	Unflatten(&stream);

	if (m_atomLists && m_status >= B_OK) {
		for (int32 i=0;i<m_atomListCount;i++) {
			if (m_atomLists[i].list && m_atomLists[i].index && m_atomLists[i].listSize &&
					m_atomLists[i].index(m_atomLists[i].list,0)->IsVariable()) {
				for (int32 j=0;j<m_atomLists[i].listSize;j++) {
					AddVariable((DecorVariable*)m_atomLists[i].index(m_atomLists[i].list,j));
				}
			}
		}
	}
}

DecorDef::~DecorDef()
{
	Reset();
}

void DecorDef::Reset()
{
	m_status = B_NO_INIT;
	
#if defined(DECOR_CLIENT)
	if (m_atoms) {
		uint32 size,typeCount;
		type_map *atomMap = (type_map*)m_atoms;
		type_map_iter iter(atomMap->begin());
		typeCount = atomMap->size();
		for (uint32 i=0;i<typeCount;i++,iter++) {
			size = iter->second.size();
			for (uint32 j=0;j<size;j++) {
				delete iter->second[j];
			}
		}
		delete m_atoms;
		m_atoms = NULL;
	}
#endif
	
	if (m_atomLists) {
		for (int32 i=0;i<m_atomListCount;i++) {
			if (m_atomLists[i].list && m_atomLists[i].deleteList)
				m_atomLists[i].deleteList(m_atomLists[i].list);
		}
		delete[] m_atomLists;
		m_atomLists = NULL;
	};
	
	grFree(m_units); // mathias - 12/9/2000: Don't leak units
	m_units = NULL;
	grFree(m_vars); // mathias - 12/9/2000: Don't leak vars
	m_vars = NULL;
}

status_t DecorDef::InitCheck() const
{
	return m_status;
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"
	#include "ctype.h"

	DecorDef::DecorDef(DecorTable *t)
	{
		type_map *atomMap;
		int32 proceed;
		gCurrentDef = this;
		m_status = B_OK;
		m_units = NULL;
		m_vars = NULL;
		m_varCount = 0;
		m_atoms = (void*)(atomMap = new type_map);
		m_atomLists = NULL;

		t = t->GetDecorTable("Units",ERROR);

		m_unitCount = t->Count();
		m_units = (DecorAtom**)grMalloc(sizeof(DecorAtom*)*m_unitCount, "decor",MALLOC_CANNOT_FAIL);
		for (int32 i=0;i<m_unitCount;i++) {
			m_units[i] = t->GetAtom(i+1);
			proceed = 1;
			while (proceed) m_units[i] = m_units[i]->Reduce(&proceed);
			m_units[i]->RegisterAtomTree();
			if (!m_units[i]->IsTypeOf(typeid(DecorAnchorage)))
				panic(("Every top-level part must be an anchorage!\n"));
		};

		delete t;
	
		type_map_iter iter(atomMap->begin());
		
		if (DecorPrintMode()&DECOR_PRINT_STATS) {
			printf(	"\nFinal Statistics"
					"\n~~~~~~~~~~~~~~~~\n"
					"\nObjects created:\n");
			while (iter != atomMap->end()) {
				const char* name = iter->first->name();
				while (*name >= '0' && *name <= '9') name++;
				printf("  %32s: %ld instances\n",name,iter->second.size());
				iter++;
			};
		}
	
		m_mouseContainerCount = m_bitmaskSize = m_varSpace = m_limitCache = m_layoutSize = 0;

		uint32 size,typeCount;
		iter = atomMap->begin();
		typeCount = atomMap->size();
		for (uint32 i=0;i<typeCount;i++,iter++) {
			size = iter->second.size();
			for (uint32 j=0;j<size;j++) {
				iter->second[j]->AllocateLocal(this);
			};
		};
		
		if (DecorPrintMode()&DECOR_PRINT_STATS) {
			printf(	"\nVariables defined:\n");
		}
		
		grFree(m_vars);
		m_vars = NULL;
		m_varCount = 0;
		iter = atomMap->begin();
		for (int32 i=0;i<typeCount;i++) {
			if (iter->second.size() > 0 && iter->second[0]->IsVariable()) {
				for (int32 j=0;j<iter->second.size();j++) {
					DecorVariable* var = dynamic_cast<DecorVariable*>(iter->second[j]);
					if (!var) {
						fprintf(stderr, "Atom %p is not a variable!\n", iter->second[j]);
						continue;
					}
					if (DecorPrintMode()&DECOR_PRINT_STATS) {
						const char* name = iter->first->name();
						while (*name >= '0' && *name <= '9') name++;
						printf("  %32s named %s with type code %ld\n",
								name, var->Name(), var->Type());
					}
					AddVariable(var);
				};
			};
			iter++;
		};

		if (DecorPrintMode()&DECOR_PRINT_STATS) {
			printf(	"\nPer-state memory usage:\n"
					"    Layout data: %ld bytes\n"
					"    Limits data: %ld bytes\n"
					"  Variable data: %ld bytes\n"
					"   BitMask data: %ld bytes\n"
					"  ------------------------\n"
					"          Total: %ld bytes\n",
					m_layoutSize, m_limitCache, m_varSpace, m_bitmaskSize,
					m_layoutSize+m_limitCache+m_varSpace+m_bitmaskSize);
		}

		DecorVariable *widthVar = GetVariable("@width");
		DecorVariable *heightVar = GetVariable("@height");
		DecorVariable *focusVar = GetVariable("@active?");

		m_widthVar = dynamic_cast<DecorVariableInteger*>(widthVar);
		m_heightVar = dynamic_cast<DecorVariableInteger*>(heightVar);
		m_focusVar = dynamic_cast<DecorVariableInteger*>(focusVar);

		if (widthVar && !m_widthVar)
			panic(("Special variable \"@width\" must be an integer\n"));
		if (!m_widthVar) printf("Warning: var @width not referenced\n");

		if (heightVar && !m_heightVar)
			panic(("Special variable \"@height\" must be an integer\n"));
		if (!m_heightVar) printf("Warning: var @height not referenced\n");

		if (focusVar && !m_focusVar)
			panic(("Special variable \"@active?\" must be an integer\n"));
		if (!m_focusVar) printf("Warning: var @active? not referenced\n");

		/*
		typeCount = atomMap->size();
		m_atomLists = (AtomList*)grMalloc(sizeof(AtomList)*typeCount, "decor",MALLOC_CANNOT_FAIL);

		char funcName[256];		
		iter = atomMap->begin();
		for (int32 i=0;i<typeCount;i++,iter++) {
			m_atomLists[i].className = strdup(class_name(iter->first));
			m_atomLists[i].listSize = iter->second.size();
			sprintf(funcName, "Instantiate__%ld%sl", strlen(m_atomLists[i].className), m_atomLists[i].className);
			m_atomLists[i].createList = (void *(*)(int32))lookup_function(funcName);
			sprintf(funcName, "Index__%ld%sPvl", strlen(m_atomLists[i].className), m_atomLists[i].className);
			m_atomLists[i].index = (DecorAtom *(*)(void*,int32))lookup_function(funcName);
			sprintf(funcName, "Delete__%ld%sPv", strlen(m_atomLists[i].className), m_atomLists[i].className);
			m_atomLists[i].deleteList = (void (*)(void*))lookup_function(funcName);
			m_atomLists[i].list = m_atomLists[i].createList(m_atomLists[i].listSize);
		};
		*/
	}

	int32 DecorDef::AllocateVarSpace(int32 size)
	{
		int32 vs = m_varSpace;
		m_varSpace += size;
		return vs;
	}

	int32 DecorDef::AllocateLimitCache(int32 size)
	{
		int32 vs = m_limitCache;
		m_limitCache += size;
		return vs;
	}

	int32 DecorDef::AllocateLayout(int32 size)
	{
		int32 vs = m_layoutSize;
		m_layoutSize += size;
		return vs;
	}

	int32 DecorDef::AllocateBitmask(int32 bits)
	{
		int32 vs = m_bitmaskSize;
		m_bitmaskSize += bits;
		return vs;
	};

	int32 DecorDef::AllocateMouseContainer(int32 bits)
	{
		int32 vs = m_mouseContainerCount;
		m_mouseContainerCount += bits;
		return vs;
	}

	status_t DecorDef::Flatten(DecorStream *f)
	{
		const char *name;
		int32 typeCount,size;
		type_map *atomMap = (type_map*)m_atoms;

		status_t result;
		
		// Write header information.
		
		f->StartWriteBytes();
		
		f->Write(&DECOR_TAG);
		f->Write(&CURRENT_DECOR_VERSION);
		f->Write(&m_varSpace);
		f->Write(&m_bitmaskSize);
		f->Write(&m_limitCache);
		f->Write(&m_layoutSize);
		f->Write(&m_mouseContainerCount);
		
		typeCount = atomMap->size();
		f->Write(&typeCount);

		f->Write(&m_unitCount);
		
		if ((result=f->FinishWriteBytes()) < B_OK)
			return result;
		
		type_map_iter iter(atomMap->begin());
		for (int32 i=0;i<typeCount;i++,iter++) {
			size = iter->second.size();
			name = iter->first->name();
			#if __GNUC__
				while (*name && isdigit(*name)) name++;
			#endif
			f->StartWriteBytes();
			f->Write(name);
			f->Write(&size);
			if ((result=f->FinishWriteBytes()) < B_OK)
				return result;
		};
		
		iter = atomMap->begin();
		for (int32 i=0;i<typeCount;i++,iter++) {
			size = iter->second.size();
			for (int32 j=0;j<size;j++) {
				iter->second[j]->Flatten(f);
			};
		};

		for (int32 i=0;i<m_unitCount;i++) {
			f->StartWriteBytes();
			f->Write(m_units[i]);
			if ((result=f->FinishWriteBytes()) < B_OK)
				return result;
		}
		
		f->StartWriteBytes();
		f->Write(m_widthVar);
		f->Write(m_heightVar);
		f->Write(m_focusVar);
		return f->FinishWriteBytes();
	}

	status_t DecorDef::Flatten(BDataIO *f)
	{
		DecorStream stream(this,f);
		return Flatten(&stream);
	}

	void DecorDef::RegisterAtom(DecorAtom *atom)
	{
		atom_list &atoms = (*((type_map*)m_atoms))[&typeid(*atom)];
		for (int32 i=0;i<atoms.size();i++)
			if (atoms[i] == atom) return;
		atoms.push_back(atom);
	}

	uint32 DecorDef::GetRefFor(const DecorAtom *atom)
	{
		if (atom == NULL) return 0;
	
		uint32 size,typeCount;
		type_map *atomMap = (type_map*)m_atoms;
		type_map_iter iter(atomMap->begin());
		typeCount = atomMap->size();
		for (uint32 i=0;i<typeCount;i++,iter++) {
			size = iter->second.size();
			for (uint32 j=0;j<size;j++) {
				if (iter->second[j] == atom) return ((i+1)<<16) | j;
			};
		};
		debugger("ref");
		panic(("Could not get ref for 0x%08lx %s\n",(uint32)atom,typeid(*atom).name()));
	}
	
#endif
